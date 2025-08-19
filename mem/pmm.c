/*

Copyright 2024, 2025 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.

*/

#include "pmm.h"
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"

#include <stdint.h>

struct buddy_allocator_t buddy;

// split a block into 2 blocks of half the size
void buddy_split(uintptr_t addr, uint32_t order) {
    // fetch addr of block to split (offset by half the current block size)
    uintptr_t buddy_addr = addr + (1 << (order - 1)) * PAGE_SIZE;
    
    // fetch page info structs for both blocks
    struct Page* page = phys_to_page_index(addr);
    struct Page* buddy_page = phys_to_page_index(buddy_addr);
    

    if (page && buddy_page) {
        page->order = order - 1;
        buddy_page->order = order - 1;
        
        // Add first block to free list of lower order
        page->next = buddy.free_list[order - 1];
        buddy.free_list[order - 1] = page;
    }
}

// merge a block with its buddy if possible
// recursively merges blocks until a block cannot be merged
void buddy_merge(uintptr_t addr, uint32_t order) {
    uintptr_t buddy_addr = addr ^ ((1 << order) * PAGE_SIZE);
    
    struct Page* page = phys_to_page_index(addr);
    struct Page* buddy_page = phys_to_page_index(buddy_addr);

    // check if buddy can be merged (must be free and same order)
    if (buddy_page && buddy_page->is_free && buddy_page->order == order) {
        // remove buddy from its free list
        struct Page** prev = &buddy.free_list[order];
        while (*prev && *prev != buddy_page) {
            prev = &(*prev)->next;
        }

        if (*prev) {
            *prev = buddy_page->next;
        }
        
        // fetch lower address of the two blocks
        uintptr_t merged_addr = (addr < buddy_addr) ? addr : buddy_addr;
        
        // recursively try to merge the larger block
        buddy_merge(merged_addr, order + 1);
    } else {
        // if can't merge, add to free list of current order
        page->next = buddy.free_list[order];
        buddy.free_list[order] = page;
    }
}
static void _add_page_to_free_list(struct Page* page, uintptr_t address, uint32_t order) {
    page->address = address;
    page->order = order;
    page->is_free = 1;
    page->next = buddy.free_list[order];
    buddy.free_list[order] = page;
}

static void _create_free_list(void) {
    for (uint32_t i = 0; i < buddy.total_pages; i++) {
        struct Page* page = &buddy.page_info[i];
        _add_page_to_free_list(page, buddy.memory_start + (i * PAGE_SIZE), MAX_ORDER);
    }
}

static void buddy_init(uint64_t total_memory, uintptr_t memory_start) {
    buddy.total_pages = total_memory / PAGE_SIZE;
    buddy.memory_start = memory_start;
    buddy.memory_end = buddy.memory_start + buddy.total_pages * PAGE_SIZE;

    buddy.page_info = (struct Page*)buddy.memory_start;
    buddy.memory_start += buddy.total_pages * sizeof(struct Page);

    _create_free_list();
}

static uint64_t iterate_mb_mmap(multiboot_info_t* mb_info) {
    uint8_t* mmap_ptr = (uint8_t*)mb_info->mmap_addr;
    uint8_t* mmap_end = mmap_ptr + mb_info->mmap_length;

    uint64_t total_memory = 0;

    while (mmap_ptr < mmap_end) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mmap_ptr;
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            total_memory += mmap->len;
        }
        mmap_ptr += mmap->size + sizeof(mmap->size);
    }

    return total_memory;
}

