#include "util/vga.h"
#include "util/config.h"
#include "kernel.h"

void k_main() {
    kernel_init();
    
    text_set_color(TEXT_COLOR_LIGHT_CYAN, TEXT_COLOR_BLACK);
    printk("=================================\n");
    printk("   Welcome to LiteCore kernel!     \n");
    printk("=================================\n\n");
    
    text_set_color(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK);
    printk("LiteCore kernel v");
    printk(KERNEL_VERSION);
    printk("\n");
    printk("Built with love in C and Assembly\n\n");
    
    text_set_color(TEXT_COLOR_YELLOW, TEXT_COLOR_BLACK);
    printk("System Information:\n");
    
    // Features demonstration
    text_set_color(TEXT_COLOR_LIGHT_GREEN, TEXT_COLOR_BLACK);
    printk("Features Demo:\n");
    text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    printk("- Text output: ");
    text_set_color(TEXT_COLOR_GREEN, TEXT_COLOR_BLACK);
    printk("OK\n");
    
    // Color support demonstration
    text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    printk("- Color support: ");
    text_set_color(TEXT_COLOR_RED, TEXT_COLOR_BLACK);
    printk("R");
    text_set_color(TEXT_COLOR_YELLOW, TEXT_COLOR_BLACK);
    printk("A");
    text_set_color(TEXT_COLOR_GREEN, TEXT_COLOR_BLACK);
    printk("I");
    text_set_color(TEXT_COLOR_CYAN, TEXT_COLOR_BLACK);
    printk("N");
    text_set_color(TEXT_COLOR_MAGENTA, TEXT_COLOR_BLACK);
    printk("B");
    text_set_color(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK);
    printk("O");
    text_set_color(TEXT_COLOR_LIGHT_RED, TEXT_COLOR_BLACK);
    printk("W");
    text_set_color(TEXT_COLOR_GREEN, TEXT_COLOR_BLACK);
    printk(" OK\n");
    
    text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    printk("- Cursor management: ");
    text_set_color(TEXT_COLOR_GREEN, TEXT_COLOR_BLACK);
    printk("OK\n\n");
    
    text_set_color(TEXT_COLOR_LIGHT_CYAN, TEXT_COLOR_BLACK);
    printk("Kernel is now running. System ready!\n");
    
    kernel_main_loop();
}