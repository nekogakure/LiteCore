ENTRY(kernel_entry)
OUTPUT_ARCH(i386)

SECTIONS {
        . = 0x10000;

        .text : {
                KEEP(*(.text.kernel_entry))
                *(.text)
                *(.text.*)
        }

        . = ALIGN(0x1000);

        .rodata : {
                *(.rodata)
                *(.rodata.*)
                *(.eh_frame)
        }

        . = ALIGN(0x1000);

        .data : {
                *(.data)
                *(.data.*)
                *(.got.plt)
        }

        .bss ALIGN(4) : {
                *(.bss)
        }

        __end = .;
}
