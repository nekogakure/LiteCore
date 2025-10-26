#include <tests/define.h>

#ifdef MEM_TEST

#include <config.h>
#include <mem/manager.h>
#include <mem/map.h>
#include <console.h>

/**
 * @fn mem_test
 * @brief メモリのテスト
 */
void mem_test() {
        // メモリマップの表示
        const memmap_t* mm = memmap_get();
        if (mm) {
                printk("memmap: start=%x end=%x frames=%d start_frame=%d\n",
                       (unsigned int)mm->start_addr, (unsigned int)mm->end_addr,
                       (unsigned int)mm->frames, (unsigned int)mm->start_frame);
        }

        // 空き判定テスト
        if (mem_has_space(MEM_TYPE_FRAME, FRAME_SIZE)) {
                printk("frame: at least 1 frame available\n");
        } else {
                printk("frame: no single frame available\n");
        }

        if (mem_has_space(MEM_TYPE_FRAME, FRAME_SIZE * 8)) {
                printk("frame: at least 8 contiguous frames available\n");
        } else {
                printk("frame: less than 8 contiguous frames available\n");
        }

        if (mem_has_space(MEM_TYPE_HEAP, 128)) {
                printk("heap: at least 128 bytes available\n");
        } else {
                printk("heap: less than 128 bytes available\n");
        }

        // 実際に割当ててみる
        void* frame = alloc_frame();
        if (frame) {
                printk("alloc_frame: %x\n", (unsigned int)frame);
                free_frame(frame);
                printk("free_frame called\n");
        } else {
                printk("alloc_frame failed\n");
        }

        void* p = kmalloc(128);
        if (p) {
                printk("kmalloc returned: %x\n", (unsigned int)p);
                kfree(p);
                printk("kfree called\n");
        } else {
                printk("kmalloc failed\n");
        }

        return;
}

#endif /* MEM_TEST */