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
#include "alloc.h"
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

/**
 * Virtual address space setup for floppaOS, the Flopperating System:
 *
 *   Virtual Address Space (4 GiB total):
 *
 *     0xFFFFFFFF  +--------------------------+
 *                 |      Kernel Space        |  (1 GiB)
 *                 |  [Supervisor Only]       |
 *                 |  gbl, rw, non-user       |
 *                 +--------------------------+ 0xC0000000 (KERNEL_VADDR_BASE)
 *                 |      User Space          |  (3 GiB)
 *                 |  [User + Kernel Access]  |
 *                 |  Demand-paged, RW/User   |
 *                 +--------------------------+ 0x00000000
 *
 *    Physical memory layout to virtual address mapping:
 * 
 *     Example: 96 MiB physical memory
 *
 *     0x05FFFFFF  +--------------------------+
 *                 |  User Physical Memory    |  (2/3 of total)
 *                 |                          |
 *                 |                          |
 *                 |                          |
 *     0x02000000  +--------------------------+
 *                 | Kernel physical memory   |  (1/3 of total)
 *                 | Kernel image + kmalloc   |
 *     0x00000000  +--------------------------+
 *
 * Final virtual address mapping:
 *
 *     Kernel Physical 0x00000000 → Kernel Virtual 0xC0000000
 *     User Physical   0x02000000 → User Virtual   0x00000000
 *
 */
int vmm_init(void) {
    uintptr_t k_start_addr = (uintptr_t)&_kernel_start;
    uintptr_t k_end_addr   = (uintptr_t)&_kernel_end;
    size_t k_size = PAGE_ALIGN(k_end_addr - k_start_addr);

    if (k_start_addr == 0 || k_end_addr == 0 || k_size == 0)
        return -1;

    for (uintptr_t offset = 0; offset < k_size; offset += PAGE_SIZE) {
        uintptr_t vaddr = KERNEL_VADDR_BASE + offset;
        uintptr_t paddr = k_start_addr + offset;
        page_attrs_t attrs = {
            .present    = 1, 
            .rw         = 1, 
            .user       = 0, 
            .global     = 0,
            .write_thru = 0,
            .cache_dis  = 0, 
            .accessed   = 0,
            .dirty      = 0, 
            .available  = 0, 
            .frame_addr = 0
        };
        map_page(vaddr, paddr, attrs);
    }

    size_t phys_mem_size = pmm_get_memory_size();
    if (phys_mem_size == 0)
        return -1;

    size_t kernel_phys_size = phys_mem_size / 3;
    size_t user_phys_size   = phys_mem_size - kernel_phys_size;

    uintptr_t kernel_phys_start = 0x00000000;
    uintptr_t kernel_phys_end   = kernel_phys_start + kernel_phys_size;

    uintptr_t user_phys_start   = kernel_phys_end;
    uintptr_t user_phys_end     = user_phys_start + user_phys_size;

    for (uintptr_t paddr = kernel_phys_start; paddr < kernel_phys_end; paddr += PAGE_SIZE) {
        uintptr_t vaddr = KERNEL_VADDR_BASE + (paddr - kernel_phys_start);
        page_attrs_t attrs = {
            .present    = 1, 
            .rw         = 1, 
            .user       = 0, 
            .global     = 1,
            .write_thru = 0, 
            .cache_dis  = 0, 
            .accessed   = 0,
            .dirty      = 0, 
            .available  = 0, 
            .frame_addr = 0
        };
        map_page(vaddr, paddr, attrs);
    }

    uintptr_t heap_start = KERNEL_HEAP_START;
    uintptr_t heap_end   = heap_start + MAX_HEAP_SIZE;

    for (uintptr_t vaddr = heap_start; vaddr < heap_end; vaddr += PAGE_SIZE) {
        uintptr_t paddr = pmm_alloc_page();
        if (paddr == 0)
            return -1;

        page_attrs_t attrs = {
            .present    = 1, 
            .rw         = 1, 
            .user       = 0, 
            .global     = 1,
            .write_thru = 0, 
            .cache_dis  = 0, 
            .accessed   = 0,
            .dirty      = 0, 
            .available  = 0, 
            .frame_addr = 0
        };
        map_page(vaddr, paddr, attrs);
    }

    for (uintptr_t paddr = user_phys_start; paddr < user_phys_end; paddr += PAGE_SIZE) {
        uintptr_t vaddr = USER_VADDR_BASE + (paddr - user_phys_start);
        page_attrs_t attrs = {
            .present    = 1, 
            .rw         = 1, 
            .user       = 1, 
            .global     = 0,
            .write_thru = 0, 
            .cache_dis  = 0, 
            .accessed   = 0,
            .dirty      = 0, 
            .available  = 0, 
            .frame_addr = 0
        };
        map_page(vaddr, paddr, attrs);
    }

    return 0;
}

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
    _zero_pt((pte_t*)pt);
    return _align_down((uintptr_t)pt);
}

