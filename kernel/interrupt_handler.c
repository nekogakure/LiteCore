/**
 * @file interrupt_handler.c
 * @brief Common interrupt handler implementation
 * @details
 * Implements common functions for interrupt handling.
 * @author nekogakure
 * @version 0.1
 * @date 2025-09-12
 */

#include "interrupt_handler.h"
#include "console.h"

/**
 * @brief Common interrupt handler
 * @param int_no Interrupt number
 */
void handle_interrupt(uint64_t int_no) {
    // Call the appropriate handler according to the interrupt number
    switch(int_no) {
        case 0:
            console_puts("Divide by zero exception\n");
            break;
        case 13:
            console_puts("General protection fault\n");
            break;
        case 14:
            console_puts("Page fault\n");
            break;
        default:
            console_puts("Unknown interrupt\n");
            break;
    }
}
