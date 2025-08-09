[bits 16]

global _start
extern k_main
extern kernel_arch_init

section .text.boot
_start:
start_16bit:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    cli

    mov bx, gdt_start
    mov ax, _start
    sub bx, ax

    mov ax, bx
    add ax, 0x7C00
    mov [gdt_ptr + 2], ax

    mov ax, 0
    adc ax, 0
    mov [gdt_ptr + 4], ax

    lgdt [gdt_ptr]

enable_protected_mode:
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protmode

[bits 32]
protmode:
protected_mode:
start_32bit:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

setup_for_long_mode:
    call setup_paging

    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

enable_long_mode:
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    wbinvd
    jmp 0x18:.longmode

[bits 64]
.longmode:

[bits 64]
align 8
start_64bit:
    mov ax, 0x20
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

    cli
.halt:
    hlt
    jmp .halt

extern setup_paging_tables

[bits 32]
setup_paging:
    mov edi, 0x70000
    xor eax, eax
    mov ecx, 4096
    rep stosd

    call setup_paging_tables

    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    mov eax, 0x70000
    mov cr3, eax

    ret

align 16
gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00AF9A000000FFFF
    dq 0x00AF92000000FFFF
gdt_end:

align 8
gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd 0
