/**
 * @file util/interrupt_handler.h
 * @brief Common interrupt handler declarations
 * @details
 * Defines common functions for interrupt handling.
 * @author nekogakure
 * @version 0.1
 * @date 2025-09-12
 */

#ifndef _INTERRUPT_HANDLER_H
#define _INTERRUPT_HANDLER_H

#include <stdint.h>

/**
 * @brief Common interrupt handler
 * @param int_no Interrupt number
 * @details Common handler function called from assembly stubs
 */
void handle_interrupt(uint64_t int_no);

#endif /* _INTERRUPT_HANDLER_H */