void pmm_init(multiboot_info_t* mb_info) {
    if (!mb_info || !(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        log("pmm: Invalid Multiboot info!\n", RED);
        return;
    }

    uint64_t total_memory = iterate_mb_mmap(mb_info);

    buddy_init(total_memory, (uintptr_t)mb_info->mmap_addr);

    static spinlock_t buddy_lock_initializer = SPINLOCK_INIT;
    buddy.lock = buddy_lock_initializer;
    spinlock_init(&buddy.lock);

    log("pmm_init(): Buddy allocator initialized\n", GREEN);
    log_uint("pmm: total pages: ", buddy.total_pages);
    log_uint("pmm: total memory size: ", total_memory);
    log_address("pmm: memory start: ", buddy.memory_start);
    log_address("pmm: memory end: ", buddy.memory_end);
    log_uint("pmm: memory available kb: ", total_memory / 1024);
    log_uint("pmm: memory available mb: ", total_memory / 1024 / 1024);
}

void pmm_copy_page(void* dst, void* src) {
    spinlock(&buddy.lock);
    for (int i = 0; i < PAGE_SIZE / sizeof(uint32_t); i++) {
        ((uint32_t*)dst)[i] = ((uint32_t*)src)[i];
    }
    spinlock_unlock(&buddy.lock, true);
}

void* pmm_alloc_pages(uint32_t order, uint32_t count) {

    spinlock(&buddy.lock);

    if (order > MAX_ORDER || count == 0) return NULL;

    void* first_page = NULL;

    for (uint32_t i = 0; i < count; i++) {
        void* page = NULL;
        for (uint32_t j = order; j <= MAX_ORDER; j++) {
            if (buddy.free_list[j]) {
                struct Page* p = buddy.free_list[j];
                buddy.free_list[j] = p->next;
                p->is_free = 0;
                p->order = order;
                while (j > order) {
                    j--;
                    buddy_split(p->address, j);
                }
                page = (void*)p->address;
                break;
            }
        }
        if (!page) {
            if (first_page) {
                pmm_free_pages(first_page, order, i);
            }
            log("pmm: Out of memory!\n", RED);
            return NULL;
        }
        if (!first_page) {
            first_page = page;
        }
    }
    spinlock_unlock(&buddy.lock, true);
    return first_page;
}

void pmm_free_pages(void* addr, uint32_t order, uint32_t count) {
    spinlock(&buddy.lock);
    if (!addr || order > MAX_ORDER || count == 0) return;

    uintptr_t current_addr = (uintptr_t)addr;
    for (uint32_t i = 0; i < count; i++) {
        struct Page* page = phys_to_page_index(current_addr);
        if (!page) return;
        page->is_free = 1;
        buddy_merge(page->address, order);
        current_addr += (1 << order) * PAGE_SIZE;
    }
    spinlock_unlock(&buddy.lock, true);
}
int pmm_is_valid_addr(uintptr_t addr) {
    if (addr % PAGE_SIZE != 0) return 0;

    if (addr < buddy.memory_start || addr >= buddy.memory_end) return 0;

    uint32_t index = page_index(addr);
    if (index >= buddy.total_pages) return 0;

    return 1;
}

void* pmm_alloc_page(void) {
    return pmm_alloc_pages(0, 1);
}

void pmm_free_page(void* addr) {
    pmm_free_pages(addr, 0, 1);
}

uint32_t pmm_get_memory_size(void) {
    return buddy.total_pages * PAGE_SIZE;
}

uint32_t pmm_get_page_count() {
    return buddy.total_pages;
}
uint32_t pmm_get_free_memory_size(void) {
    int free_pages = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        struct Page* page = buddy.free_list[i];
        while (page) {
            free_pages++;
            page = page->next;
        }
    }
    return free_pages * PAGE_SIZE;
}

struct Page* pmm_get_last_used_page(void) {
    for (int page_index = buddy.total_pages - 1; page_index >= 0; page_index--) {
        struct Page* page = &buddy.page_info[page_index];
        if (!page->is_free) return page;
    }
    return NULL;
}

uintptr_t page_to_phys_addr(struct Page* page) {
    return page->address;
}

uint32_t page_index(uintptr_t addr) {
    return (addr - buddy.memory_start) / PAGE_SIZE;
}

struct Page* phys_to_page_index(uintptr_t addr) {
    uint32_t index = page_index(addr);
    if (index >= buddy.total_pages) return NULL;
    return &buddy.page_info[index];
}

typedef struct page_cache_entry {
    uintptr_t phys;
    uint64_t idx;
    struct page_cache_entry *prev_lru;
    struct page_cache_entry *next_lru;
    bool dirty;
    uint32_t refcount;
} page_cache_entry_t;

typedef struct {
    uint8_t *keys;
    void   **vals;
    uint8_t *state;  /* 0=empty,1=used,2=deleted */
    size_t   cap;
    size_t   len;
} child_map_t;

typedef struct radix_node {
    page_cache_entry_t *entry;
    child_map_t map;
} radix_node_t;

typedef struct {
    radix_node_t *root;
} radix_tree_t;

