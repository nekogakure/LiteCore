[BITS 32]

; Multiboot2 constants
MAGIC    equ 0xE85250D6        ; Multiboot2 magic number
ARCH     equ 0                 ; Protected mode i386
LENGTH   equ (header_end - header_start)
CHECKSUM equ -(MAGIC + ARCH + LENGTH)

section .multiboot progbits align=8 alloc exec
global multiboot_header
multiboot_header:
header_start:
    dd MAGIC                   ; Multiboot2 magic number (0xE85250D6)
    dd ARCH                    ; Architecture (0 = i386 protected mode)
    dd LENGTH                  ; Header length
    dd CHECKSUM               ; Checksum

    ; Entry address tag
    align 8
    dw 3                      ; Type: entry address
    dw 0                      ; Flags
    dd 12                     ; Size
    dd _start                 ; Entry point address

    ; Module alignment tag
    align 8
    dw 6                      ; Type: module alignment
    dw 0                      ; Flags
    dd 8                      ; Size

    ; Required end tag
    align 8
    dw 0                      ; Type
    dw 0                      ; Flags
    dd 8                      ; Size
header_end:

section .bss nobits align=16
stack_bottom:
    resb 16384               ; 16 KiB
stack_top:

; Page tables
align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2:
    resb 4096

section .rodata
; End markers for Multiboot2 header
global _end_load
_end_load:
    dq 0

section .bss nobits
global _end_bss
align 8
_end_bss:
    resq 1

section .text progbits alloc exec align=16
global _start
extern kernel_main

_start:
    ; Setup stack
    mov esp, stack_top

    ; Save Multiboot2 info
    push ebx                ; Info structure pointer
    push eax                ; Magic value

    ; Check Multiboot2 magic
    cmp eax, 0x36D76289
    jne .no_multiboot

    ; Check for CPUID
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
    jz .no_cpuid

    ; Check for long mode
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

    ; Initialize page tables
    mov edi, page_table_l4
    mov ecx, 3072          ; Number of words to clear (3 pages)
    xor eax, eax
    rep stosd              ; Clear tables

    ; Setup identity mapping for first 2MB
    ; P4 Table
    mov eax, page_table_l3
    or eax, 0b11          ; Present + writable
    mov [page_table_l4], eax

    ; P3 Table
    mov eax, page_table_l2
    or eax, 0b11          ; Present + writable
    mov [page_table_l3], eax

    ; P2 Table - identity map first 2MB
    mov eax, 0x0          ; Start address
    or eax, 0b10000011    ; Present + writable + huge (2MB)
    mov [page_table_l2], eax

    ; Also map higher half kernel space
    mov eax, page_table_l3
    or eax, 0b11          ; Present + writable
    mov [page_table_l4 + 256 * 8], eax  ; Map at 0xFFFF800000000000

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set long mode bit
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Set CR3
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Load GDT
    lgdt [gdt64.pointer]

    ; Update selectors
    jmp gdt64.code:long_mode_start

.no_multiboot:
    mov al, "0"
    jmp error

.no_cpuid:
    mov al, "1"
    jmp error

.no_long_mode:
    mov al, "2"
    jmp error

error:
    ; Print "ERR: X" where X is the error code
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov byte  [0xb800a], al
    hlt

section .rodata
gdt64:
    dq 0                    ; Zero entry
.code: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
                            ; Code segment: 64-bit, present, code
.pointer:
    dw $ - gdt64 - 1       ; Length
    dq gdt64               ; Base address

[BITS 64]
long_mode_start:
    ; Initialize all segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Restore multiboot parameters
    pop rdi                ; Magic value
    pop rsi                ; Info structure pointer

    ; Call kernel
    call kernel_main

    ; Halt if we return from kernel
.halt:
    cli
    hlt
    jmp .halt
