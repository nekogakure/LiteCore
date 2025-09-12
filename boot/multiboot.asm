[BITS 32]

section .multiboot
global _start
global multiboot_header
align 8

; Constants
MB2_MAGIC    equ 0xE85250D6
MB2_ARCH     equ 0
MB2_LENGTH   equ (multiboot_header_end - multiboot_header_start)
MB2_CHECKSUM equ -(MB2_MAGIC + MB2_ARCH + MB2_LENGTH)

; Multiboot2 header
multiboot_header_start:
    dd MB2_MAGIC        ; Magic value
    dd MB2_ARCH         ; Architecture
    dd MB2_LENGTH       ; Header length
    dd MB2_CHECKSUM     ; Checksum

    ; Address tag
    align 8
    dw 2               ; Type: address
    dw 0               ; Flags
    dd 24              ; Size
    dd multiboot_header_start  ; Header addr
    dd _start          ; Load addr
    dd _start          ; Load end addr
    dd _start          ; BSS end addr

    ; Required end tag
    align 8
    dw 0    ; Type
    dw 0    ; Flags
    dd 8    ; Size
multiboot_header_end:
    align 8  ; Ensure proper alignment

; Page table structures
section .data
align 4096
; Level 4 page table (PML4)
global pml4_table
pml4_table:
    resb 4096
; Level 3 page table (PDP)
global pdp_table
pdp_table:
    resb 4096
; Level 2 page table (PD)
global pd_table
pd_table:
    resb 4096

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

    ; Save multiboot info
    push ebx    ; Multiboot structure pointer
    push eax    ; Magic number

    ; Check for CPUID support
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz no_long_mode    ; CPUID not supported

    ; Check for long mode support
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_long_mode    ; Long mode not supported
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_long_mode    ; Long mode not supported

    ; Set up page tables
    call setup_page_tables
    
    ; Enable paging and switch to long mode
    call enable_paging

    ; Load 64-bit GDT
    lgdt [gdt64.pointer]

    ; Jump to long mode
    jmp gdt64.code:long_mode_start

; Error handler
no_long_mode:
    mov dword [0xb8000], 0x4f524f45 ; "ER"
    mov dword [0xb8004], 0x4f3a4f52 ; "R:"
    mov dword [0xb8008], 0x4f204f20 ; "  "
    mov dword [0xb800c], 0x4f544f4e ; "NO"
    mov dword [0xb8010], 0x4f204f20 ; "  "
    mov dword [0xb8014], 0x4f4d4f4c ; "LM"
    hlt

setup_page_tables:
    ; Clear tables
    mov edi, pml4_table
    mov ecx, 3072      ; 3 tables * 1024 entries
    xor eax, eax
    rep stosd

    ; PML4[0] -> PDP
    mov eax, pdp_table
    or eax, 0b11       ; Present + Writable
    mov [pml4_table], eax

    ; PDP[0] -> PD
    mov eax, pd_table
    or eax, 0b11       ; Present + Writable
    mov [pdp_table], eax

    ; Identity map first 2MB with 2MB pages
    mov eax, 0
    or eax, 0b10000011 ; Present + Writable + Huge
    mov [pd_table], eax

    ret

enable_paging:
    ; Load PML4 address to CR3
    mov eax, pml4_table
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5     ; PAE bit
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8     ; Long mode bit
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31    ; Paging bit
    mov cr0, eax

    ret

; GDT for 64-bit mode
section .rodata
gdt64:
    dq 0 ; Zero entry
.code: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; Code segment
.pointer:
    dw $ - gdt64 - 1   ; Length
    dq gdt64           ; Address

[BITS 64]
long_mode_start:
    ; Update segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Restore multiboot parameters
    pop rdi    ; Magic number (first parameter)
    pop rsi    ; Info structure pointer (second parameter)

    ; Call kernel
    call kernel_main

    ; Halt if kernel returns
.halt:
    cli
    hlt
    jmp .halt
