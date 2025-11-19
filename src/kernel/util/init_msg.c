#include <util/config.h>
#include <util/console.h>
#include <util/bdf.h>
#include <device/pci.h>
#include <device/keyboard.h>
#include <interrupt/irq.h>
#include <interrupt/idt.h>
#include <shell/shell.h>
#include <shell/shell_integration.h>
#include <util/io.h>
#include <util/init_msg.h>
#include <util/bdf.h>
#include <mem/map.h>
#include <mem/manager.h>
#include <mem/segment.h>
#include <driver/ata.h>
#include <fs/ext/ext2.h>
#include <fs/block_cache.h>
#include <task/multi_task.h>

#ifdef UEFI_MODE
#include <driver/timer/uefi_timer.h>
#else
#include <driver/timer/apic.h>
#endif

extern struct ext2_super *g_ext2_sb;

void init_font();

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

#ifdef INIT_MSG
	new_line();
	printk("> DEVICE INIT\n");
#endif
	keyboard_init();
#ifdef INIT_MSG
	printk("ok\n");
#endif

#ifdef INIT_MSG
	new_line();
	printk("> TIMER INIT\n");
#endif

#ifdef UEFI_MODE
	// UEFI環境ではPITタイマーを使用
	int timer_result = uefi_timer_init();
	if (timer_result != 0) {
		printk("UEFI Timer initialization failed\n");
	}
	// タイマー割り込み（IRQ 0）を登録
	interrupt_register(32, uefi_timer_tick, NULL);
#else
	// レガシーBIOS環境ではAPIC Timerを使用
	interrupt_register(48, apic_timer_tick, NULL);
	int timer_result = apic_timer_init();
	if (timer_result != 0) {
		printk("APIC Timer initialization failed\n");
	}
#endif

#ifdef INIT_MSG
	printk("ok\n");
#endif

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
		struct block_cache *cache = block_cache_init(1, 4096, 32);
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

				/* initialize font and then allow console to allocate gfx buffer */
				init_font();
				console_post_font_init();
                        
		}
	}
#ifdef INIT_MSG
	printk("ok\n");
#endif

#ifdef INIT_MSG
	new_line();
	printk("> MULTI TASK INIT\n");
#endif
	task_init();
#ifdef INIT_MSG
	printk("ok\n");
#endif
}

void init_font() {
	if (g_ext2_sb != NULL) {
		if (bdf_init("kernel/fonts/ter-u12b.bdf")) {
		} else {
			printk("Warning: Failed to load BDF font\n");
		}
	} else {
		printk("Warning: Filesystem not available, skipping font loading\n");
	}
}