static size_t cm_next_pow2(size_t x) {
    if (x <= 4) return 4;
    x--;
    for (size_t shift = 1; shift < sizeof(size_t) * 8; shift <<= 1) {
        x |= x >> shift;
    }
    return x + 1;
}
static uint32_t cm_hash(uint8_t k){
    uint32_t x = k;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x ? x : 1;
}
static int cm_init(child_map_t *m, size_t cap) {
    size_t c = cm_next_pow2(cap);
    m->keys = (uint8_t *) kmalloc(c);
    m->vals = (void **) kmalloc(sizeof(void *) * c);
    m->state = (uint8_t *) kmalloc(c);
    if (!m->keys || !m->vals || !m->state) {
        if (m->keys) {
            kfree(m->keys);
        }
        if (m->vals) {
            kfree(m->vals);
        }
        if (m->state) {
            kfree(m->state);
        }
        return -1;
    }
    memset(m->state, 0, c);
    m->cap = c;
    m->len = 0;
    return 0;
}

static void cm_free(child_map_t *m) {
    if (!m) return;
    if (m->keys) {
        kfree(m->keys);
    }
    if (m->vals) {
        kfree(m->vals);
    }
    if (m->state) {
        kfree(m->state);
    }
    m->keys = NULL;
    m->vals = NULL;
    m->state = NULL;
    m->cap = 0;
    m->len = 0;
}

static int cm_resize(child_map_t *m, size_t newcap) {
    child_map_t n;
    if (cm_init(&n, newcap)) {
        return -1;
    }
    for (size_t i = 0; i < m->cap; i++) {
        if (m->state[i] == 1) {
            uint8_t k = m->keys[i];
            void *v = m->vals[i];
            size_t mask = n.cap - 1;
            size_t pos = cm_hash(k) & mask;
            while (n.state[pos] == 1) {
                pos = (pos + 1) & mask;
            }
            n.state[pos] = 1;
            n.keys[pos] = k;
            n.vals[pos] = v;
            n.len++;
        }
    }
    cm_free(m);
    *m = n;
    return 0;
}

static void **cm_get_ref(child_map_t *m, uint8_t key) {
    if (!m->cap) {
        if (cm_init(m, 4)) return NULL;
    }
    size_t mask = m->cap - 1;
    size_t pos = cm_hash(key) & mask;
    size_t first_del = (size_t) -1;
    for (;;) {
        uint8_t st = m->state[pos];
        if (st == 0) {
            size_t use = (first_del != (size_t) -1) ? first_del : pos;
            m->state[use] = 1;
            m->keys[use] = key;
            m->vals[use] = NULL;
            m->len++;
            if (m->len * 10 > m->cap * 7) {
                if (cm_resize(m, m->cap << 1)) {
                    return NULL;
                }
                return cm_get_ref(m, key);
            }
            return &m->vals[use];
        } else if (st == 2) {
            if (first_del == (size_t) -1) {
                first_del = pos;
            }
        } else {
            if (m->keys[pos] == key) {
                return &m->vals[pos];
            }
        }
        pos = (pos + 1) & mask;
    }
}

static void *cm_find(child_map_t *m, uint8_t key) {
    if (!m->cap) return NULL;
    size_t mask = m->cap - 1;
    size_t pos = cm_hash(key) & mask;
    for (;;) {
        uint8_t st = m->state[pos];
        if (st == 0)
            return NULL;
        if (st == 1 && m->keys[pos] == key)
            return m->vals[pos];
        pos = (pos + 1) & mask;
    }
}

static int cm_del(child_map_t *m, uint8_t key) {
    if (!m->cap) return -1;
    size_t mask = m->cap - 1;
    size_t pos = cm_hash(key) & mask;
    for (;;) {
        uint8_t st = m->state[pos];
        if (st == 0) return -1;
        if (st == 1 && m->keys[pos] == key) {
            m->state[pos] = 2;
            m->vals[pos] = NULL;
            m->len--;
            return 0;
        }
        pos = (pos + 1) & mask;
    }
}

static radix_node_t *rt_new_node(void) {
    radix_node_t *n = (radix_node_t *) kmalloc(sizeof(radix_node_t));
    if (!n) return NULL;
    n->entry = NULL;
    n->map.cap = 0;
    n->map.len = 0;
    n->map.keys = NULL;
    n->map.vals = NULL;
    n->map.state = NULL;
    return n;
}

