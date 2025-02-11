section .multiboot               ; Multiboot header
    dd 0x1BADB002                ; Multiboot magic number
    dd 0x0                       ; Flags
    dd -(0x1BADB002 + 0x0)       ; Checksum

section .text
global start
extern kmain                     ; Declared in the kernel

start:
    cli                          ; Disable interrupts
    mov esp, stack_space         ; Set up stack

    push ebx                     ; Push mb_info pointer
    push eax                     ; Push mb_magic number

    call kmain                   ; Call kernel main function

    cli                          ; Disable interrupts
    hlt                          ; Halt if main returns

section .bss
align 16                         ; Ensure proper stack alignment
resb 8192                        ; 8KB stack space
stack_space:
