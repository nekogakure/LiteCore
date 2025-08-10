/**
 * @file kernel/arch/arch.h
 * @brief archtecture-specific definitions and functions
 * @details This file contains architecture-specific definitions and functions
 * for the LiteCore kernel, including GDT setup, paging, and mode switching.
 */

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

/**
 * @brief define a GDT entry structre
 */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

/**
 * @brief define a GDT pointer structure
 * @details This structure is used to load the GDT using the LGDT instruction.
 */
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/**
 * @brief init GDT
 */
void gdt_init();

/**
 * @brief change to protected mode
 */
void switch_to_protected_mode();

/**
 * @brief 64bit mode preparation
 */
void prepare_long_mode();

/**
 * @brief change to 64bit mode
 */
void switch_to_long_mode();

/**
 * @brief assembly function to switch to long mode
 * @details this function is defined in assembly to perform the necessary operations
 */
void switch_to_long_mode_asm();

#endif // ARCH_H
