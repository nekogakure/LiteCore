#include <util/config.h>
#include <stdint.h>
#include <mem/paging.h>
#include <mem/map.h>
#include <mem/vmem.h>
#include <stddef.h>
#include <util/console.h>
#include <interrupt/irq.h>

// ページディレクトリ（4KBアライン）
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
// 最初の4MBの同一マッピング用テーブル（4KBアライン）
static uint32_t first_table[1024] __attribute__((aligned(4096)));

// TLB内の単一ページを無効化するヘルパー
static inline void invlpg(void *addr) {
	asm volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

// ゼロクリアされたページテーブルを確保する（仮想アドレスを返す。失敗時NULL）
void *alloc_page_table(void) {
	void *frame = alloc_frame();
	if (!frame)
		return NULL;
	// alloc_frame() は物理フレームの物理アドレスを返す実装の可能性がある。
	// 実行環境によらず仮想ポインタを得るため vmem_phys_to_virt を用いる。
	uint32_t phys = (uint32_t)(uintptr_t)frame;
	uint32_t virt = vmem_phys_to_virt(phys);
	if (virt == 0) {
		printk("alloc_page_table: vmem_phys_to_virt returned 0 for phys=0x%x\n",
		       (unsigned)phys);
		return NULL;
	}
	uint32_t *tbl = (uint32_t *)(uintptr_t)virt;
	for (size_t i = 0; i < 1024; ++i)
		tbl[i] = 0;
	// return the virtual pointer for convenience to callers
	return (void *)(uintptr_t)virt;
}

/**
 * @fn map_page
 * @brief 物理アドレスphysを仮想アドレスvirtにflags属性でマッピングする
 * @return 0: 成功 -1: 失敗
 */
int map_page(uint32_t phys, uint32_t virt, uint32_t flags) {
	if ((flags & PAGING_PRESENT) == 0)
		flags |= PAGING_PRESENT;
	uint32_t pd_idx = (virt >> 22) & 0x3FF;
	uint32_t pt_idx = (virt >> 12) & 0x3FF;

	uint32_t pde = page_directory[pd_idx];
	uint32_t *pt;
	if ((pde & PAGING_PRESENT) == 0) {
		void *new_pt_virt = alloc_page_table();
		if (!new_pt_virt) {
			printk("map_page: alloc_page_table failed for pd_idx=%u\n",
			       (unsigned)pd_idx);
			return -1;
		}
		uint32_t new_pt_phys =
			vmem_virt_to_phys((uint32_t)(uintptr_t)new_pt_virt);
		if (new_pt_phys == 0) {
			printk("map_page: vmem_virt_to_phys returned 0 for new_pt_virt=0x%x\n",
			       (unsigned)(uintptr_t)new_pt_virt);
			return -1;
		}
		page_directory[pd_idx] = (new_pt_phys & 0xFFFFF000) |
					 (PAGING_PRESENT | PAGING_RW);
		pt = (uint32_t *)new_pt_virt;
	} else {
		uint32_t pt_phys = pde & 0xFFFFF000;
		uint32_t pt_virt = vmem_phys_to_virt(pt_phys);
		if (pt_virt == 0) {
			printk("map_page: vmem_phys_to_virt returned 0 for pt_phys=0x%x (pd_idx=%u)\n",
			       (unsigned)pt_phys, (unsigned)pd_idx);
			return -1;
		}
		pt = (uint32_t *)(uintptr_t)pt_virt;
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
	if ((pde & PAGING_PRESENT) == 0)
		return -1;
	uint32_t pt_phys = pde & 0xFFFFF000;
	uint32_t pt_virt = vmem_phys_to_virt(pt_phys);
	if (pt_virt == 0) {
		printk("unmap_page: vmem_phys_to_virt returned 0 for pt_phys=0x%x (pd_idx=%u)\n",
		       (unsigned)pt_phys, (unsigned)pd_idx);
		return -1;
	}
	uint32_t *pt = (uint32_t *)(uintptr_t)pt_virt;
	if ((pt[pt_idx] & PAGING_PRESENT) == 0)
		return -1;
	pt[pt_idx] = 0;
	invlpg((void *)virt);

	/* check if the page table became empty -> free it and clear PDE */
	int empty = 1;
	for (int i = 0; i < 1024; ++i) {
		if (pt[i] & PAGING_PRESENT) {
			empty = 0;
			break;
		}
	}
	if (empty) {
		uint32_t pt_phys = vmem_virt_to_phys((uint32_t)(uintptr_t)pt);
		if (pt_phys == 0) {
			printk("unmap_page: vmem_virt_to_phys returned 0 for pt_virt=0x%x\n",
			       (unsigned)(uintptr_t)pt);
		} else {
			page_directory[pd_idx] = 0x00000000;
			free_frame((void *)(uintptr_t)pt_phys);
		}
	}

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

	// ディレクトリエントリ0はfirst_tableを指す。first_tableは静的で既に仮想アドレスとしてアクセス可能だが、PDEには物理アドレスを置く必要がある。
	uint32_t first_table_phys =
		vmem_virt_to_phys((uint32_t)(uintptr_t)first_table);
	if (first_table_phys == 0) {
		printk("paging_init_identity: vmem_virt_to_phys returned 0 for first_table!\n");
		return;
	}
	page_directory[0] = (first_table_phys & 0xFFFFF000) | 3u;

	// 残りのPDEをnot presentにする
	for (uint32_t i = 1; i < 1024; ++i)
		page_directory[i] = 0x00000000;

	printk("paging: identity map initialized for %u MB (pages=%u)\n",
	       (unsigned)map_mb, (unsigned)pages);
}

/**
 * @fn map_range
 * @brief 指定範囲を連続ページとしてマップする
 */
int map_range(uint32_t phys_start, uint32_t virt_start, size_t size,
	      uint32_t flags) {
	if (phys_start % 0x1000 || virt_start % 0x1000)
		return -1;
	uint32_t pages = (size + 0xFFF) / 0x1000;
	for (uint32_t i = 0; i < pages; ++i) {
		if (map_page(phys_start + i * 0x1000, virt_start + i * 0x1000,
			     flags) != 0) {
			return -1;
		}
	}
	return 0;
}

void page_fault_handler_ex(uint32_t vec, uint32_t error_code, uint32_t eip) {
	uint32_t fault_addr;
	asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
	printk("PAGE FAULT: vec=%u err=0x%x eip=0x%x cr2=0x%x\n", (unsigned)vec,
	       (unsigned)error_code, (unsigned)eip, (unsigned)fault_addr);
	while (1) {
	}
}

/**
 * @fn paging_enable
 * @brief ページングを有効化
 */
void paging_enable(void) {
	// CR3をロード
	uint32_t pd_phys =
		vmem_virt_to_phys((uint32_t)(uintptr_t)page_directory);
	if (pd_phys == 0) {
		printk("paging_enable: vmem_virt_to_phys returned 0 for page_directory! skipping paging enable\n");
		return;
	}
	if (pd_phys & 0xFFF) {
		printk("paging_enable: pd_phys 0x%x not 4KB aligned! skipping\n",
		       (unsigned)pd_phys);
		return;
	}
	asm volatile("mov %%eax, %%cr3" ::"a"(pd_phys));
	// CR0のPGビットを有効化
	uint32_t cr0;
	asm volatile("mov %%cr0, %%eax" : "=a"(cr0));
	cr0 |= 0x80000000u; // PGビットをセット
	asm volatile("mov %%eax, %%cr0" ::"a"(cr0));
}

void page_fault_handler(uint32_t vec) {
	(void)vec;
	uint32_t fault_addr;
	asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
	printk("PAGE FAULT at 0x%x\n", (unsigned)fault_addr);
	while (1) {
	}
}