#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include "config.h"



typedef uint64_t page_table_entry_t;

typedef struct {
    page_table_entry_t entries[512];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

/**
 * @brief Initialize paging
 * @details Sets up the initial page tables and enables paging
 * This function should be called during kernel initialization.
 */
void paging_init();

/**
 * @brief Allocate virtual memory
 * @details Allocates a block of virtual memory of the specified size
 * @param size Size of the memory block to allocate
 * @return Pointer to the allocated virtual memory, or NULL on failure
 */
void* valloc(size_t size);

/**
 * @brief Free virtual memory
 * @details Free a previously allocated block of virtual memory
 * @param ptr Pointer to the virtual memory block to free
 */
void vfree(void* ptr);

/**
 * @brief Map a physical address to a virtual address
 * @details Maps a physical address to a virtual address with the specified flags
 * @param phys_addr Physical address to map
 * @param size Size of the memory block to map
 * @param flags Flags for the page table entry (e.g., PAGE_WRITE)
 * @return Pointer to the mapped virtual address, or NULL on failure
 */
void* vmmap(uint64_t phys_addr, size_t size, uint64_t flags);

/**
 * @brief Unmap a virtual address
 * @details Unmaps a previously mapped virtual address
 * @param virt_addr Pointer to the virtual address to unmap
 * @param size Size of the memory block to unmap
 */
void vmunmap(void* virt_addr, size_t size);

/**
 * @brief Check if a page is mapped
 * @details Checks if a virtual address is mapped in the page tables
 * @param virt Virtual address to check
 * @return 1 if the page is mapped, 0 otherwise
 */
int is_page_mapped(uint64_t virt);

/**
 * @brief Get the physical address corresponding to a virtual address
 * @details Translates a virtual address to its corresponding physical address
 * @param virt_addr Pointer to the virtual address
 * @return Physical address corresponding to the virtual address, or 0 if not mapped
 */
uint64_t virt_to_phys(void* virt_addr);

/**
 * @brief Print paging information
 * @details Prints the current paging information, including page tables and mappings
 * This function is useful for debugging and understanding the current paging state.
 */
void paging_print_info();

/**
 * @brief Map a page to a physical address
 * @details Maps a physical address to a virtual address with the specified flags
 * @param phys Physical address to map
 * @param virt Virtual address to map to
 * @param flags Flags for the page table entry (e.g., PAGE_WRITE)
 */
void map_page(uint64_t phys, uint64_t virt, uint64_t flags);

/**
 * @brief Unmap a page from the virtual address space
 * @details Unmaps a page from the virtual address space at the specified virtual address
 * @param virt Virtual address to unmap
 * This function clears the page table entry and frees the associated frame.
 */
void unmap_page(uint64_t virt);

/**
 * @brief Flush the TLB for a specific virtual address
 * @details Flushes the Translation Lookaside Buffer (TLB) for the specified virtual address
 * @param virt Virtual address to flush
 */
void flush_tlb(uint64_t virt);

/**
 * @brief Flush the entire TLB
 * @details Flushes the entire Translation Lookaside Buffer (TLB)
 * This function is used to ensure that all cached translations are cleared.
 */
void flush_tlb_all();

#endif // PAGING_H
