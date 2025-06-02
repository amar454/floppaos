global _thread_entry

section .text

_thread_entry:

    ; [esp+4] = function pointer
    ; [esp+8] = argument
    
    mov eax, [esp + 4]    
    push dword [esp + 8]  
    call eax              
    add esp, 4            ; Clean up 
    
    ret
