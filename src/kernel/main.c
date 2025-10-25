#include <config.h>
#include <console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <util/io.h>

/**
 * @fn kmain
 * @brief LiteCoreのメイン関数（kernel_entryより）
 */
void kmain() {
        console_init();

        printk("Welcome to Litecore kernel!\n");
        printk("printk test: %d\n", 123456);
        new_line();

        pci_enumerate();
        keyboard_init();

        while(1) {
                keyboard_poll();
        }
}