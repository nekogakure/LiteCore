#include <util/config.h>
#include <util/console.h>
#include <mem/vmem.h>
#include <mem/map.h>
#include <tests/define.h>

void vmem_test(void) {
	printk("default identity check\n");
	uint32_t p = 0x1000;
	uint32_t v = vmem_phys_to_virt(p);
	uint32_t p2 = vmem_virt_to_phys(v);
	printk("phys=0x%x -> virt=0x%x -> phys=0x%x\n", (unsigned)p,
	       (unsigned)v, (unsigned)p2);

	printk("offset mode check\n");
	vmem_set_offset(0xC0000000); // example kernel-high offset
	uint32_t v_off = vmem_phys_to_virt(p);
	uint32_t p_off = vmem_virt_to_phys(v_off);
	printk("phys=0x%x -> virt=0x%x -> phys=0x%x\n", (unsigned)p,
	       (unsigned)v_off, (unsigned)p_off);

	vmem_reset();
}
