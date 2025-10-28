#include <config.h>
#include <console.h>
#include <mem/paging.h>
#include <mem/map.h>

void paging_test(void) {
    printk("[PAGING TEST] start\n");

    /* Set up a small identity mapping for low memory so kernel stays accessible */
    paging_init_identity(1); /* map first 1 MB */
    printk("[PAGING TEST] identity map (1MB) set\n");

    /* Allocate a physical frame to map */
    void *frame = alloc_frame();
    if (!frame) {
        printk("[PAGING TEST] alloc_frame failed\n");
        return;
    }
    uint32_t phys = (uint32_t)frame;
    printk("[PAGING TEST] allocated phys frame=0x%x\n", (unsigned)phys);

    /* choose a virtual address likely unmapped (above identity region) */
    uint32_t virt = 0x4000000; /* 64MB */

    /* map the page (present + rw) */
    if (map_page(phys, virt, PAGING_PRESENT | PAGING_RW) != 0) {
        printk("[PAGING TEST] map_page failed\n");
        return;
    }
    printk("[PAGING TEST] mapped phys 0x%x -> virt 0x%x\n", (unsigned)phys, (unsigned)virt);

    /* enable paging */
    paging_enable();
    printk("[PAGING TEST] paging enabled\n");

    /* Access the newly mapped virtual address */
    volatile uint32_t *p = (uint32_t *)virt;
    *p = 0xdeadbeef;
    uint32_t v = *p;
    printk("[PAGING TEST] write/read virt 0x%x -> 0x%x\n", (unsigned)virt, (unsigned)v);

    /* unmap the page (do not access after this to avoid page fault) */
    if (unmap_page(virt) == 0) {
        printk("[PAGING TEST] unmapped virt 0x%x\n", (unsigned)virt);
    } else {
        printk("[PAGING TEST] unmap_page failed for 0x%x\n", (unsigned)virt);
    }

    /* remap the same physical frame to a different virtual address and verify */
    uint32_t virt2 = 0x4100000; /* 65MB */
    if (map_page(phys, virt2, PAGING_PRESENT | PAGING_RW) != 0) {
        printk("[PAGING TEST] remap failed\n");
        return;
    }
    printk("[PAGING TEST] remapped phys 0x%x -> virt 0x%x\n", (unsigned)phys, (unsigned)virt2);
    volatile uint32_t *p2 = (uint32_t *)virt2;
    uint32_t v2 = *p2; /* should reflect previous write if caching coherent */
    printk("[PAGING TEST] read virt2 0x%x -> 0x%x\n", (unsigned)virt2, (unsigned)v2);

    printk("[PAGING TEST] done\n");
}
