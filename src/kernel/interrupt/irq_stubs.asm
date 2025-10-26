[BITS 32]

global isr_stub_table
isr_stub_table:

extern irq_handler_c

global isr32
isr32:
        pusha
        push dword 32
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr33
isr33:
        pusha
        push dword 33
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr34
isr34:
        pusha
        push dword 34
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr35
isr35:
        pusha
        push dword 35
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr36
isr36:
        pusha
        push dword 36
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr37
isr37:
        pusha
        push dword 37
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr38
isr38:
        pusha
        push dword 38
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr39
isr39:
        pusha
        push dword 39
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr40
isr40:
        pusha
        push dword 40
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr41
isr41:
        pusha
        push dword 41
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr42
isr42:
        pusha
        push dword 42
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr43
isr43:
        pusha
        push dword 43
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr44
isr44:
        pusha
        push dword 44
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr45
isr45:
        pusha
        push dword 45
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr46
isr46:
        pusha
        push dword 46
        call irq_handler_c
        add esp, 4
        popa
        iretd

global isr47
isr47:
        pusha
        push dword 47
        call irq_handler_c
        add esp, 4
        popa
        iretd
