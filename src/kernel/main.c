#include <config.h>
#include <util/console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <interrupt/irq.h>
#include <interrupt/idt.h>
#include <mem/map.h>
#include <mem/manager.h>
#include <mem/segment.h>

#include <tests/define.h>
#include <tests/run.h>

void kloop();

/**
 * @fn kmain
 * @brief LiteCoreのメイン関数（kernel_entryより）
 */
void kmain() {
	console_init();
	gdt_build();
	gdt_install_lgdt();

	printk("Welcome to Litecore kernel!\n");
	printk("    Version: %s\n", VERSION);
	printk("    Author : nekogakure\n");

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

#ifdef TEST_TRUE
	new_line();
	printk("====== TESTS ======\n");
	run_test();
#endif /* TEST_TRUE */

	new_line();

	new_line();
	printk("Startup process complete\n");

	while (1) {
		kloop();
	}
}

/**
 * @fn kloop
 * @brief kmainの処理が終了した後常に動き続ける処理
 */
void kloop() {
	/* ポーリングによるフォールバック: キーボードのscancodeを回収 */
	keyboard_poll();
	/* FIFOに入ったイベントを処理 */
	interrupt_dispatch_all();
}