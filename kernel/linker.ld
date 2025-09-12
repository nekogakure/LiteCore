ENTRY(_start)

SECTIONS
{
    . = 1M;

    .text ALIGN(4K) : {
        *(.multiboot)
        . = ALIGN(8);
        *(.text)
        *(.text.*)
        . = ALIGN(8);
    }

    .rodata ALIGN(4K) : {
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(8);
    }

    .data ALIGN(4K) : {
        _data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(8);
    }

    .bss ALIGN(4K) : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4K);
        _bss_end = .;
    }

    _end = .;

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
