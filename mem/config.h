/**
 * @file mem/config.h
 * @brief Memory configuration and constants
 * @details This file defines memory-related constants and macros for the LiteCore kernel,
 * including heap configuration, page size, and paging flags.
 */

#ifndef MEM_CONFIG_H
#define MEM_CONFIG_H

#define HEAP_START 0x300000
#define HEAP_SIZE  0x100000  // 1MB
#define HEAP_END   (HEAP_START + HEAP_SIZE)

#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

#define PML4_ENTRIES 512
#define PDPT_ENTRIES 512
#define PD_ENTRIES 512
#define PT_ENTRIES 512

#include "../kernel/util/config.h"

// Extended paging flags not defined in kernel config
#define PAGE_USER       (1ULL << 2)  // user mode writable
#define PAGE_WRITETHROUGH (1ULL << 3)  // cache write policy
#define PAGE_CACHE_DISABLE (1ULL << 4)  // disable cache
#define PAGE_ACCESSED   (1ULL << 5)  // accessed
#define PAGE_DIRTY      (1ULL << 6)  // written
#define PAGE_GLOBAL     (1ULL << 8)  // global (keep on TLB flush)
#define PAGE_NX         (1ULL << 63) // disallow execution

#define KERNEL_BASE     0xFFFFFFFF80000000  // kernel virtual base address
#define KERNEL_PHYS_BASE 0x100000            // kernel physical base address
#define KERNEL_VIRT_TO_PHYS(addr) ((addr) - KERNEL_BASE + KERNEL_PHYS_BASE)
#define KERNEL_PHYS_TO_VIRT(addr) ((addr) - KERNEL_PHYS_BASE + KERNEL_BASE)

#endif // MEM_CONFIG_H
