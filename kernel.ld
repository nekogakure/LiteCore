/* LiteCore Kernel Linker Script */
/* x86-64 BIOS/Multiboot2 カーネル用リンカースクリプト */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(boot_main)

SECTIONS
{
        /* カーネルは1MBからロード*/
        . = 0x100000;

        /* Multiboot2ヘッダー*/
        .multiboot : ALIGN(8)
        {
                KEEP(*(.multiboot*))
        }

        .text : ALIGN(4K)
        {
                *(.text*)
        }

        .rodata : ALIGN(4K)
        {
                *(.rodata*)
        }

        .data : ALIGN(4K)
        {
                *(.data*)
        }

        .bss : ALIGN(4K)
        {
                *(COMMON)
                *(.bss*)
        }

        /* カーネルの終端アドレス */
        _kernel_end = .;
}