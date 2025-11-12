#include <tests/define.h>

#ifdef GDT_TEST

#include <util/config.h>
#include <util/console.h>

static inline uint16_t read_cs(void) {
	uint16_t sel;
	asm volatile("mov %%cs, %0" : "=r"(sel));
	return sel;
}

static inline uint16_t read_ds(void) {
	uint16_t sel;
	asm volatile("mov %%ds, %0" : "=r"(sel));
	return sel;
}

void gdt_test() {
	uint16_t cs = read_cs();
	uint16_t ds = read_ds();
	printk("CS = 0x%x\n", (unsigned)cs);
	printk("DS = 0x%x\n", (unsigned)ds);
	/* check expected values: kernel code=0x08, data=0x10 */
	if (cs == 0x08 && ds == 0x10) {
		printk("GDT test: OK\n");
	} else {
		printk("GDT test: FAILED (cs=0x%x ds=0x%x)\n", (unsigned)cs,
		       (unsigned)ds);
	}
}

#endif /* GDT_TEST */