static int _pd_lvl(pte_t* tables[2], uint32_t indices[2], page_attrs_t attrs) {
    if (!tables[0][indices[0]].present) {
        pte_t* new_pt = (pte_t*)pmm_alloc_page();
        if (!new_pt) return -1;
        _zero_pt((pte_t*)((uintptr_t)new_pt));

        page_attrs_t new_attrs = {
            .present    = attrs.present,
            .rw         = attrs.rw,
            .user       = attrs.user,
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
                break;
            case 1:
                _pt_lvl(tables, indices, paddr, attrs);
                break;
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

    pmm_free_page((void*)(pt[pt_idx].frame_addr << PAGE_SIZE_SHIFT));

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
        pmm_free_page((void*)(RESOLVE_RECURSIVE_PD[pd_idx].table_addr << PAGE_SIZE_SHIFT));
        page_attrs_t zero_pd = {0};
        SET_PF((pte_t*)&RESOLVE_RECURSIVE_PD[pd_idx], zero_pd);
    }

    __asm__ volatile("invlpg (%0)" :: "a"(vaddr));
    return 0;
}

int vmm_protect(uintptr_t vaddr, page_attrs_t attrs) {
    vaddr = _align_down(vaddr);
    uint32_t pd_idx = _pd_index(vaddr);
    uint32_t pt_idx = _pt_index(vaddr);

    if (!RESOLVE_RECURSIVE_PD[pd_idx].present) return -1;

    pte_t* pt = RESOLVE_RECURSIVE_PT(pd_idx);
    if (!pt[pt_idx].present) return -1;

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

        if (!RESOLVE_RECURSIVE_PD[src_pd_idx].present) continue;

        pte_t* src_pt = RESOLVE_RECURSIVE_PT(src_pd_idx);
        if (!src_pt[src_pt_idx].present) continue;

        uintptr_t src_paddr = src_pt[src_pt_idx].frame_addr << PAGE_SIZE_SHIFT;

        page_attrs_t attrs = {
            .present = src_pt[src_pt_idx].present,
            .rw      = src_pt[src_pt_idx].rw,
            .user    = src_pt[src_pt_idx].user
        };

        void* dst_frame = pmm_alloc_page();
        if (!dst_frame) return -1;

        flop_memcpy(dst_frame, (void*)src_paddr, PAGE_SIZE);
        attrs.frame_addr = ((uintptr_t)dst_frame) >> PAGE_SIZE_SHIFT;

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
        if (!pt[pt_idx].present) continue;

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

static vm_region_t* _merge_regions(vm_region_t* a, vm_region_t* b) {
    if (a->end < b->start) return NULL;
    if (b->end < a->start) return NULL;
    return _make_region(a->start, b->end, a->attrs);
}

static void _free_region(vm_region_t* r) {
    if (!r) return;
    pmm_free_page(r);
}

int map_range(uintptr_t vaddr, uintptr_t paddr, size_t size, page_attrs_t attrs) {
    size_t pages = _align_up(size) / PAGE_SIZE;

    vm_region_t* r = _make_region(vaddr, vaddr + size, attrs);
    if (!r) return -1;

    r->next = vm_regions;
    vm_regions = r;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = vaddr + i * PAGE_SIZE;
        uintptr_t pa = paddr + i * PAGE_SIZE;

        if (map_page(va, pa, attrs) < 0) {
            for (size_t j = 0; j < i; j++) {
                uintptr_t undo_va = vaddr + j * PAGE_SIZE;
                unmap_page(undo_va);
            }
            vm_regions = r->next;
            _free_region(r);
            return -1;
        }
    }

    return 0;
}

int unmap_range(uintptr_t vaddr, size_t size) {
    size_t pages = _align_up(size) / PAGE_SIZE;

    vm_region_t *cur = vm_regions, *prev = NULL;

    while (cur) {
        if (cur->start == vaddr && cur->end == vaddr + size) {
            if (prev) prev->next = cur->next;
            else vm_regions = cur->next;

            _free_region(cur);
            break;
        }

        prev = cur;
        cur = cur->next;
    }

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = vaddr + i * PAGE_SIZE;
        if (unmap_page(va) < 0) {
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

vm_region_t* vmm_find_region_by_start(uintptr_t addr) {
    vm_region_t *r = vm_regions;
    while (r) {
        if (r->start == addr) {
            return r;
        }
        r = r->next;
    }
    return NULL;
}

int vmm_unlink_region(vm_region_t *region) {
    if (!region) return -1;
    vm_region_t **p = &vm_regions;
    while (*p && *p != region) {
        p = &(*p)->next;
    }
    if (*p == region) {
        *p = region->next;
        return 0;
    }
    return -1;
}

int vmm_attrs_cmp(const page_attrs_t *a, const page_attrs_t *b) {
    return flop_memcmp(a, b, sizeof(page_attrs_t)) == 0;
}

void vmm_unmap_region(vm_region_t *r) {
    size_t pages = (r->end - r->start) / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = r->start + i * PAGE_SIZE;
        if (vmm_is_mapped(va)) {
            uintptr_t pa = virt_to_phys(va);
            unmap_page(va);
            pmm_free_page((void*)pa);
        }
    }
}

void vmm_free_region(vm_region_t *r) {
    if (!r) return;
    vmm_unmap_region(r);
    vmm_unlink_region(r);
    _free_region(r);
}

vm_region_t* vmm_find_region_containing(uintptr_t addr) {
    vm_region_t *r = vm_regions;
    while (r) {
        if (addr >= r->start && addr < r->end) {
            return r;
        }
        r = r->next;
    }
    return NULL;
}

int vmm_is_range_free(uintptr_t start, size_t size) {
    uintptr_t end = start + size;
    vm_region_t *r = vm_regions;
    while (r) {
        if (!(end <= r->start || start >= r->end)) {
            return 0; 
        }
        r = r->next;
    }
    return 1; // No overlap, range is free
}

void _vmm_secure_page_free(void *p) {
    // we memset here to prevent data leakage.
    flop_memset(p, 0, PAGE_SIZE);
    pmm_free_page(p);
}

int vmm_check_access(uintptr_t vaddr, int write_access, int user_mode) {
    vm_region_t *r = vmm_find_region_containing(vaddr);
    if (!r) return -1;
    if (user_mode && !r->attrs.user) return -2;
    if (write_access && !r->attrs.rw) return -3;
    return 0;
}

void* vmm_create_address_space(size_t size, page_attrs_t attrs) {
    size_t pages = _align_up(size) / PAGE_SIZE;
    uintptr_t vaddr, paddr;
    vm_region_t *r = NULL;

    int retry_count = 0;
    const int max_retries = 3;

    while (retry_count++ <= max_retries) {
        vaddr = vmm_find_free_region(KERNEL_VADDR_BASE, size);
        if (vaddr == 0) {
            vaddr = vmm_expand_kernel_space(size);
            if (vaddr == 0) return NULL;
        }

        paddr = (uintptr_t)pmm_alloc_pages(0, pages);
        if (paddr == 0) return NULL;

        r = _make_region(vaddr, vaddr + size, attrs);
        if (r) break;  

        pmm_free_pages((void*)paddr, 0, pages);


    }

    if (!r) return NULL;  // exhausted retries

    r->next = vm_regions;
    vm_regions = r;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = vaddr + i * PAGE_SIZE;
        uintptr_t pa = paddr + i * PAGE_SIZE;

        page_attrs_t pg_attrs = {
            .present    = 1,
            .rw         = attrs.rw,
            .user       = attrs.user,
            .frame_addr = pa >> PAGE_SIZE_SHIFT
        };

        if (map_page(va, pa, pg_attrs) < 0) {
            // rollback
            for (size_t j = 0; j < i; j++) {
                unmap_page(vaddr + j * PAGE_SIZE);
            }
            vm_regions = r->next;
            _free_region(r);
            pmm_free_pages((void*)paddr, 0, pages);
            return NULL;
        }
    }

    return (void*)vaddr;
}

void vmm_free_address_space(void* base, size_t size) {
    vm_region_t *r = vmm_find_region_by_start((uintptr_t)base);
    if (r && ((r->end - r->start) == size)) {
        vmm_unmap_range((uintptr_t)base, size);
        vmm_unlink_region(r);
        _free_region(r);
    }
}

int vmm_merge_address_spaces(void* base1, void* base2) {
    vm_region_t *r1 = vmm_find_region_by_start((uintptr_t)base1);
    vm_region_t *r2 = vmm_find_region_by_start((uintptr_t)base2);

    if (!r1 || !r2) return -1;
    if (r1->end != r2->start) return -2;
    if (!vmm_attrs_equal(&r1->attrs, &r2->attrs)) return -3;

    r1->end = r2->end;
    vmm_unlink_region(r2);
    _free_region(r2);

    return 0;
}

void* vmm_clone_address_space(void* base, size_t size) {
    size_t pages = _align_up(size) / PAGE_SIZE;
    uintptr_t src_base = (uintptr_t)base;

    uintptr_t dst_vaddr = vmm_find_free_region(KERNEL_VADDR_BASE, size);
    if (dst_vaddr == 0) {
        dst_vaddr = vmm_expand_kernel_space(size);
        if (dst_vaddr == 0) return NULL;
    }

    for (size_t i = 0; i < pages; i++) {
        uintptr_t src_va = src_base + i * PAGE_SIZE;
        uintptr_t dst_va = dst_vaddr + i * PAGE_SIZE;

        if (!vmm_is_mapped(src_va)) continue;

        uintptr_t src_pa = virt_to_phys(src_va);
        void* dst_frame = pmm_alloc_page();
        if (!dst_frame) {
            vmm_unmap_range(dst_vaddr, i * PAGE_SIZE);
            return NULL;
        }

        flop_memcpy(dst_frame, (void*)src_pa, PAGE_SIZE);

        // Use original page attrs
        page_attrs_t attrs = {
            .present    = 1,
            .rw         = 1,
            .user       = 1,
            .frame_addr = ((uintptr_t)dst_frame) >> PAGE_SIZE_SHIFT
        };

        if (map_page(dst_va, (uintptr_t)dst_frame, attrs) < 0) {
            pmm_free_page(dst_frame);
            vmm_unmap_range(dst_vaddr, i * PAGE_SIZE);
            return NULL;
        }
    }

    vm_region_t *r = _make_region(dst_vaddr, dst_vaddr + size, 
        (page_attrs_t) {
            .present= 1, 
            .rw     = 1, 
            .user   = 1
        }
    );
    if (!r) {
        vmm_unmap_range(dst_vaddr, size);
        return NULL;
    }

    r->next = vm_regions;
    vm_regions = r;

    return (void*)dst_vaddr;
}

int vmm_protect_address_space(void* base, size_t size, page_attrs_t new_attrs) {
    if (!base || size == 0) return -1;

    uintptr_t vaddr = (uintptr_t)base;
    size_t pages = _align_up(size) / PAGE_SIZE;

    vm_region_t *r = vmm_find_region_by_start(vaddr);
    if (!r || (r->end - r->start) != size) return -2;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = vaddr + (i * PAGE_SIZE);

        if (!vmm_is_mapped(va)) continue;

        uintptr_t pa = virt_to_phys(va);

        page_attrs_t pg_attrs = {
            .present    = new_attrs.present,
            .rw         = new_attrs.rw,
            .user       = new_attrs.user,
            .frame_addr = pa >> PAGE_SIZE_SHIFT
        };

        if (map_page(va, pa, pg_attrs) < 0) {
            return -3;
        }
    }

    r->attrs = new_attrs;
    return 0;
}

int vmm_verify_region_mappings(uintptr_t base, size_t size, page_attrs_t expected_attrs) {
    size_t pages = _align_up(size) / PAGE_SIZE;
    vm_region_t* r = vmm_find_region_by_start(base);
    if (!r || (r->end - r->start) != size) return -1;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = base + i * PAGE_SIZE;
        if (!vmm_is_mapped(va)) {
            return -2; 
        }

        uintptr_t pa = virt_to_phys(va);
        if (!pa) return -3;

        page_attrs_t actual = vaddr_fetch_attrs(va);

        if (actual.present != expected_attrs.present ||
            actual.rw != expected_attrs.rw ||
            actual.user != expected_attrs.user) {
            return -4; 
        }
    }

    return 0; 
}

int vmm_verify_region_consistency(vm_region_t* r) {
    if (!r) return -1;
    if (r->start >= r->end) return -2;

    size_t size = r->end - r->start;
    size_t pages = _align_up(size) / PAGE_SIZE;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = r->start + i * PAGE_SIZE;
        if (!vmm_is_mapped(va)) return -3;

        page_attrs_t attrs = vaddr_fetch_attrs(va);
        if (
            attrs.present != r->attrs.present ||
            attrs.rw != r->attrs.rw           ||
            attrs.user != r->attrs.user
        ) return -4;
    }

    return 0;
}

