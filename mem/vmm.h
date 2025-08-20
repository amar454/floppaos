#ifndef VMM_H
#define VMM_H

#include <stdint.h>

int vmm_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags);
int vmm_unmap_page(uint32_t vaddr);
uint32_t vmm_resolve(uint32_t vaddr);
static inline uint32_t pde_index(uint32_t v){ return (v >> 22) & 0x3FF; }
static inline uint32_t pte_index(uint32_t v){ return (v >> 12) & 0x3FF; }
static inline void invlpg(void* a){ __asm__ volatile("invlpg (%0)"::"r"(a):"memory"); }

static inline uint32_t* virt_pd(void){ return (uint32_t*)0xFFFFF000; }
static inline uint32_t* virt_pt(uint32_t di){ return (uint32_t*)(0xFFC00000 + (di << 12)); }

void vmm_init();
#endif
