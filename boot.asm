; boot.asm
bits 16                        ; We are in 16-bit mode

extern boot_main               ; Declare the C function as external

section .text                  ; Code section
    global start               ; Make start function visible to linker

start:
    cli                        ; Disable interrupts
    xor ax, ax                 ; Clear AX register
    mov ds, ax                 ; Set DS segment register to 0
    mov es, ax                 ; Set ES segment register to 0
    mov ss, ax                 ; Set SS segment register to 0
    mov sp, 0x7C00             ; Stack pointer at 0x7C00

    call boot_main             ; Call the C function boot_main()

hang:
    jmp hang                   ; Infinite loop to halt execution
