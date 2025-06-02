// SPDX-License-Identifier: GPL-3.0 
#ifndef ACPI_H
#define ACPI_H

#include <stddef.h>

static unsigned int *SMI_CMD;
static unsigned char ACPI_ENABLE;
static unsigned char ACPI_DISABLE;
static unsigned int *PM1a_CNT;
static unsigned int *PM1b_CNT;
static unsigned short SLP_TYPa;
static unsigned short SLP_TYPb;
static unsigned short SLP_EN;
static unsigned short SCI_EN;
static unsigned char PM1_CNT_LEN;

// root system description pointer
struct RSDPtr {
    char Signature[8];
    unsigned char CheckSum;
    char OemID[6];
    unsigned char Revision;
    unsigned int *RsdtAddress;
};

//
struct FACP {
    char Signature[4];
    unsigned int Length;
    char unused1[32];
    unsigned int *DSDT;
    char unused2[4];
    unsigned int *SMI_CMD;
    unsigned char ACPI_ENABLE;
    unsigned char ACPI_DISABLE;
    char unused3[10];
    unsigned int *PM1a_CNT_BLK;
    unsigned int *PM1b_CNT_BLK;
    char unused4[17];
    unsigned char PM1_CNT_LEN;
};
// Initializes ACPI and prepares it for power management.
// Returns 0 on success, -1 on failure.
int init_acpi(void);

// Powers off the system using ACPI. 
// Requires `init_acpi()` to be called first.
void acpi_power_off(void);
void qemu_power_off();
#endif // ACPI_H
