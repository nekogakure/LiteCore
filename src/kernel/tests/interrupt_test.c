#include <tests/define.h>

#ifdef INTERRUPT_TEST

#include <config.h>
#include <util/console.h>
#include <interrupt/irq.h>

static void test_handler(uint32_t payload, void *ctx) {
	(void)ctx;
	printk("interrupt handler called: payload=%u\n", (unsigned)payload);
}

void interrupt_test() {
	/* register handler for irq 1 */
	interrupt_register(1, test_handler, NULL);

	/* push a few events: encode irq=1, payload=42/7 */
	interrupt_raise((1u << 16) | 42u);
	interrupt_raise((1u << 16) | 7u);

	/* dispatch and observe printk output */
	interrupt_dispatch_all();
}

#endif /* INTERRUPT_TEST */
