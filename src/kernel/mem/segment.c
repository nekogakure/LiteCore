#include <config.h>
#include <stdint.h>

/* GDTエントリ構造体 */
struct gdt_entry {
        uint16_t limit_low;
        uint16_t base_low;
        uint8_t base_middle;
        uint8_t access;
        uint8_t granularity;
        uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
        uint16_t limit;
        uint32_t base;
} __attribute__((packed));

/* ここではNULL, kernel code, kernel data, user code, user dataの5つ */
static struct gdt_entry gdt_entries[5];
static struct gdt_ptr gp;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
        gdt_entries[num].base_low = (base & 0xFFFF);
        gdt_entries[num].base_middle = (base >> 16) & 0xFF;
        gdt_entries[num].base_high = (base >> 24) & 0xFF;

        gdt_entries[num].limit_low = (limit & 0xFFFF);
        gdt_entries[num].granularity = (limit >> 16) & 0x0F;

        gdt_entries[num].granularity |= (gran & 0xF0);
        gdt_entries[num].access = access;
}

/**
 * @fn gdt_install
 * @brief カーネル側でGDTを再構築してロードする
 */
void gdt_install() {
        gdt_set_gate(0, 0, 0, 0, 0);

        /* Kernel code segment: base=0, limit=0xFFFFF, access=0x9A, gran=0xCF */
        gdt_set_gate(1, 0x0, 0xFFFFF, 0x9A, 0xCF);
        /* Kernel data segment */
        gdt_set_gate(2, 0x0, 0xFFFFF, 0x92, 0xCF);
        /* User code (ring3) */
        gdt_set_gate(3, 0x0, 0xFFFFF, 0xFA, 0xCF);
        /* User data (ring3) */
        gdt_set_gate(4, 0x0, 0xFFFFF, 0xF2, 0xCF);

        gp.limit = (sizeof(struct gdt_entry) * 5) - 1;
        gp.base = (uint32_t)&gdt_entries;

        asm volatile(
                "lgdt (%0)\n"
                "pushl $0x08\n"
                "pushl $1f\n"
                "lret\n"
                "1:\n"
                "mov $0x10, %%ax\n"
                "mov %%ax, %%ds\n"
                "mov %%ax, %%es\n"
                "mov %%ax, %%fs\n"
                "mov %%ax, %%gs\n"
                "mov %%ax, %%ss\n"
                :: "r"(&gp) : "eax", "memory"
        );
}
