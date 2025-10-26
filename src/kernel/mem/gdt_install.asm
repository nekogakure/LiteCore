BITS 32
SECTION .text
global gdt_install
global gdt_install_lgdt
global gdt_install_jump
extern gp

; lgdt only
gdt_install_lgdt:
        mov eax, gp
        lgdt [eax]
        ret

; jump + reload data segments
gdt_install_jump:
        ; far return via call/pop to get offset into eax, then push selector and offset
        call get_eip
get_eip:
        pop eax                ; eax = offset to next instruction
        push word 0x08         ; push selector (16-bit)
        push eax               ; push offset (32-bit)
        retf
jump_continue:
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        ret

; default wrapper: perform lgdt and then jump
gdt_install:
        call gdt_install_lgdt
        call gdt_install_jump
        ret
