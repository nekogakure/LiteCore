/**
 * @file main.c
 * @brief LiteCoreカーネルのメイン処理
 * @details カーネルの初期化とメインループ（BIOSブート対応）
 */

#include "../include/kernel/types.h"
#include "../include/kernel/cpu.h"
#include "../util/logger.h"

/**
 * @brief 簡単なVGAテキストモード出力関数
 */
static void simple_vga_output(void)
{
        volatile char* vga_buffer = (volatile char*)0xB8000;
        const char* message = "LiteCore Kernel - BIOS Boot";
        
        /* VGAテキストバッファをクリア */
        for (int i = 0; i < 80 * 25 * 2; i += 2) {
                vga_buffer[i] = ' ';     /* 文字 */
                vga_buffer[i + 1] = 0x07; /* 属性（白文字、黒背景） */
        }
        
        /* メッセージを表示 */
        for (int i = 0; message[i] != '\0'; i++) {
                vga_buffer[i * 2] = message[i];
                vga_buffer[i * 2 + 1] = 0x0F; /* 白文字 */
        }
}

/**
 * @brief カーネルのメイン関数（BIOSブート対応）
 * @param multiboot_magic Multiboot2マジックナンバー
 * @param multiboot_info Multiboot2情報構造体のポインタ
 * @return カーネルの終了コード
 */
kernel_result_t kernel_main(uint32_t multiboot_magic, void* multiboot_info)
{
        /* 割り込みを無効化 */
        cpu_disable_interrupts();

        /* ログシステムを初期化 */
        logger_init();

        /* 初期化メッセージ */
        debug("LiteCore Kernel Starting... (BIOS Boot)\n");
        
        /* Multiboot2チェック */
        if (multiboot_magic != 0x36d76289) {
                debug("Warning: Invalid multiboot magic: 0x%x\n", multiboot_magic);
        }
        else {
                debug("Multiboot2 detected successfully\n");
        }
        
        if (multiboot_info != NULL) {
                debug("Multiboot info at: 0x%lx\n", (uint64_t)multiboot_info);
        }
        else {
                debug("Warning: multiboot_info is NULL\n");
        }

        /* VGAテキストモード出力で動作確認 */
        simple_vga_output();

        debug("LiteCore Kernel initialized successfully\n");

        /* メインループ：シンプルな停止ループ */
        while (1) {
                cpu_halt();
        }

        return KERNEL_SUCCESS;
}