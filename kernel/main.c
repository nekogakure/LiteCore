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
#include "lib/serial.h"
#include "lib/debug.h"
#include "lib/early_debug.h"

/**
 * @brief Kernel main entry point
 * @param magic GRUB magic number for verification
 * @param addr Address of the multiboot information structure
 * @return Never returns
 * @note This function is called by the bootloader
 */
void kernel_main(uint32_t magic, uint32_t addr) {
    // Very early boot markers
    DEBUG_MARKER('0', 0); // Mark kernel entry point
    
    // Direct screen write to verify we reached the kernel
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    const char *msg = "K-MAIN";
    for (int i = 0; msg[i]; i++) {
        vga[20 + i] = 0x0F00 | msg[i]; // White on black
    }
    
    DEBUG_MARKER('1', 1); // Mark after initial VGA write
    
    // Use debug port (0xE9) for earliest possible output
    debug_print("DEBUG: Kernel starting...\n");
    debug_print("DEBUG: LiteCore Kernel Version ");
    debug_print(CONF_PROJECT_VERSION);
    debug_print("\n");
    
    DEBUG_MARKER('2', 2); // Mark after debug port writes
    
    init_serial();
    DEBUG_MARKER('3', 3); // Mark after serial init
    
    serial_write_string("=== LiteCore kernel ===\n");
    serial_write_string("Version: ");
    serial_write_string(CONF_PROJECT_VERSION);
    serial_write_string("\n");
    
    DEBUG_MARKER('4', 4); // Mark after serial writes
    
    serial_write_string("Initializing console...\n");
    
    init_console();
    DEBUG_MARKER('5', 5); // Mark after console init
    
    serial_write_string("Console initialized\n");
    
    // Test safe VGA write first
    serial_write_string("Testing VGA output...\n");
    console_putchar('T');
    console_putchar('E');
    console_putchar('S');
    console_putchar('T');
    console_putchar('\n');
    serial_write_string("VGA test completed\n");
    
    // Now try VGA console output
    DEBUG_MARKER('6', 6); // Mark before first printk
    serial_write_string("Calling printk...\n");
    printk("=== LiteCore kernel ===\n");
    
    DEBUG_MARKER('7', 7); // Mark after first printk
    serial_write_string("First printk completed\n");
    printk("version %s\n", CONF_PROJECT_VERSION);
    
    DEBUG_MARKER('8', 8); // Mark after second printk
    
    console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    DEBUG_MARKER('9', 9); // Mark after color set
    
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
