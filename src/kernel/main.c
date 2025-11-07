#include <config.h>
#include <util/console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <interrupt/irq.h>
#include <interrupt/idt.h>
#include <shell/commands.h>
#include <shell/shell.h>
#include <shell/shell_integration.h>
#include <util/io.h>
#include <util/init_msg.h>
#include <mem/map.h>
#include <mem/manager.h>
#include <mem/segment.h>
#include <driver/ata.h>
#include <fs/ext/ext2.h>
#include <driver/timer/timer.h>

#include <tests/define.h>
#include <tests/run.h>

void kloop();

// グローバルext2ハンドル
struct ext2_super *g_ext2_sb = NULL;

/**
 * @fn kmain
 * @brief LiteCoreのメイン関数（kernel_entryより）
 */
void kmain() {
	console_init();
	gdt_build();
	gdt_install_lgdt();

	printk("Welcome to Litecore kernel!\n");
	printk("    Version : %s\n", VERSION);
	printk("    Build   : %s %s\n", __DATE__, __TIME__);
	printk("    Author  : nekogakure\n");

	new_line();

	kernel_init();

#ifdef TEST_TRUE
	new_line();
	printk("====== TESTS ======\n");
	run_test();
#endif /* TEST_TRUE */

	new_line();

	new_line();

	printk("Startup process complete :D\n");
	printk("initializing shell...\n");

	init_full_shell();

	while (1) {
		kloop();
	}
}

/**
 * @fn kloop
 * @brief kmainの処理が終了した後常に動き続ける処理
 */
void kloop() {
	int activity =
		0; // このループで何か処理したかフラグ（分かりづらい仕事しろ、そうだこの変数だ）

	/* ポーリングによるフォールバック: キーボードのscancodeを回収 */
	keyboard_poll();

	/* FIFOに入ったイベントを処理 */
	int event_count = 0;
	while (interrupt_dispatch_one()) {
		activity = 1;
		event_count++;
	}

	shell_readline_and_execute();

	/* 何も処理しなかった場合はCPUを休止（次の割り込みまで） */
	if (!activity) {
		cpu_halt();
	}

	// TODO: もっといい感じの処理にしなければならないなと思った
}