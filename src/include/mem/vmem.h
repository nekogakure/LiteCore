#ifndef _MEM_VMEM_H
#define _MEM_VMEM_H

#include <stdint.h>

#include <stdint.h>

// vmem modes
typedef enum {
	VMEM_MODE_IDENTITY = 0,
	VMEM_MODE_OFFSET,
	VMEM_MODE_WALK
} vmem_mode_t;

uint32_t vmem_virt_to_phys(uint32_t virt);
uint32_t vmem_phys_to_virt(uint32_t phys);
void vmem_set_offset(int32_t offset);
void vmem_reset(void);
void vmem_set_mode(vmem_mode_t mode);
typedef uint32_t (*vmem_phys2virt_fn)(uint32_t phys);
void vmem_set_phys2virt(vmem_phys2virt_fn fn);

#endif /* _MEM_VMEM_H */