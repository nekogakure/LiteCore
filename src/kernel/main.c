#include <config.h>
#include <console.h>
#include <device/pci.h>

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

        while(1) {
                __asm__ volatile ("cli");
                __asm__ volatile ("hlt");
        }
}