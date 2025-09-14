[BITS 32]  ; Start in 32-bit mode

; Multiboot2 header constants
MULTIBOOT2_MAGIC    equ 0xE85250D6
MULTIBOOT2_ARCH     equ 0
HEADER_LENGTH       equ header_end - multiboot_header
CHECKSUM            equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + HEADER_LENGTH)

; Place Multiboot2 header within the first 8KB
section .multiboot
align 8  ; 8-byte alignment

global multiboot_header
multiboot_header:
    dd MULTIBOOT2_MAGIC          ; Magic number
    dd MULTIBOOT2_ARCH           ; Architecture (0=32bit)
    dd HEADER_LENGTH             ; Header length
    dd CHECKSUM                  ; Checksum

    ; Information request tag
    align 8
    dw 1                        ; Type: information request
    dw 0                        ; Flags
    dd 16                       ; Size
    dd 4                        ; Request basic memory information
    dd 6                        ; Request memory map

    ; Address tag
    align 8
    dw 2                        ; Type: address tag
    dw 0                        ; Flags
    dd 24                       ; Size
    dd multiboot_header         ; Header address
    dd multiboot_start          ; Load address
    dd _edata                   ; Load end address
    dd _end                     ; BSS end address

    ; Entry point tag
    align 8
    dw 3                        ; Type: entry tag
    dw 0                        ; Flags
    dd 12                       ; Size
    dd multiboot_start          ; Entry address

    ; End tag
    align 8
    dw 0                        ; Type: end tag
    dw 0                        ; Flags
    dd 8                        ; Size
header_end:

; Text section (code)
section .text
align 4096                      ; Page alignment
global multiboot_start
extern kernel_main

multiboot_start:
    ; Debug code - Early serial port output 
    mov dx, 0x3F8               ; COM1 serial port
    mov al, 'M'                 ; 'M' for Multiboot
    out dx, al
    
    ; Stack and interrupt setup
    cli                         ; Disable interrupts
    cld                         ; Clear direction flag
    
    ; Set up simple GDT for 32-bit protected mode
    lgdt [gdt32_ptr]
    
    ; Reload segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up stack
    mov esp, stack_top
    
    ; Reset EFLAGS
    push dword 0
    popfd
    
    ; Save Multiboot info
    push ebx                    ; Multiboot structure pointer
    push eax                    ; Magic number
    
    ; Clear BSS section
    mov edi, _bss_start
    mov ecx, _bss_size
    xor eax, eax
    rep stosb
    
    ; Debug code - Before calling kernel_main
    mov dx, 0x3F8               ; COM1 serial port
    mov al, 'K'                 ; 'K' for Kernel main
    out dx, al

    mov dx, 0x3F8           ; COM1 base port
    mov al, 'E'
    out dx, al
    mov al, 'R'
    out dx, al
    mov al, 'R'
    out dx, al
    mov al, 'O'
    out dx, al
    mov al, 'R'
    out dx, al
    mov al, ':'
    out dx, al
    mov al, ' '
    out dx, al
    
    ; call kernel main (no call _start)
    call kernel_main
    
    ; If we return, halt
.halt:
    cli
    hlt
    jmp .halt

; Uninitialized data section (BSS)
section .bss
align 4096
global _bss_start
_bss_start:

; Stack space
stack_bottom:
    resb 16384  ; 16 KB stack
stack_top:

global _bss_size
_bss_size equ $ - _bss_start

; Data section
section .data
align 4096
global _edata
_edata:

; 32-bit protected mode GDT
section .data
align 8
gdt32:
    ; Null descriptor
    dw 0, 0, 0, 0
    
    ; 32-bit code segment
    dw 0xFFFF    ; Limit (bits 0-15)
    dw 0         ; Base (bits 0-15)
    db 0         ; Base (bits 16-23)
    db 0x9A      ; Access byte - code segment, readable, present
    db 0xCF      ; Flags and Limit (bits 16-19)
    db 0         ; Base (bits 24-31)
    
    ; 32-bit data segment
    dw 0xFFFF    ; Limit (bits 0-15)
    dw 0         ; Base (bits 0-15)
    db 0         ; Base (bits 16-23)
    db 0x92      ; Access byte - data segment, writable, present
    db 0xCF      ; Flags and Limit (bits 16-19)
    db 0         ; Base (bits 24-31)
    
gdt32_ptr:
    dw $ - gdt32 - 1    ; GDT size
    dd gdt32            ; GDT address

; End marker
section .end
align 4096
global _end
_end:
