/**
 * @file console.c
 * @brief Console output implementation
 * @details
 * Implementation of basic console output functions using VGA text mode.
 * @author nekogakure
 */

#include "console.h"

/** @brief VGA text buffer address */
#define VGA_BUFFER 0xB8000

/** @brief Console width in characters */
#define CONSOLE_WIDTH 80

/** @brief Console height in characters */
#define CONSOLE_HEIGHT 25

/** @brief Current cursor position X */
static int cursor_x = 0;

/** @brief Current cursor position Y */
static int cursor_y = 0;

/** @brief Current text color */
static uint8_t text_color = 0x0F; // White on black

/**
 * @brief Create a VGA entry from character and color
 * @param c Character to display
 * @param color Color attribute
 * @return Combined VGA entry
 */
static uint16_t make_vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

void init_console(void) {
    console_clear();
}

void console_clear(void) {
    uint16_t *buffer = (uint16_t*)VGA_BUFFER;
    uint16_t blank = make_vga_entry(' ', text_color);
    
    for (int i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        buffer[i] = blank;
    }
    
    cursor_x = 0;
    cursor_y = 0;
}

void console_puts(const char *str) {
    uint16_t *buffer = (uint16_t*)VGA_BUFFER;
    
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            const int index = cursor_y * CONSOLE_WIDTH + cursor_x;
            buffer[index] = make_vga_entry(*str, text_color);
            cursor_x++;
        }
        
        if (cursor_x >= CONSOLE_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
        
        if (cursor_y >= CONSOLE_HEIGHT) {
            // Scroll screen
            for (int i = 0; i < (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH; i++) {
                buffer[i] = buffer[i + CONSOLE_WIDTH];
            }
            
            // Clear last line
            for (int i = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
                 i < CONSOLE_HEIGHT * CONSOLE_WIDTH; i++) {
                buffer[i] = make_vga_entry(' ', text_color);
            }
            
            cursor_y = CONSOLE_HEIGHT - 1;
        }
        
        str++;
    }
}

void console_set_color(enum vga_color fg, enum vga_color bg) {
    text_color = (bg << 4) | (fg & 0x0F);
}
