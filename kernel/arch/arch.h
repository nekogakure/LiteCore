#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>

/**
 * @brief Initializes the architecture-specific components of the kernel.
 */
void kernel_arch_init();

/**
 * @brief Sets up the paging tables for memory management.
 */
void setup_paging_tables();

/**
 * @brief Checks if the CPU supports long mode (64-bit mode).
 */
int cpu_has_long_mode();

#endif // ARCH_H
