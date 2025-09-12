/**
 * @file interrupt.h
 * @brief Interrupt handlers definitions
 * @details
 * Provides definitions for various interrupt handlers.
 * @author nekogakure
 * @version 0.1
 * @date 2025-09-12
 */

#ifndef _INTERRUPT_H
#define _INTERRUPT_H

/** @defgroup interrupt Interrupt Handlers
 *  @brief Group of interrupt handlers
 *  @{
 */

/**
 * @brief Default interrupt handler
 */
void default_handler(void);

/**
 * @brief Divide-by-zero exception handler
 */
void divide_by_zero_handler(void);

/**
 * @brief General protection fault handler
 */
void general_protection_fault_handler(void);

/**
 * @brief Page fault handler
 */
void page_fault_handler(void);

/** @} */ /* end of interrupt group */

#endif /* _INTERRUPT_H */
