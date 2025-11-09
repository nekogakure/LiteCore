#include "boot_info.h"

extern void kmain(BOOT_INFO *boot_info);

__attribute__((section(".text.kernel_entry")))

/**
 * @fn kernel_entry
 * @brief カーネルのエントリーポイント
 */
void kernel_entry(BOOT_INFO *boot_info) {
	kmain(boot_info);
}