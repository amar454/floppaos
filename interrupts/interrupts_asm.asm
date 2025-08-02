
global isr0
global isr6
global isr13
global isr14

global irq0
global irq1

; funcntions prefixed with 'c_' are C functions 
extern c_isr0
extern c_isr6
extern c_isr13
extern c_isr14

extern c_irq0
extern c_irq1

section .text

; div by zero exception 
isr0:
    pusha
    cld
    call c_isr0
    popa
    iret

; invalid opcode exception 
isr6:
    pusha
    cld
    call c_isr6
    popa
    iret

; general protection fault exception 
isr13:
    pusha
    cld
    call c_isr13
    popa
    iret

; page fault 
isr14:
    pusha
    cld
    call c_isr14
    popa
    add esp, 4
    iret

; pit
irq0:
    pusha               
    call c_irq0   
    popa                
    iret

; keyboard
irq1:
    pusha              
    call c_irq1 
    popa                
    iret