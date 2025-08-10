[bits 16]

global _start
extern gdt_init
extern prepare_long_mode
extern switch_to_long_mode
extern kernel_panic

section .text.boot
_start:
start_16bit:
    mov ax, 0x0100
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov sp, 0x9000
    mov si, boot_banner
    call print_debug_msg

    mov si, kernel_started_msg
    call print_debug_msg

    cli
    
    mov si, a20_enabling_msg
    call print_debug_msg
    
    call enable_a20
    
    call gdt_init
    
    mov si, gdt_loaded_msg
    call print_debug_msg
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    mov si, prot_mode_msg
    call print_debug_msg
    
    jmp dword 0x08:protmode

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

enable_a20:
    call a20_wait_input
    mov al, 0xAD
    out 0x64, al
    
    call a20_wait_input
    mov al, 0xD0
    out 0x64, al
    
    call a20_wait_output
    in al, 0x60
    push ax
    
    call a20_wait_input
    mov al, 0xD1
    out 0x64, al
    
    call a20_wait_input
    pop ax
    or al, 2
    out 0x60, al
    
    call a20_wait_input
    mov al, 0xAE
    out 0x64, al
    
    call a20_wait_input
    ret

a20_wait_input:
    in al, 0x64
    test al, 2
    jnz a20_wait_input
    ret

a20_wait_output:
    in al, 0x64
    test al, 1
    jz a20_wait_output
    ret

section .data
boot_banner: db "LiteCore Kernel for 64bit", 13, 10, 0
kernel_started_msg: db "[BOOT] Kernel loaded successfully at 0x1000", 13, 10, 0
a20_enabling_msg: db "[BOOT] Enabling A20 line", 13, 10, 0
gdt_loaded_msg: db "[BOOT] GDT initialized and loaded", 13, 10, 0
prot_mode_msg: db "[BOOT] Switching to protected mode...", 13, 10, 0
boot_failure_message: db "[ERROR] Failed to switch to long mode", 0
