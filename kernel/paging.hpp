#pragma once
#include <minstd/types.hpp>

#define PAGE_SIZE        0x1000
#define PAGE_PRESENT     (1 << 0)
#define PAGE_RW          (1 << 1)
#define PAGE_USER        (1 << 2)

#define PD_INDEX(va)    (((va) >> 22) & 0x3FF)
#define PT_INDEX(va)    (((va) >> 12) & 0x3FF)
#define PAGE_ALIGN(a)   ((a) & ~0xFFF)

// Kernel page directory - set once in init_paging, used by all kernel threads
extern uint32_t *kernel_page_directory;

void init_paging();
void identity_map(uint32_t *page_directory, uint32_t vaddr_start, uint32_t vaddr_end);
uint32_t *alloc_page_directory();