static void rt_free_node_recursive(radix_node_t *n) {
    if (!n) return;
    if (n->map.cap) {
        for (size_t i = 0; i < n->map.cap; i++) {
            if (n->map.state[i] == 1) {
                radix_node_t *child = (radix_node_t *) n->map.vals[i];
                rt_free_node_recursive(child);
            }
        }
    }
    if (n->entry) {
        pmm_free_page((void *) n->entry->phys);
        kfree(n->entry);
    }
    cm_free(&n->map);
    kfree(n);
}

static inline uint8_t rt_key_part(uint64_t k, int level) {
    return (uint8_t) ((k >> (level * 8)) & 0xFF);
}

static int radix_init(radix_tree_t **out) {
    if (!out) return -1;
    radix_tree_t *t = (radix_tree_t *) kmalloc(sizeof(radix_tree_t));
    if (!t) return -1;
    t->root = NULL;
    *out = t;
    return 0;
}

static void radix_free(radix_tree_t *t) {
    if (!t) return;
    if (t->root) rt_free_node_recursive(t->root);
    kfree(t);
}

static page_cache_entry_t *radix_get_entry(radix_tree_t *t, uint64_t key) {
    if (!t || !t->root) return NULL;
    radix_node_t *n = t->root;
    for (int level = 7; level > 0; --level) {
        void *child = cm_find(&n->map, rt_key_part(key, level));
        if (!child) return NULL;
        n = (radix_node_t *) child;
    }
    return n->entry;
}

static int radix_set_entry(radix_tree_t *t, uint64_t key, page_cache_entry_t *entry) {
    if (!t) return -1;
    if (!t->root) {
        t->root = rt_new_node();
        if (!t->root) return -1;
    }
    radix_node_t *n = t->root;
    for (int level = 7; level > 0; --level) {
        uint8_t part = rt_key_part(key, level);
        void **slot = cm_get_ref(&n->map, part);
        if (!slot) return -1;
        if (!*slot) {
            radix_node_t *nn = rt_new_node();
            if (!nn) return -1;
            *slot = nn;
        }
        n = (radix_node_t *) (*slot);
    }
    if (n->entry) {
        pmm_free_page((void *) n->entry->phys);
        kfree(n->entry);
    }
    n->entry = entry;
    return 0;
}

static void radix_del_entry(radix_tree_t *t, uint64_t key) {
    if (!t || !t->root) {
        return;
    }
    radix_node_t *stack[9];
    uint8_t part_stack[9];
    radix_node_t *n = t->root;
    int depth = 0;
    stack[depth] = n;
    for (int level = 7; level > 0; --level) {
        uint8_t part = rt_key_part(key, level);
        void *child = cm_find(&n->map, part);

        if (!child) {
            return;
        }

        n = (radix_node_t *) child;
        stack[++depth] = n;
        part_stack[depth] = part;
    }
    if (!n->entry) return;
    pmm_free_page((void *) n->entry->phys);
    kfree(n->entry);
    n->entry = NULL;
    for (int i = depth; i > 0; --i) {
        radix_node_t *cur = stack[i];
        if (cur->entry) {
            break;
        }

        if (cur->map.len == 0) {
            radix_node_t *parent = stack[i - 1];
            cm_del(&parent->map, part_stack[i]);
            cm_free(&cur->map);
            kfree(cur);
        } else break;
    }
    if (t->root && t->root->entry == NULL && t->root->map.len == 0) {
        cm_free(&t->root->map);
        kfree(t->root);
        t->root = NULL;
    }
}

typedef struct {
    radix_tree_t *tree;
    page_cache_entry_t *lru_head;
    page_cache_entry_t *lru_tail;
    spinlock_t lock;
    uint64_t page_count;
} page_cache_t;

static page_cache_t page_cache;

void page_cache_init(void) {
    page_cache.tree = NULL;
    radix_init(&page_cache.tree);
    page_cache.lru_head = page_cache.lru_tail = NULL;
    page_cache.page_count = 0;
    static spinlock_t initializer = SPINLOCK_INIT;
    page_cache.lock = initializer;
    spinlock_init(&page_cache.lock);
}

static void _lru_remove(page_cache_entry_t *e) {
    if (!e) return;
    if (e->prev_lru) e->prev_lru->next_lru = e->next_lru;
    if (e->next_lru) e->next_lru->prev_lru = e->prev_lru;
    if (page_cache.lru_head == e) page_cache.lru_head = e->next_lru;
    if (page_cache.lru_tail == e) page_cache.lru_tail = e->prev_lru;
    e->prev_lru = e->next_lru = NULL;
}

