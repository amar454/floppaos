/* 

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

------------------------------------------------------------------------------

    vmm.c - functions for mapping and unmapping physical pages to virtual addresses

        map_page(vaddr, paddr, attrs) - maps a page at paddr to vaddr with the given attributes

        unmap_page(vaddr) - unmaps a page 


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


extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

extern pde_t pd[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PAGE_SIZE)));

static inline GNU_INLINE uint32_t _pd_index(uintptr_t addr) {
   return (addr >> 22) & 0x3FF;
}

static inline GNU_INLINE uint32_t _pt_index(uintptr_t addr) {
   return (addr >> 12) & 0x3FF;
}

static inline GNU_INLINE uintptr_t _align_down(uintptr_t addr) {
    return addr & ~(PAGE_SIZE - 1);
}

static inline GNU_INLINE uintptr_t _align_up(uintptr_t addr) {
    return PAGE_ALIGN(addr);
}

static void _zero_pt(pte_t* pt) {
    
    flop_memset(pt, 0, PAGE_TABLE_SIZE * sizeof(pte_t));
}

static void _zero_pd(void) {
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        page_attrs_t zero = {0};
        SET_PF((pte_t*)&pd[i], zero);
    }
}

static pte_t* _alloc_pt(void) {
    void* pt = pmm_alloc_page();
    if (!pt) return NULL;
    return (pte_t*)pt;
}

static int _pd_lvl(pte_t* tables[2], uint32_t indices[2], page_attrs_t attrs) {
    if (!tables[0][indices[0]].present) {
        pte_t* new_pt = (pte_t*)pmm_alloc_page();
        if (!new_pt) return -1;
        _zero_pt((pte_t*)((uintptr_t)new_pt));

        page_attrs_t new_attrs = {
            .present = attrs.present,
            .rw = attrs.rw,
            .user = attrs.user,
            .frame_addr = ((uintptr_t)new_pt) >> 12 // SDM: bits 12-31 are base address
        };
        SET_PF((pte_t*)&tables[0][indices[0]], new_attrs);
    }
    tables[1] = RESOLVE_RECURSIVE_PT(indices[0]);
    return 0;
}

static void _pt_lvl(pte_t* tables[2], uint32_t indices[2], uintptr_t paddr, page_attrs_t attrs) {
    if (tables[1][indices[1]].present) {
        pmm_free_page((void*)(tables[1][indices[1]].frame_addr << 12));
    }
    attrs.frame_addr = paddr >> 12; // SDM: bits 12-31 are base address
    SET_PF(&tables[1][indices[1]], attrs);
}

int map_page(uintptr_t vaddr, uintptr_t paddr, page_attrs_t attrs) {
    vaddr = _align_down(vaddr);
    paddr = _align_down(paddr);
    if (vaddr == 0) return -1; 

    uint32_t indices[2] = {_pd_index(vaddr), _pt_index(vaddr)};
    pte_t* tables[2] = {(pte_t*)RESOLVE_RECURSIVE_PD, NULL};
    for (int i = 0; i < 2; i++) { // walk page table levels
        switch (i) { // in ia32 we have two levels 
            case 0:
                _pd_lvl(tables, indices, attrs);
            case 1:
                _pt_lvl(tables, indices, paddr, attrs);
        }
    }

    __asm__ volatile("invlpg (%0)" :: "a"(vaddr));
    return 0;
}

int unmap_page(uintptr_t vaddr) {
    vaddr = _align_down(vaddr);
    uint32_t pd_idx = _pd_index(vaddr);
    uint32_t pt_idx = _pt_index(vaddr);

    if (!RESOLVE_RECURSIVE_PD[pd_idx].present) return -1;

    pte_t* pt = RESOLVE_RECURSIVE_PT(pd_idx); 

    if (!pt[pt_idx].present) return -1;
    pmm_free_page((void*)(pt[pt_idx].frame_addr << 12));
    page_attrs_t zero = {0};
    SET_PF(&pt[pt_idx], zero);
    bool pt_empty = true;
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        if (pt[i].present) {
            pt_empty = false;
            break;
        }
    }
    if (pt_empty) {
        pmm_free_page((void*)(RESOLVE_RECURSIVE_PD[pd_idx].table_addr << 12));
        page_attrs_t zero_pd = {0};
        SET_PF((pte_t*)&RESOLVE_RECURSIVE_PD[pd_idx], zero_pd);
    }

    asm volatile("invlpg (%0)" :: "a"(vaddr));
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
int nuke_pagemap(uintptr_t base, size_t size) {
    size_t pages = _align_up(size) / PAGE_SIZE;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t vaddr = base + i * PAGE_SIZE;

        uint32_t pd_idx = _pd_index(vaddr);
        uint32_t pt_idx = _pt_index(vaddr);

        if (!RESOLVE_RECURSIVE_PD[pd_idx].present) {
            i += PAGE_TABLE_SIZE - pt_idx - 1;
            continue;
        }

        pte_t *pt = RESOLVE_RECURSIVE_PT(pd_idx);
        if (!pt[pt_idx].present)
            continue;

        uintptr_t paddr = pt[pt_idx].frame_addr << PAGE_SIZE_SHIFT;
        pmm_free_page((void *)paddr);

        unmap_page(vaddr);
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

static void _free_region(vm_region_t* r) {
    if (!r) return;
    pmm_free_page(r);
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
#define KERNEL_VADDR_LIMIT   0xFF000000UL
static uintptr_t vmm_expand_kernel_space(size_t size) {

    uintptr_t max_end = KERNEL_VADDR_BASE;

    for (vm_region_t *it = vm_regions; it; it = it->next) {
        if (it->end > max_end)
            max_end = it->end;
    }

    uintptr_t candidate = PAGE_ALIGN(max_end);

    if (candidate < KERNEL_VADDR_BASE)
        candidate = KERNEL_VADDR_BASE;

    if ((candidate + size) >= KERNEL_VADDR_LIMIT)
        return 0;

    return candidate;
}
void* vmm_create_address_space(size_t size, page_attrs_t attrs) {
    size_t pages = _align_up(size) / PAGE_SIZE;

    uintptr_t vaddr;
    uintptr_t paddr;
    vm_region_t *r;

retry_find:
    vaddr = vmm_find_free_region(KERNEL_VADDR_BASE, size);
    if (vaddr == 0) {
        /* no free region found — try to expand kernel virtual space */
        vaddr = vmm_expand_kernel_space(size);
        if (vaddr == 0) {
            return NULL; /* completely out of virtual address space */
        }
    }

    /* 2) allocate physical pages */
    paddr = (uintptr_t)pmm_alloc_pages(0, pages);
    if (paddr == 0) {
        return NULL; /* no free physical memory */
    }

    /* create region descriptor */
    r = _make_region(vaddr, vaddr + size, attrs);
    if (!r) {
        pmm_free_pages((void*)paddr, 0, pages);

        /*
         * If _make_region failed because of internal fragmentation or collisions,
         * try again: either find a different region or expand further.
         * Use the goto retry to attempt another allocation/region find.
         */
        goto retry_find;
    }

    /* link into region list */
    r->next = vm_regions;
    vm_regions = r;

    /* map pages */
    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = vaddr + i * PAGE_SIZE;
        uintptr_t pa = paddr + i * PAGE_SIZE;

        page_attrs_t pg_attrs = {
            .present = 1,
            .rw = attrs.rw,
            .user = attrs.user,
            .frame_addr = pa >> PAGE_SIZE_SHIFT
        };

        if (map_page(va, pa, pg_attrs) < 0) {
            /* Rollback on failure */
            for (size_t j = 0; j < i; j++) {
                unmap_page(vaddr + j * PAGE_SIZE);
            }

            /* unlink and free region metadata */
            vm_regions = r->next;
            _free_region(r);
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
            .frame_addr = 0  
        };
        map_page(vaddr, paddr, attrs);
    }
}

