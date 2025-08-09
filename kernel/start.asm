[bits 16]

global _start
extern k_main

section .text
_start:
    mov ax, 0
    mov ss, ax
    mov sp, 0x7C00
    
    mov ax, 0xB800
    mov es, ax
    mov word [es:0], 0x0741

    cli
    lgdt [gdt_descriptor]
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp 0x08:protected_mode

[bits 32]
protected_mode:
    [bits 32]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    mov dword [0xB8000], 0x074200
    
    call setup_paging
    
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr
    
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax
    
    jmp 0x18:long_mode

[bits 64]
long_mode:
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
    
    call k_main
    cli
.halt:
    hlt
    jmp .halt

[bits 32]
setup_paging:
    mov edi, 0x70000
    xor eax, eax
    mov ecx, 4096
    rep stosd
    
    mov dword [0x70000], 0x71003
    mov dword [0x70004], 0
    
    mov dword [0x71000], 0x72003
    mov dword [0x71004], 0
    
    mov dword [0x72000], 0x83
    mov dword [0x72004], 0
    mov dword [0x72008], 0x200083
    mov dword [0x7200C], 0
    mov dword [0x72010], 0x400083
    mov dword [0x72014], 0
    mov dword [0x72018], 0x600083
    mov dword [0x7201C], 0
    
    mov eax, 0x70000
    mov cr3, eax
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax
    ret

section .data
align 16
gdt_start:
    dq 0

gdt_code_32:
    ; 32-bit code segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10011010b    ; Access Byte: Present=1, DPL=00, S=1, Type=1010 (code,read)
    db 11001111b    ; Flags: Granularity=1, Size=1 (32-bit), Limit(bits 16-19)
    db 0x00         ; Base (bits 24-31)

gdt_data_32:
    ; 32-bit data segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10010010b    ; Access Byte: Present=1, DPL=00, S=1, Type=0010 (data,write)
    db 11001111b    ; Flags: Granularity=1, Size=1 (32-bit), Limit(bits 16-19)
    db 0x00         ; Base (bits 24-31)

gdt_code_64:
    ; 64-bit code segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10011010b    ; Access Byte: Present=1, DPL=00, S=1, Type=1010 (code,read)
    db 10101111b    ; Flags: Granularity=1, Size=0, Long=1 (64-bit), Limit(16-19)
    db 0x00         ; Base (bits 24-31)

gdt_data_64:
    ; 64-bit data segment
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10010010b    ; Access Byte: Present=1, DPL=00, S=1, Type=0010 (data,write)
    db 10101111b    ; Flags: Granularity=1, Size=0, Long=1 (64-bit), Limit(16-19)
    db 0x00         ; Base (bits 24-31)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
