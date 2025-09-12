[BITS 64]
section .text

global gdt_flush

gdt_flush:
    ; Load GDT
    lgdt [rdi]
    
    ; Reload CS through far return
    push qword 0x08        ; Code segment selector
    lea rax, [rel .reload_cs]
    push rax
    retfq
.reload_cs:
    
    ; Reload data segments
    mov ax, 0x10          ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ret
