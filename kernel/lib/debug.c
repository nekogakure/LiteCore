/**
 * @file debug.c
 * @brief Debug port output implementation
 * @details
 * Implements simple debugging via port 0xE9 (QEMU debug port)
 * @author nekogakure
 */

#include "lib/debug.h"
#include <stdint.h>

/**
 * @brief Debug port address (0xE9 works in QEMU)
 */
#define DEBUG_PORT 0xE9

/**
 * @brief Write a byte to a port
 * @param port Port address
 * @param value Value to write
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Output a character to the debug port
 * @param c Character to output
 */
void debug_putchar(char c) {
    outb(DEBUG_PORT, c);
}

/**
 * @brief Output a string to the debug port
 * @param s String to output
 */
void debug_print(const char *s) {
    while (*s) {
        debug_putchar(*s++);
    }
}
