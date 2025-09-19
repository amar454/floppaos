/**
 * vmm 
 */

#include <stdint.h>
#include "pmm.h"
#include "vmm.h"
#include "paging.h"
#include "utils.h"
#include "../cpu/cpu.h"
#include "../lib/logging.h"

extern uint32_t *pg_dir;
extern uint32_t *pg_tbls;
extern uint32_t *current_pg_dir;

static vmm_region_t *region_list = 0;
static vmm_region_t kernel_region;

static inline uint32_t pd_index(uintptr_t va) {
    return (va >> 22) & 0x3FF;
}

static inline uint32_t pt_index(uintptr_t va) {
    return (va >> 12) & 0x3FF;
}

static inline uint32_t page_offset(uintptr_t va) {
    return va & 0xFFF;
}

#define RECURSIVE_ADDR 0xFFC00000
#define RECURSIVE_PT(pdi) ((uint32_t *)(RECURSIVE_ADDR + (pdi) * PAGE_SIZE))

int vmm_map(vmm_region_t *region, uintptr_t va, uintptr_t pa, uint32_t flags) {
    uint32_t pdi = pd_index(va);
    uint32_t pti = pt_index(va);

    if (!(region->pg_dir[pdi] & PAGE_PRESENT)) {
        uintptr_t pt_phys = (uintptr_t)pmm_alloc_page();
        if (!pt_phys) return -1;

        region->pg_dir[pdi] = (pt_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        flop_memset(RECURSIVE_PT(pdi), 0, PAGE_SIZE);
    }

    uint32_t *pt = RECURSIVE_PT(pdi);
    pt[pti] = (pa & PAGE_MASK) | flags | PAGE_PRESENT;

    invlpg((void *)va);
    return 0;
}

int vmm_unmap(vmm_region_t *region, uintptr_t va) {
    uint32_t pdi = pd_index(va);
    uint32_t pti = pt_index(va);

    if (!(region->pg_dir[pdi] & PAGE_PRESENT)) return -1;

    uint32_t *pt = RECURSIVE_PT(pdi);
    pt[pti] = 0;

    invlpg((void *)va);
    return 0;
}

uintptr_t vmm_resolve(vmm_region_t *region, uintptr_t va) {
    uint32_t pdi = pd_index(va);
    uint32_t pti = pt_index(va);

    if (!(region->pg_dir[pdi] & PAGE_PRESENT)) return 0;

    uint32_t *pt = RECURSIVE_PT(pdi);
    if (!(pt[pti] & PAGE_PRESENT)) return 0;

    return (pt[pti] & PAGE_MASK) | (va & ~PAGE_MASK);
}


static void region_insert(vmm_region_t *region) {
    region->next = region_list;
    region_list = region;
}

static void region_remove(vmm_region_t *region) {
    vmm_region_t **iter = &region_list;
    while (*iter) {
        if (*iter == region) {
            *iter = region->next;
            break;
        }
        iter = &(*iter)->next;
    }
}

vmm_region_t *vmm_region_create() {
    uintptr_t dir_phys = (uintptr_t)pmm_alloc_page();
    if (!dir_phys) return 0;
    uint32_t *dir = (uint32_t *)dir_phys;
    flop_memset(dir, 0, PAGE_SIZE);
    dir[RECURSIVE_PDE] = (dir_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;
    vmm_region_t *region = (vmm_region_t *)kmalloc(sizeof(vmm_region_t));
    region->pg_dir = dir;
    region->next = 0;
    region_insert(region);
    return region;
}

void vmm_region_destroy(vmm_region_t *region) {
    region_remove(region);
    pmm_free_page(region->pg_dir);
    kfree(region, sizeof(vmm_region_t));
}

void vmm_switch(vmm_region_t *region) {
    current_pg_dir = region->pg_dir;
    load_pd(region->pg_dir);
}

void vmm_init() {
    kernel_region.pg_dir = pg_dir;
    kernel_region.next = 0;
    current_pg_dir = pg_dir;
    pg_dir[RECURSIVE_PDE] = ((uintptr_t)pg_dir & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;
    region_insert(&kernel_region);
    // we do not need to load the kernel region because it has
    // been loaded into cr3 by the paging init function.
    log("vmm init - ok\n", GREEN);
}

// copy a region's mappings into a new region and add it to the linked list of virtual regions
vmm_region_t *vmm_copy_pagemap(vmm_region_t *src) {
    uintptr_t new_dir_phys = (uintptr_t)pmm_alloc_page();
    if (!new_dir_phys) return 0;
    uint32_t *new_dir = (uint32_t *)new_dir_phys;
    flop_memset(new_dir, 0, PAGE_SIZE);

    vmm_region_t *dst = (vmm_region_t *)kmalloc(sizeof(vmm_region_t));
    dst->pg_dir = new_dir;
    dst->next = 0;

    // pt allocation
    for (int pdi = 0; pdi < 1024; pdi++) {
        // do it
        if (!(src->pg_dir[pdi] & PAGE_PRESENT)) continue;
        uintptr_t pt_phys = (uintptr_t)pmm_alloc_page();

        // fall back if page alloc fails (important)
        if (!pt_phys) { 
            vmm_region_destroy(dst); 
            return 0; 
        }

        // determine src pt (index pdi*1024 in pg_tbls)
        uint32_t *src_pt = &pg_tbls[pdi * PAGE_ENTRIES];

        // set target pt to page we allocated
        uint32_t *dst_pt = (uint32_t *)pt_phys;
        flop_memset(dst_pt, 0, PAGE_SIZE);

        // frame allocation
        for (int pti = 0; pti < PAGE_ENTRIES; pti++) {
            // check if page is ok
            if (!(src_pt[pti] & PAGE_PRESENT)) continue; 
            uintptr_t new_page = (uintptr_t)pmm_alloc_page();

            // before copying frame we must check if the new page exists
            // fall back if frame allocation doesnt work out
            if (!new_page) { 
                vmm_region_destroy(dst); 
                return 0;
            }

            // copy frame
            flop_memcpy((void *)new_page, (void *)(src_pt[pti] & PAGE_MASK), PAGE_SIZE);
            dst_pt[pti] = (new_page & PAGE_MASK) | (src_pt[pti] & ~PAGE_MASK);
        }

        new_dir[pdi] = (pt_phys & PAGE_MASK) | (src->pg_dir[pdi] & ~PAGE_MASK);
    }

    // point last entry of the new dir to itself (recursively)
    new_dir[RECURSIVE_PDE] = (new_dir_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;

    // insert new region into linked list 
    region_insert(dst);
    return dst;
}

void vmm_nuke_pagemap(vmm_region_t *region) {
    for (int pdi = 0; pdi < 1024; pdi++) {
        if (!(region->pg_dir[pdi] & PAGE_PRESENT)) continue;

        uint32_t *pt = &pg_tbls[pdi * PAGE_ENTRIES];
        for (int pti = 0; pti < PAGE_ENTRIES; pti++) {
            if (pt[pti] & PAGE_PRESENT) {
                uintptr_t pa = pt[pti] & PAGE_MASK;
                pmm_free_page((void *)pa);
            }
        }

        uintptr_t pt_phys = region->pg_dir[pdi] & PAGE_MASK;
        pmm_free_page((void *)pt_phys);
    }

    uintptr_t dir_phys = (uintptr_t)region->pg_dir;
    pmm_free_page((void *)dir_phys);

    region_remove(region);
    kfree(region, sizeof(vmm_region_t));
}

vmm_region_t *vmm_find_region(uintptr_t va) {
    vmm_region_t *iter = region_list;
    while (iter) {
        if (vmm_resolve(iter, va)) return iter;
        iter = iter->next;
    }
    return 0;
}

size_t vmm_count_regions() {
    size_t n = 0;
    vmm_region_t *iter = region_list;
    while (iter) {
        n++;
        iter = iter->next;
    }
    return n;
}

int vmm_map_range(vmm_region_t *region, uintptr_t va, uintptr_t pa, size_t pages, uint32_t flags) {
    for (size_t i = 0; i < pages; i++) {
        if (vmm_map(region, va + i * PAGE_SIZE, pa + i * PAGE_SIZE, flags) < 0)
            return -1;
    }
    return 0;
}

int vmm_unmap_range(vmm_region_t *region, uintptr_t va, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        if (vmm_unmap(region, va + i * PAGE_SIZE) < 0)
            return -1;
    }
    return 0;
}

int vmm_protect(vmm_region_t *region, uintptr_t va, uint32_t flags) {
    uint32_t pdi = pd_index(va);
    uint32_t pti = pt_index(va);
    if (!(region->pg_dir[pdi] & PAGE_PRESENT)) return -1;
    uint32_t *pt = &pg_tbls[pdi * PAGE_ENTRIES];
    if (!(pt[pti] & PAGE_PRESENT)) return -1;
    pt[pti] = (pt[pti] & PAGE_MASK) | flags | PAGE_PRESENT;
    invlpg((void *)va);
    return 0;
}

uint32_t *vmm_get_pt(vmm_region_t *region, uintptr_t va) {
    uint32_t pdi = pd_index(va);
    if (!(region->pg_dir[pdi] & PAGE_PRESENT)) return 0;
    return &pg_tbls[pdi * PAGE_ENTRIES];
}

uint32_t vmm_get_pde(vmm_region_t *region, uintptr_t va) {
    return region->pg_dir[pd_index(va)];
}

uintptr_t vmm_find_free_range(vmm_region_t *region, size_t pages) {
    size_t run = 0;
    uintptr_t start = 0;

    for (uintptr_t va = 0; va < 0xFFFFFFFF; va += PAGE_SIZE) {
        uint32_t *pt = vmm_get_pt(region, va);
        uint32_t pde = vmm_get_pde(region, va);

        int used = 0;
        if (pde & PAGE_PRESENT) {
            if (pt && (pt[pt_index(va)] & PAGE_PRESENT)) {
                used = 1;
            }
        }

        if (!used) {
            if (run == 0) start = va;
            run++;
            if (run >= pages) {
                return start;
            }
        } else {
            run = 0;
        }
    }

    return 0; // no range found
}

int vmm_map_shared(vmm_region_t *a, vmm_region_t *b, uintptr_t va_a, uintptr_t va_b, uintptr_t pa, size_t pages, uint32_t flags) {
    for (size_t i = 0; i < pages; i++) {
        if (vmm_map(a, va_a + i * PAGE_SIZE, pa + i * PAGE_SIZE, flags) < 0) return -1;
        if (vmm_map(b, va_b + i * PAGE_SIZE, pa + i * PAGE_SIZE, flags) < 0) return -1;
    }
    return 0;
}

int vmm_identity_map(vmm_region_t *region, uintptr_t base, size_t pages, uint32_t flags) {
    return vmm_map_range(region, base, base, pages, flags);
}

int vmm_is_mapped(vmm_region_t *region, uintptr_t va) {
    return vmm_resolve(region, va) != 0;
}

size_t vmm_count_mapped(vmm_region_t *region) {
    size_t n = 0;
    for (uintptr_t va = 0; va < KERNEL_VIRT_BASE; va += PAGE_SIZE)
        if (vmm_is_mapped(region, va)) n++;
    return n;
}

static void vmm_aslr_init_region(vmm_region_t *region) {
    if (!region->random_table) {
        region->random_table = kmalloc(sizeof(aslr_entry_t) * 16);
        region->random_count = 0;
        region->random_capacity = 16;
    }
}

static void vmm_aslr_record(vmm_region_t *region, uintptr_t va, size_t pages, size_t align, uint32_t flags) {
    if (region->random_count >= region->random_capacity) {
        size_t new_cap = region->random_capacity * 2;
        region->random_table = krealloc(
            region->random_table,
            new_cap * sizeof(aslr_entry_t),
            region->random_capacity * sizeof(aslr_entry_t)
        );
        region->random_capacity = new_cap;
    }

    aslr_entry_t *entry = &region->random_table[region->random_count++];
    entry->va = va;
    entry->pages = pages;
    entry->align = align;
    entry->flags = flags;
}

static uint32_t _vmm_aslr_rng_state = 0x12345678;
static uint32_t rand32(void) {
    _vmm_aslr_rng_state = _vmm_aslr_rng_state * 1664525 + 1013904223;
    return _vmm_aslr_rng_state;
}

uintptr_t vmm_aslr_alloc(vmm_region_t *region, size_t pages, size_t align, uint32_t flags) {
    vmm_aslr_init_region(region);

    for (int attempt = 0; attempt < 1024; attempt++) {
        uintptr_t base = USER_SPACE_START + (rand32() % (USER_SPACE_END - USER_SPACE_START - pages * PAGE_SIZE));
        uintptr_t aligned_base = (base + (align - 1)) & ~(align - 1);

        int free = 1;
        for (size_t i = 0; i < pages; i++) {
            if (vmm_is_mapped(region, aligned_base + i * PAGE_SIZE)) {
                free = 0;
                break;
            }
        }

        if (free) {
            vmm_aslr_record(region, aligned_base, pages, align, flags);
            return aligned_base;
        }
    }

    return 0;
}

void vmm_aslr_free(vmm_region_t *region, uintptr_t va) {
    if (!region->random_table) return;

    for (size_t i = 0; i < region->random_count; i++) {
        aslr_entry_t *entry = &region->random_table[i];
        if (entry->va == va) {
            vmm_unmap_range(region, entry->va, entry->pages);
            region->random_table[i] = region->random_table[--region->random_count];
            return;
        }
    }
}

int vmm_map_direct(vmm_region_t *region, uintptr_t phys, size_t pages, uint32_t flags) {
    for (size_t i = 0; i < pages; i++) {
        if (vmm_map(region, phys + i * PAGE_SIZE, phys + i * PAGE_SIZE, flags) < 0)
            return -1;
    }
    return 0;
}

uintptr_t vmm_map_anonymous(vmm_region_t *region, size_t pages, uint32_t flags) {
    uintptr_t va = vmm_find_free_range(region, pages);
    if (!va) return 0;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = (uintptr_t)pmm_alloc_page();
        if (!pa) {
            for (size_t j = 0; j < i; j++)
                vmm_unmap(region, va + j * PAGE_SIZE);
            return 0;
        }
        if (vmm_map(region, va + i * PAGE_SIZE, pa, flags) < 0) {
            for (size_t j = 0; j <= i; j++)
                vmm_unmap(region, va + j * PAGE_SIZE);
            pmm_free_page((void *)pa);
            return 0;
        }
    }

    return va;
}

uintptr_t vmm_map_direct_aslr(vmm_region_t *region, uintptr_t phys, size_t pages, uint32_t flags, size_t align) {
    uintptr_t va = vmm_aslr_alloc(region, pages, align, flags);
    if (!va) return 0;

    for (size_t i = 0; i < pages; i++) {
        if (vmm_map(region, va + i * PAGE_SIZE, phys + i * PAGE_SIZE, flags) < 0) {
            for (size_t j = 0; j < i; j++)
                vmm_unmap(region, va + j * PAGE_SIZE);
            vmm_aslr_free(region, va);
            return 0;
        }
    }

    return va;
}

uintptr_t vmm_map_anonymous_aslr(vmm_region_t *region, size_t pages, uint32_t flags, size_t align) {
    uintptr_t va = vmm_aslr_alloc(region, pages, align, flags);
    if (!va) return 0;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = (uintptr_t)pmm_alloc_page();
        if (!pa) {
            for (size_t j = 0; j < i; j++)
                vmm_unmap(region, va + j * PAGE_SIZE);
            vmm_aslr_free(region, va);
            return 0;
        }
        if (vmm_map(region, va + i * PAGE_SIZE, pa, flags) < 0) {
            for (size_t j = 0; j <= i; j++)
                vmm_unmap(region, va + j * PAGE_SIZE);
            pmm_free_page((void *)pa);
            vmm_aslr_free(region, va);
            return 0;
        }
    }

    return va;
}

int vmm_unmap_direct_aslr(vmm_region_t *region, uintptr_t va, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = vmm_resolve(region, va + i * PAGE_SIZE);
        if (!pa) continue;
        vmm_unmap(region, va + i * PAGE_SIZE);
    }
    vmm_aslr_free(region, va);
    return 0;
}

