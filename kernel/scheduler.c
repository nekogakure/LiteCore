/**
 * @file scheduler.c
 * @brief Task scheduler implementation
 * @details
 * Implements task scheduling operations
 * @author nekogakure
 */

#include "scheduler.h"
#include "console.h"

void init_scheduler(void) {
    // Temporary implementation
    console_puts("Task scheduler initialized\n");
}

void start_scheduler(void) {
    // Temporary implementation
    console_puts("Task scheduler started\n");
    while(1) {
        __asm__ volatile("hlt");
    }
}
