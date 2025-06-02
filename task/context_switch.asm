extern current_thread

; i dont care how many debug functions are here.
extern debug_context_switch_entry
extern debug_context_switch_got_param
extern debug_context_switch_saved_regs
extern debug_context_switch_checked_current
extern debug_context_switch_about_to_load_esp
extern debug_context_switch_loaded_new
extern debug_context_switch_before_ret
extern debug_print_thread_pointer
extern debug_print_esp_value

global context_switch

section .text

context_switch:
    call debug_context_switch_entry
    
    mov eax, [esp + 4]
    
    call debug_context_switch_got_param
    
    push ebp
    push edi
    push esi
    push ebx
    
    call debug_context_switch_saved_regs
    
    mov edi, [current_thread]
    test edi, edi
    jz .load_new_thread
    mov [edi + 8], esp

.load_new_thread:
    call debug_context_switch_checked_current
    
    push eax
    call debug_print_thread_pointer
    add esp, 4
    
    push dword [eax + 8]
    call debug_print_esp_value
    add esp, 4
    
    call debug_context_switch_about_to_load_esp
    
    mov [current_thread], eax
    
    mov esp, [eax + 8]
    
    call debug_context_switch_loaded_new
    
    call debug_context_switch_before_ret
    
    pop ebx
    pop esi
    pop edi
    pop ebp
    
    ret