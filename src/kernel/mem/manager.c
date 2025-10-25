#include <config.h>
#include <mem/manager.h>
#include <util/io.h>

// ブロックヘッダは8バイト境界
typedef struct block_header {
        uint32_t size;
        struct block_header* next;
} block_header_t;

// ヒープの先頭とフリーリストのヘッド
static block_header_t* free_list = NULL;
static uint32_t heap_start_addr = 0;
static uint32_t heap_end_addr = 0;

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
        free_list = (block_header_t*)start;
        free_list->size = end - start;
        free_list->next = NULL;

        printk("Memory initialized: heap %x - %x (size=%d)\n", start, end, end - start);
}

/**
 * @fn kmalloc
 * @brief 指定したサイズのメモリを確保します
 *
 * @param size 確保するバイト数
 * @return 確保したメモリ領域へのポインタ。失敗時はNULL
 */
void* kmalloc(uint32_t size) {
        if (size == 0 || free_list == NULL) {
                return NULL;
        }

        uint32_t wanted = align_up(size);

        // ブロック全体の必要サイズ
        uint32_t total_size = wanted + sizeof(block_header_t);

        block_header_t* prev = NULL;
        block_header_t* cur = free_list;

        while (cur) {
                if (cur->size >= total_size) {
                        if (cur->size >= total_size + sizeof(block_header_t) + ALIGN) {
                                // 分割可能 -> 残りを新しいフリーブロックにする
                                uint32_t cur_addr = (uint32_t)cur;
                                block_header_t* next_block = (block_header_t*)(cur_addr + total_size);
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
                        void* user_ptr = (void*)((uint32_t)cur + sizeof(block_header_t));
                        return user_ptr;
                }

                prev = cur;
                cur = cur->next;
        }

        // 見つからなかった
        return NULL;
}

/**
 * @fn kfree
 * @brief メモリを解放します
 *
 * @param ptr 解放するメモリ領域へのポインタ
 */
void kfree(void* ptr) {
        if (ptr == NULL) {
                return;
        }

        // ヘッダはユーザポインタの前にある
        block_header_t* hdr = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));

        // 範囲チェック
        uint32_t hdr_addr = (uint32_t)hdr;
        if (hdr_addr < heap_start_addr || hdr_addr + hdr->size > heap_end_addr) {
                // 範囲外のポインタは無視
                return;
        }

        // フリーリストに挿入（アドレス順に保つ）
        if (free_list == NULL || hdr < free_list) {
                hdr->next = free_list;
                free_list = hdr;
        } else {
                block_header_t* cur = free_list;
                while (cur->next && cur->next < hdr) {
                        cur = cur->next;
                }
                hdr->next = cur->next;
                cur->next = hdr;
        }

        block_header_t* cur = free_list;
        while (cur && cur->next) {
                uint32_t cur_end = (uint32_t)cur + cur->size;
                uint32_t next_addr = (uint32_t)cur->next;
                if (cur_end == next_addr) {
                        // 連続している -> 併合
                        cur->size += cur->next->size;
                        cur->next = cur->next->next;
                        // 続けて併合の可能性があるのでループを継続
                } else {
                        cur = cur->next;
                }
        }
}
