section .text
global usermode_entry_routine

; arguments: u32 stack, u32 ip
; enters usermode at given ip with given stack
; user threads will automatically jump to this.
; this sets up the stack to execute the entry of the user thread
usermode_entry_routine:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push dword 0x23
    push dword [esp+4]
    pushfd
    or dword [esp], 0x200
    push dword 0x1B
    push dword [esp+12]
    iretd