int vmm_unmap_anonymous_aslr(vmm_region_t *region, uintptr_t va, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = vmm_resolve(region, va + i * PAGE_SIZE);
        if (!pa) continue;
        vmm_unmap(region, va + i * PAGE_SIZE);
        pmm_free_page((void *)pa);
    }
    vmm_aslr_free(region, va);
    return 0;
}

static void shuffle_array(uintptr_t *array, size_t n) {
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand32() % (i + 1);
        uintptr_t tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

flop_randframe_region_t *rand_frames_create(vmm_region_t *region) {
    size_t pages = vmm_count_mapped(region);
    if (!pages) return NULL;

    vmm_region_t *table_region = vmm_region_create();
    if (!table_region) return NULL;

    flop_randframe_region_t *rand_struct = (flop_randframe_region_t *)kmalloc(sizeof(flop_randframe_region_t));
    rand_struct->entries = (flop_rand_entry_t *)kmalloc(sizeof(flop_rand_entry_t) * pages);
    rand_struct->src_region = region;
    rand_struct->table_region = table_region;
    rand_struct->page_count = pages;

    uintptr_t *phys_pages = (uintptr_t *)kmalloc(sizeof(uintptr_t) * pages);
    size_t idx = 0;
    for (uintptr_t va = 0; va < 0xFFFFFFFF; va += PAGE_SIZE) {
        uintptr_t pa = vmm_resolve(region, va);
        if (!pa) continue;
        phys_pages[idx++] = pa;
        if (idx >= pages) break;
    }

    shuffle_array(phys_pages, pages);

    idx = 0;
    for (uintptr_t va = 0; va < 0xFFFFFFFF; va += PAGE_SIZE) {
        uintptr_t pa = vmm_resolve(region, va);
        if (!pa) continue;

        vmm_unmap(region, va);

        vmm_map(region, va, phys_pages[idx], PAGE_PRESENT | PAGE_RW);

        rand_struct->entries[idx].va = va;
        rand_struct->entries[idx].pa = phys_pages[idx];
        idx++;
    }

    kfree(phys_pages, sizeof(uintptr_t) * pages);
    return rand_struct;
}

void rand_frames_destroy(flop_randframe_region_t *rand_struct) {
    if (!rand_struct) return;

    for (size_t i = 0; i < rand_struct->page_count; i++) {
        vmm_unmap(rand_struct->src_region, rand_struct->entries[i].va);
        vmm_map(rand_struct->src_region, rand_struct->entries[i].va,
                rand_struct->entries[i].pa, PAGE_PRESENT | PAGE_RW);
    }

    vmm_region_destroy(rand_struct->table_region);
    kfree(rand_struct->entries, sizeof(flop_rand_entry_t) * rand_struct->page_count);
    kfree(rand_struct, sizeof(flop_randframe_region_t));
}

uintptr_t rand_frames_aslr_map_direct(flop_randframe_region_t *rand_struct, uintptr_t phys, size_t pages, uint32_t flags, size_t align) {
    uintptr_t va = vmm_aslr_alloc(rand_struct->src_region, pages, align, flags);
    if (!va) return 0;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = rand_struct->entries[i % rand_struct->page_count].pa;
        if (vmm_map(rand_struct->src_region, va + i * PAGE_SIZE, pa, flags) < 0) {
            for (size_t j = 0; j < i; j++)
                vmm_unmap(rand_struct->src_region, va + j * PAGE_SIZE);
            vmm_aslr_free(rand_struct->src_region, va);
            return 0;
        }
    }
    return va;
}

