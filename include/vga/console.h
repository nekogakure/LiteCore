/**
 * @file console.h
 * @brief Basic console output functions
 * @details
 * Provides functions for basic text output to the console
 * using VGA text mode.
 * @author nekogakure
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <stdint.h>
#include <stdarg.h>

/** @brief VGA color codes */
enum vga_color {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN = 14,
    VGA_WHITE = 15,
};

/**
 * @brief Initialize the console
 * @details Sets up the VGA text mode and clears the screen
 */
void init_console(void);

/**
 * @brief Write a string to the console
 * @param str Null-terminated string to write
 */
void console_puts(const char *str);

/**
 * @brief print to the console. Like printf but without formatting.
 * @param format Null-terminated string to write
 * @return Number of characters written
 */
void printk(const char* fmt, ...);


/**
 * @brief Set the console colors
 * @param fg Foreground color
 * @param bg Background color
 */
void console_set_color(enum vga_color fg, enum vga_color bg);

/**
 * @brief Clear the console screen
 */
void console_clear(void);

#endif /* _CONSOLE_H */
