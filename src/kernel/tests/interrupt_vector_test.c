#include <tests/define.h>

#ifdef INTERRUPT_VECTOR_TEST

#include <util/config.h>
#include <util/console.h>
#include <interrupt/irq.h>
#include <stdint.h>

extern void irq_handler_c(uint32_t vec);

static void vec_test_handler(uint32_t payload, void *ctx) {
	uint32_t expected_irq = (uint32_t)(uintptr_t)ctx;
	(void)payload;
	printk("vector test: handler called for irq=%u (expected=%u)\n",
	       (unsigned)expected_irq, (unsigned)expected_irq);
}

void interrupt_vector_test() {
	for (uint32_t irq = 0; irq < 4; ++irq) {
		interrupt_register(irq, vec_test_handler,
				   (void *)(uintptr_t)irq);
	}

	for (uint32_t irq = 0; irq < 4; ++irq) {
		uint32_t vec = 32 + irq;
		printk("triggering vector %u (irq %u)\n", (unsigned)vec,
		       (unsigned)irq);
		irq_handler_c(vec);
	}

	interrupt_dispatch_all();
}

#endif /* INTERRUPT_VECTOR_TEST */
