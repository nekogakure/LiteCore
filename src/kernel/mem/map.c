/**
 * @file map.c
 * @brief ビットマップ方式のフレームアロケータ（シンプル実装）
 */

#include <config.h>
#include <mem/map.h>
#include <console.h>
#include <util/io.h>

// ビットマップは1ビットで1フレームを表す。安全のため最大フレーム数を定義する。
// この値は必要に応じて調整してください。
#define MAX_FRAMES 8192 // 8192 * 4KB = 32MB 管理可能

static uint32_t bitmap[(MAX_FRAMES + 31) / 32];

/* protect bitmap operations */
static volatile uint32_t memmap_lock_storage = 0;
// reuse existing spinlock type via pointer cast to avoid header include cycles
// we'll wrap with helper functions below

// 管理情報をまとめた構造体
static memmap_t memmap = {0};

static inline void bitmap_set(uint32_t idx) {
        memmap.bitmap[idx / 32] |= (1u << (idx % 32));
}

static inline void bitmap_clear(uint32_t idx) {
        memmap.bitmap[idx / 32] &= ~(1u << (idx % 32));
}

static inline int bitmap_test(uint32_t idx) {
        return (memmap.bitmap[idx / 32] >> (idx % 32)) & 1u;
}

/**
 * @fn memmap_init
 * @brief 指定された範囲の物理メモリフレームを管理対象として初期化する
 * @param start 管理開始アドレス（物理アドレス）
 * @param end 管理終了アドレス（物理アドレス、end-1まで管理）
 */
void memmap_init(uint32_t start, uint32_t end) {
        // フレーム数計算
        uint32_t start_frame = start / FRAME_SIZE;
        uint32_t end_frame = (end + FRAME_SIZE - 1) / FRAME_SIZE; // round up
        uint32_t count = 0;

        if (end <= start) {
                return;
        }

        count = end_frame - start_frame;
        if (count > MAX_FRAMES) {
                // 管理可能数を超える場合はトリムする
                count = MAX_FRAMES;
        }

        // memmap構造体へ設定
        memmap.start_addr = start;
        memmap.end_addr = end;
        memmap.start_frame = start_frame;
        memmap.frames = count;
        memmap.max_frames = MAX_FRAMES;
        memmap.bitmap = bitmap;

        // ビットマップをクリア（0 = free）
        {
                uint32_t flags = 0;
                extern void spin_lock_irqsave(volatile uint32_t *lock, uint32_t *flagsptr);
                extern void spin_unlock_irqrestore(volatile uint32_t *lock, uint32_t flags);
                spin_lock_irqsave(&memmap_lock_storage, &flags);
                for (uint32_t i = 0; i < (memmap.frames + 31) / 32; ++i) {
                        memmap.bitmap[i] = 0;
                }
                spin_unlock_irqrestore(&memmap_lock_storage, flags);
        }

        printk("MemoryMap initialized: frames=%u start_frame=%u\n", (unsigned int)memmap.frames, (unsigned int)memmap.start_frame);
}

/**
 * @fn alloc_frame
 * @brief 空いているフレームを1つ割り当てて、そのアドレスを返す
 * @return 割り当てたフレームのアドレス。空きがなければNULL
 */
void* alloc_frame(void) {
        if (memmap.frames == 0) {
                return NULL;
        }

        uint32_t flags = 0;
        extern void spin_lock_irqsave(volatile uint32_t *lock, uint32_t *flagsptr);
        extern void spin_unlock_irqrestore(volatile uint32_t *lock, uint32_t flags);
        spin_lock_irqsave(&memmap_lock_storage, &flags);
        for (uint32_t i = 0; i < memmap.frames; ++i) {
                if (!bitmap_test(i)) {
                        bitmap_set(i);
                        uint32_t frame_no = memmap.start_frame + i;
                        void* addr = (void*)(frame_no * FRAME_SIZE);
                        spin_unlock_irqrestore(&memmap_lock_storage, flags);
                        return addr;
                }
        }
        spin_unlock_irqrestore(&memmap_lock_storage, flags);

        return NULL; // 空き無し
}

/**
 * @fn free_frame
 * @brief 指定したフレームアドレスを解放する
 * @param addr 解放するフレームのアドレス
 */
void free_frame(void* addr) {
        if (addr == NULL) {
                return;
        }

        uint32_t a = (uint32_t)addr;
        if (a % FRAME_SIZE != 0) {
                // 境界不正
                printk("MemoryMap: border is invalid: %d", a);
                return;
        }

        uint32_t frame_no = a / FRAME_SIZE;
        if (frame_no < memmap.start_frame) {
                return;
        }

        uint32_t idx = frame_no - memmap.start_frame;
        if (idx >= memmap.frames) {
                return;
        }

        {
                uint32_t flags = 0;
                extern void spin_lock_irqsave(volatile uint32_t *lock, uint32_t *flagsptr);
                extern void spin_unlock_irqrestore(volatile uint32_t *lock, uint32_t flags);
                spin_lock_irqsave(&memmap_lock_storage, &flags);
                bitmap_clear(idx);
                spin_unlock_irqrestore(&memmap_lock_storage, flags);
        }
}

/**
 * @fn frame_count
 * @brief 管理しているフレーム数を返す
 * @return 管理しているフレーム数
 */
uint32_t frame_count(void) {
        uint32_t f = 0;
        uint32_t flags = 0;
        extern void spin_lock_irqsave(volatile uint32_t *lock, uint32_t *flagsptr);
        extern void spin_unlock_irqrestore(volatile uint32_t *lock, uint32_t flags);
        spin_lock_irqsave(&memmap_lock_storage, &flags);
        f = memmap.frames;
        spin_unlock_irqrestore(&memmap_lock_storage, flags);
        return f;
}


/**
 * @fn memmap_reserve
 * @brief メモリ領域を予約する
 * @param start 予約する領域の開始地点
 * @param end 予約する領域の終了地点
 */
void memmap_reserve(uint32_t start, uint32_t end) {
        if (memmap.frames == 0) return;

        uint32_t start_frame = start / FRAME_SIZE;
        uint32_t end_frame = (end + FRAME_SIZE - 1) / FRAME_SIZE;

        // 範囲を管理領域の相対インデックスに変換
        if (end_frame <= memmap.start_frame) return;
        if (start_frame >= memmap.start_frame + memmap.frames) return;

        uint32_t s = (start_frame < memmap.start_frame) ? 0 : (start_frame - memmap.start_frame);
        uint32_t e = (end_frame > memmap.start_frame + memmap.frames) ? memmap.frames : (end_frame - memmap.start_frame);

        {
                uint32_t flags = 0;
                extern void spin_lock_irqsave(volatile uint32_t *lock, uint32_t *flagsptr);
                extern void spin_unlock_irqrestore(volatile uint32_t *lock, uint32_t flags);
                spin_lock_irqsave(&memmap_lock_storage, &flags);
                for (uint32_t i = s; i < e; ++i) {
                        bitmap_set(i);
                }
                spin_unlock_irqrestore(&memmap_lock_storage, flags);
        }
}

/**
 * @fn memmap_get
 * @brief メモリマップ構造体を返す（だけ）
 */
const memmap_t* memmap_get(void) {
        return &memmap;
}
