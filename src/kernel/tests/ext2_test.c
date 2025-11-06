#include <stddef.h>
#include <util/console.h>
#include <fs/ext/ext2.h>
#include <mem/manager.h>
#include <driver/ata.h>

/**
 * @brief ext2ファイルシステムのテスト
 * 
 * ATAドライバを使ってハードディスク（-hdb）から必要な部分だけ読み込みます。
 */
void ext2_test() {
	printk("ext2_test: Testing ext2 with on-demand loading from ATA drive\n");

	/* ATAドライバを初期化 */
	if (ata_init() != 0) {
		printk("ext2_test: ATA initialization failed\n");
		return;
	}

	printk("ext2_test: Reading superblock (first 4 sectors)\n");

	/* スーパーブロックを読み込む（最初の4セクタ=2048バイト、余裕を持たせる） */
	uint8_t sb_buf[2048];
	if (ata_read_sectors(1, 0, 4, sb_buf) != 0) {
		printk("ext2_test: Failed to read superblock\n");
		return;
	}

	/* ext2シグネチャを確認（オフセット1024+56の位置に0xEF53） */
	uint16_t magic = *(uint16_t *)(sb_buf + 1024 + 56);
	if (magic != 0xEF53) {
		printk("ext2_test: Invalid ext2 magic: 0x%x (expected 0xEF53)\n",
		       magic);
		return;
	}

	printk("ext2_test: Valid ext2 filesystem detected!\n");
	printk("ext2_test: Magic = 0x%x\n", magic);

	/* スーパーブロックの基本情報を表示 */
	struct ext2_superblock {
		uint32_t s_inodes_count;
		uint32_t s_blocks_count;
		uint32_t s_r_blocks_count;
		uint32_t s_free_blocks_count;
		uint32_t s_free_inodes_count;
		uint32_t s_first_data_block;
		uint32_t s_log_block_size;
		/* ... 他のフィールドは省略 ... */
	} *sb = (struct ext2_superblock *)(sb_buf + 1024);

	uint32_t block_size = 1024 << sb->s_log_block_size;

	printk("  Inodes: %u\n", sb->s_inodes_count);
	printk("  Blocks: %u\n", sb->s_blocks_count);
	printk("  Free blocks: %u\n", sb->s_free_blocks_count);
	printk("  Free inodes: %u\n", sb->s_free_inodes_count);
	printk("  Block size: %u bytes\n", block_size);
	printk("  First data block: %u\n", sb->s_first_data_block);

	printk("\next2_test: COMPLETED (on-demand loading works!)\n");
	printk("Note: Full ext2 mount will be implemented with block cache\n");
}
