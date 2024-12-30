#include "acpi.h"
#include "../io/io.h"
//#include "shutdown.h"
#include "../../apps/echo.h"
#include "../vga/vgahandler.h"
// Function to check for the ACPI signature
int check_signature(char *signature, char *expected_signature) {
    for (int i = 0; i < 8; i++) {
        if (signature[i] != expected_signature[i]) {
            return 0;
        }
    }
    return 1;
}

// Find the ACPI RSDP (Root System Description Pointer)
RSDP *find_rsdp() {
    for (uint32_t address = 0x000E0000; address < 0x00100000; address += 16) {
        RSDP *rsdp = (RSDP *)(uintptr_t)address;
        if (check_signature(rsdp->signature, RSDP_SIG)) {
            return rsdp;
        }
    }
    return NULL;
}

// Parse the RSDT to get the ACPI tables
void *parse_rsdt(RSDT *rsdt) {
    if (!rsdt) return NULL;
    uint32_t *entry = (uint32_t *)(uintptr_t)rsdt->entry;
    return (void *)entry;
}

// ACPI Initialization
void acpi_initialize() {
    RSDP *rsdp = find_rsdp();
    if (!rsdp) return;

    RSDT *rsdt = (RSDT *)(uintptr_t)rsdp->rsdt_address;
    if (check_signature(rsdt->signature, RSDT_SIG)) {
        void *acpi_table = parse_rsdt(rsdt);
        FADT *fadt = (FADT *)acpi_table;
        if (check_signature(fadt->signature, FADT_SIG)) {
            // Process further as needed, e.g., print FADT information
        }
    }
}

// ACPI Shutdown function
void acpi_shutdown() {
    echo("Sending shutdown signal 0x3400 to port 0x4004...", YELLOW);
    outw(0x4004, 0x3400);
}
