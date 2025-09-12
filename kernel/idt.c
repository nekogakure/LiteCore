/**
 * @file idt.c
 * @brief Interrupt Descriptor Table implementation
 * @details
 * Initializes and sets up the Interrupt Descriptor Table.
 * @author nekogakure
 * @version 0.1
 * @date 2025-09-12
 */

#include "idt.h"
#include "interrupt.h"

/** @brief Number of IDT entries */
#define IDT_ENTRIES 256

/** @brief IDT table */
static struct idt_entry idt[IDT_ENTRIES];

/** @brief Structure for the IDTR register */
static struct idt_ptr idt_pointer;

/**
 * @brief Load the IDT
 * @details Calls an assembly function to set the IDTR register
 */
extern void idt_flush(uint64_t);

/**
 * @brief Set an IDT entry
 * @param n Interrupt number
 * @param handler Address of the handler function
 */
void idt_set_gate(uint8_t n, interrupt_handler_t handler) {
    uint64_t handler_addr = (uint64_t)handler;
    
    idt[n].base_low = handler_addr & 0xFFFF;
    idt[n].base_mid = (handler_addr >> 16) & 0xFFFF;
    idt[n].base_high = (handler_addr >> 32) & 0xFFFFFFFF;
    
    idt[n].selector = 0x08; // Kernel code segment
    idt[n].ist = 0;
    idt[n].flags = 0x8E;    // Present, Ring0, Interrupt Gate
    idt[n].reserved = 0;
}

/**
 * @brief Initialize the IDT
 * @details 
 * - Set the IDTR
 * - Set the default handler
 */
void init_idt(void) {
    idt_pointer.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idt_pointer.base = (uint64_t)&idt;

    // Clear all entries
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, &default_handler);
    }

    // Set exception handlers
    idt_set_gate(0, &divide_by_zero_handler);
    idt_set_gate(13, &general_protection_fault_handler);
    idt_set_gate(14, &page_fault_handler);

    // Load the IDT
    idt_flush((uint64_t)&idt_pointer);
}
