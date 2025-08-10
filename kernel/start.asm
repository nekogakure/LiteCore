[bits 16]

global _start
extern gdt_init
extern prepare_long_mode
extern switch_to_long_mode
extern kernel_panic

section .text.boot org=0x1000
_start:
start_16bit:
    mov ax, 0x0100
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    mov sp, 0x9000

    mov si, kernel_started_msg
    call print_debug_msg

    cli
    call gdt_init
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    mov si, gdt_loaded_msg
    call print_debug_msg
    
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

print_debug_msg:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp print_debug_msg
.done:
    ret

section .data
kernel_started_msg: db "LiteCore kernel loadeed successfully", 13, 10, 0
gdt_loaded_msg: db "GDT loaded successfully", 13, 10, 0
boot_failure_message: db "Failed to switch to long mode", 0
