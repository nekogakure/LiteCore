#include <config.h>
#include <console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <util/io.h>
#include <mem/map.h>
#include <mem/manager.h>

/**
 * @fn mem_test
 * @brief メモリのテスト
 */
void mem_test() {
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

/**
 * @fn kmain
 * @brief LiteCoreのメイン関数（kernel_entryより）
 */
void kmain() {
        console_init();

        printk("Welcome to Litecore kernel!\n");
        new_line();

        memmap_init(0x100000, 0x500000); // 1MB - 5MB

        extern uint32_t __end;
        uint32_t heap_start = (uint32_t)&__end;
        uint32_t heap_end = heap_start + 0x10000; // 64KB
        mem_init(heap_start, heap_end);

        mem_test();

        new_line();


        keyboard_init();

        while(1) {
                keyboard_poll();
        }
}