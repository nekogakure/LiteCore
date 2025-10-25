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

static uint32_t managed_frames = 0;
static uint32_t managed_start_frame = 0;

static inline void bitmap_set(uint32_t idx) {
        bitmap[idx / 32] |= (1u << (idx % 32));
}

static inline void bitmap_clear(uint32_t idx) {
        bitmap[idx / 32] &= ~(1u << (idx % 32));
}

static inline int bitmap_test(uint32_t idx) {
        return (bitmap[idx / 32] >> (idx % 32)) & 1u;
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

        managed_frames = count;
        managed_start_frame = start_frame;

        // ビットマップをクリア（0 = free）
        for (uint32_t i = 0; i < (managed_frames + 31) / 32; ++i) {
                bitmap[i] = 0;
        }

        printk("memmap_init: frames=%d start_frame=%d\n", managed_frames, managed_start_frame);
}

/**
 * @fn alloc_frame
 * @brief 空いているフレームを1つ割り当てて、そのアドレスを返す
 * @return 割り当てたフレームのアドレス。空きがなければNULL
 */
void* alloc_frame(void) {
        if (managed_frames == 0) {
                return NULL;
        }

        for (uint32_t i = 0; i < managed_frames; ++i) {
                if (!bitmap_test(i)) {
                        bitmap_set(i);
                        uint32_t frame_no = managed_start_frame + i;
                        void* addr = (void*)(frame_no * FRAME_SIZE);
                        return addr;
                }
        }

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
        if (frame_no < managed_start_frame) {
                return;
        }

        uint32_t idx = frame_no - managed_start_frame;
        if (idx >= managed_frames) {
                return;
        }

        bitmap_clear(idx);
}

/**
 * @fn frame_count
 * @brief 管理しているフレーム数を返す
 * @return 管理しているフレーム数
 */
uint32_t frame_count(void) {
        return managed_frames;
}

/**
 * @fn memmap_reserve
 * @brief 指定した範囲のフレームを予約（使用不可に）する
 * @param start 予約開始アドレス（物理アドレス）
 * @param end 予約終了アドレス（物理アドレス、end-1まで予約）
 */
void memmap_reserve(uint32_t start, uint32_t end) {
        if (managed_frames == 0) return;

        uint32_t start_frame = start / FRAME_SIZE;
        uint32_t end_frame = (end + FRAME_SIZE - 1) / FRAME_SIZE;

        // 範囲を管理領域の相対インデックスに変換
        if (end_frame <= managed_start_frame) return;
        if (start_frame >= managed_start_frame + managed_frames) return;

        uint32_t s = (start_frame < managed_start_frame) ? 0 : (start_frame - managed_start_frame);
        uint32_t e = (end_frame > managed_start_frame + managed_frames) ? managed_frames : (end_frame - managed_start_frame);

        for (uint32_t i = s; i < e; ++i) {
                bitmap_set(i);
        }
}
