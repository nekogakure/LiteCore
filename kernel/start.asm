[bits 16]

global _start
extern gdt_init
extern switch_to_protected_mode
extern prepare_long_mode
extern switch_to_long_mode
extern kernel_panic

section .text.boot
_start:
start_16bit:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    cli
    call gdt_init
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp 0x08:protmode

[bits 32]
protmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    call prepare_long_mode

    call switch_to_long_mode
    
    push boot_failure_message
    call kernel_panic
    add esp, 4
    
    cli
.halt:
    hlt
    jmp .halt

section .data
boot_failure_message: db "Failed to switch to long mode", 0
