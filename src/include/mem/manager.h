#ifndef _MEM_MANAGER_H
#define _MEM_MANAGER_H

#include <config.h>

void mem_init(uint32_t start, uint32_t end);
void* kmalloc(uint32_t size);
void kfree(void* ptr);

#endif /* _MEM_MANAGER_H */
