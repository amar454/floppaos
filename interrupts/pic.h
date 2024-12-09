#ifndef PIC_H
#define PIC_H

#include <stdint.h>

#define PIC1_CMD 0x20   // Command port for PIC1 (Master)
#define PIC1_DATA 0x21  // Data port for PIC1 (Master)
#define PIC2_CMD 0xA0   // Command port for PIC2 (Slave)
#define PIC2_DATA 0xA1  // Data port for PIC2 (Slave)

// Initialization command words for PICs
#define ICW1_ICW4 0x01  // Initialize PIC
#define ICW1_SINGLE 0x02  // Single (no cascade)
#define ICW1_INTERVAL4 0x04  // Call for ICW4
#define ICW1_LEVEL 0x08  // Level triggered interrupts
#define ICW1_INIT 0x10  // Initialize command

void init_pic(void);
void init_pit();
#endif // PIC_H
