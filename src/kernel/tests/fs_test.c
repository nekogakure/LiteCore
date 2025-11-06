#include <stddef.h>
#include <util/console.h>

/**
 * @brief FAT12テスト（廃止予定）
 */
void fs_test() {
	printk("fs_test: FAT12 support has been removed\n");
	printk("Note: LiteCore now uses ext2 as the standard filesystem\n");
}

void fat12_test() {
	fs_test();
}