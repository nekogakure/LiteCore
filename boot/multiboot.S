.section .multiboot
.align 8

multiboot_header_start:
        .long 0x36d76289
        .long 0
        .long multiboot_header_end - multiboot_header_start
        .long -(0x36d76289 + 0 + (multiboot_header_end - multiboot_header_start))

        .word 0
        .word 0
        .long 8
multiboot_header_end: