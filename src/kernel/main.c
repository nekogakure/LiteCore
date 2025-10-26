#include <config.h>
#include <console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <mem/map.h>
#include <mem/manager.h>

#include <tests/define.h>

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

        memmap_reserve(heap_start, heap_end);

        #ifdef MEM_TEST
        mem_test();
        #endif

        new_line();


        keyboard_init();

        while(1) {
                keyboard_poll();
        }
}