/**
 * @file string.c
 * @brief Implementation of string manipulation functions
 * @details
 * Implements string manipulation functions for kernel mode.
 */

#include "string.h"
#include <stdarg.h>

/**
 * @brief Converts a value to a hexadecimal string
 * @param value The value to convert
 * @param str Output buffer
 * @param is_upper Whether to use uppercase letters
 * @return Number of characters written
 */
static int to_hex(unsigned long value, char *str, int is_upper) {
    const char *digits = is_upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[32];
    char *p = tmp;
    int len = 0;

    do {
        *p++ = digits[value & 0xf];
        value >>= 4;
        len++;
    } while (value);

    while (len--) {
        *str++ = *--p;
    }

    return p - tmp;
}

/**
 * @brief Converts an integer to a decimal string
 * @param value The value to convert
 * @param str Output buffer
 * @param is_signed Whether to treat as a signed integer
 * @return Number of characters written
 */
static int to_decimal(long value, char *str, int is_signed) {
    char tmp[32];
    char *p = tmp;
    int len = 0;
    int negative = 0;

    if (is_signed && value < 0) {
        negative = 1;
        value = -value;
    }

    do {
        *p++ = '0' + (value % 10);
        value /= 10;
        len++;
    } while (value);

    if (negative) {
        *str++ = '-';
    }

    while (len--) {
        *str++ = *--p;
    }

    return p - tmp + (negative ? 1 : 0);
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list args) {
    if (!size) return 0;
    
    char *start = str;
    char *end = str + size - 1;  // Leave room for null terminator
    
    while (*fmt && str < end) {
        if (*fmt != '%') {
            *str++ = *fmt++;
            continue;
        }
        
        fmt++;  // Skip '%'
        
        // Handle format specifiers
        switch (*fmt) {
            case 's': {
                const char *s = va_arg(args, const char*);
                while (*s && str < end) {
                    *str++ = *s++;
                }
                break;
            }
            case 'd':
            case 'i': {
                int value = va_arg(args, int);
                str += to_decimal(value, str, 1);
                break;
            }
            case 'u': {
                unsigned int value = va_arg(args, unsigned int);
                str += to_decimal(value, str, 0);
                break;
            }
            case 'x': {
                unsigned int value = va_arg(args, unsigned int);
                str += to_hex(value, str, 0);
                break;
            }
            case 'X': {
                unsigned int value = va_arg(args, unsigned int);
                str += to_hex(value, str, 1);
                break;
            }
            case 'p': {
                void *value = va_arg(args, void*);
                *str++ = '0';
                *str++ = 'x';
                str += to_hex((unsigned long)value, str, 0);
                break;
            }
            case '%':
                *str++ = '%';
                break;
            default:
                // Unsupported format specifier - ignore
                break;
        }
        fmt++;
    }
    
    *str = '\0';
    return str - start;
}
