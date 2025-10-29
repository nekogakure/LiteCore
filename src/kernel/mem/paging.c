#include <config.h>
#include <stdint.h>
#include <mem/paging.h>
#include <mem/map.h>
#include <stddef.h>
#include <console.h>
#include <interrupt/irq.h>

// ページディレクトリ（4KBアライン）
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
// 最初の4MBの同一マッピング用テーブル（4KBアライン）
static uint32_t first_table[1024] __attribute__((aligned(4096)));

// TLB内の単一ページを無効化するヘルパー
static inline void invlpg(void *addr) {
        asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

// ゼロクリアされたページテーブルを確保する（仮想アドレスを返す。失敗時NULL）
void *alloc_page_table(void) {
        void *frame = alloc_frame();
        if (!frame) return NULL;
        // ページテーブルをゼロクリア
        uint32_t *tbl = (uint32_t *)frame;
        for (size_t i = 0; i < 1024; ++i) tbl[i] = 0;
        return frame;
}

/**
 * @fn map_page
 * @brief 物理アドレスphysを仮想アドレスvirtにflags属性でマッピングする
 * @return 0: 成功 -1: 失敗
 */
int map_page(uint32_t phys, uint32_t virt, uint32_t flags) {
        if ((flags & PAGING_PRESENT) == 0) flags |= PAGING_PRESENT;
        uint32_t pd_idx = (virt >> 22) & 0x3FF;
        uint32_t pt_idx = (virt >> 12) & 0x3FF;

        uint32_t pde = page_directory[pd_idx];
        uint32_t *pt;
        if ((pde & PAGING_PRESENT) == 0) {
                void *new_pt = alloc_page_table();
                if (!new_pt) return -1;
                page_directory[pd_idx] = ((uint32_t)new_pt) | (PAGING_PRESENT | PAGING_RW);
                pt = (uint32_t *)new_pt;
        } else {
                pt = (uint32_t *)(pde & 0xFFFFF000);
        }

        pt[pt_idx] = (phys & 0xFFFFF000) | (flags & 0xFFF);
        invlpg((void *)virt);
        return 0;
}

/**
 * @fn unmap_page
 * @brief 仮想アドレスvirtに対応するページをアンマップする
 * @param virt アンマップする仮想アドレス
 * @return 0: 成功 -1: 失敗
 */
int unmap_page(uint32_t virt) {
        uint32_t pd_idx = (virt >> 22) & 0x3FF;
        uint32_t pt_idx = (virt >> 12) & 0x3FF;

        uint32_t pde = page_directory[pd_idx];
        if ((pde & PAGING_PRESENT) == 0) return -1;
        uint32_t *pt = (uint32_t *)(pde & 0xFFFFF000);
        if ((pt[pt_idx] & PAGING_PRESENT) == 0) return -1;
        pt[pt_idx] = 0;
        invlpg((void *)virt);
        return 0;
}

/**
 * @fn paging_init_identity
 * @brief 最初のmap_mb MB分を同一マッピングで初期化する
 * @param map_mb 同一マッピングするMB数
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
 * @brief ページングを有効化
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

void page_fault_handler(uint32_t vec) {
        (void)vec;
        uint32_t fault_addr;
        asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));
        printk("PAGE FAULT at 0x%x\n", (unsigned)fault_addr);
        while (1) {}
}
