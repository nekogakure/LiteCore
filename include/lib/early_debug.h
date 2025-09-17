/**
 * @file early_debug.h
 * @brief Early boot stage debugging macros
 * @details
 * Simple macros for debugging during early boot before proper output is available
 * @author nekogakure
 */

#ifndef _EARLY_DEBUG_H
#define _EARLY_DEBUG_H

/**
 * @brief Write a single character to VGA buffer at specified position
 * @param c Character to write
 * @param x X position (0-79)
 * @param y Y position (0-24)
 * @param color VGA color attribute
 */
#define EARLY_DEBUG_CHAR(c, x, y, color) \
    do { \
        volatile unsigned short *vga = (volatile unsigned short*)0xB8000; \
        vga[(y) * 80 + (x)] = (c) | ((color) << 8); \
    } while (0)

/**
 * @brief Write a marker character at specified position
 * @param c Character to write
 * @param pos Position (0-79)
 */
#define DEBUG_MARKER(c, pos) EARLY_DEBUG_CHAR(c, pos, 0, 0x0A) // Green on black

/**
 * @brief Simple spin delay function
 * @param count Number of iterations to spin
 */
#define DELAY(count) \
    do { \
        volatile int i; \
        for (i = 0; i < (count); i++) \
            __asm__ volatile("nop"); \
    } while (0)

#endif /* _EARLY_DEBUG_H */