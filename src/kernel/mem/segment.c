#include <config.h>
#include <stdint.h>
#include <stddef.h>
#include <mem/segment.h>

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
struct gdt_entry gdt_entries[5];
struct gdt_ptr gp;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
        gdt_entries[num].base_low = (base & 0xFFFF);
        gdt_entries[num].base_middle = (base >> 16) & 0xFF;
        gdt_entries[num].base_high = (base >> 24) & 0xFF;

        gdt_entries[num].limit_low = (limit & 0xFFFF);
        gdt_entries[num].granularity = (limit >> 16) & 0x0F;

        gdt_entries[num].granularity |= (gran & 0xF0);
        gdt_entries[num].access = access;
}

void gdt_build() {
        gdt_set_gate(0, 0, 0, 0, 0);
        gdt_set_gate(1, 0x0, 0xFFFFF, 0x9A, 0xCF); /* code */
        gdt_set_gate(2, 0x0, 0xFFFFF, 0x92, 0xCF); /* data */
        gdt_set_gate(3, 0x0, 0xFFFFF, 0xFA, 0xCF); /* user code */
        gdt_set_gate(4, 0x0, 0xFFFFF, 0xF2, 0xCF); /* user data */
        gp.limit = (sizeof(struct gdt_entry) * 5) - 1;
        gp.base = (uint32_t)&gdt_entries;
}

void gdt_install();

void gdt_dump(void) {
        extern void printk(const char *fmt, ...);
        printk("[GDT DUMP] gp.base=0x%08x gp.limit=0x%04x\n", gp.base, gp.limit);
        for (int i = 0; i < 3; i++) {
                unsigned char *b = (unsigned char *)&gdt_entries[i];
                printk("gdt[%d]: ", i);
                for (size_t j = 0; j < sizeof(struct gdt_entry); j++) {
                        printk("%02x", b[j]);
                        if (j != sizeof(struct gdt_entry)-1) printk(":");
                }
                printk("\n");
        }
}
