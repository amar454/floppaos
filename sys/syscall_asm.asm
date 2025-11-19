global syscall_routine
section .text
extern c_syscall_routine
syscall_routine:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    call c_syscall_routine

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd
