/* 

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

vmm.c - functions for mapping and unmapping physical pages to virtual addresses

    map_page(vaddr, paddr, attrs) - maps a page at paddr to vaddr with the given attributes

    unmap_page(vaddr) unmaps


------------------------------------------------------------------------------
*/

#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include "utils.h"
#include "../lib/logging.h"
#include "../lib/str.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define KERNEL_VADDR_BASE  0xC0000000
#define USER_SPACE_END     0xBFFFFFFF
#define GNU_INLINE      __attribute__((always_inline))
#define PAGE_ALIGN(addr)   (((addr) + 0xFFF) & ~0xFFF)
#define RESOLVE_RECURSIVE_PD      ((pde_t*)0xFFFFF000)
#define RESOLVE_RECURSIVE_PT(i)   ((pte_t*)(0xFFC00000 + ((i) << 12)))

typedef struct vm_region {
    uintptr_t start;
    uintptr_t end;
    page_attrs_t attrs;
    struct vm_region* next;
} vm_region_t;

// to follow the ia32 sdm
// - Page directory/table entries must be 4KB aligned
// - Present, RW, User, and frame_addr fields must match SDM bit layout
// - All page table/directory allocations are 4KB aligned
// - INVLPG used for TLB flush per SDM
// - All reserved bits are zeroed

static vm_region_t* vm_regions = NULL;
uintptr_t kernel_heap_start;
uintptr_t kernel_heap_end;

extern pde_t pd[PAGE_DIRECTORY_SIZE];
extern pte_t first_pt[PAGE_TABLE_SIZE];

extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

extern pde_t pd[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PAGE_SIZE)));

static inline uint32_t pd_index(uintptr_t vaddr) {
    return (vaddr >> 22) & 0x3FF;
}

static inline uint32_t pt_index(uintptr_t vaddr) {
    return (vaddr >> 12) & 0x3FF;
}

static inline uintptr_t align_down(uintptr_t addr) {
    return addr & ~(PAGE_SIZE - 1);
}

static void zero_page_table(pte_t* pt) {
    flop_memset(pt, 0, PAGE_TABLE_SIZE * sizeof(pte_t));
}

// Allocate a new page table, zero it, return pointer or NULL
static pte_t* alloc_page_table(void) {
    void* page = pmm_alloc_page();
    if (!page) {
        log("alloc_page_table: pmm_alloc_page failed\n", RED);
        return NULL;
    }
    zero_page_table((pte_t*)page);
    return (pte_t*)page;
}

static void _paging_init_recursive(void) {
    // Clear page directory
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        pde_attrs_t zero_attrs = {0};
        SET_PD(&pd[i], zero_attrs);
    }

    // Set recursive entry at index 1023 to point to pd itself
    pde_attrs_t recursive_attrs = {
        .present = 1,
        .rw = 1,
        .user = 0,
        .write_thru = 0,
        .cache_dis = 0,
        .accessed = 0,
        .page_size = 0,
        .global = 0,
        .available = 0,
        .table_addr = ((uintptr_t)pd) >> 12
    };
    SET_PD(&pd[1023], recursive_attrs);

    uintptr_t pd_phys = virt_to_phys((uintptr_t)pd);
    __asm__ volatile ("mov %0, %%cr3" :: "r" (pd_phys) : "memory");
}


static pte_t* get_page_table(uint32_t pd_idx) {
    // The recursive entry maps page directory at virtual address: 0xFFFFF000
    // So page tables start at 0xFFC00000 + (pd_idx * PAGE_SIZE)
    uintptr_t pt_virt = 0xFFC00000 + (pd_idx * PAGE_SIZE);
    return (pte_t*)pt_virt;
}

