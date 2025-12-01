global syscall_routine
extern c_syscall_routine

section .text
syscall_routine:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    push edi          ; a5
    push esi          ; a4
    push edx          ; a3
    push ecx          ; a2
    push ebx          ; a1
    push eax          ; num

    call c_syscall_routine

    add esp, 6 * 4     
    pop gs
    pop fs
    pop es
    pop ds
    popa

    iretd
