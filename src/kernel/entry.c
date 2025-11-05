extern void kmain();

__attribute__((section(".text.kernel_entry")))

/**
 * @fn kernel_entry
 * @brief カーネルのエントリーポイント
 */
void kernel_entry() {
	kmain();
}