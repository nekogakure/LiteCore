#include <config.h>
#include <util/console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <interrupt/irq.h>
#include <interrupt/idt.h>
#include <util/shell.h>
#include <util/shell_integration.h>
#include <util/io.h>
#include <mem/map.h>
#include <mem/manager.h>
#include <mem/segment.h>

#include <tests/define.h>
#include <tests/run.h>

void kloop();
static int shell_started = 0;


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
	
	// タイマー割り込み（IRQ 0）を登録
	interrupt_register(0, timer_handler, NULL);

	new_line();
	printk("> DEVICE INIT\n");
	keyboard_init();
	printk("ok\n");
	
	// 割り込みを有効化
	__asm__ volatile("sti");
	printk("> Interrupts enabled\n");

#ifdef TEST_TRUE
	new_line();
	printk("====== TESTS ======\n");
	run_test();
#endif /* TEST_TRUE */

	new_line();

	new_line();
	printk("Startup process complete :D\n");
	printk("Please press any key to continue to shell...\n");

	while (1) {
		kloop();
	}
}

/**
 * @fn kloop
 * @brief kmainの処理が終了した後常に動き続ける処理
 */
void kloop() {
	int activity = 0;  // このループで何か処理したかフラグ
	
	/* ポーリングによるフォールバック: キーボードのscancodeを回収 */
	keyboard_poll();
	
	/* FIFOに入ったイベントを処理 */
	int event_count = 0;
	while (interrupt_dispatch_one()) {
		activity = 1;
		event_count++;
	}
	
	/* シェルが開始されていない場合は、キー入力待ち */
	if (!shell_started) {
		char c = keyboard_getchar_poll();
		if (c != 0) {
			/* 何かキーが押されたらシェルを開始 */
			shell_started = 1;
			printk("\n");
			init_full_shell();
			activity = 1;
		}
	} else {
		/* シェル実行中 */
		int processed = shell_readline_and_execute();
		if (processed != 0) {
			activity = 1;
		}
	}
	
	/* 何も処理しなかった場合はCPUを休止（次の割り込みまで） */
	if (!activity) {
		cpu_halt();
	}
}