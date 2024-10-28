bits 32

section .multiboot               ; multiboot header
    dd 0x1BADB002                ; magic bootloader number idfk
    dd 0x0                       ; flags
    dd -(0x1BADB002 + 0x0)       ; chksum

section .text
global start
extern main                      ; declared from C file

start:
    cli                          ; disable interrupts
    mov esp, stack_space         ; set up stack
    call main                    ; call main in C
    hlt                          ; halt

section .bss
resb 8192                        ; 8KB stack space
stack_space:
