#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

enum memory_map_type {
    MEMORY_AVAILABLE = 1,
    MEMORY_RESERVED = 2,
    MEMORY_ACPI_RECLAIMABLE = 3,
    MEMORY_NVS = 4,
    MEMORY_BADRAM = 5
};

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} memory_map_entry_t;

typedef struct memory_block {
    size_t size;
    uint8_t is_free;
    struct memory_block* next;
} memory_block_t;

void memory_init();
void* kmalloc(size_t size);
void kfree(void* ptr);
void memory_print_stats();
size_t get_free_memory();

#endif // MEMORY_H
