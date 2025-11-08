#include <util/config.h>
#include <util/console.h>
#include <interrupt/idt.h>
#include <interrupt/irq.h>

/* PIC ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Remap PIC to vectors starting at 32 */
static void pic_remap(void) {
	unsigned char a1, a2;
	a1 = inb(PIC1_DATA);
	a2 = inb(PIC2_DATA);

	outb(PIC1_COMMAND, 0x11);
	outb(PIC2_COMMAND, 0x11);
	outb(PIC1_DATA, 0x20); /* Master PIC vector offset */
	outb(PIC2_DATA, 0x28); /* Slave PIC vector offset */
	outb(PIC1_DATA, 0x04);
	outb(PIC2_DATA, 0x02);
	outb(PIC1_DATA, 0x01);
	outb(PIC2_DATA, 0x01);

	outb(PIC1_DATA, a1);
	outb(PIC2_DATA, a2);
}

extern void load_idt(void *ptr, unsigned size);

/* Minimal IDT entry struct */
struct idt_entry {
	uint16_t base_lo;
	uint16_t sel;
	uint8_t always0;
	uint8_t flags;
	uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

extern void isr_stub_table(void); /* assembly stubs */
extern void isr14(void);

#define IDT_ENTRIES 256
static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;

static void idt_set_gate(int n, uint32_t handler) {
	idt[n].base_lo = handler & 0xFFFF;
	idt[n].sel = 0x08; // code segment
	idt[n].always0 = 0;
	idt[n].flags = 0x8E;
	idt[n].base_hi = (handler >> 16) & 0xFFFF;
}

/**
 * @fn irq_handler_c
 * @param vec 割り込みベクター番号
 */
void irq_handler_c(uint32_t vec) {
	if (vec >= 32 && vec < 32 + 16) {
		uint32_t irq = vec - 32;
		interrupt_raise((irq << 16) | 0u);

		if (irq >= 8)
			outb(PIC2_COMMAND, 0x20);
		outb(PIC1_COMMAND, 0x20);
	} else if (vec >= 32) {
		interrupt_raise((vec << 16) | 0u);
	}
}

extern void page_fault_handler(uint32_t vec);
extern void page_fault_handler_ex(uint32_t vec, uint32_t error_code,
				  uint32_t eip);

void irq_exception_c(uint32_t vec) {
	if (vec == 14) {
		page_fault_handler(vec);
	} else {
		/* other exceptions: just spin */
		printk("exception vec=%u\n", (unsigned)vec);
		while (1) {
		}
	}
}

/**
 * @fn irq_exception_ex
 * @brief 例外発生時の拡張ハンドラ
 */
void irq_exception_ex(uint32_t vec, uint32_t error_code) {
	if (vec == 14) {
		// C言語からEIPを正確に取得するのは難しいため、ここでは0を渡す
		page_fault_handler_ex(vec, error_code, 0);
	} else {
		printk("exception ex vec=%u err=0x%x\n", (unsigned)vec,
		       (unsigned)error_code);
		while (1) {
		}
	}
}

/**
 * @fn idt_init
 * @brief IDTを初期化する
 */
void idt_init(void) {
	pic_remap();

	/* 明示的に各isrシンボルをextern宣言し、それをIDTに登録する（脳筋だぜぇ〜ｗｗｗ */
	extern void isr32(void);
	extern void isr33(void);
	extern void isr34(void);
	extern void isr35(void);
	extern void isr36(void);
	extern void isr37(void);
	extern void isr38(void);
	extern void isr39(void);
	extern void isr40(void);
	extern void isr41(void);
	extern void isr42(void);
	extern void isr43(void);
	extern void isr44(void);
	extern void isr45(void);
	extern void isr46(void);
	extern void isr47(void);
	extern void isr48(void);

	idt_set_gate(14, (uint32_t)isr14); /* page fault */
	idt_set_gate(32, (uint32_t)isr32);
	idt_set_gate(33, (uint32_t)isr33);
	idt_set_gate(34, (uint32_t)isr34);
	idt_set_gate(35, (uint32_t)isr35);
	idt_set_gate(36, (uint32_t)isr36);
	idt_set_gate(37, (uint32_t)isr37);
	idt_set_gate(38, (uint32_t)isr38);
	idt_set_gate(39, (uint32_t)isr39);
	idt_set_gate(40, (uint32_t)isr40);
	idt_set_gate(41, (uint32_t)isr41);
	idt_set_gate(42, (uint32_t)isr42);
	idt_set_gate(43, (uint32_t)isr43);
	idt_set_gate(44, (uint32_t)isr44);
	idt_set_gate(45, (uint32_t)isr45);
	idt_set_gate(46, (uint32_t)isr46);
	idt_set_gate(47, (uint32_t)isr47);
	idt_set_gate(48, (uint32_t)isr48); /* APIC Timer */

	idtp.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
	idtp.base = (uint32_t)&idt;
	load_idt(&idtp, sizeof(idtp));
}
