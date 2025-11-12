#include <util/config.h>
#include <mem/manager.h>
#include <util/io.h>
#include <mem/map.h>
#include <util/console.h>
#include <interrupt/irq.h>
#include <sync/spinlock.h>
#include <stdint.h>

// ブロックヘッダは8バイト境界
typedef struct block_header {
	uint32_t size;
	struct block_header *next;
} block_header_t;

// ヒープの先頭とフリーリストのヘッド
static block_header_t *free_list = NULL;
static uintptr_t heap_start_addr = 0;
static uintptr_t heap_end_addr = 0;
static spinlock_t heap_lock = { 0 };

#define ALIGN 8

// sizeをALIGNに丸める（ヘッダを除くユーザ領域のサイズ）
static inline uint32_t align_up(uint32_t size) {
	return (size + (ALIGN - 1)) & ~(ALIGN - 1);
}

/**
 * @fn mem_init
 * @brief メモリを初期化します
 *
 * @param start ヒープ領域の開始アドレス
 * @param end   ヒープ領域の終了アドレス
 */
void mem_init(uint32_t start, uint32_t end) {
	if (end <= start || (end - start) < sizeof(block_header_t)) {
		return;
	}

	heap_start_addr = start;
	heap_end_addr = end;

	// 最初のフリーブロック
	free_list = (block_header_t *)(uintptr_t)start;
	free_list->size = end - start;
	free_list->next = NULL;
#ifdef INIT_MSG
	printk("Memory initialized: heap %x - %x (size=%u)\n",
	       (unsigned int)start, (unsigned int)end,
	       (unsigned int)(end - start));
#endif
}

/**
 * @fn kmalloc
 * @brief 指定したサイズのメモリを確保します
 *
 * @param size 確保するバイト数
 * @return 確保したメモリ領域へのポインタ。失敗時はNULL
 */
void *kmalloc(uint32_t size) {
	if (size == 0 || free_list == NULL) {
		return NULL;
	}

	uint32_t flags = 0;
	spin_lock_irqsave(&heap_lock, &flags);

	uint32_t wanted = align_up(size);

	// ブロック全体の必要サイズ
	uint32_t total_size = wanted + sizeof(block_header_t);

	block_header_t *prev = NULL;
	block_header_t *cur = free_list;

	while (cur) {
		if (cur->size >= total_size) {
			if (cur->size >=
			    total_size + sizeof(block_header_t) + ALIGN) {
				// 分割可能 -> 残りを新しいフリーブロックにする
				uintptr_t cur_addr = (uintptr_t)cur;
				block_header_t *next_block =
					(block_header_t *)(cur_addr +
							   total_size);
				next_block->size = cur->size - total_size;
				next_block->next = cur->next;

				// 現在のブロックを返却対象としてサイズを調整
				cur->size = total_size;

				if (prev) {
					prev->next = next_block;
				} else {
					free_list = next_block;
				}
			} else {
				// 分割しないで全体を返す
				if (prev) {
					prev->next = cur->next;
				} else {
					free_list = cur->next;
				}
			}

			// ユーザ領域はヘッダの直後
			void *user_ptr = (void *)((uintptr_t)cur +
						  sizeof(block_header_t));
			spin_unlock_irqrestore(&heap_lock, flags);
			return user_ptr;
		}

		prev = cur;
		cur = cur->next;
	}

	// 見つからなかった
	spin_unlock_irqrestore(&heap_lock, flags);
	return NULL;
}

/**
 * @fn kfree
 * @brief メモリを解放します
 *
 * @param ptr 解放するメモリ領域へのポインタ
 */
void kfree(void *ptr) {
	if (ptr == NULL) {
		return;
	}

	uint32_t flags = 0;
	spin_lock_irqsave(&heap_lock, &flags);

	// ヘッダはユーザポインタの前にある
	block_header_t *hdr =
		(block_header_t *)((uintptr_t)ptr - sizeof(block_header_t));

	// 範囲チェック
	uintptr_t hdr_addr = (uintptr_t)hdr;
	if (hdr_addr < heap_start_addr ||
	    hdr_addr + hdr->size > heap_end_addr) {
		// 範囲外のポインタは無視
		spin_unlock_irqrestore(&heap_lock, flags);
		return;
	}

	// フリーリストに挿入（アドレス順に保つ）
	if (free_list == NULL || hdr < free_list) {
		hdr->next = free_list;
		free_list = hdr;
	} else {
		block_header_t *cur = free_list;
		while (cur->next && cur->next < hdr) {
			cur = cur->next;
		}
		hdr->next = cur->next;
		cur->next = hdr;
	}

	block_header_t *cur = free_list;
	while (cur && cur->next) {
		uintptr_t cur_end = (uintptr_t)cur + cur->size;
		uintptr_t next_addr = (uintptr_t)cur->next;
		if (cur_end == next_addr) {
			// 連続している -> 併合
			cur->size += cur->next->size;
			cur->next = cur->next->next;
			// 続けて併合の可能性があるのでループを継続
		} else {
			cur = cur->next;
		}
	}

	spin_unlock_irqrestore(&heap_lock, flags);
}

/**
 * @fn mem_has_space
 * @brief 指定したメモリタイプでsizeバイト分の空きがあるか判定する
 *
 * - MEM_TYPE_HEAP: 連続するsizeバイトを割当可能なフリーブロックが存在するか
 * - MEM_TYPE_FRAME: 連続するceil(size/FRAME_SIZE)フレームが存在するか
 */
