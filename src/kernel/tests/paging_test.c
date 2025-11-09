#include <util/config.h>
#include <util/console.h>
#include <mem/paging.h>
#include <mem/map.h>

void paging_test(void) {
	// カーネルがアクセス可能なように低メモリをアイデンティティマッピング
	paging_init_identity(1); // 最初の1MBをマップ
	printk("identity map (1MB) set\n");

	// マッピングする物理フレームを確保
	void *frame = alloc_frame();
	if (!frame) {
		printk("alloc_frame failed\n");
		return;
	}
	uint32_t phys = (uint32_t)(uintptr_t)frame;
	printk("allocated phys frame=0x%x\n", (unsigned)phys);

	// アイデンティティ領域の上に未マップと思われる仮想アドレスを選択
	uint32_t virt = 0x4000000; // 64MB

	// ページをマップ（present + rw）
	if (map_page(phys, virt, PAGING_PRESENT | PAGING_RW) != 0) {
		printk("map_page failed\n");
		return;
	}
	printk("mapped phys 0x%x -> virt 0x%x\n", (unsigned)phys,
	       (unsigned)virt);

	// ページングを有効化
	paging_enable();
	printk("paging enabled\n");

	// 新たにマップした仮想アドレスへアクセス
	volatile uint32_t *p = (uint32_t *)(uintptr_t)virt;
	*p = 0xdeadbeef;
	uint32_t v = *p;
	printk("write/read virt 0x%x -> 0x%x\n", (unsigned)virt, (unsigned)v);

	// ページのアンマップ（この後アクセスするとページフォルトになるので注意）
	if (unmap_page(virt) == 0) {
		printk("unmapped virt 0x%x\n", (unsigned)virt);
	} else {
		printk("unmap_page failed for 0x%x\n", (unsigned)virt);
	}

	// 同じ物理フレームを別の仮想アドレスに再マップし、値を確認
	uint32_t virt2 = 0x4100000; // 65MB
	if (map_page(phys, virt2, PAGING_PRESENT | PAGING_RW) != 0) {
		printk("remap failed\n");
		return;
	}
	printk("remapped phys 0x%x -> virt 0x%x\n", (unsigned)phys,
	       (unsigned)virt2);
	volatile uint32_t *p2 = (uint32_t *)(uintptr_t)virt2;
	uint32_t v2 =
		*p2; // キャッシュが一貫していれば前回の書き込みが反映されているはず
	printk("read virt2 0x%x -> 0x%x\n", (unsigned)virt2, (unsigned)v2);

	printk("done\n");
}
