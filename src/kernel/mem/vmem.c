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
static vmem_phys2virt_fn phys2virt = default_phys2virt;

// 仮想アドレス→物理アドレス変換
uint32_t vmem_virt_to_phys(uint32_t virt) {
        if (current_mode == VMEM_MODE_IDENTITY) return virt;
        if (current_mode == VMEM_MODE_OFFSET) {
                if ((int32_t)virt - vmem_offset < 0) return 0;
                return (uint32_t)((int32_t)virt - vmem_offset);
        }

        uint32_t cr3;
        asm volatile("mov %%cr3, %%eax" : "=a" (cr3));
        uint32_t pd_phys = cr3 & 0xFFFFF000;
        uint32_t pd_virt = phys2virt(pd_phys);
        if (pd_virt == 0) return 0;

        uint32_t pd_idx = (virt >> 22) & 0x3FF;
        uint32_t pt_idx = (virt >> 12) & 0x3FF;

        uint32_t *pd = (uint32_t *)(uintptr_t)pd_virt;
        uint32_t pde = pd[pd_idx];
        if ((pde & 0x1) == 0) return 0; // 存在しない

        // 4MBページのチェック
        if (pde & 0x80) {
                // 4MBページ: ビット22..31が物理ベース
                uint32_t page_base = pde & 0xFFC00000;
                uint32_t offset = virt & 0x003FFFFF;
                return page_base | offset;
        }

        uint32_t pt_phys = pde & 0xFFFFF000;
        uint32_t pt_virt = phys2virt(pt_phys);
        if (pt_virt == 0) return 0;
        uint32_t *pt = (uint32_t *)(uintptr_t)pt_virt;
        uint32_t pte = pt[pt_idx];
        if ((pte & 0x1) == 0) return 0;
        uint32_t page_base = pte & 0xFFFFF000;
        uint32_t offset = virt & 0xFFF;
        return page_base | offset;
}

// 物理アドレス→仮想アドレス変換
uint32_t vmem_phys_to_virt(uint32_t phys) {
        if (phys2virt) return phys2virt(phys);
        return phys;
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
