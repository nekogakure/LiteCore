/**
 * @file kernel/panic.h
 * @brief Kernel panic and error handling functions
 * @details This file defines functions for handling kernel panics and printing error messages.
 * It includes the necessary headers and function declarations for panic handling.
 */

#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include <stdarg.h>

/**
 * @brief kernel panic
 * @param fmt format string
 */
void kernel_panic(const char *fmt, ...);

/**
 * @brief print an err message without panic
 * @param fmt format string
 */
void kernel_print_error(const char *fmt, ...);

#endif /* KERNEL_PANIC_H */
