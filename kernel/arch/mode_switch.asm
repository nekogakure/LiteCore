[bits 32]

global switch_to_long_mode_asm
extern kernel_arch_init
extern k_main
extern kernel_panic

section .text
switch_to_long_mode_asm:
    mov eax, 0x0BADC0DE
    mov ebx, 0x12345678

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.longmode

[bits 64]
.longmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, 0x200000
    xor rdi, rdi
    xor rsi, rsi[bits 32]

global switch_to_long_mode_asm
extern kernel_arch_init
extern k_main
extern kernel_panic

section .text
switch_to_long_mode_asm:
    mov eax, 0x0BADC0DE
    mov ebx, 0x12345678

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.longmode

[bits 64]
.longmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, 0x200000
    xor rdi, rdi
    xor rsi, rsi
    xor rdx, rdx
    xor rcx, rcx
    xor r8, r8
    xor r9, r9

    and rsp, -16

    call kernel_arch_init

    call k_main

    mov rdi, kernel_exit_message
    call kernel_panic

    cli
.halt:
    hlt
    jmp .halt

section .data
kernel_exit_message: db "Kernel main function returned unexpectedly", 0

    xor rdx, rdx
    xor rcx, rcx
    xor r8, r8
    xor r9, r9

    and rsp, -16

    call kernel_arch_init

    call k_main

    mov rdi, kernel_exit_message
    call kernel_panic

    cli
.halt:
    hlt
    jmp .halt

section .data
kernel_exit_message: db "Kernel main function returned unexpectedly", 0