uintptr_t rand_frames_aslr_map_anonymous(flop_randframe_region_t *rand_struct, size_t pages, uint32_t flags, size_t align) {
    uintptr_t va = vmm_aslr_alloc(rand_struct->src_region, pages, align, flags);
    if (!va) return 0;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = (uintptr_t)pmm_alloc_page();
        if (!pa) {
            for (size_t j = 0; j < i; j++)
                vmm_unmap(rand_struct->src_region, va + j * PAGE_SIZE);
            vmm_aslr_free(rand_struct->src_region, va);
            return 0;
        }
        pa = rand_struct->entries[i % rand_struct->page_count].pa;

        if (vmm_map(rand_struct->src_region, va + i * PAGE_SIZE, pa, flags) < 0) {
            for (size_t j = 0; j <= i; j++)
                vmm_unmap(rand_struct->src_region, va + j * PAGE_SIZE);
            pmm_free_page((void *)pa);
            vmm_aslr_free(rand_struct->src_region, va);
            return 0;
        }
    }
    return va;
}

int rand_frames_aslr_unmap_direct(flop_randframe_region_t *rand_struct,
                                 uintptr_t va, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = vmm_resolve(rand_struct->src_region, va + i * PAGE_SIZE);
        if (!pa) continue;
        vmm_unmap(rand_struct->src_region, va + i * PAGE_SIZE);
    }
    vmm_aslr_free(rand_struct->src_region, va);
    return 0;
}

