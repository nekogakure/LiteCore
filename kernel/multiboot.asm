[BITS 32]
section .multiboot
align 8

; header
header_start:
    ; Magic number
    dd 0xE85250D6                ; Multiboot2 magic
    dd 0                         ; Protected mode i386
    dd header_end - header_start ; Header length
    dd -(0xE85250D6 + 0 + (header_end - header_start)) ; Checksum

    ; Required end tag
    dw 0    ; Type
    dw 0    ; Flags
    dd 8    ; Size
header_end:

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB
stack_top:

section .text
global start
extern kernel_main

start:
    ; Set up stack
    mov esp, stack_top

    ; Preserve multiboot info
    push ebx    ; Multiboot information structure pointer
    push eax    ; Multiboot magic number

    ; Switch to long mode
    call setup_long_mode
    
    ; Jump to 64-bit code
    jmp 0x08:long_mode_start

setup_long_mode:
    ; Disable interrupts
    cli

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set long mode bit
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

[BITS 64]
long_mode_start:
    ; Update segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Convert stack pointer to 64-bit
    mov rax, 0
    mov eax, esp
    mov rsp, rax

    ; Restore multiboot parameters for kernel_main
    pop rdi    ; Magic number (first parameter in System V AMD64 ABI)
    pop rsi    ; Info structure pointer (second parameter)

    ; Call kernel
    call kernel_main

    ; Halt if kernel returns
.halt:
    cli
    hlt
    jmp .halt
