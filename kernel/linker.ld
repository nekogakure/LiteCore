ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5); /* r-x */
  rodata PT_LOAD FLAGS(4); /* r-- */
  data PT_LOAD FLAGS(6); /* rw- */
}

SECTIONS
{
    . = 0x1000;
    
    .text.boot ALIGN(16) : {
        *(.text.boot)
    } :text
    
    .text ALIGN(16) : {
        *(.text)
        *(.text.*)
    } :text
    
    .rodata ALIGN(16) : {
        *(.rodata)
        *(.rodata.*)
        *(.eh_frame)
        *(.rela.dyn)
    } :rodata
    
    .data ALIGN(16) : {
        *(.data)
        *(.data.*)
        *(.got)
        *(.got.plt)
        *(.igot.plt)
    } :data
    
    .bss ALIGN(16) : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    } :data
}