int rand_frames_aslr_unmap_anonymous(flop_randframe_region_t *rand_struct,
                                    uintptr_t va, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = vmm_resolve(rand_struct->src_region, va + i * PAGE_SIZE);
        if (!pa) continue;
        vmm_unmap(rand_struct->src_region, va + i * PAGE_SIZE);
        pmm_free_page((void *)pa);
    }
    vmm_aslr_free(rand_struct->src_region, va);
    return 0;
}

int rand_frames_reshuffle(flop_randframe_region_t *rand_struct) {
    if (!rand_struct || !rand_struct->entries || !rand_struct->page_count)
        return -1;

    size_t pages = rand_struct->page_count;

    uintptr_t *phys_pages = (uintptr_t *)kmalloc(sizeof(uintptr_t) * pages);
    if (!phys_pages)
        return -1;

    for (size_t i = 0; i < pages; i++)
        phys_pages[i] = rand_struct->entries[i].pa;

    
    shuffle_array(phys_pages, pages);

    for (size_t i = 0; i < pages; i++) {
        uintptr_t va = rand_struct->entries[i].va;

        vmm_unmap(rand_struct->src_region, va);

        if (vmm_map(rand_struct->src_region, va, phys_pages[i],
                    PAGE_PRESENT | PAGE_RW) < 0) {
            // rollback on failure
            for (size_t j = 0; j < i; j++) {
                vmm_unmap(rand_struct->src_region, rand_struct->entries[j].va);
                vmm_map(rand_struct->src_region, rand_struct->entries[j].va,
                        rand_struct->entries[j].pa, PAGE_PRESENT | PAGE_RW);
            }
            kfree(phys_pages, sizeof(uintptr_t) * pages);
            return -1;
        }

        rand_struct->entries[i].pa = phys_pages[i];
    }

    kfree(phys_pages, sizeof(uintptr_t) * pages);
    return 0;
}

