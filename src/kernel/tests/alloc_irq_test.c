#include <tests/define.h>

#ifdef ALLOC_IRQ_TEST

#include <config.h>
#include <util/console.h>
#include <interrupt/irq.h>
#include <mem/map.h>

static void irq_alloc_handler(uint32_t payload, void *ctx) {
	(void)ctx;
	(void)payload;
	void *a = alloc_frame();
	if (a) {
		printk("alloc_irq: alloc_frame -> %x\n", (unsigned)a);
		free_frame(a);
		printk("alloc_irq: free_frame done\n");
	} else {
		printk("alloc_irq: alloc_frame failed\n");
	}
}

void alloc_irq_test() {
	/* register handler to IRQ 2 (arbitrary) */
	interrupt_register(2, irq_alloc_handler, NULL);

	/* simulate IRQ 2 events */
	for (int i = 0; i < 4; ++i) {
		interrupt_raise((2u << 16) | (uint32_t)i);
	}

	/* dispatch queued events synchronously */
	interrupt_dispatch_all();
}

#endif /* ALLOC_IRQ_TEST */