// Map a single page: vaddr -> paddr with given attrs
int map_page(uintptr_t vaddr, uintptr_t paddr, page_attrs_t attrs) {
    vaddr = align_down(vaddr);
    paddr = align_down(paddr);

    if (vaddr == 0) return -1;

    uint32_t pd_idx = pd_index(vaddr);
    uint32_t pt_idx = pt_index(vaddr);

    pde_t* pd_entry = &pd[pd_idx];

    if (!pd_entry->present) {
        pte_t* new_pt = alloc_page_table();
        if (!new_pt) return -2;

        uintptr_t pt_phys = virt_to_phys((uintptr_t)new_pt);
        pde_attrs_t pt_attrs = {
            .present = 1,
            .rw = 1,
            .user = attrs.user,
            .write_thru = 0,
            .cache_dis = 0,
            .accessed = 0,
            .page_size = 0,
            .global = 0,
            .available = 0,
            .table_addr = pt_phys >> 12
        };
        SET_PD(pd_entry, pt_attrs);
    }

    // Get page table virtual address from recursive mapping
    pte_t* pt = get_page_table(pd_idx);
    pte_t* pte_entry = &pt[pt_idx];

    // If already present, free old page frame
    if (pte_entry->present) {
        uintptr_t old_paddr = pte_entry->frame_addr << 12;
        if (pmm_is_valid_addr(old_paddr)) {
            pmm_free_page((void*)old_paddr);
        }
    }

    attrs.frame_addr = paddr >> 12;
    SET_PF(pte_entry, attrs);

    // Flush TLB for this page
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");

    return 0;
}

// Unmap a page at vaddr
int unmap_page(uintptr_t vaddr) {
    vaddr = align_down(vaddr);

    uint32_t pd_idx = pd_index(vaddr);
    uint32_t pt_idx = pt_index(vaddr);

    pde_t* pd_entry = &pd[pd_idx];
    if (!pd_entry->present) return -1;

    pte_t* pt = get_page_table(pd_idx);
    pte_t* pte_entry = &pt[pt_idx];

    if (!pte_entry->present) return -1;

    uintptr_t old_paddr = pte_entry->frame_addr << 12;
    if (pmm_is_valid_addr(old_paddr)) {
        pmm_free_page((void*)old_paddr);
    }

    page_attrs_t zero_attrs = {0};
    SET_PF(pte_entry, zero_attrs);

    // Check if page table empty, free it if so
    bool empty = true;
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        if (pt[i].present) {
            empty = false;
            break;
        }
    }
    if (empty) {
        uintptr_t pt_phys = pd_entry->table_addr << 12;
        if (pmm_is_valid_addr(pt_phys)) {
            pmm_free_page((void*)pt_phys);
        }
        page_attrs_t zero_attrs_pd = {0};
        SET_PF((pte_t*)pd_entry, zero_attrs_pd);
    }

    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
    return 0;
}

int vmm_protect(uintptr_t vaddr, page_attrs_t attrs) {
    vaddr = _align_down(vaddr);
    uint32_t pd_idx = _pd_index(vaddr);
    uint32_t pt_idx = _pt_index(vaddr);

    if (!RESOLVE_RECURSIVE_PD[pd_idx].present)
        return -1;

    pte_t* pt = RESOLVE_RECURSIVE_PT(pd_idx);
    if (!pt[pt_idx].present)
        return -1;

    attrs.frame_addr = pt[pt_idx].frame_addr;
    SET_PF(&pt[pt_idx], attrs);

    __asm__ volatile("invlpg (%0)" :: "a"(vaddr));
    return 0;
}

int vmm_is_mapped(uintptr_t vaddr) {
    vaddr = _align_down(vaddr);
    uint32_t pd_idx = _pd_index(vaddr);
    uint32_t pt_idx = _pt_index(vaddr);
    if (!RESOLVE_RECURSIVE_PD[pd_idx].present) return 0;

    pte_t* pt = RESOLVE_RECURSIVE_PT(pd_idx);
    if (!pt[pt_idx].present) return 0;

    return 1;
}