int rand_frames_reshuffle_partial(flop_randframe_region_t *rand_struct,
                                  size_t start, size_t count) {
    if (!rand_struct || start >= rand_struct->page_count)
        return -1;
    if (start + count > rand_struct->page_count)
        count = rand_struct->page_count - start;

    uintptr_t *phys_pages = (uintptr_t *)kmalloc(sizeof(uintptr_t) * count);
    if (!phys_pages)
        return -1;

    for (size_t i = 0; i < count; i++)
        phys_pages[i] = rand_struct->entries[start + i].pa;

    shuffle_array(phys_pages, count);

    for (size_t i = 0; i < count; i++) {
        uintptr_t va = rand_struct->entries[start + i].va;
        vmm_unmap(rand_struct->src_region, va);

        if (vmm_map(rand_struct->src_region, va, phys_pages[i],
                    PAGE_PRESENT | PAGE_RW) < 0) {
            for (size_t j = 0; j < i; j++) {
                vmm_unmap(rand_struct->src_region,
                          rand_struct->entries[start + j].va);
                vmm_map(rand_struct->src_region,
                        rand_struct->entries[start + j].va,
                        rand_struct->entries[start + j].pa,
                        PAGE_PRESENT | PAGE_RW);
            }
            kfree(phys_pages, sizeof(uintptr_t) * count);
            return -1;
        }

        rand_struct->entries[start + i].pa = phys_pages[i];
    }

    kfree(phys_pages, sizeof(uintptr_t) * count);
    return 0;
}

