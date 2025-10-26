#include <config.h>
#include <console.h>
#include <mem/paging.h>

void paging_test(void) {
    paging_init_identity(16);
    printk("identity map set\n");

    paging_enable();
    printk("paging enabled\n");

    /* Access a low physical address (identity mapped) to ensure no PF */
    volatile uint32_t *p = (uint32_t *)0x1000;
    uint32_t val = *p; /* might be zero or garbage but should not fault */
    (void)val;
    printk("read from 0x1000 OK\n");

    /* Touch a high unmapped address to trigger PF (optional) */
#ifdef PAGING_TEST_TRIGGER_PF
    volatile uint32_t *bad = (uint32_t *)0xC0000000; /* likely unmapped */
    uint32_t badval = *bad; /* expect page fault handler */
    (void)badval;
    printk("unexpected: no page fault\n");
#endif

    printk("done\n");
}
