ENTRY(_start)

SECTIONS
{
    . = 0x1000;
    
    .text.boot : ALIGN(16)
    {
        *(.text)
    }
    
    .text : ALIGN(16)
    {
        *(.text.*)
    }
    
    .rodata : ALIGN(16)
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data : ALIGN(16)
    {
        *(.data)
        *(.data.*)
    }
    
    .bss : ALIGN(16)
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
}
