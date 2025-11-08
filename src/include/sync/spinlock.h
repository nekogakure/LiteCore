#ifndef _SYNC_SPINLOCK_H
#define _SYNC_SPINLOCK_H

#include <util/config.h>
#include <stdint.h>

typedef struct spinlock {
	volatile uint32_t lock;
} spinlock_t;

void spin_lock_irqsave(spinlock_t *lock, uint32_t *flags);
void spin_unlock_irqrestore(spinlock_t *lock, uint32_t flags);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif /* _SYNC_SPINLOCK_H */
