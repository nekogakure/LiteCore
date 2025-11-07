#include <config.h>
#include <util/console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <driver/timer/apic.h>
#include <interrupt/irq.h>
#include <interrupt/idt.h>
#include <shell/shell.h>
#include <shell/shell_integration.h>
#include <util/io.h>
#include <util/init_msg.h>
#include <mem/map.h>
#include <mem/manager.h>
#include <mem/segment.h>
#include <driver/ata.h>
#include <driver/timer/apic.h>
#include <fs/ext/ext2.h>
#include <fs/block_cache.h>

extern struct ext2_super *g_ext2_sb;

void kernel_init() {
#ifdef INIT_MSG
	printk("=== KERNEL INIT ===\n");
	printk("> MEMORY INIT\n");
#endif
        memory_init();
#ifdef INIT_MSG
        printk("ok\n");
#endif

#ifdef INIT_MSG
	new_line();
	printk("> INTERRUPT INIT\n");
#endif

	idt_init();
	interrupt_init();
#ifdef INIT_MSG	
        printk("ok\n");
#endif

	// タイマー割り込み（IRQ 0）を登録
	interrupt_register(0, timer_handler, NULL);

#ifdef INIT_MSG
	new_line();
	printk("> DEVICE INIT\n");
#endif
	keyboard_init();
#ifdef INIT_MSG
	printk("ok\n");
#endif

#ifdef INIT_MSG
	// APIC Timer初期化
	new_line();
	printk("> APIC TIMER INIT\n");
#endif

        // タイマー割り込みハンドラを登録 (IRQ 48)
	interrupt_register(48, apic_timer_tick, NULL);
	int apic_result = apic_timer_init();
	if (apic_result != 0) {
		printk("APIC Timer initialization failed\n");
	}
#ifdef INIT_MSG
	printk("ok\n");
#endif

	// 割り込みを有効化
	__asm__ volatile("sti");

#ifdef INIT_MSG
	new_line();
	printk("> FILESYSTEM INIT (ext2)\n");
#endif
	// ATAドライバを初期化
	if (ata_init() != 0) {
		printk("Warning: ATA initialization failed\n");
		printk("Filesystem will not be available\n");
	} else {
#ifdef INIT_MSG
		printk("ATA driver initialized\n");
#endif

		// ブロックキャッシュを初期化 (drive=1, block_size=4096, num_entries=32)
		struct block_cache *cache = block_cache_init(0, 4096, 32);
		if (cache == NULL) {
			printk("Error: Failed to initialize block cache\n");
		} else {
#ifdef INIT_MSG
			printk("Block cache initialized (32 KB, 32 entries)\n");
#endif

			// ext2ファイルシステムをキャッシュ経由でマウント
			if (ext2_mount_with_cache(cache, &g_ext2_sb) == 0) {
#ifdef INIT_MSG
				printk("ext2 filesystem mounted successfully\n");
				printk("  Block size: %u bytes\n",
				       g_ext2_sb->block_size);
				printk("  Total blocks: %u\n",
				       g_ext2_sb->sb.s_blocks_count);
				printk("  Free blocks: %u\n",
				       g_ext2_sb->sb.s_free_blocks_count);
				printk("  Total inodes: %u\n",
				       g_ext2_sb->sb.s_inodes_count);
#endif
			} else {
				printk("Error: Failed to mount ext2 filesystem\n");
				block_cache_destroy(cache);
			}
		}
	}
#ifdef INIT_MSG
	printk("ok\n");
#endif
}