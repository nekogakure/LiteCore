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
extern kernel_main              ; C kernel main function
extern _start

multiboot_start:
    ; Stack and interrupt setup
    cli                         ; Disable interrupts
    cld                         ; Clear direction flag
    
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

    ; Call kernel directly
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

; End marker
section .end
align 4096
global _end
_end:
