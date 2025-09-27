global sv_eflags
global restore_eflags
global fetch_eflags
global sti
global cli
global xchg
global atomic_increment
global atomic_decrement
global fetch_cr3
global push_cr3
global fetch_cr0
global push_cr0
global enable_paging
global disable_paging
global reload_cr3
global invlpg
global jump_ring3
global halt
global fetch_gs
global set_gs
global rdmsr
global cpuid
global load_tss
global clts
global setts
global fpu_rstor
global fpu_sv


section .text
    

sv_eflags:
    ; sv_eflags(uint32_t * flags)

    ; stack frame 
    push ebp
    mov ebp, esp

    ; save eax and ebx
    push eax
    push ebx

    ; push flags into eax
    pushf
    pop ebx
    mov eax, [ebp+8]
    mov [eax], ebx

    ; restore registers
    pop ebx
    pop eax
    leave
    ret

restore_eflags:
    ; restore_eflags(uint32_t * flags)

    ; stack frame
    push ebp
    mov ebp, esp

    ; save...
    push eax
    push ebx

    ; pop flags into eax
    mov eax, [ebp+8]
    mov ebx, [eax]
    push ebx
    popf

    ; restore registers
    pop ebx
    pop eax
    leave


    ret

fetch_eflags:
    pushf
    pop eax
    ret

fetch_gs:
    xor eax, eax
    mov ax, gs
    ret

set_gs:
    push ebp
    mov ebp, esp
    push eax
    mov eax, [ebp+8]
    mov gs, ax
    pop eax
    leave
    ret

clear_ts:
    clts
    ret

set_ts:
    push ebp
    mov ebp, esp
    push eax
    mov eax, cr0
    or eax, 0x8
    mov cr0, eax
    pop eax
    leave
    ret

fpu_rstor:
    push ebp
    mov ebp, esp
    push eax
    mov eax, [ebp+8]
    fxrstor [eax]
    pop eax
    leave
    ret

fpu_sv:
    push ebp
    mov ebp, esp
    push eax
    mov eax, [ebp+8]
    fxsv [eax]
    pop eax
    leave
    ret

atomic_xchg:
    push ebp
    mov ebp, esp
    push ebx
    mov ebx, [ebp+12]
    mov eax, [ebp+8]
    xchg eax, [ebx]
    pop ebx
    leave
    ret

atomic_increment:
    push ebp
    mov ebp, esp
    push eax
    mov eax, [ebp+8]
    lock inc dword [eax]
    pop eax
    leave
    ret

atomic_decrement:
    push ebp
    mov ebp, esp
    push eax
    mov eax, [ebp+8]
    lock dec dword [eax]
    pop eax
    leave
    ret

sti:
    sti
    ret

cli:
    cli
    ret

fetch_cr3:
    mov eax, cr3
    ret

push_cr3:
    push ebp
    mov ebp, esp
    push eax
    mov eax, [ebp+8]
    mov cr3, eax
    pop eax
    leave
    ret

enable_paging:
    mov eax, cr0
    or eax, 0x80000000
    or eax, 0x10000
    mov cr0, eax
    ret

disable_paging:
    mov eax, cr0
    and eax, 0x7fffffff
    ret

reload_cr3:
    mov eax, cr3
    mov cr3, eax
    ret

fetch_cr0:
    mov eax, cr0
    ret

push_cr0:
    push ebp
    mov ebp, esp
    push eax
    mov eax, [ebp+8]
    mov cr0, eax
    pop eax
    leave
    ret

invlpg:
    push ebp
    mov ebp, esp
    push eax
    mov eax, [ebp+8]
    invlpg [eax]
    pop eax
    leave
    ret

jump_ring3:
    push ebp
    mov ebp, esp
    cli
    mov eax, SELECTOR_STACK_USER+3
    push eax
    mov eax, [ebp+12]
    push eax
    pushf
    pop eax
    or eax, 0x200
    push eax
    mov eax, SELECTOR_CODE_USER+3
    push eax
    mov eax, [ebp+8]
    push eax
    mov eax, SELECTOR_DATA_USER+3
    mov ds, ax
    mov fs, ax
    mov es, ax
    iret

halt:
    push ebp
    mov ebp, esp
    hlt
    leave
    ret

rdmsr:
    push ebp
    mov ebp, esp
    push ecx
    push edx
    push ebx
    mov ecx, [ebp+8]
    rdmsr
    mov ebx, eax
    mov eax, [ebp+12]
    mov [eax], ebx
    mov eax, [ebp+16]
    mov [eax], edx
    pop ebx
    pop edx
    pop ecx
    leave
    ret

cpuid:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    push edx
    mov eax, [ebp+8]
    cpuid
    push eax
    mov eax, [ebp+12]
    mov [eax], ebx
    mov eax, [ebp+16]
    mov [eax], ecx
    mov eax, [ebp+20]
    mov [eax], edx
    pop eax
    pop edx
    pop ecx
    pop ebx
    leave
    ret

load_tss:
    push ebp
    mov ebp, esp
    push eax
    mov eax, SELECTOR_TSS
    ltr ax
    pop eax
    leave
    ret
