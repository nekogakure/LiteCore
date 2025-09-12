/**
 * @file main.c
 * @brief LiteCore kernel main entry point
 * @details
 * This is the main entry point for the LiteCore kernel. It initializes
 * all necessary subsystems and starts the scheduler.
 * @author nekogakure
 */

#include <stdint.h>
#include <stddef.h>
#include "multiboot.h"
#include "vga/console.h"
#include "system.h"
#include "memory.h"
#include "scheduler.h"
#include "config.h"

/**
 * @brief Kernel main entry point
 * @param magic GRUB magic number for verification
 * @param addr Address of the multiboot information structure
 * @return Never returns
 * @note This function is called by the bootloader
 */
void kernel_main(uint32_t magic, uintptr_t addr) {
    // Initialize console for early debug output
    init_console();
    
    set_text_color(VGA_LIGHT_GREEN, VGA_BLACK);
    
    printk("=== LiteCore kernel ===\n");
    printk("version %s\n", CONF_PROJECT_VERSION);
    set_text_color(VGA_LIGHT_GREY, VGA_BLACK);
    
    // Verify multiboot magic number
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        console_puts("Invalid multiboot magic number\n");
        return;
    }

    // Initialize core kernel subsystems
    init_gdt();        // Global Descriptor Table
    init_idt();        // Interrupt Descriptor Table
    init_memory();     // Memory Management
    init_scheduler();  // Task Scheduler
    
    // Start the scheduler
    start_scheduler();
    
    // We should never reach this point
    while(1) {
        __asm__ volatile("hlt");
    }
}
