#include <config.h>
#include <util/console.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <interrupt/irq.h>
#include <interrupt/idt.h>
#include <util/shell.h>
#include <util/shell_integration.h>
#include <util/io.h>
#include <util/init_msg.h>
#include <mem/map.h>
#include <mem/manager.h>
#include <mem/segment.h>
#include <driver/ata.h>
#include <fs/ext/ext2.h>
#include <fs/block_cache.h>

extern struct ext2_super *g_ext2_sb;

void kernel_init() {
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

	new_line();
	printk("> FILESYSTEM INIT (ext2)\n");

	// ATAドライバを初期化
	if (ata_init() != 0) {
		printk("Warning: ATA initialization failed\n");
		printk("Filesystem will not be available\n");
	} else {
		printk("ATA driver initialized\n");

		// ブロックキャッシュを初期化 (drive=1, block_size=4096, num_entries=32)
		struct block_cache *cache = block_cache_init(0, 4096, 32);
		if (cache == NULL) {
			printk("Error: Failed to initialize block cache\n");
		} else {
			printk("Block cache initialized (32 KB, 32 entries)\n");

			// ext2ファイルシステムをキャッシュ経由でマウント
			if (ext2_mount_with_cache(cache, &g_ext2_sb) == 0) {
				printk("ext2 filesystem mounted successfully\n");
				printk("  Block size: %u bytes\n",
				       g_ext2_sb->block_size);
				printk("  Total blocks: %u\n",
				       g_ext2_sb->sb.s_blocks_count);
				printk("  Free blocks: %u\n",
				       g_ext2_sb->sb.s_free_blocks_count);
				printk("  Total inodes: %u\n",
				       g_ext2_sb->sb.s_inodes_count);
			} else {
				printk("Error: Failed to mount ext2 filesystem\n");
				block_cache_destroy(cache);
			}
		}
	}
	printk("ok\n");
}