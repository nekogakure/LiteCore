section .multiboot
align 8

global multiboot_header
multiboot_header:
    dd 0xE85250D6        ; Magic number
    dd 0                 ; Architecture (i386)
    dd header_end - multiboot_header ; Header length
    dd -(0xE85250D6 + 0 + (header_end - multiboot_header)) ; Checksum

    ; End tag
    dw 0                ; Type
    dw 0                ; Flags
    dd 8                ; Size
header_end:

; Kernel entry point
section .text
global _start          ; Make entry point visible to linker
extern kernel_main     ; Kernel main function is defined elsewhere

_start:
    cli               ; Disable interrupts
    mov esp, stack_top; Set up stack
    push ebx          ; Multiboot info structure
    push eax          ; Magic value
    call kernel_main  ; Call kernel
    
.hang:               ; Infinite loop if we return
    hlt
    jmp .hang

; Stack
section .bss
align 16
stack_bottom:
    resb 16384       ; 16 KiB
stack_top:

    ; Required end tag
    dw 0    ; Type
    dw 0    ; Flags
    dd 8    ; Size
header_end:

section .bss
align 16
stack_bottom:
    resb 16384         ; 16 KB for stack
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; Set up stack
    mov esp, stack_top
    
    ; Save multiboot info
    push ebx            ; Multiboot info structure pointer
    push eax            ; Magic number
    
    ; Enter kernel
    call kernel_main
    
    ; If we return from kernel_main, halt the system
.halt:
    cli                 ; Disable interrupts
    hlt                 ; Halt the CPU
    jmp .halt           ; Just in case
MB2_LENGTH   equ (multiboot_header_end - multiboot_header_start)
MB2_CHECKSUM equ -(MB2_MAGIC + MB2_ARCH + MB2_LENGTH)

; Multiboot2 header
multiboot_header:
multiboot_header_start:
    dd MB2_MAGIC        ; Magic number
    dd MB2_ARCH         ; Architecture
    dd MB2_LENGTH       ; Header length
    dd MB2_CHECKSUM     ; Checksum

    ; Information request tag
    align 8
    dw 1                ; Type: information request
    dw 0                ; Flags
    dd 16               ; Size
    dd 4                ; Request basic memory information
    dd 6                ; Request memory map

    ; Address tag
    align 8
    dw 2                ; Type: address
    dw 0                ; Flags
    dd 24               ; Size
    dd multiboot_header ; Header addr
    dd _start           ; Load addr
    dd _end             ; Load end addr
    dd _bss_end         ; BSS end addr

    ; Entry tag
    align 8
    dw 3                ; Type: entry address
    dw 0                ; Flags
    dd 12               ; Size
    dd _start           ; Entry point address

    ; Required end tag
    align 8
    dw 0                ; Type
    dw 0                ; Flags
    dd 8                ; Size
multiboot_header_end:
    align 8             ; Ensure proper alignment

section .bss nobits align=4096
global _bss_start
_bss_start:

; Stack
align 16
stack_bottom:
    resb 16384               ; 16 KiB
stack_top:

; Page tables
align 4096
global page_table_l4
page_table_l4:
    resb 4096
global page_table_l3
page_table_l3:
    resb 4096
global page_table_l2
page_table_l2:
    resb 4096

global _bss_end
_bss_end:

section .data align=4096
global _data_start
_data_start:

; End marker for loadable sections
global _end
_end:
    dq 0
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