int clone_pagemap(uintptr_t src_base, uintptr_t dst_base, size_t size) {
    size_t pages = _align_up(size) / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        uintptr_t src_vaddr = src_base + i * PAGE_SIZE;
        uintptr_t dst_vaddr = dst_base + i * PAGE_SIZE;

        uint32_t src_pd_idx = _pd_index(src_vaddr);
        uint32_t src_pt_idx = _pt_index(src_vaddr);

        if (!RESOLVE_RECURSIVE_PD[src_pd_idx].present)
            continue; 

        pte_t* src_pt = RESOLVE_RECURSIVE_PT(src_pd_idx);
        if (!src_pt[src_pt_idx].present)
            continue; 

        uintptr_t src_paddr = src_pt[src_pt_idx].frame_addr << 12;
        page_attrs_t attrs = {
            .present = src_pt[src_pt_idx].present,
            .rw = src_pt[src_pt_idx].rw,
            .user = src_pt[src_pt_idx].user
        };

        void* dst_frame = pmm_alloc_page();
        if (!dst_frame)
            return -1;

        void* src_frame = (void*)src_paddr;
        flop_memcpy(dst_frame, src_frame, PAGE_SIZE);

        attrs.frame_addr = ((uintptr_t)dst_frame) >> 12;

        if (map_page(dst_vaddr, (uintptr_t)dst_frame, attrs) < 0) {
            // Rollback previous mappings
            for (size_t j = 0; j < i; j++) {
                uintptr_t undo_vaddr = dst_base + j * PAGE_SIZE;
                unmap_page(undo_vaddr);
            }
            pmm_free_page(dst_frame);
            return -1;
        }
    }
    return 0;
}

static vm_region_t* _make_region(uintptr_t start, uintptr_t end, page_attrs_t attrs) {
    vm_region_t* r = pmm_alloc_page();
    if (!r) return NULL;
    r->start = start;
    r->end = end;
    r->attrs = attrs;
    r->next = NULL;
    return r;
}

static vm_region_t* _clone_region(vm_region_t* src) {
    vm_region_t* r = pmm_alloc_page();
    if (!r) return NULL;
    r->start = src->start;
    r->end = src->end;
    r->attrs = src->attrs;
    r->next = NULL;
    return r;
}

int map_range(uintptr_t vaddr, uintptr_t paddr, size_t size, page_attrs_t attrs) {
    size_t pages = _align_up(size) / PAGE_SIZE;

    vm_region_t* r = _make_region(vaddr, vaddr + size, attrs);
    if (!r) {
        return -1;
    }

    r->next = vm_regions;
    vm_regions = r;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = vaddr + i * PAGE_SIZE;
        uintptr_t pa = paddr + i * PAGE_SIZE;
        if (map_page(va, pa, attrs) < 0) {
            // rollback if failed 
            for (size_t j = 0; j < i; j++) {
                uintptr_t undo_va = vaddr + j * PAGE_SIZE;
                unmap_page(undo_va);
            }
            vm_regions = r->next;
            return -1;
        }
    }
    return 0;
}

int unmap_range(uintptr_t vaddr, size_t size) {
    size_t pages = _align_up(size) / PAGE_SIZE;

    vm_region_t *cur = vm_regions, *prev = NULL;

    while (cur) {
        // check if the range is within the current region
        if (cur->start == vaddr && cur->end == vaddr + size) {

            if (prev) prev->next = cur->next;

            else vm_regions = cur->next;
            
            break;
        }
        // move to next region
        prev = cur;
        cur = cur->next;
    }

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = vaddr + i * PAGE_SIZE;
        if (unmap_page(va) < 0) {
            // todo: handle error
            return -1;
        }
    }
    return 0;
}

