global syscall_handler
extern c_syscall_handler
syscall_handler:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    call c_syscall_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd
