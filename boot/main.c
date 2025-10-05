/**
 * @file main.c
 * @brief BIOSブートのエントリーポイント
 * @details GRUB2のMultiboot2からカーネルメインに制御を渡す
 */

#include "../include/kernel/types.h"

/**
 * @brief カーネルメイン関数の宣言
 * @param multiboot_magic Multiboot2マジックナンバー
 * @param multiboot_info Multiboot2情報構造体
 * @return カーネルの終了コード
 */
extern kernel_result_t kernel_main(uint32_t multiboot_magic, void* multiboot_info);

/**
 * @brief GRUB2からのブートエントリーポイント
 * @param multiboot_magic Multiboot2マジックナンバー
 * @param multiboot_info Multiboot2情報構造体のポインタ
 * @return カーネルの終了コード
 */
kernel_result_t boot_main(uint32_t multiboot_magic, void* multiboot_info)
{
        /* カーネルメイン関数を呼び出し */
        return kernel_main(multiboot_magic, multiboot_info);
}