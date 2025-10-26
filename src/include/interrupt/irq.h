#ifndef _INTERRUPT_IRQ_H
#define _INTERRUPT_IRQ_H

#include <config.h>
#include <stdint.h>

uint32_t irq_save(void);
void irq_restore(uint32_t flags);

#endif /* _INTERRUPT_IRQ_H */
