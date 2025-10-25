/**
 * @file entry.c
 * @brief LiteCoreKernelのエントリーポイント
 */

extern void kmain();

__attribute__((section(".text.kernel_entry")))

void kernel_entry() {
        kmain();
}