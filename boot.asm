[org 0x7c00]
[bits 16]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov si, boot_msg
    call print_string

    mov ah, 0x00
    mov dl, 0x80
    int 0x13
    
    mov bx, 0x1000
    mov ah, 0x02
    mov al, 20
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    int 0x13

    jc disk_error

    jmp 0x1000


disk_error:
    mov si, error_msg
    call print_string
    hlt
    mov ah, 0x0e
    int 0x10
    
    cli
    hlt

print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0e
    int 0x10
    jmp print_string
done:
    ret


boot_msg db 'LiteCore kernel Booting...', 13, 10, 0
error_msg db 'Boot Error!', 13, 10, 0

times 510-($-$$) db 0
dw 0xaa55
