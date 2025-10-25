#include <config.h>
#include <console.h>

/**
 * @fn kmain()
 * @brief LiteCoreのメイン関数
 */
void kmain() {
        console_init();

        printk("Welcome to Litecore kernel!\n");
        printk("printk test: %d", 123456);
        
        while(1) {
                __asm__ volatile ("cli");
                __asm__ volatile ("hlt");
        }
}