static void _lru_add_head(page_cache_entry_t *e) {
    e->prev_lru = NULL;
    e->next_lru = page_cache.lru_head;
    if (page_cache.lru_head) page_cache.lru_head->prev_lru = e;
    page_cache.lru_head = e;
    if (!page_cache.lru_tail) page_cache.lru_tail = e;
}

void *page_cache_get(uint64_t idx) {
    spinlock(&page_cache.lock);
    page_cache_entry_t *e = radix_get_entry(page_cache.tree, idx);
    if (e) {
        e->refcount++;
        _lru_remove(e);
        _lru_add_head(e);
        spinlock_unlock(&page_cache.lock, true);
        return (void *) e->phys;
    }
    void *page = pmm_alloc_page();
    if (!page) {
        spinlock_unlock(&page_cache.lock, true);
        return NULL;
    }
    page_cache_entry_t *entry = (page_cache_entry_t *) kmalloc(sizeof(page_cache_entry_t));
    entry->phys = (uintptr_t) page;
    entry->idx = idx;
    entry->prev_lru = entry->next_lru = NULL;
    entry->dirty = false;
    entry->refcount = 1;
    if (radix_set_entry(page_cache.tree, idx, entry) < 0) {
        pmm_free_page(page);
        kfree(entry);
        spinlock_unlock(&page_cache.lock, true);
        return NULL;
    }
    _lru_add_head(entry);
    page_cache.page_count++;
    spinlock_unlock(&page_cache.lock, true);
    return page;
}

void page_cache_mark_dirty(uint64_t idx) {
    spinlock(&page_cache.lock);
    page_cache_entry_t *e = radix_get_entry(page_cache.tree, idx);
    if (e) e->dirty = true;
    spinlock_unlock(&page_cache.lock, true);
}

void page_cache_release(uint64_t idx) {
    spinlock(&page_cache.lock);
    page_cache_entry_t *e = radix_get_entry(page_cache.tree, idx);
    if (e) {
        if (e->refcount > 0) e->refcount--;
    }
    spinlock_unlock(&page_cache.lock, true);
}

/* evict one least-recently-used page if any. */
int page_cache_evict_one(void) {
    spinlock(&page_cache.lock);
    page_cache_entry_t *victim = page_cache.lru_tail;
    if (!victim) {
        spinlock_unlock(&page_cache.lock, true);
        return 0;
    }
    if (victim->refcount > 0) {
        spinlock_unlock(&page_cache.lock, true);
        return 0;
    }
    _lru_remove(victim);
    radix_del_entry(page_cache.tree, victim->idx);
    page_cache.page_count--;
    spinlock_unlock(&page_cache.lock, true);
    return 1;
}

void page_cache_remove(uint64_t idx) {
    spinlock(&page_cache.lock);
    page_cache_entry_t *e = radix_get_entry(page_cache.tree, idx);
    if (!e) {
        spinlock_unlock(&page_cache.lock, true);
        return;
    }
    if (e->refcount > 0) {
        spinlock_unlock(&page_cache.lock, true);
        return;
    }
    _lru_remove(e);
    radix_del_entry(page_cache.tree, idx);
    page_cache.page_count--;
    spinlock_unlock(&page_cache.lock, true);
}

void page_cache_free_all(void) {
    spinlock(&page_cache.lock);
    if (page_cache.tree) {
        radix_free(page_cache.tree);
        page_cache.tree = NULL;
    }
    page_cache.lru_head = page_cache.lru_tail = NULL;
    page_cache.page_count = 0;
    spinlock_unlock(&page_cache.lock, true);
}

void log_page_info(struct Page *page) {
    log_address("pmm: Page address: ", page->address);
    log_uint("pmm: Page order: ", page->order);
    log_uint("pmm: Page is_free: ", page->is_free);
    log_address("pmm: Page next: ", (uintptr_t) page->next);
}

void print_mem_info(void) {
    log("Memory Info:\n", LIGHT_GRAY);
    log("Total pages: ", LIGHT_GRAY);
    log_uint("", buddy.total_pages);
    log("\nFree pages: ", LIGHT_GRAY);
    for (int i = 0; i <= MAX_ORDER; i++) {
        log_uint("", (uint32_t) buddy.free_list[i]);
        log(" ", LIGHT_GRAY);
    }
    log("\n", LIGHT_GRAY);
}
