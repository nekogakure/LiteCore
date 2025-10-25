#ifndef _MEM_MAP_H
#define _MEM_MAP_H

#include <config.h>

// 4KB
#define FRAME_SIZE 4096

void memmap_init(uint32_t start, uint32_t end);
void* alloc_frame(void);
void free_frame(void* addr);
uint32_t frame_count(void);
void memmap_reserve(uint32_t start, uint32_t end);

#endif /* _MEM_MAP_H */
