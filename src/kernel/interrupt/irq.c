#include <util/config.h>

/**
 * @fn irq_save
 * Save RFLAGS and disable interrupts.
 */
uint64_t irq_save(void) {
	uint64_t flags;
	asm volatile("pushfq\n\t"
		     "pop %0\n\t"
		     : "=r"(flags)
		     :
		     : "memory");
	asm volatile("cli" ::: "memory");
	return flags;
}

/**
 * @fn irq_restore
 * Restore RFLAGS.
 */
void irq_restore(uint64_t flags) {
	asm volatile("push %0\n\t"
		     "popfq\n\t"
		     :
		     : "r"(flags)
		     : "memory");
}