int vmm_verify_region_list(void) {
    vm_region_t* slow = vm_regions;
    vm_region_t* fast = vm_regions;

    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return -1; // cycle detected
    }

    vm_region_t* cur = vm_regions;
    while (cur) {
        if (cur->start >= cur->end) return -2; // invalid region range
        cur = cur->next;
    }

    return 0;
}

int vmm_verify_access_range(uintptr_t base, size_t size, int write_access, int user_mode) {
    size_t pages = _align_up(size) / PAGE_SIZE;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = base + i * PAGE_SIZE;
        int ret = vmm_check_access(va, write_access, user_mode);
        if (ret != 0) return ret;
    }

    return 0;
}

int vmm_verify_no_overlap(uintptr_t base, size_t size) {
    uintptr_t end = base + size;
    vm_region_t* cur = vm_regions;

    while (cur) {
        if (!(cur->end <= base || cur->start >= end)) {
            return -1; 
        }
        cur = cur->next;
    }
    return 0;
}

int vmm_verify_all_regions(void) {
    vm_region_t* cur = vm_regions;
    while (cur) {
        int ret = vmm_verify_region_consistency(cur);
        if (ret != 0) return ret;
        cur = cur->next;
    }
    return 0;
}

static page_attrs_t make_attrs(int present, int rw, int user) {
    page_attrs_t a = {0};
    a.present = present;
    a.rw = rw;
    a.user = user;
    return a;
}

int vmm_ro(void* base, size_t size) {
    return vmm_protect_address_space(base, size, make_attrs(1, 0, 1));
}

int vmm_rw(void* base, size_t size) {
    return vmm_protect_address_space(base, size, make_attrs(1, 1, 1));
}

int vmm_user(void* base, size_t size) {
    //rw=1, user=1, present=1
    return vmm_protect_address_space(base, size, make_attrs(1, 1, 1));
}

int vmm_ker(void* base, size_t size) {
    // user=0, rw=1, present=1
    return vmm_protect_address_space(base, size, make_attrs(1, 1, 0));
}

int vmm_flip_rw(void* base, size_t size) {
    vm_region_t *r = vmm_find_region_by_start((uintptr_t)base);
    if (!r || (r->end - r->start) != size) return -2;

    int new_rw = !r->attrs.rw;
    return vmm_protect_address_space(base, size, make_attrs(r->attrs.present, new_rw, r->attrs.user));
}

int vmm_is_rw(void* base) {
    vm_region_t *r = vmm_find_region_by_start((uintptr_t)base);
    return r ? r->attrs.rw : 0;
}

int vmm_is_user(void* base) {
    vm_region_t *r = vmm_find_region_by_start((uintptr_t)base);
    return r ? r->attrs.user : 0;
}

