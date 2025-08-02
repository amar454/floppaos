#include <stdint.h>
#include "pmm.h"
#include "utils.h"
#include "vmm.h"
#include "paging.h"
#include "../apps/echo.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"

#define PAGE_TABLE_SIZE 1024
#define PAGE_DIRECTORY_SIZE 1024
#define K_PD_INDEX 768
#define K_HEAP_PD_INDEX 769
#define PAGE_SIZE_SHIFT 12
#define K_HEAP_START 0x00400000
#define CR0_PG_BIT 0x80000000
#define K_PHYS_START 0x00100000 // from linker 

pde_t pd[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PAGE_SIZE)));
pte_t pt0[PAGE_TABLE_SIZE]    __attribute__((aligned(PAGE_SIZE)));
pte_t pt1[PAGE_TABLE_SIZE]    __attribute__((aligned(PAGE_SIZE)));
pte_t pt2[PAGE_TABLE_SIZE]    __attribute__((aligned(PAGE_SIZE)));

static void load_page_directory(uintptr_t _pd_addr) {
    asm volatile (
        "mov %0, %%cr3" :: "r"(_pd_addr) : "memory"
    );
}

static void enable_paging(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= CR0_PG_BIT;
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}

void paging_init(void) {
    log("paging: Initializing paging...\n", LIGHT_GRAY);

    // Zero page directory
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        page_attrs_t attrs = {0};
        SET_PF((pte_t *)&pd[i], attrs);
    }

    // Identity map first 4MB 
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_attrs_t attrs = {
            .present = 1,
            .rw = 1,
            .user = 0,
            .frame_addr = i
        };
        SET_PF(&pt0[i], attrs);
    }
    pd[0].present = 1;
    pd[0].rw = 1;
    pd[0].user = 0;
    pd[0].table_addr = ((uintptr_t)pt0) >> PAGE_SIZE_SHIFT;

    // Map kernel at 3GB (virtual) to physical 1MB
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_attrs_t attrs = {
            .present = 1,
            .rw = 1,
            .user = 0,
            .frame_addr = (K_PHYS_START >> PAGE_SIZE_SHIFT) + i
        };
        SET_PF(&pt1[i], attrs);
    }
    pd[K_PD_INDEX].present = 1;
    pd[K_PD_INDEX].rw = 1;
    pd[K_PD_INDEX].user = 0;
    pd[K_PD_INDEX].table_addr = ((uintptr_t)pt1) >> PAGE_SIZE_SHIFT;

    // Map kernel heap (next 4MB)
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_attrs_t attrs = {
            .present = (i != PAGE_TABLE_SIZE - 1) ? 1 : 0,
            .rw = 1,
            .user = 0,
            .frame_addr = (K_HEAP_START >> PAGE_SIZE_SHIFT) + i
        };
        SET_PF(&pt2[i], attrs);
    }
    pd[K_HEAP_PD_INDEX].present = 1;
    pd[K_HEAP_PD_INDEX].rw = 1;
    pd[K_HEAP_PD_INDEX].user = 0;
    pd[K_HEAP_PD_INDEX].table_addr = ((uintptr_t)pt2) >> PAGE_SIZE_SHIFT;

    // Mark all other user entries as not present, but user-accessible
    for (int i = 1; i < K_PD_INDEX; i++) {
        pd[i].present = 0;
        pd[i].rw = 1;
        pd[i].user = 1;
    }

    // Fractal mapping: last entry points to page directory itself
    pd[PAGE_DIRECTORY_SIZE - 1].present = 1;
    pd[PAGE_DIRECTORY_SIZE - 1].rw = 1;
    pd[PAGE_DIRECTORY_SIZE - 1].user = 0;
    pd[PAGE_DIRECTORY_SIZE - 1].table_addr = ((uintptr_t)pd) >> PAGE_SIZE_SHIFT;

    load_page_directory((uintptr_t)pd);
    enable_paging();
    log("paging: Paging initialized.\n", GREEN);
}

void _flush_tlb(void) {
    uintptr_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3));
}

void remove_id_map(void) {
    pd[0].present = 0;
    _flush_tlb();
    log("paging: Removed identity map.\n", GREEN);
}
