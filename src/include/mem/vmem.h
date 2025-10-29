#ifndef _MEM_VMEM_H
#define _MEM_VMEM_H

#include <stdint.h>

uint32_t vmem_virt_to_phys(uint32_t virt);
uint32_t vmem_phys_to_virt(uint32_t phys);
void vmem_set_offset(int32_t offset);
void vmem_reset(void);

#endif /* _MEM_VMEM_H */