flop_randframe_region_t *rand_frames_clone_exact(flop_randframe_region_t *src) {
    if (!src || !src->entries || !src->page_count)
        return NULL;

    vmm_region_t *table_region = vmm_region_create();
    if (!table_region)
        return NULL;

    flop_randframe_region_t *clone =
        (flop_randframe_region_t *)kmalloc(sizeof(flop_randframe_region_t));
    if (!clone)
        return NULL;

    clone->entries = (flop_rand_entry_t *)
        kmalloc(sizeof(flop_rand_entry_t) * src->page_count);
    if (!clone->entries) {
        kfree(clone, sizeof(flop_randframe_region_t));
        return NULL;
    }

    clone->src_region = src->src_region;
    clone->table_region = table_region;
    clone->page_count = src->page_count;

    for (size_t i = 0; i < src->page_count; i++) {
        clone->entries[i].va = src->entries[i].va;
        clone->entries[i].pa = src->entries[i].pa;
    }

    return clone;
}

flop_randframe_region_t *rand_frames_clone_randomized(flop_randframe_region_t *src) {
    if (!src || !src->entries || !src->page_count)
        return NULL;

    vmm_region_t *table_region = vmm_region_create();
    if (!table_region)
        return NULL;

    flop_randframe_region_t *clone =
        (flop_randframe_region_t *)kmalloc(sizeof(flop_randframe_region_t));
    if (!clone)
        return NULL;

    clone->entries = (flop_rand_entry_t *)
        kmalloc(sizeof(flop_rand_entry_t) * src->page_count);
    if (!clone->entries) {
        kfree(clone, sizeof(flop_randframe_region_t));
        return NULL;
    }
    clone->src_region = src->src_region;
    clone->table_region = table_region;
    clone->page_count = src->page_count;
    for (size_t i = 0; i < src->page_count; i++)
        clone->entries[i].va = src->entries[i].va;
    uintptr_t *phys_pages =
        (uintptr_t *)kmalloc(sizeof(uintptr_t) * src->page_count);
    if (!phys_pages) {
        kfree(clone->entries, sizeof(flop_rand_entry_t) * src->page_count);
        kfree(clone, sizeof(flop_randframe_region_t));
        return NULL;
    }

    for (size_t i = 0; i < src->page_count; i++)
        phys_pages[i] = src->entries[i].pa;

    shuffle_array(phys_pages, src->page_count);

    for (size_t i = 0; i < src->page_count; i++) {
        clone->entries[i].pa = phys_pages[i];
        vmm_unmap(clone->src_region, clone->entries[i].va);
        vmm_map(clone->src_region,
                clone->entries[i].va,
                phys_pages[i],
                PAGE_PRESENT | PAGE_RW);
    }

    kfree(phys_pages, sizeof(uintptr_t) * src->page_count);
    return clone;
}

