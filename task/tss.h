#include <stdint.h>
#include <stddef.h>

struct taskstate {
    unsigned link;                
    unsigned esp0;                
    unsigned short ss0;                
    unsigned short p1;
    unsigned *esp1;
    unsigned short ss1;
    unsigned short p2;
    unsigned *esp2;
    unsigned short ss2;
    unsigned short p3;
    void *cr3;                 
    unsigned *eip;               
    unsigned eflags;
    unsigned eax;                   
    unsigned ecx;
    unsigned edx;
    unsigned ebx;
    unsigned *esp;
    unsigned *ebp;
    unsigned esi;
    unsigned edi;
    unsigned short es;             
    unsigned short p4;
    unsigned short cs;
    unsigned short p5;
    unsigned short ss;
    unsigned short p6;
    unsigned short ds;
    unsigned short p7;
    unsigned short fs;
    unsigned short p8;
    unsigned short gs;
    unsigned short p9;
    unsigned short ldt;
    unsigned short p10;
    unsigned short t;                   
    unsigned short iomb;           
};