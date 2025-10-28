#ifndef _MEM_PAGING_H
#define _MEM_PAGING_H

void paging_init_identity(uint32_t map_mb);
void paging_enable(void);

#define PAGING_PRESENT 0x1
#define PAGING_RW      0x2
#define PAGING_USER    0x4

void *alloc_page_table(void);
int map_page(uint32_t phys, uint32_t virt, uint32_t flags);
int unmap_page(uint32_t virt);

#endif /* _MEM_PAGING_H */
