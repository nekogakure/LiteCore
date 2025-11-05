#include <stddef.h>
#include <util/console.h>
#include <fs/ext/ext2.h>
#include <mem/manager.h>
#include <driver/ata.h>

/**
 * @brief ext2ファイルシステムのテスト
 * 
 * ATAドライバを使ってハードディスク（-hdb）からext2.imgを読み込みます。
 */
void ext2_test() {
	printk("ext2_test: Loading ext2.img from ATA drive\n");
	
	/* ATAドライバを初期化 */
	if (ata_init() != 0) {
		printk("ext2_test: ATA initialization failed\n");
		return;
	}
	
	/* ext2.imgのサイズを取得（とりあえず2MB=4096セクタと仮定） */
	uint32_t img_sectors = 4096;  /* 2MB = 2048KB = 4096 sectors */
	size_t img_size = img_sectors * 512;
	
	printk("ext2_test: Allocating %u bytes for ext2 image\n", (unsigned)img_size);
	
	/* メモリを確保 */
	void *img = kmalloc(img_size);
	if (!img) {
		printk("ext2_test: Failed to allocate memory\n");
		return;
	}
	
	printk("ext2_test: Reading %u sectors from ATA drive 1 (Secondary Master)\n", img_sectors);
	
	/* ATAドライブ1（-hdb）からイメージを読み込む */
	/* QEMUでは-hdbはSecondary Master (drive=2) */
	if (ata_read_sectors(2, 0, 255, img) != 0) {
		printk("ext2_test: Failed to read first 255 sectors\n");
		kfree(img);
		return;
	}
	
	/* 残りのセクタを読み込む（255セクタずつ） */
	uint32_t sectors_read = 255;
	while (sectors_read < img_sectors) {
		uint8_t count = (img_sectors - sectors_read > 255) ? 255 : (img_sectors - sectors_read);
		if (ata_read_sectors(2, sectors_read, count, (uint8_t *)img + sectors_read * 512) != 0) {
			printk("ext2_test: Failed to read at sector %u\n", sectors_read);
			kfree(img);
			return;
		}
		sectors_read += count;
	}
	
	printk("ext2_test: Successfully loaded ext2 image (%u bytes)\n", (unsigned)img_size);
	
	/* ext2をマウント */
	struct ext2_super *sb = NULL;
	int r = ext2_mount(img, img_size, &sb);
	if (r != 0) {
		printk("ext2_mount failed: %d\n", r);
		kfree(img);
		return;
	}
	printk("ext2_mount: SUCCESS\n");
	
	/* スーパーブロック情報を表示 */
	printk("  Block size: %u bytes\n", sb->block_size);
	printk("  Inodes: %u\n", sb->sb.s_inodes_count);
	printk("  Blocks: %u\n", sb->sb.s_blocks_count);
	printk("  Block groups: %u\n", sb->num_groups);
	printk("  Inodes per group: %u\n", sb->sb.s_inodes_per_group);
	
	/* ルートディレクトリの内容を表示 */
	printk("\nListing root directory:\n");
	ext2_list_root(sb);
	
	/* ルートディレクトリのファイルを読み取る */
	printk("\nTest 1: Reading test.txt from root:\n");
	char buf[256];
	size_t out_len = 0;
	r = ext2_read_file_by_path(sb, "/test.txt", buf, sizeof(buf) - 1, 0, &out_len);
	if (r == 0) {
		buf[out_len] = '\0';
		printk("File content (%u bytes):\n%s\n", (unsigned)out_len, buf);
	} else {
		printk("Failed to read /test.txt: %d\n", r);
	}
	
	/* サブディレクトリのファイルを読み取る */
	printk("\nTest 2: Reading /dir_example/ext2.txt:\n");
	r = ext2_read_file_by_path(sb, "/dir_example/ext2.txt", buf, sizeof(buf) - 1, 0, &out_len);
	if (r == 0) {
		buf[out_len] = '\0';
		printk("File content (%u bytes):\n%s\n", (unsigned)out_len, buf);
	} else {
		printk("Failed to read /dir_example/ext2.txt: %d\n", r);
	}
	
	/* リソース解放 */
	kfree(sb);
	kfree(img);
	printk("\next2_test: COMPLETED\n");
}
