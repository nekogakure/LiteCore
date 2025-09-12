/**
 * @file gdt.c
 * @brief Global Descriptor Table implementation
 * @details
 * Initializes and sets up the GDT.
 * On x86_64, segmentation is basically disabled,
 * but minimal setup is required for compatibility.
 * @author nekogakure
 * @version 0.1
 * @date 2025-09-12
 */

#include "gdt.h"

/** @brief Number of GDT entries */
#define GDT_ENTRIES 5

/** @brief GDT table */
static struct gdt_entry gdt[GDT_ENTRIES];

/** @brief Structure for GDTR register */
static struct gdt_ptr gdt_pointer;

/**
 * @brief Set a GDT entry
 * @param num Entry number
 * @param base Base address
 * @param limit Limit
 * @param access Access rights
 * @param gran Granularity
 */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);

    gdt[num].access = access;
}

/**
 * @brief Load the GDT
 * @details Calls an assembly function to set the GDTR register
 */
extern void gdt_flush(uint64_t);

/**
 * @brief Initialize the GDT
 * @details 
 * Sets up the following segments:
 * - NULL segment
 * - Kernel code segment
 * - Kernel data segment
 * - User code segment
 * - User data segment
 */
void init_gdt(void) {
    gdt_pointer.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_pointer.base = (uint64_t)&gdt;

    // NULL segment
    gdt_set_gate(0, 0, 0, 0, 0);

    // Kernel code segment
    gdt_set_gate(1, 0, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING0 | GDT_SYSTEM | GDT_EXECUTABLE | GDT_RW,
                 GDT_GRANULARITY | GDT_64BIT);

    // Kernel data segment
    gdt_set_gate(2, 0, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING0 | GDT_SYSTEM | GDT_RW,
                 GDT_GRANULARITY | GDT_64BIT);

    // User code segment
    gdt_set_gate(3, 0, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING3 | GDT_SYSTEM | GDT_EXECUTABLE | GDT_RW,
                 GDT_GRANULARITY | GDT_64BIT);

    // User data segment
    gdt_set_gate(4, 0, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING3 | GDT_SYSTEM | GDT_RW,
                 GDT_GRANULARITY | GDT_64BIT);

    // Load the GDT
    gdt_flush((uint64_t)&gdt_pointer);
}
