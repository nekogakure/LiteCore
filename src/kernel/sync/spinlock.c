#include <sync/spinlock.h>
#include <interrupt/irq.h>

static inline uint32_t xchg_u32(volatile uint32_t *ptr, uint32_t newval) {
    uint32_t old;
    asm volatile("xchgl %0, %1"
                 : "=r" (old), "+m" (*ptr)
                 : "0" (newval)
                 : "memory");
    return old;
}

/**
 * @brief スピンロックを取得します。
 * @param lock 取得対象のspinlock_t構造体へのポインタ
 */
void spin_lock(spinlock_t *lock) {
    while (xchg_u32(&lock->lock, 1) != 0) {
        /* busy wait */
    }
}

/**
 * @fn spin_unlock
 * @brief スピンロックを解放する
 * @param lock 解放するスピンロックへのポインタ
 */
void spin_unlock(spinlock_t *lock) {
        asm volatile("movl $0, %0" : "+m" (lock->lock) :: "memory");
}

/**
 * @fn spin_lock_irqsave
 * @brief スピンロックを取得し、割り込みフラグを保存する
 * @param lock 取得するスピンロックへのポインタ
 * @param flags 割り込みフラグを保存するためのポインタ
 */
void spin_lock_irqsave(spinlock_t *lock, uint32_t *flags) {
    *flags = irq_save();
    spin_lock(lock);
}


/**
 * @fn spin_unlock_irqrestore
 * @brief スピンロックを解放し、割り込みフラグを復元
 * @param lock 解放するスピンロックへのポインタ
 * @param flags 復元する割り込みフラグ
 */
void spin_unlock_irqrestore(spinlock_t *lock, uint32_t flags) {
    spin_unlock(lock);
    irq_restore(flags);
}
