/**
 * @file interrupt.c
 * @brief Interrupt handlers implementation
 * @details
 * Provides implementations of interrupt handlers.
 * @author nekogakure
 * @version 0.1
 * @date 2025-09-12
 */

#include "util/interrupt.h"
#include "vga/console.h"

void interrupt_default_handler(void) {
    console_puts("Unknown interrupt occurred!\n");
    while(1) {
        __asm__ volatile("hlt");
    }
}

void interrupt_divide_by_zero(void) {
    console_puts("Divide by zero error!\n");
    while(1) {
        __asm__ volatile("hlt");
    }
}

void interrupt_general_protection(void) {
    console_puts("General Protection Fault!\n");
    while(1) {
        __asm__ volatile("hlt");
    }
}

void interrupt_page_fault(void) {
    console_puts("Page Fault!\n");
    while(1) {
        __asm__ volatile("hlt");
    }
}
