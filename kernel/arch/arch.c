#include "arch.h"
#include "../kernel.h"
#include "../util/vga.h"
#include "../util/config.h"
#include <stdint.h>

struct gdt_entry gdt[5];
struct gdt_ptr gp;

void kernel_arch_init() {
    vga_init();
    text_set_color(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK);
    printk("LiteCore 64bit kernel initialized\n");
}

void setup_paging_tables() {
    uint64_t* pml4 = (uint64_t*)PML4_ADDRESS;
    uint64_t* pdpt = (uint64_t*)PDPT_ADDRESS;
    uint64_t* pd = (uint64_t*)PD_ADDRESS;

    for (int i = 0; i < 512; i++) {
        pml4[i] = 0;
        pdpt[i] = 0;
        pd[i] = 0;
    }

    pml4[0] = (uint64_t)PDPT_ADDRESS | PAGE_PRESENT | PAGE_WRITE;
    pdpt[0] = (uint64_t)PD_ADDRESS | PAGE_PRESENT | PAGE_WRITE;

    for (int i = 0; i < 512; i++) {
        pd[i] = ((uint64_t)i * 0x200000ULL) | PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE;
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
    gp.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gp.base = (uintptr_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);               /* NULL */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);/* 32bit code */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);/* 32bit data */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0x9A, 0xAF);/* 64bit code */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0x92, 0xAF);/* 64bit data */

    struct __attribute__((packed)) {
        uint16_t limit;
        uint64_t base;
    } gdtr;

    gdtr.limit = gp.limit;
    gdtr.base = (uint64_t)gp.base;

    asm volatile ("lgdt %0" :: "m"(gdtr) : "memory");
}

void switch_to_protected_mode() {
    gdt_init();

    asm volatile (
        "mov %%cr0, %%rax\n\t"
        "or $1, %%rax\n\t"
        "mov %%rax, %%cr0\n\t"
        ::: "rax", "memory"
    );

    asm volatile (
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        ::: "ax", "memory"
    );
}

void prepare_long_mode() {
    setup_paging_tables();

    uint64_t pml4 = (uint64_t)PML4_ADDRESS;
    asm volatile (
        "mov %0, %%rax\n\t"
        "mov %%rax, %%cr3\n\t"
        :: "r"(pml4) : "rax", "memory"
    );

    asm volatile (
        "mov %%cr4, %%rax\n\t"
        "or $0x20, %%rax\n\t"
        "mov %%rax, %%cr4\n\t"
        ::: "rax", "memory"
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
    uint64_t pg_bit = 0x80000000ULL;
    asm volatile (
        "mov %%cr0, %%rax\n\t"
        "or %0, %%rax\n\t"
        "mov %%rax, %%cr0\n\t"
        :: "r"(pg_bit) : "rax", "memory"
    );

    asm volatile ("wbinvd" ::: "memory");

    extern void switch_to_long_mode_asm();
    switch_to_long_mode_asm();

    kernel_panic("Failed to switch to long mode");
}