int vmm_set_flags(void* base, size_t size, int present, int rw, int user) {
    return vmm_protect_address_space(base, size, make_attrs(present, rw, user));
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

void vmm_dump_region_addr(vm_region_t* r) {
    if (!r) return;
    char buf[64];
    flopsnprintf(buf, sizeof(buf), "Region: [0x%p - 0x%p]\n", r->start, r->end);
    log(buf, LIGHT_GRAY);
}

void vmm_dump_region_basic(vm_region_t* r) {
    if (!r) return;
    char buf[96];
    flopsnprintf(buf, sizeof(buf),
                 "Region: [0x%p - 0x%p], present: %d, rw: %d, user: %d\n",
                 r->start, r->end, r->attrs.present, r->attrs.rw, r->attrs.user);
    log(buf, LIGHT_GRAY);
}

void vmm_dump_region_ext(vm_region_t* r) {
    if (!r) return;
    char buf[128];
    flopsnprintf(buf, sizeof(buf),
        "Region: [0x%p - 0x%p], present: %d, rw: %d, user: %d, global: %d, writethru: %d, cachedis: %d\n",
        r->start, r->end, r->attrs.present, r->attrs.rw, r->attrs.user,
        r->attrs.global, r->attrs.write_thru, r->attrs.cache_dis);
    log(buf, LIGHT_GRAY);
}

void vmm_dump_region_full(vm_region_t* r) {
    if (!r) return;
    char buf[160];
    flopsnprintf(buf, sizeof(buf),
        "Region: [0x%p - 0x%p], present: %d, rw: %d, user: %d, global: %d, writethru: %d, cachedis: %d, accessed: %d, dirty: %d, avail: %d\n",
        r->start, r->end, r->attrs.present, r->attrs.rw, r->attrs.user,
        r->attrs.global, r->attrs.write_thru, r->attrs.cache_dis,
        r->attrs.accessed, r->attrs.dirty, r->attrs.available);
    log(buf, LIGHT_GRAY);
}

void vmm_dump_all_addrs(void) {
    log("vmm: Dumping all regions (addresses only):\n", GREEN);
    for (vm_region_t* cur = vm_regions; cur; cur = cur->next) {
        vmm_dump_region_addr(cur);
    }
    log("\n", GREEN);
}

void vmm_dump_all_basic(void) {
    log("vmm: Dumping all regions (basic attrs):\n", GREEN);
    for (vm_region_t* cur = vm_regions; cur; cur = cur->next) {
        vmm_dump_region_basic(cur);
    }
    log("\n", GREEN);
}

void vmm_dump_all_ext(void) {
    log("vmm: Dumping all regions (extended attrs):\n", GREEN);
    for (vm_region_t* cur = vm_regions; cur; cur = cur->next) {
        vmm_dump_region_ext(cur);
    }
    log("\n", GREEN);
}

void vmm_dump_all_full(void) {
    log("vmm: Dumping all regions (full attrs):\n", GREEN);
    for (vm_region_t* cur = vm_regions; cur; cur = cur->next) {
        vmm_dump_region_full(cur);
    }
    log("\n", GREEN);
}

void vmm_dump_region_at_basic(uintptr_t addr) {
    vm_region_t* r = find_region(addr);
    if (!r) {
        log("vmm: No region found at address\n", RED);
        return;
    }
    vmm_dump_region_basic(r);
}

void vmm_dump_page_attrs(uintptr_t vaddr) {
    page_attrs_t attrs = vaddr_fetch_attrs(vaddr);
    char buf[128];
    flopsnprintf(buf, sizeof(buf),
                 "Page 0x%p attrs: present=%d, rw=%d, user=%d, global=%d, writethru=%d, cachedis=%d, accessed=%d, dirty=%d, avail=%d\n",
                 vaddr, attrs.present, attrs.rw, attrs.user, attrs.global,
                 attrs.write_thru, attrs.cache_dis, attrs.accessed, attrs.dirty, attrs.available);
    log(buf, LIGHT_GRAY);
}

void vmm_dump_range_basic(uintptr_t start, size_t size) {
    uintptr_t end = start + size;
    log("vmm: Dumping range regions (basic):\n", GREEN);
    for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE) {
        vmm_dump_region_at_basic(addr);
    }
    log("\n", GREEN);
}