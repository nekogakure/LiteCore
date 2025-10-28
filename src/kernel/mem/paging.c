#include <config.h>
#include <stdint.h>
#include <mem/paging.h>
#include <mem/map.h>
#include <stddef.h>

/* helpers: in current identity-map boot situation, phys==virt, so these are identity
 * conversions. If kernel later runs with higher-half mapping, update these helpers
 * to translate between physical and kernel virtual addresses accordingly.
 */
static inline void *phys_to_virt(uint32_t phys) {
        return (void *)phys;
}
static inline uint32_t virt_to_phys(void *virt) {
        return (uint32_t)virt;
}
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

// 1つの4KBページをマッピングする: phys -> virt（flags付き）。成功時0、失敗時-1
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

// 1つの4KBページをアンマップする。成功時0、未マップ時-1
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

// 指定されたMB数分のメモリを同一マッピングで初期化する
void paging_init_identity(uint32_t map_mb) {
        /* map_mb MB (rounded up to full pages) as identity mapping */
        uint32_t pages = ((map_mb * 1024u * 1024u) + 0xFFF) / 0x1000;
        uint32_t pages_remaining = pages;

        /* number of page tables needed (1024 entries per table -> 4MB each) */
        uint32_t needed_pt = (pages + 1023) / 1024;

        /* initialize page directory to not present */
        for (uint32_t i = 0; i < 1024; ++i) page_directory[i] = 0x00000000;

        for (uint32_t pt_index = 0; pt_index < needed_pt; ++pt_index) {
                /* use first_table for pt_index==0 for static allocation */
                uint32_t *pt = NULL;
                if (pt_index == 0) {
                        pt = first_table;
                } else {
                        void *alloc = alloc_page_table();
                        if (!alloc) {
                                printk("paging: failed to alloc page table for pt_index=%u\n", (unsigned)pt_index);
                                break;
                        }
                        pt = (uint32_t *)alloc;
                }

                /* fill table entries for this PT */
                for (uint32_t i = 0; i < 1024 && pages_remaining > 0; ++i, --pages_remaining) {
                        uint32_t phys = (pt_index * 1024u + i) * 0x1000u;
                        pt[i] = (phys & 0xFFFFF000u) | (PAGING_PRESENT | PAGING_RW);
                }

                /* write PDE: PDE must contain physical address of pt */
                uint32_t pt_phys = virt_to_phys((void *)pt);
                page_directory[pt_index] = (pt_phys & 0xFFFFF000u) | (PAGING_PRESENT | PAGING_RW);
        }

        printk("paging: identity map initialized for %u MB (pages=%u, tables=%u)\n",
               (unsigned)map_mb, (unsigned)pages, (unsigned)needed_pt);
}

// ページング機能を有効化する
void paging_enable(void) {
        // CR3をロード
        asm volatile("mov %%eax, %%cr3" :: "a"((uint32_t)page_directory));
        // CR0のPGビットを有効化
        uint32_t cr0;
        asm volatile("mov %%cr0, %%eax" : "=a" (cr0));
        cr0 |= 0x80000000u; // PGビットをセット
        asm volatile("mov %%eax, %%cr0" :: "a" (cr0));
}

// ページフォルト発生時のハンドラ
void page_fault_handler(uint32_t vec) {
        (void)vec;
        uint32_t fault_addr;
        asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));
        printk("PAGE FAULT at 0x%x\n", (unsigned)fault_addr);
        while (1) {}
}
