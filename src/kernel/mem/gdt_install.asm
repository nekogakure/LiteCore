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
        ; Reload data segments with selector 0x10 (kernel data)
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        
        ; Reload CS with selector 0x08 (kernel code) using far return
        ; Push new CS selector
        push 0x08
        ; Push return address (next instruction)
        lea rax, [rel .reload_cs]
        push rax
        ; Perform far return to reload CS
        retfq
        
.reload_cs:
        ret

; default wrapper: perform lgdt and then reload segments
gdt_install:
        call gdt_install_lgdt
        call gdt_install_jump
        ret
