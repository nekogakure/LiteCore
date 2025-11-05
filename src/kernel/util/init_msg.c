#include <config.h>
#include <util/console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <interrupt/irq.h>
#include <interrupt/idt.h>
#include <mem/map.h>
#include <mem/manager.h>
#include <mem/segment.h>

void kernel_init() {
	new_line();
	printk("=== KERNEL INIT ===\n");
	printk("> MEMORY INIT\n");
	memory_init();
	printk("ok\n");

	new_line();
	printk("> INTERRUPT INIT\n");
	idt_init();
	interrupt_init();
	printk("ok\n");

	new_line();
	printk("> DEVICE INIT\n");
	keyboard_init();
	printk("ok\n");
}