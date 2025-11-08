BITS 64
SECTION .text
global gdt_install
global gdt_install_lgdt
global gdt_install_jump
extern gp

; lgdt only
gdt_install_lgdt:
        mov rax, gp
        lgdt [rax]
        ret

; jump + reload data segments
gdt_install_jump:
        ; In 64-bit mode, use a simple approach
        ; Load data segments with selector 0x10
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        ret

; default wrapper: perform lgdt and then reload segments
gdt_install:
        call gdt_install_lgdt
        call gdt_install_jump
        ret