uintptr_t rand_frames_clone_aslr_map_direct(flop_randframe_region_t *clone, uintptr_t phys, size_t pages, uint32_t flags, size_t align) {
    if (!clone || !clone->entries) return 0;

    uintptr_t va = vmm_aslr_alloc(clone->src_region, pages, align, flags);
    if (!va) return 0;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = clone->entries[i % clone->page_count].pa;
        if (vmm_map(clone->src_region, va + i * PAGE_SIZE, pa, flags) < 0) {
            for (size_t j = 0; j < i; j++) {
                vmm_unmap(clone->src_region, va + j * PAGE_SIZE);
            }
            vmm_aslr_free(clone->src_region, va);
            return 0;
        }
    }
    return va;
}

uintptr_t rand_frames_clone_aslr_map_anonymous(flop_randframe_region_t *clone, size_t pages, uint32_t flags, size_t align) {
    if (!clone || !clone->entries) return 0;

    uintptr_t va = vmm_aslr_alloc(clone->src_region, pages, align, flags);
    if (!va) return 0;

    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = (uintptr_t)pmm_alloc_page();
        if (!pa) {
            for (size_t j = 0; j < i; j++)
                vmm_unmap(clone->src_region, va + j * PAGE_SIZE);
            vmm_aslr_free(clone->src_region, va);
            return 0;
        }

        // replace with a randomized PA from the clone table
        pa = clone->entries[i % clone->page_count].pa;

        if (vmm_map(clone->src_region, va + i * PAGE_SIZE, pa, flags) < 0) {
            for (size_t j = 0; j <= i; j++)
                vmm_unmap(clone->src_region, va + j * PAGE_SIZE);
            pmm_free_page((void *)pa);
            vmm_aslr_free(clone->src_region, va);
            return 0;
        }
    }
    return va;
}

int rand_frames_clone_aslr_unmap_direct(flop_randframe_region_t *clone, uintptr_t va, size_t pages) {
    if (!clone) return -1;
    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = vmm_resolve(clone->src_region, va + i * PAGE_SIZE);
        if (!pa) continue;
        vmm_unmap(clone->src_region, va + i * PAGE_SIZE);
    }
    vmm_aslr_free(clone->src_region, va);
    return 0;
}

int rand_frames_clone_aslr_unmap_anonymous(flop_randframe_region_t *clone,
                                           uintptr_t va, size_t pages) {
    if (!clone) return -1;
    for (size_t i = 0; i < pages; i++) {
        uintptr_t pa = vmm_resolve(clone->src_region, va + i * PAGE_SIZE);
        if (!pa) continue;
        vmm_unmap(clone->src_region, va + i * PAGE_SIZE);
        pmm_free_page((void *)pa);
    }
    vmm_aslr_free(clone->src_region, va);
    return 0;
}
