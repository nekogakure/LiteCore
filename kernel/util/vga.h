#ifndef VGA_H
#define VGA_H
#include "config.h"

// Text color definitions
enum text_color {
    TEXT_COLOR_BLACK = 0,
    TEXT_COLOR_BLUE = 1,
    TEXT_COLOR_GREEN = 2,
    TEXT_COLOR_CYAN = 3,
    TEXT_COLOR_RED = 4,
    TEXT_COLOR_MAGENTA = 5,
    TEXT_COLOR_BROWN = 6,
    TEXT_COLOR_LIGHT_GREY = 7,
    TEXT_COLOR_DARK_GREY = 8,
    TEXT_COLOR_LIGHT_BLUE = 9,
    TEXT_COLOR_LIGHT_GREEN = 10,
    TEXT_COLOR_LIGHT_CYAN = 11,
    TEXT_COLOR_LIGHT_RED = 12,
    TEXT_COLOR_LIGHT_MAGENTA = 13,
    TEXT_COLOR_YELLOW = 14,
    TEXT_COLOR_WHITE = 15,
};

/**
 * @brief Initialize the text mode
 */
void text_init();

/**
 * @brief Clear the text buffer
 */
void text_clear();

/**
 * @brief Print a character to the text buffer
 * @param c Character to print
 */
void text_putchar(char c);

/**
 * @brief Print a string to the text buffer
 * @param str String to print
 */
void printk(const char* str);

/**
 * @brief Print a formatted string to the text buffer (simplified version)
 * @param format Format string
 * @param value Integer value to format
 */
void printk_int(const char* format, int value);

/**
 * @brief Print a formatted hex value to the text buffer
 * @param format Format string
 * @param value Hex value to format
 */
void printk_hex(const char* format, unsigned long long value);

/**
 * @brief Set the text color
 * @param fg Foreground color
 * @param bg Background color
 */
void text_set_color(enum text_color fg, enum text_color bg);

#endif