int mem_has_space(mem_type_t type, uint32_t size) {
	if (type == MEM_TYPE_HEAP) {
		// 連続領域が必要なので、フリーリスト上にsizeバイト以上のブロックがあるか探す
		uint32_t flags = 0;
		spin_lock_irqsave(&heap_lock, &flags);
		uint32_t wanted = align_up(size);
		block_header_t *cur = free_list;
		while (cur) {
			if (cur->size >= wanted + sizeof(block_header_t)) {
				spin_unlock_irqrestore(&heap_lock, flags);
				return 1;
			}
			cur = cur->next;
		}
		spin_unlock_irqrestore(&heap_lock, flags);
		return 0;
	} else if (type == MEM_TYPE_FRAME) {
		// 必要なフレーム数を計算し、memmapのビットマップ上で連続する空きフレームを探す
		const memmap_t *mm = memmap_get();
		if (!mm || mm->frames == 0)
			return 0;

		uint32_t need_frames = (size + FRAME_SIZE - 1) / FRAME_SIZE;
		if (need_frames == 0)
			need_frames = 1;

		uint32_t consecutive = 0;
		for (uint32_t i = 0; i < mm->frames; ++i) {
			// ビットが0なら空き
			uint32_t word = mm->bitmap[i / 32];
			uint32_t bit = (word >> (i % 32)) & 1u;
			if (bit == 0) {
				consecutive++;
				if (consecutive >= need_frames)
					return 1;
			} else {
				consecutive = 0;
			}
		}
		return 0;
	}

	return 0; // なんやこれ知らんぞ用
}

/**
 * @fn memory_init
 * @brief メモリマップなどを初期化します
 */
void memory_init() {
	memmap_init(0x100000, 0x500000); // 1MB - 5MB

	extern uint32_t __end;
	const memmap_t *mm = memmap_get();
	uintptr_t base_end = (uintptr_t)&__end;
	uintptr_t bitmap_end = base_end;
	// mm、mm->bitmap、mm->max_framesが有効か確認
	if (mm && mm->bitmap && mm->max_frames) {
		// bitmapはmemmap内でバイト配列として格納されているので、その終了アドレスを計算
		uint32_t bitmap_bytes = (mm->max_frames + 7) / 8;
		bitmap_end = (uintptr_t)mm->bitmap + bitmap_bytes;
	}
	// kernelの終了アドレスとbitmapの終了アドレスの大きい方を選択
	uintptr_t heap_start = (base_end > bitmap_end) ? base_end : bitmap_end;
	// 4KBページ境界に切り上げてアライン
	heap_start = (heap_start + 0x0FFF) & ~0x0FFF;
	/* 増やす: ブロックキャッシュなどで大きな動的確保が必要になるためヒープを256KBに拡張 */
	uintptr_t heap_end = heap_start + 0x40000; // 256KBのヒープ領域
	mem_init((uint32_t)heap_start, (uint32_t)heap_end);
	memmap_reserve((uint32_t)heap_start, (uint32_t)heap_end);
}

/**
 * @fn stack_alloc
 * @brief 下方向に伸びるカーネルスタック領域を確保する
 * @param size 要求サイズ（バイト）。内部で ALIGN に丸められる。
 * @return スタックのトップ（高位アドレス）。失敗時はNULL。
 */
void *stack_alloc(uint32_t size) {
	if (size == 0)
		return NULL;
	uint32_t wanted = align_up(size);
	void *p = kmalloc(wanted);
	if (!p)
		return NULL;
	return (void *)((uintptr_t)p + wanted);
}

/**
 * @fn stack_free
 * @brief stack_alloc で確保したスタックを解放する
 * @param top stack_alloc が返したトップアドレス
 * @param size 元の要求サイズ
 */
void stack_free(void *top, uint32_t size) {
	if (!top || size == 0)
		return;
	uint32_t wanted = align_up(size);
	/* top は p + wanted なので p = top - wanted */
	uintptr_t p = (uintptr_t)top - wanted;
	kfree((void *)p);
}

/* Heap statistics helpers (外部から利用可能にする) */
uint32_t heap_total_bytes(void) {
	if (heap_end_addr > heap_start_addr) {
		return heap_end_addr - heap_start_addr;
	}
	return 0;
}

uint32_t heap_free_bytes(void) {
	uint32_t total_free = 0;
	uint32_t flags = 0;
	spin_lock_irqsave(&heap_lock, &flags);
	block_header_t *cur = free_list;
	while (cur) {
		uint32_t user_bytes = 0;
		if (cur->size > sizeof(block_header_t)) {
			user_bytes = cur->size - sizeof(block_header_t);
		}
		total_free += user_bytes;
		cur = cur->next;
	}
	spin_unlock_irqrestore(&heap_lock, flags);
	return total_free;
}

uint32_t heap_largest_free_block(void) {
	uint32_t largest = 0;
	uint32_t flags = 0;
	spin_lock_irqsave(&heap_lock, &flags);
	block_header_t *cur = free_list;
	while (cur) {
		if (cur->size > sizeof(block_header_t)) {
			uint32_t user_bytes =
				cur->size - sizeof(block_header_t);
			if (user_bytes > largest)
				largest = user_bytes;
		}
		cur = cur->next;
	}
	spin_unlock_irqrestore(&heap_lock, flags);
	return largest;
}
