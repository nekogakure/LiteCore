#include <config.h>
#include <util/io.h>

/**
 * @fn kmain()
 * @brief LiteCoreのメイン関数
 */
void kmain() {
        clear_screen();
        printk("Welcome to Litecore kernel!");

        while(1) {
                __asm__ volatile ("cli");
                __asm__ volatile ("hlt");
        }
}