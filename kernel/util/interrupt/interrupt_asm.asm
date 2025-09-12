[BITS 64]
section .text

; Declare external C function
extern interrupt_default_handler
extern interrupt_divide_by_zero
extern interrupt_general_protection
extern interrupt_page_fault

%macro INTERRUPT_HANDLER 1
global int_handler_%1
int_handler_%1:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    
    mov rdi, %1          ; Pass interrupt number as parameter
    
    ; Call appropriate handler based on interrupt number
    cmp rdi, 0
    je .divide_zero
    cmp rdi, 13
    je .general_protection
    cmp rdi, 14
    je .page_fault
    jmp .default
    
.divide_zero:
    call interrupt_divide_by_zero
    jmp .end
.general_protection:
    call interrupt_general_protection
    jmp .end
.page_fault:
    call interrupt_page_fault
    jmp .end
.default:
    call interrupt_default_handler
.end:
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    iretq
%endmacro

; Generate handlers for interrupts 0-31 (CPU exceptions)
%assign i 0
%rep 32
INTERRUPT_HANDLER i
%assign i i+1
%endrep

; Export symbols for backward compatibility
global default_handler
global divide_by_zero_handler
global general_protection_fault_handler
global page_fault_handler

; Map handlers to specific interrupt handlers
default_handler equ int_handler_0
divide_by_zero_handler equ int_handler_0
general_protection_fault_handler equ int_handler_13
page_fault_handler equ int_handler_14
