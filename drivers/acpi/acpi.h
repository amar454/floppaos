#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <stddef.h>

// ACPI Signature Definitions
#define RSDP_SIG "RSD PTR "
#define RSDT_SIG "RSDT"
#define XSDT_SIG "XSDT"
#define FADT_SIG "FACP"

// ACPI Table Structures
#pragma pack(push, 1)
typedef struct {
    char signature[8];          // "RSD PTR "
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} RSDP;

typedef struct {
    char signature[4];          // "RSDT"
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint32_t entry[1];          // Array of ACPI table pointers (32-bit entries)
} RSDT;

typedef struct {
    char signature[4];          // "XSDT"
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint64_t entry[1];          // Array of ACPI table pointers (64-bit entries)
} XSDT;

typedef struct {
    char signature[4];          // "FACP"
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t model;
    uint8_t reserved[3];
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1a_evt_blk;
    uint8_t pm1b_evt_blk;
    uint8_t pm1a_ctrl_blk;
    uint8_t pm1b_ctrl_blk;
    uint32_t sleep_ctrl;
    uint32_t sleep_status;
} FADT;
#pragma pack(pop)

// ACPI Functions
RSDP *find_rsdp();
void *parse_rsdt(RSDT *rsdt);
void acpi_initialize();
void acpi_shutdown();
int check_signature(char *signature, char *expected_signature);

#endif // ACPI_H
