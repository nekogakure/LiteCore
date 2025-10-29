#include <stdint.h>
#include <mem/vmem.h>

static int32_t vmem_offset = 0;

uint32_t vmem_virt_to_phys(uint32_t virt) {
    if (vmem_offset == 0) return virt;
    if ((int32_t)virt - vmem_offset < 0) return 0;
    return (uint32_t)((int32_t)virt - vmem_offset);
}

uint32_t vmem_phys_to_virt(uint32_t phys) {
    if (vmem_offset == 0) return phys;
    return (uint32_t)((int32_t)phys + vmem_offset);
}

void vmem_set_offset(int32_t offset) {
    vmem_offset = offset;
}

void vmem_reset(void) {
    vmem_offset = 0;
}
