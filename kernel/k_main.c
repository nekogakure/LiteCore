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
    printk("OK\n");
    
    text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    printk("- Memory management: ");
    text_set_color(TEXT_COLOR_GREEN, TEXT_COLOR_BLACK);
    printk("OK\n\n");
    
    memory_print_stats();
    
    // memory mng demo
    text_set_color(TEXT_COLOR_LIGHT_MAGENTA, TEXT_COLOR_BLACK);
    printk("\nTesting memory allocation...\n");
    text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    void* ptr1 = kmalloc(1024);  // 1KB
    printk("Allocated 1KB at 0x");
    printk_hex("%x", (uint64_t)ptr1);
    printk("\n");
    memory_print_stats();
    
    void* ptr2 = kmalloc(4096);  // 4KB
    printk("Allocated 4KB at 0x");
    printk_hex("%x", (uint64_t)ptr2);
    printk("\n");
    memory_print_stats();
    
    printk("Freeing 1KB block...\n");
    kfree(ptr1);
    memory_print_stats();
    
    void* ptr3 = kmalloc(2048);  // 2KB
    printk("Allocated 2KB at 0x");
    printk_hex("%x", (uint64_t)ptr3);
    printk("\n");
    memory_print_stats();
    printk("Freeing all allocated memory...\n");
    kfree(ptr2);
    kfree(ptr3);
    memory_print_stats();
    
    // paging demo
    text_set_color(TEXT_COLOR_LIGHT_MAGENTA, TEXT_COLOR_BLACK);
    printk("\nTesting virtual memory...\n");
    text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    paging_print_info();
    
    // virtual memory demo
    printk("Allocating virtual memory...\n");
    void* vptr1 = valloc(8192); // 2pages
    if (vptr1) {
        printk("Virtual memory allocated at 0x");
        printk_hex("%x", (uint64_t)vptr1);
        printk("\n");
        
        uint8_t* test_ptr = (uint8_t*)vptr1;
        for (int i = 0; i < 8192; i++) {
            test_ptr[i] = (uint8_t)i;
        }
        
        int success = 1;
        for (int i = 0; i < 8192; i++) {
            if (test_ptr[i] != (uint8_t)i) {
                success = 0;
                break;
            }
        }
        
        if (success) {
            text_set_color(TEXT_COLOR_GREEN, TEXT_COLOR_BLACK);
            printk("Virtual memory test: OK\n");
        } else {
            text_set_color(TEXT_COLOR_RED, TEXT_COLOR_BLACK);
            printk("Virtual memory test: FAILED\n");
        }
        
        text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
        
        printk("Freeing virtual memory...\n");
        vfree(vptr1);
    } else {
        text_set_color(TEXT_COLOR_RED, TEXT_COLOR_BLACK);
        printk("Failed to allocate virtual memory\n");
        text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    }
    
    printk("Testing physical memory mapping...\n");
    
    uint8_t* vga_direct = (uint8_t*)vmmap(0xB8000, 4096, PAGE_WRITE);
    if (vga_direct) {
        printk("VGA buffer mapped at 0x");
        printk_hex("%x", (uint64_t)vga_direct);
        printk("\n");
        
        vga_direct[0] = '*';
        vga_direct[1] = 0x1E;
        
        vmunmap(vga_direct, 4096);
        printk("VGA buffer unmapped\n");
    } else {
        text_set_color(TEXT_COLOR_RED, TEXT_COLOR_BLACK);
        printk("Failed to map VGA buffer\n");
        text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    }
    
    paging_print_info();
    
    text_set_color(TEXT_COLOR_LIGHT_CYAN, TEXT_COLOR_BLACK);
    printk("\nKernel is now running. System ready!\n");
    
    kernel_main_loop();
}