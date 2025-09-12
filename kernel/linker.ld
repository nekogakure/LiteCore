ENTRY(_start)

SECTIONS
{
    . = 1M;

    .text : {
        /* Multiboot header must be at the very beginning */
        *(.multiboot)
        *(.text.boot)
        *(.text)
    }
    . = ALIGN(8);

    /* Read-only data */
    .rodata :
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data :
    {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss :
    {
        *(COMMON)
        *(.bss)
    }
}