int vmm_init(void) {
    uintptr_t k_start_addr = (uintptr_t)&_kernel_start;
    uintptr_t k_end_addr   = (uintptr_t)&_kernel_end;
    size_t k_size = PAGE_ALIGN(k_end_addr - k_start_addr);

    _map_kernel(k_start_addr, k_size);

    size_t phys_mem_size = pmm_get_memory_size();   

    size_t kernel_space_size = phys_mem_size / 3;
    size_t user_space_size   = phys_mem_size - kernel_space_size;

    uintptr_t kernel_phys_start = 0x00000000;
    uintptr_t kernel_phys_end   = kernel_phys_start + kernel_space_size;

    uintptr_t user_phys_start   = kernel_phys_end;
    uintptr_t user_phys_end     = user_phys_start + user_space_size;

    for (uintptr_t paddr = kernel_phys_start; paddr < kernel_phys_end; paddr += PAGE_SIZE) {
        uintptr_t vaddr = KERNEL_VADDR_BASE + (paddr - kernel_phys_start);
        page_attrs_t attrs = {
            .present = 1,
            .rw = 1,
            .user = 0,
            .write_thru = 0,
            .cache_dis = 0,
            .accessed = 0,
            .dirty = 0,
            .global = 1,
            .available = 0,
            .frame_addr = 0
        };
        map_page(vaddr, paddr, attrs);
    }

    for (uintptr_t paddr = user_phys_start; paddr < user_phys_end; paddr += PAGE_SIZE) {
        uintptr_t vaddr = USER_VADDR_BASE + (paddr - user_phys_start);
        page_attrs_t attrs = {
            .present = 1,
            .rw = 1,
            .user = 1,
            .write_thru = 0,
            .cache_dis = 0,
            .accessed = 0,
            .dirty = 0,
            .global = 0,
            .available = 0,
            .frame_addr = 0
        };
        map_page(vaddr, paddr, attrs);
    }
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
