

isr0:
    pusha

    cld
    call isr0_handler

    popa

    hlt

isr6:
    pusha

    cld
    call isr6_handler

    popa

    hlt

isr13:
    pusha

    cld
    call isr13_handler

    popa

    hlt

isr14:
    pusha              

    cld
    call isr14_handler

    popa

    add esp, 4
    iret

irq0:
    pusha
    call pit_handler
    popa
    iret

irq1:
    pusha
    call irq1_handler
    popa
    iret