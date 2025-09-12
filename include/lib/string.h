/**
 * @file string.h
 * @brief Header file for string manipulation functions
 * @details
 * Provides string manipulation functions for kernel mode.
 */

#ifndef _STRING_H
#define _STRING_H

#include <stdarg.h>
#include <stddef.h>

/**
 * @brief Generates a formatted string
 * @param str Output buffer
 * @param size Size of the buffer
 * @param fmt Format string
 * @param args Variable argument list
 * @return Number of characters written
 */
int vsnprintf(char *str, size_t size, const char *fmt, va_list args);

#endif /* _STRING_H */