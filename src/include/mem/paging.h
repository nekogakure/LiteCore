#ifndef _MEM_PAGING_H
#define _MEM_PAGING_H

void paging_init_identity(uint32_t map_mb);
void paging_enable(void);

#include <stddef.h>

#define PAGING_PRESENT 0x1
#define PAGING_RW      0x2
#define PAGING_USER    0x4

void *alloc_page_table(void);
int map_page(uint32_t phys, uint32_t virt, uint32_t flags);
int unmap_page(uint32_t virt);
int map_range(uint32_t phys_start, uint32_t virt_start, size_t size, uint32_t flags);
void page_fault_handler_ex(uint32_t vec, uint32_t error_code, uint32_t eip);

#endif /* _MEM_PAGING_H */
