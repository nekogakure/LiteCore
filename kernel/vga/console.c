/**
 * @file console.c
 * @brief Console output implementation
 * @details
 * Implementation of basic console output functions using VGA text mode.
 * @author nekogakure
 */

#include <stdarg.h>
#include "lib/string.h"
#include "vga/console.h"
#include "config.h"

/** 
 * @brief VGA text buffer address 
 * 0xB8000 is the standard VGA text buffer address in physical memory
 */
#define VGA_BUFFER 0xB8000

/** @brief Console width in characters */
#define CONSOLE_WIDTH 80

/** @brief Console height in characters */
#define CONSOLE_HEIGHT 25

/** @brief Buffer for printk */
#define PRINTK_BUFFER_SIZE 1024

/** @brief Current cursor position X */
static int cursor_x = 0;

/** @brief Current cursor position Y */
static int cursor_y = 0;

/** @brief Current text color */
static uint8_t text_color = 0x0F; // White on black

/** @brief Console initialization flag */
static int console_initialized = 0;

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
    // Force reset all static variables to known values
    cursor_x = 0;
    cursor_y = 0;
    text_color = 0x0F; // White on black
    console_initialized = 1;
    
    // Clear the entire VGA buffer to be sure
    volatile uint16_t* buffer = (volatile uint16_t*)VGA_BUFFER;
    for (int i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        buffer[i] = 0x0F20; // White space on black
    }
}

void console_putchar(char c) {
    // Check if console is initialized
    if (!console_initialized) {
        init_console();
    }
    
    // Cast to a 32-bit pointer for 32-bit mode
    volatile uint16_t* buffer = (volatile uint16_t*)VGA_BUFFER;

    // Aggressive safety bounds check
    if (cursor_x < 0 || cursor_x >= CONSOLE_WIDTH) cursor_x = 0;
    if (cursor_y < 0 || cursor_y >= CONSOLE_HEIGHT) cursor_y = 0;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        // Ensure memory writes are performed correctly
        uint16_t entry = make_vga_entry(c, text_color);
        int index = cursor_y * CONSOLE_WIDTH + cursor_x;

        // Double check the index
        if (index >= 0 && index < CONSOLE_WIDTH * CONSOLE_HEIGHT) {
            buffer[index] = entry;
        } else {
            // Emergency reset if index is out of bounds
            cursor_x = cursor_y = 0;
            index = 0;
            buffer[index] = entry;
        }
        cursor_x++;
    }

    if (cursor_x >= CONSOLE_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= CONSOLE_HEIGHT) {
        // Scroll the screen
        for (int y = 0; y < CONSOLE_HEIGHT - 1; y++) {
            for (int x = 0; x < CONSOLE_WIDTH; x++) {
                buffer[y * CONSOLE_WIDTH + x] = buffer[(y + 1) * CONSOLE_WIDTH + x];
            }
        }
        // Clear the last line
        for (int x = 0; x < CONSOLE_WIDTH; x++) {
            buffer[(CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH + x] = make_vga_entry(' ', text_color);
        }
        cursor_y = CONSOLE_HEIGHT - 1;
    }
}

void console_write(const char* str) {
    while (*str) {
        console_putchar(*str++);
    }
}

void printk(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    const char* p = fmt;
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;
            switch (*p) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (str) {
                        console_puts(str);
                    } else {
                        console_puts("(null)");
                    }
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    // Simple integer to string conversion
                    if (num == 0) {
                        console_putchar('0');
                    } else {
                        char buffer[12]; // enough for 32-bit int
                        int i = 0;
                        int is_negative = 0;
                        
                        if (num < 0) {
                            is_negative = 1;
                            num = -num;
                        }
                        
                        while (num > 0) {
                            buffer[i++] = '0' + (num % 10);
                            num /= 10;
                        }
                        
                        if (is_negative) {
                            console_putchar('-');
                        }
                        
                        // Print digits in reverse order
                        for (int j = i - 1; j >= 0; j--) {
                            console_putchar(buffer[j]);
                        }
                    }
                    break;
                }
                case '%':
                    console_putchar('%');
                    break;
                default:
                    console_putchar('%');
                    console_putchar(*p);
                    break;
            }
        } else {
            console_putchar(*p);
        }
        p++;
    }
    
    va_end(args);
}

void console_clear(void) {
    volatile uint16_t* buffer = (volatile uint16_t*)VGA_BUFFER;
    uint16_t blank = make_vga_entry(' ', text_color);
    
    for (int i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        buffer[i] = blank;
    }
    
    cursor_x = cursor_y = 0;
}

void console_puts(const char *str) {
    volatile uint16_t *buffer = (volatile uint16_t*)VGA_BUFFER;
    
    while (*str) {
        if (cursor_x < 0) cursor_x = 0;
        if (cursor_y < 0) cursor_y = 0;
        if (cursor_x >= CONSOLE_WIDTH) cursor_x = CONSOLE_WIDTH - 1;
        if (cursor_y >= CONSOLE_HEIGHT) cursor_y = CONSOLE_HEIGHT - 1;
        
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            const int index = cursor_y * CONSOLE_WIDTH + cursor_x;
            if (index >= 0 && index < CONSOLE_WIDTH * CONSOLE_HEIGHT) {
                buffer[index] = make_vga_entry(*str, text_color);
            }
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