void* vmm_create_address_space(size_t size, page_attrs_t attrs) {
    size_t pages = _align_up(size) / PAGE_SIZE;
    uintptr_t vaddr = vmm_find_free_region(KERNEL_VADDR_BASE, size);
    if (vaddr == 0) {
        return NULL; // no free region found
    }
    uintptr_t paddr = (uintptr_t)pmm_alloc_pages(0, pages);
    if (paddr == 0) {
        return NULL; // no free pages available
    }


    vm_region_t* r = _make_region(vaddr, vaddr + size, attrs);
    if (!r) {
        pmm_free_pages((void*)paddr, 0, pages);
        return NULL;
    }
    r->next = vm_regions;
    vm_regions = r;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = vaddr + i * PAGE_SIZE;
        uintptr_t pa = paddr + i * PAGE_SIZE;

        page_attrs_t pg_attrs = {
            .present = 1,
            .rw = attrs.rw,
            .user = attrs.user,
            .frame_addr = pa >> 12
        };
        if (map_page(va, pa, pg_attrs) < 0) {
            // Rollback on failure
            for (size_t j = 0; j < i; j++) {
                uintptr_t undo_va = vaddr + j * PAGE_SIZE;
                unmap_page(undo_va);
            }
            vm_regions = r->next;
            pmm_free_pages((void*)paddr, 0, pages);
            return NULL;
        }
    }
    return (void*)vaddr;
}

void vmm_free_address_space(void* base, size_t size) {
    uintptr_t vaddr = (uintptr_t)base;
    size_t pages = _align_up(size) / PAGE_SIZE;

    vm_region_t **prev = &vm_regions;
    vm_region_t *r = vm_regions;

    while (r) {
        if (r->start == vaddr && (r->end - r->start) == size) {
            *prev = r->next;

            for (size_t i = 0; i < pages; ++i) {
                uintptr_t va = vaddr + i * PAGE_SIZE;

                if (!vmm_is_mapped(va)) continue;

                uintptr_t pa = virt_to_phys(va);
                unmap_page(va);
                pmm_free_page((void*)pa);
            }


            flop_memset(r, 0, sizeof(vm_region_t)); 
            return;
        }

        prev = &r->next;
        r = r->next;
    }
}

static void _id_map(void) {
    for (uintptr_t addr = 0; addr < 0x100000; addr += PAGE_SIZE) {
        page_attrs_t attrs = {
            .present = 1,
            .rw = 1,
            .user = 0,
            .write_thru = 0,
            .cache_dis = 0,
            .accessed = 0,
            .dirty = 0,
            .global = 0,
            .available = 0,
            .frame_addr = 0  // will be set by map_page
        };
        // Identity map: vaddr == paddr
        map_page(addr, addr, attrs);
    }
}

static void _undo_id_map(void) {
    for (uintptr_t addr = 0; addr < 0x100000; addr += PAGE_SIZE) {
        unmap_page(addr);
    }
}

static void _map_kernel(uintptr_t k_start_addr, size_t size) {
    for (uintptr_t offset = 0; offset < size; offset += PAGE_SIZE) {
        uintptr_t vaddr = KERNEL_VADDR_BASE + offset;
        uintptr_t paddr = k_start_addr + offset;
        page_attrs_t attrs = {
            .present = 1,
            .rw = 1,
            .user = 0,
            .write_thru = 0,
            .cache_dis = 0,
            .accessed = 0,
            .dirty = 0,
            .global = 0,
            .available = 0,
            .frame_addr = 0  // will be set by map_page
        };
        map_page(vaddr, paddr, attrs);
    }
}

int vmm_init(void) {
    uintptr_t k_start_addr = (uintptr_t)&_kernel_start;
    uintptr_t k_end_addr   = (uintptr_t)&_kernel_end;
    size_t k_size = PAGE_ALIGN(k_end_addr - k_start_addr);

    _zero_pd();

    _paging_init_recursive();

    _id_map();

    _map_kernel(k_start_addr, k_size);

    _undo_id_map();

    log("vmm: initialized\n", GREEN);
    return 0;
}

static inline uintptr_t _fetch_frame_paddr(uintptr_t frame_addr) {
    return frame_addr << 12;
}

static inline uintptr_t _fetch_page_offset(uintptr_t addr) {
    return addr & 0xFFF;
}

static inline uintptr_t _resolve_paddr(uintptr_t paddr, uintptr_t vaddr) {
    return paddr | _fetch_page_offset(vaddr);
}

