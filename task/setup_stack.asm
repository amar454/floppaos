extern _thread_entry
extern thread_exit

global setup_thread_stack

section .text

setup_thread_stack:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+8]
    mov ebx, [ebp+12]
    mov ecx, [ebp+16]
    
    test eax, eax
    jz .error
    test ebx, ebx
    jz .error
    
    sub eax, 4
    mov dword [eax], ecx
    
    sub eax, 4
    mov dword [eax], ebx
    
    sub eax, 4
    mov dword [eax], thread_exit
    
    sub eax, 4
    mov dword [eax], _thread_entry
    
    sub eax, 4
    mov dword [eax], 0
    sub eax, 4  
    mov dword [eax], 0
    sub eax, 4
    mov dword [eax], 0
    sub eax, 4
    mov dword [eax], 0
    
    test eax, eax
    jz .error
    
    jmp .done

.error:
    xor eax, eax

.done:
    mov esp, ebp
    pop ebp
    ret