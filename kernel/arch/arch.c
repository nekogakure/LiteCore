#include "arch.h"
#include "../kernel.h"
#include "../util/vga.h"
#include "../util/config.h"

void kernel_arch_init() {
    text_init();
    text_set_color(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK);
    printk("LiteCore 64bit kernel initialized\n");
}

void setup_paging_tables() {
    uint32_t* pml4 = (uint32_t*)PML4_ADDRESS;
    uint32_t* pdpt = (uint32_t*)PDPT_ADDRESS;
    uint32_t* pd = (uint32_t*)PD_ADDRESS;
    
    for (int i = 0; i < 1024; i++) {
        pml4[i] = 0;
        pdpt[i] = 0;
        pd[i] = 0;
    }
    
    pml4[0] = PDPT_ADDRESS | PAGE_PRESENT | PAGE_WRITE;
    
    pdpt[0] = PD_ADDRESS | PAGE_PRESENT | PAGE_WRITE;
    
    for (int i = 0; i < 16; i++) {
        pd[i*2] = (i * 0x200000) | PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE;
        pd[i*2 + 1] = 0;
    }
}

int cpu_has_long_mode() {
    uint32_t eax, ebx, ecx, edx;
    
    asm volatile("cpuid"
                 : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
                 : "a" (0x80000000)
                 : "memory");
    
    if (eax < 0x80000001) {
        return 0;
    }
    
    asm volatile("cpuid"
                 : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
                 : "a" (0x80000001)
                 : "memory");
    
    return (edx & (1 << 29)) != 0;
}
