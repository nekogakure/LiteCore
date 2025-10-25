#include <config.h>
#include <console.h>
#include <device/pci.h>
#include <device/keyboard.h>

/**
 * @fn kmain
 * @brief LiteCoreのメイン関数
 */
void kmain() {
        console_init();

        printk("Welcome to Litecore kernel!\n");
        printk("printk test: %d\n", 123456);
        new_line();

        pci_enumerate();

        keyboard_init();

        // Kernel main loop
        while(1) {
                keyboard_poll();
                for (volatile int i = 0; i < 10000; ++i) {
                        __asm__ volatile ("nop");
                }
        }
}