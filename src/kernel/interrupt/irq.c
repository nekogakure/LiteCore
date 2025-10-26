 #include <config.h>

 /**
  * @fn irq_save
  * Save EFLAGS and disable interrupts.
  */
 uint32_t irq_save(void) {
        uint32_t flags;
        asm volatile(
                "pushf\n\t"
                "pop %0\n\t"
                : "=r" (flags)
                :
                : "memory"
        );
        asm volatile("cli" ::: "memory");
        return flags;
 }

 /**
  * @fn irq_restore
  * Restore EFLAGS.
  */
 void irq_restore(uint32_t flags) {
        asm volatile(
                "push %0\n\t"
                "popf\n\t"
                :
                : "r" (flags)
                : "memory"
        );
 }