static inline uintptr_t _resolve_vaddr(uint32_t pd_idx, uint32_t pt_idx, uintptr_t offset) {
    return (pd_idx << 22) | (pt_idx << 12) | offset;
}

uintptr_t virt_to_phys(uintptr_t vaddr) {
    vaddr = _align_down(vaddr);

    uint32_t pd_idx = _pd_index(vaddr);
    uint32_t pt_idx = _pt_index(vaddr);

    if (!RESOLVE_RECURSIVE_PD[pd_idx].present) return 0;
    pte_t* pt = RESOLVE_RECURSIVE_PT(pd_idx);
    if (!pt[pt_idx].present) return 0;

    uintptr_t paddr = _fetch_frame_paddr(pt[pt_idx].frame_addr);
    return _resolve_paddr(paddr, vaddr);
}

uintptr_t phys_to_virt(uintptr_t paddr) {
    paddr = _align_down(paddr);

    for (uint32_t pd_idx = 0; pd_idx < PAGE_DIRECTORY_SIZE; pd_idx++) {
        if (!RESOLVE_RECURSIVE_PD[pd_idx].present) continue;

        pte_t* pt = RESOLVE_RECURSIVE_PT(pd_idx);
        for (uint32_t pt_idx = 0; pt_idx < PAGE_TABLE_SIZE; pt_idx++) {
            if (!pt[pt_idx].present) continue;

            if (_fetch_frame_paddr(pt[pt_idx].frame_addr) == paddr) {
                return _resolve_vaddr(pd_idx, pt_idx, _fetch_page_offset(paddr));
            }
        }
    }
    return 0;
}
int map_kernel_space(uintptr_t vaddr, size_t size) {
    if (vaddr < KERNEL_VADDR_BASE) {
        return -1;
    }

    page_attrs_t attrs = {
        .present = 1,
        .rw = 1,
        .user = 0
    };

    uintptr_t phys_addr = (uintptr_t)pmm_alloc_pages(0, size / PAGE_SIZE);
    return map_range(vaddr, phys_addr, size, attrs);
}


int map_user_space(uintptr_t vaddr, size_t size) {
    if (vaddr >= USER_SPACE_END) {
        return -1;
    }

    page_attrs_t attrs = {
        .present = 1,
        .rw = 1,
        .user = 1
    };

    uintptr_t phys_addr = (uintptr_t)pmm_alloc_pages(0, size / PAGE_SIZE);
    return map_range(vaddr, phys_addr, size, attrs);
}


// Find region containing vaddr
static vm_region_t* find_region(uintptr_t vaddr) {
    vm_region_t* cur = vm_regions;
    while (cur) {
        if (vaddr >= cur->start && vaddr < cur->end)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

page_attrs_t vaddr_fetch_attrs(uintptr_t vaddr) {
    vm_region_t* r = find_region(vaddr);
    if (!r) {
        page_attrs_t zero = {0};
        return zero;
    }
    return r->attrs;
}

uintptr_t vmm_find_free_region(uintptr_t min, size_t size) {
    uintptr_t aligned_min = _align_up(min);
    uintptr_t end = aligned_min + size;
    vm_region_t* cur = vm_regions;
    while (cur) {
        if (cur->end <= aligned_min) {
            cur = cur->next;
            continue;
        }
        if (cur->start >= end) {
            return aligned_min; // Found a free region
        }
        aligned_min = cur->end; // Move to the end of the current region
        end = aligned_min + size; 
    }
    return 0;
}

void vmm_dump_mappings(void) {
    log("vmm: Dumping mappings:\n", GREEN);
    vm_region_t* cur = vm_regions;
    while (cur) {
        char buf[64];
        flopsnprintf(buf, sizeof(buf), "Region: [0x%p - 0x%p], attrs: {present: %d, rw: %d, user: %d}\n",
                 cur->start, cur->end, cur->attrs.present, cur->attrs.rw, cur->attrs.user);
        log(buf, LIGHT_GRAY);
        cur = cur->next;
    }
    log("\n", GREEN);
}

