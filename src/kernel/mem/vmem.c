#include <util/console.h>
#include <stdint.h>
#include <mem/vmem.h>
#include <interrupt/irq.h>

// 仮想メモリオフセット
static int32_t vmem_offset = 0;

// デフォルトの物理アドレス→仮想アドレス変換関数
static uint32_t default_phys2virt(uint32_t phys) {
        if (vmem_offset == 0) return phys;
        return (uint32_t)((int32_t)phys + vmem_offset);
}

// 現在の仮想メモリモード
static vmem_mode_t current_mode = VMEM_MODE_IDENTITY;
// 物理→仮想変換関数
static vmem_phys2virt_fn phys2virt = NULL;

// 仮想アドレス→物理アドレス変換
uint32_t vmem_virt_to_phys(uint32_t virt) {
        printk("vmem_virt_to_phys: virt=0x%x mode=%d\n", (unsigned)virt, (int)current_mode);
        if (current_mode == VMEM_MODE_IDENTITY) {
                printk("vmem_virt_to_phys: identity -> 0x%x\n", (unsigned)virt);
                return virt;
        }
        if (current_mode == VMEM_MODE_OFFSET) {
                if ((int32_t)virt - vmem_offset < 0) return 0;
                uint32_t phys = (uint32_t)((int32_t)virt - vmem_offset);
                printk("vmem_virt_to_phys: offset -> phys=0x%x\n", (unsigned)phys);
                return phys;
        }

        uint32_t cr3;
        asm volatile("mov %%cr3, %%eax" : "=a" (cr3));
                uint32_t pd_phys = cr3 & 0xFFFFF000;
        uint32_t pd_virt = phys2virt(pd_phys);
                if (pd_virt == 0) {
                        printk("vmem_virt_to_phys: phys2virt returned 0 for pd_phys=0x%x\n", (unsigned)pd_phys);
                        return 0;
                }

        uint32_t pd_idx = (virt >> 22) & 0x3FF;
        uint32_t pt_idx = (virt >> 12) & 0x3FF;

        uint32_t *pd = (uint32_t *)(uintptr_t)pd_virt;
        uint32_t pde = pd[pd_idx];
                if ((pde & 0x1) == 0) {
                        printk("vmem_virt_to_phys: PDE not present pd_idx=%u pde=0x%x\n", (unsigned)pd_idx, (unsigned)pde);
                        return 0; // 存在しない
                }

        // 4MBページのチェック
                if (pde & 0x80) {
                // 4MBページ: ビット22..31が物理ベース
                uint32_t page_base = pde & 0xFFC00000;
                uint32_t offset = virt & 0x003FFFFF;
                        uint32_t phys = page_base | offset;
                        printk("vmem_virt_to_phys: 4MB page -> phys=0x%x\n", (unsigned)phys);
                        return phys;
        }

        uint32_t pt_phys = pde & 0xFFFFF000;
        uint32_t pt_virt = phys2virt(pt_phys);
                if (pt_virt == 0) {
                        printk("vmem_virt_to_phys: phys2virt returned 0 for pt_phys=0x%x\n", (unsigned)pt_phys);
                        return 0;
                }
        uint32_t *pt = (uint32_t *)(uintptr_t)pt_virt;
        uint32_t pte = pt[pt_idx];
                if ((pte & 0x1) == 0) {
                        printk("vmem_virt_to_phys: PTE not present pt_idx=%u pte=0x%x\n", (unsigned)pt_idx, (unsigned)pte);
                        return 0;
                }
                uint32_t page_base = pte & 0xFFFFF000;
                uint32_t offset = virt & 0xFFF;
                uint32_t phys = page_base | offset;
                printk("vmem_virt_to_phys: resolved phys=0x%x\n", (unsigned)phys);
                return phys;
}

// 物理アドレス→仮想アドレス変換
uint32_t vmem_phys_to_virt(uint32_t phys) {
        /* Treat UINT32_MAX as error sentinel instead of 0 to allow phys==0 */
        if (phys == UINT32_MAX) {
                return UINT32_MAX;
        }

        switch (current_mode) {
        case VMEM_MODE_IDENTITY:
                return phys;
        case VMEM_MODE_OFFSET:
                return phys + (uint32_t)vmem_offset;
        case VMEM_MODE_WALK:
                /* Prefer phys2virt if provided; otherwise fall back to offset/identity */
                if (phys2virt) {
                        uint32_t v = phys2virt(phys);
                        if (v != 0 && v != UINT32_MAX) {
                                return v;
                        }
                        /* fall through to fallback below */
                }
                /* If phys2virt missing or returned invalid, try offset/identity fallback */
                if (vmem_offset != 0) {
                        return phys + (uint32_t)vmem_offset;
                }
                return phys; /* identity fallback */
        default:
                return UINT32_MAX;
        }
}

// オフセット設定
void vmem_set_offset(int32_t offset) {
        vmem_offset = offset;
}

// リセット
void vmem_reset(void) {
        vmem_offset = 0;
}

// モード設定
void vmem_set_mode(vmem_mode_t mode) {
        current_mode = mode;
}

// 物理→仮想変換関数の設定
void vmem_set_phys2virt(vmem_phys2virt_fn fn) {
        if (fn) phys2virt = fn;
        else phys2virt = default_phys2virt;
}
