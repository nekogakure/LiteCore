#ifndef _INTERRUPT_IRQ_H
#define _INTERRUPT_IRQ_H

#include <config.h>

uint32_t irq_save(void);
void irq_restore(uint32_t flags);

int interrupt_register(uint32_t irq, void (*handler)(uint32_t, void*), void* ctx);
int interrupt_unregister(uint32_t irq);
int interrupt_raise(uint32_t event);
int interrupt_dispatch_one(void);
void interrupt_dispatch_all(void);
void interrupt_init(void);

#endif /* _INTERRUPT_IRQ_H */