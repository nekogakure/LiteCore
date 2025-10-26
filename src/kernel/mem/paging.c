#include <config.h>
#include <stdint.h>
#include <mem/paging.h>
#include <console.h>
#include <interrupt/irq.h>

static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_table[1024] __attribute__((aligned(4096)));

/**
 * @fn paging_init_identity
 * @brief 指定されたMB数分のメモリを同一マッピングで初期化する
 * @param map_mb マッピングするメモリ容量（MB単位）
 */
void paging_init_identity(uint32_t map_mb) {
        uint32_t pages = ((map_mb * 1024u * 1024u) + 0xFFF) / 0x1000;

        // 最初のテーブル: 最初の4MBを同一マッピング（1024エントリ）
        for (uint32_t i = 0; i < 1024; ++i) {
                first_table[i] = (i * 0x1000) | 3u; // present + rw
        }

        // ディレクトリエントリ0はfirst_tableを指す
        page_directory[0] = ((uint32_t)first_table) | 3u;

        // 残りのPDEをnot presentにする
        for (uint32_t i = 1; i < 1024; ++i) page_directory[i] = 0x00000000;

        printk("paging: identity map initialized for %u MB (pages=%u)\n", (unsigned)map_mb, (unsigned)pages);
}

/**
 * @fn paging_enable
 * @brief ページング機能を有効化する
 */
void paging_enable(void) {
        // CR3をロード
        asm volatile("mov %%eax, %%cr3" :: "a"((uint32_t)page_directory));
        // CR0のPGビットを有効化
        uint32_t cr0;
        asm volatile("mov %%cr0, %%eax" : "=a" (cr0));
        cr0 |= 0x80000000u; // PGビットをセット
        asm volatile("mov %%eax, %%cr0" :: "a" (cr0));
}

/**
 * @fn page_fault_handler
 * @brief ページフォルト発生時のハンドラ
 * @param vec 割り込みベクタ番号
 */
void page_fault_handler(uint32_t vec) {
        (void)vec;
        uint32_t fault_addr;
        asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));
        printk("PAGE FAULT at 0x%x\n", (unsigned)fault_addr);
        while (1) {}
}
