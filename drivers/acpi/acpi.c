/*

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

acpi.c

    This is the ACPI (Advanced Configuration and Power Interface) driver for FloppaOS.
    It initializes the ACPI tables and provides functions to read and manipulate the ACPI tables.
    The RSDP (Root System Description Pointer) is used to locate the RSDT (Root System Description Table).

    init_acpi(...) initializes the ACPI tables.

    acpi_check_rsdp(...) checks if the RSDP (Root System Description Pointer) is valid.

    acpi_get_rsdp(...) returns the RSDP (Root System Description Pointer) address.

    acpi_check_header(...) checks if the ACPI header is valid.

    acpi_power_off(...) powers off the computer.
    
*/

#include "acpi.h"
#include "../vga/vgahandler.h"
#include "../../lib/logging.h"
#include "../../mem/utils.h"
#include "../io/io.h"
#include "../time/floptime.h"
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


static unsigned int *acpi_check_rsdp(unsigned int *ptr) {
    struct RSDPtr *rsdp = (struct RSDPtr *)ptr;
    const char *sig = "RSD PTR ";
    unsigned char *bptr;
    unsigned char check = 0;
    int i;

    if (flop_memcmp(sig, rsdp, 8) == 0) {
        bptr = (unsigned char *)ptr;
        for (i = 0; i < sizeof(struct RSDPtr); i++) {
            check += *bptr;
            bptr++;
        }
        if (check == 0) {
            return (unsigned int *)rsdp->RsdtAddress;
        }
    }
    return NULL;
}

static unsigned int *acpi_get_rsdp(void) {
    unsigned int *addr;
    unsigned int *rsdp;

    for (addr = (unsigned int *)0x000E0000; (int)addr < 0x00100000; addr += 4) {
        rsdp = acpi_check_rsdp(addr);
        if (rsdp != NULL)
            return rsdp;
    }

    int ebda = *((short *)0x40E);
    ebda = ebda * 0x10 & 0x000FFFFF;

    for (addr = (unsigned int *)ebda; (int)addr < ebda + 1024; addr += 4) {
        rsdp = acpi_check_rsdp(addr);
        if (rsdp != NULL)
            return rsdp;
    }

    return NULL;
}

static int acpi_check_header(unsigned int *ptr, const char *sig) {
    if (flop_memcmp(ptr, sig, 4) == 0) {
        char *check_ptr = (char *)ptr;
        int len = *(ptr + 1);
        char check = 0;
        while (len-- > 0) {
            check += *check_ptr;
            check_ptr++;
        }
        return (check == 0) ? 0 : -1;
    }
    return -1;
}

static int acpi_enable(void) {
    if ((inw((unsigned int)PM1a_CNT) & SCI_EN) == 0) {
        if (SMI_CMD && ACPI_ENABLE) {
            outb((unsigned int)SMI_CMD, ACPI_ENABLE);
            for (int i = 0; i < 300; i++) {
                if ((inw((unsigned int)PM1a_CNT) & SCI_EN) == 1)
                    break;
            }
            if (PM1b_CNT) {
                for (int i = 0; i < 300; i++) {
                    if ((inw((unsigned int)PM1b_CNT) & SCI_EN) == 1)
                        break;
                }
            }
            log("ACPI enabled.", GREEN);
            return 0;
        } else {
            log("No known way to enable ACPI.", RED);
            return -1;
        }
    }
    return 0;
}

int init_acpi(void) {
    unsigned int *ptr = acpi_get_rsdp();

    if (ptr && acpi_check_header(ptr, "RSDT") == 0) {
        int entries = *(ptr + 1);
        entries = (entries - 36) / 4;
        ptr += 9;

        while (entries-- > 0) {
            if (acpi_check_header((unsigned int *)*ptr, "FACP") == 0) {
                struct FACP *facp = (struct FACP *)*ptr;
                if (acpi_check_header((unsigned int *)facp->DSDT, "DSDT") == 0) {
                    char *S5Addr = (char *)facp->DSDT + 36;
                    int dsdt_length = *(facp->DSDT + 1) - 36;

                    while (dsdt_length-- > 0) {
                        if (flop_memcmp(S5Addr, "_S5_", 4) == 0)
                            break;
                        S5Addr++;
                    }

                    if (dsdt_length > 0) {
                        if ((*(S5Addr - 1) == 0x08 || (*(S5Addr - 2) == 0x08 && *(S5Addr - 1) == '\\')) && *(S5Addr + 4) == 0x12) {
                            S5Addr += 5;
                            S5Addr += ((*S5Addr & 0xC0) >> 6) + 2;

                            if (*S5Addr == 0x0A) S5Addr++;
                            SLP_TYPa = *(S5Addr) << 10;
                            S5Addr++;

                            if (*S5Addr == 0x0A) S5Addr++;
                            SLP_TYPb = *(S5Addr) << 10;

                            SMI_CMD = facp->SMI_CMD;
                            ACPI_ENABLE = facp->ACPI_ENABLE;
                            ACPI_DISABLE = facp->ACPI_DISABLE;
                            PM1a_CNT = facp->PM1a_CNT_BLK;
                            PM1b_CNT = facp->PM1b_CNT_BLK;
                            PM1_CNT_LEN = facp->PM1_CNT_LEN;

                            SLP_EN = 1 << 13;
                            SCI_EN = 1;

                            log("ACPI initialized.", GREEN);
                            return 0;
                        } else {
                            log("Error parsing \\_S5.", RED);
                        }
                    } else {
                        log("\\_S5 not present.", RED);
                    }
                } else {
                    log("Invalid DSDT.", RED);
                }
            }
            ptr++;
        }
        log("No valid FACP found.", RED);
    } else {
        log("No ACPI detected.", RED);
    }
    return -1;
}

void acpi_power_off(void) {
    if (!SCI_EN)
        return;

    acpi_enable();

    outw((unsigned int)PM1a_CNT, SLP_TYPa | SLP_EN);
    if (PM1b_CNT)
        outw((unsigned int)PM1b_CNT, SLP_TYPb | SLP_EN);

    log("ACPI power-off failed.", RED);
}


void qemu_legacy_power_off() {
    outw(0xB004, 0x2000);
}

void qemu_power_off() {
    outw(0x604, 0x2000);
}

void vbox_power_off() {
    outw(0x4004, 0x3400);
}

void cloud_hypervisor_power_off() {
    outw(0x600, 0x34);
}