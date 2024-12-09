section .multiboot               ; Multiboot header
    dd 0x1BADB002                ; Multiboot magic number
    dd 0x0                       ; Flags
    dd -(0x1BADB002 + 0x0)       ; Checksum

section .text
global start
extern main                      ; Declared in the kernel

start:
    cli                          ; Disable interrupts
    mov esp, stack_space         ; Set up stack
    push dword 0                 ; Push dummy argument count (argc)
    mov ebx, [ebx]               ; Move Multiboot info pointer from ebx register
    push ebx                     ; Push Multiboot pointer (argv[1])
    call main                    ; Call main with arguments
    hlt                          ; Halt

section .bss
resb 8192                        ; 8KB stack space
stack_space:                     ; Define the stack space
