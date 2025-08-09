#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#define KERNEL_VERSION "0.0.1"

/* VGA CONFIG */
#define TEXT_WIDTH 80
#define TEXT_HEIGHT 25
#define TEXT_MEMORY 0xB8000

/* ARCH */
#define PML4_ADDRESS 0x70000
#define PDPT_ADDRESS 0x71000
#define PD_ADDRESS   0x72000
#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE   (1 << 1)
#define PAGE_HUGE    (1 << 7)

#endif // KERNEL_CONFIG_H