section .text
global usermode_entry_routine

usermode_entry_routine:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword 0x23             ; SS (ring3)
    push dword [esp + 20]       ; user ESP (arg2)
    pushfd
    or dword [esp], 0x200      
    push dword 0x1B             ; CS (ring3)
    push dword [esp + 20]       ; user EIP (arg1)
    iretd
