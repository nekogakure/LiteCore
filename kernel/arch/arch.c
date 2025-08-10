#include "arch.h"
#include "../kernel.h"
#include "../util/vga.h"
#include "../util/config.h"
#include <stdint.h>

struct gdt_entry gdt[3];
struct gdt_ptr gp;

void kernel_arch_init() {
    vga_init();
    text_set_color(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK);
    printk("LiteCore 64bit kernel initialized\n");
}

void setup_paging_tables() {
    uint32_t* pml4 = (uint32_t*)PML4_ADDRESS;
    uint32_t* pdpt = (uint32_t*)PDPT_ADDRESS;
    uint32_t* pd = (uint32_t*)PD_ADDRESS;

    for (int i = 0; i < 512; i++) {
        pml4[i] = 0;
        pdpt[i] = 0;
        pd[i] = 0;
    }

    pml4[0] = (uint32_t)PDPT_ADDRESS | PAGE_PRESENT | PAGE_WRITE;
    pdpt[0] = (uint32_t)PD_ADDRESS | PAGE_PRESENT | PAGE_WRITE;

    for (int i = 0; i < 512; i++) {
        pd[i] = ((uint32_t)i * 0x200000) | PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE;
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

    return (edx & (1u << 29)) != 0;
}

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);

    gdt[num].access = access;
}

void gdt_init() {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base = (uintptr_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);               /* NULL */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF);/* 64bit code */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xAF);/* 64bit data */

    struct __attribute__((packed)) {
        uint16_t limit;
        uint32_t base;
    } gdtr;

    gdtr.limit = gp.limit;
    gdtr.base = (uint32_t)gp.base;

    asm volatile ("lgdt %0" :: "m"(gdtr) : "memory");
}


void prepare_long_mode() {
    setup_paging_tables();

    uint32_t pml4 = (uint32_t)PML4_ADDRESS;
    asm volatile (
        "mov %0, %%eax\n\t"
        "mov %%eax, %%cr3\n\t"
        :: "r"(pml4) : "eax", "memory"
    );

    asm volatile (
        "mov %%cr4, %%eax\n\t"
        "or $0x20, %%eax\n\t"
        "mov %%eax, %%cr4\n\t"
        ::: "eax", "memory"
    );

    uint32_t msr = 0xC0000080u;
    asm volatile (
        "mov %0, %%ecx\n\t"
        "rdmsr\n\t"
        "or $0x100, %%eax\n\t"
        "wrmsr\n\t"
        :: "r"(msr) : "eax", "edx", "ecx", "memory"
    );
}

void switch_to_long_mode() {
    uint32_t pg_bit = 0x80000000;
    asm volatile (
        "mov %%cr0, %%eax\n\t"
        "or %0, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        :: "r"(pg_bit) : "eax", "memory"
    );

    asm volatile ("wbinvd" ::: "memory");

    extern void switch_to_long_mode_asm();
    switch_to_long_mode_asm();

    kernel_panic("Failed to switch to long mode");
}
