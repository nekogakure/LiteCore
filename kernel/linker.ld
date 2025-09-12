OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    . = 1M;
    
    .multiboot ALIGN(4K) : {
        *(.multiboot)
    }

    .text ALIGN(4K) : {
        *(.text)
        *(.text.*)
        . = ALIGN(4K);
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
