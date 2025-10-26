#include <config.h>
#include <console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <mem/map.h>
#include <mem/manager.h>

#include <tests/define.h>
#include <tests/run.h>

/**
 * @fn memory_init
 * @brief メモリマップなどを初期化します
 */
void memory_init() {
        memmap_init(0x100000, 0x500000); // 1MB - 5MB

        extern uint32_t __end;
        memmap_t *mm = memmap_get();
        uint32_t base_end = (uint32_t)&__end;
        uint32_t bitmap_end = base_end;
        // mm、mm->bitmap、mm->max_framesが有効か確認
        if (mm && mm->bitmap && mm->max_frames) {
                // bitmapはmemmap内でバイト配列として格納されているので、その終了アドレスを計算
                uint32_t bitmap_bytes = (mm->max_frames + 7) / 8;
                bitmap_end = (uint32_t)mm->bitmap + bitmap_bytes;
        }
        // kernelの終了アドレスとbitmapの終了アドレスの大きい方を選択
        uint32_t heap_start = (base_end > bitmap_end) ? base_end : bitmap_end;
        // 4KBページ境界に切り上げてアライン
        heap_start = (heap_start + 0x0FFF) & ~0x0FFF;
        uint32_t heap_end = heap_start + 0x10000; // 64KBのヒープ領域
        mem_init(heap_start, heap_end);
        memmap_reserve(heap_start, heap_end);
}

/**
 * @fn kmain
 * @brief LiteCoreのメイン関数（kernel_entryより）
 */
void kmain() {
        console_init();

        printk("Welcome to Litecore kernel!\n\n");

        memory_init();
        keyboard_init();

        run_test();

        while(1) {
                kloop();
        }
}

/**
 * @fn kloop
 * @brief kmainの処理が終了した後常に動き続ける処理
 */
void kloop() {
        keyboard_poll();
}