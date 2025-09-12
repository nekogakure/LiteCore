/**
 * @file idt.h
 * @brief Interrupt Descriptor Table definitions
 * @details
 * Provides definitions and initialization functions for interrupts and exception handlers.
 * @author nekogakure
 * @version 0.1
 * @date 2025-09-12
 */

#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>

/** @defgroup idt IDT Management
 *  @brief Interrupt Descriptor Table management functions
 *  @{
 */

/**
 * @brief IDT entry structure
 */
struct idt_entry {
    uint16_t base_low;     /**< Handler address bits 0-15 */
    uint16_t selector;     /**< Code segment selector */
    uint8_t  ist;          /**< Interrupt Stack Table (lower 3 bits used) */
    uint8_t  flags;        /**< Type and attribute flags */
    uint16_t base_mid;     /**< Handler address bits 16-31 */
    uint32_t base_high;    /**< Handler address bits 32-63 */
    uint32_t reserved;     /**< Reserved */
} __attribute__((packed));

/**
 * @brief IDTR structure
 */
struct idt_ptr {
    uint16_t limit;        /**< Size of IDT - 1 */
    uint64_t base;         /**< Base address of IDT */
} __attribute__((packed));

/**
 * @brief Interrupt handler function type
 */
typedef void (*interrupt_handler_t)(void);

/**
 * @brief Set an interrupt handler
 * @param n Interrupt number
 * @param handler Handler function
 */
void idt_set_gate(uint8_t n, interrupt_handler_t handler);

/** @} */ /* end of idt group */

#endif /* _IDT_H */
