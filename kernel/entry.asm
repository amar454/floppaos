section .multiboot               
    MAGIC equ 0x1BADB002
    FLAGS equ 7
    CHECKSUM equ -(MAGIC + FLAGS)
    MODE_TYPE equ 0
    W equ 1280
    H equ 720
    D equ 32

    dd MAGIC                
    dd FLAGS               
    dd CHECKSUM            
    dd 0                    ; header_addr
    dd 0                    ; load_addr
    dd 0                    ; load_end_addr
    dd 0                    ; bss_end_addr
    dd 0                    ; entry_addr (they dont really matter so)
    dd MODE_TYPE           
    dd W                ; width                
    dd H                ; height                
    dd D                ; depth

section .text
global start
global stack_space
extern kmain                     ; c main function

start:
    cli                         
    mov esp, stack_space         

    push ebx                     
    push eax                     

    call kmain                   ; jump to C

    ; we cannot be serious

    cli                          
    hlt                        

section .bss
align 16                       
resb 8192                        ; 8KB kernel stack space
stack_space: