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

    asm volatile("lgdt (%0)" : : "r" (&gp) : "memory");
}

void switch_to_protected_mode() {
    gdt_init();
    
    asm volatile(
        "movq %%cr0, %%rax\n"
        "or $1, %%rax\n"
        "movq %%rax, %%cr0\n"
        ::: "rax", "memory"
    );
    
    asm volatile(
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"
        ::: "ax", "memory"
    );
}

void prepare_long_mode() {
    setup_paging_tables();
    
    asm volatile(
        "mov %0, %%rax\n"
        "mov %%rax, %%cr3\n"
        :: "r" ((uint64_t)PML4_ADDRESS)
        : "rax", "memory"
    );
    
    asm volatile(
        "movq %%cr4, %%rax\n"
        "or $ (1 << 5), %%rax\n"
        "movq %%rax, %%cr4\n"
        ::: "rax", "memory"
    );
    
    asm volatile(
        "mov $0xC0000080, %%ecx\n"
        "rdmsr\n"
        "or $ (1 << 8), %%eax\n"
        "wrmsr\n"
        ::: "rax", "rdx", "rcx", "memory"
    );
}


void switch_to_long_mode() {
    asm volatile(
        "movq %%cr0, %%rax\n"
        "movabs $0x8000000000000000, %%rcx\n"
        "orq %%rcx, %%rax\n"
        "movq %%rax, %%cr0\n"
        ::: "rax", "rcx", "memory"
    );
    
    asm volatile("wbinvd" ::: "memory");
    
    extern void switch_to_long_mode_asm();
    switch_to_long_mode_asm();
    
    kernel_panic("Failed to switch to long mode");
    kernel_print_error("Failed to switch to long mode");
}
