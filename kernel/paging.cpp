#include "paging.hpp"
#include "memory.hpp"
#include "print.hpp"

uint32_t *kernel_page_dir = nullptr;

static uint32_t *alloc_page_table() {
    // Allocate 2 pages so we can align within the allocation
    uintptr_t addr = kcalloc(PAGE_SIZE * 2);
    if (!addr) return nullptr;
    uintptr_t aligned_addr = (addr + PAGE_SIZE - 1) & ~(uintptr_t)(PAGE_SIZE - 1);
    // kcalloc zeroed the whole region, so aligned portion is already zero
    kprintf("alloc_page_table: got %x\n", (uint32_t)addr);
    return (uint32_t *)aligned_addr;
}

void identity_map(uint32_t *page_dir, uint32_t vaddr_start, uint32_t vaddr_end) {
    vaddr_start = PAGE_ALIGN(vaddr_start);

    for (uint32_t va = vaddr_start; va < vaddr_end; va += PAGE_SIZE) {
        uint32_t pdi = PD_INDEX(va);
        uint32_t pti = PT_INDEX(va);

        if (!(page_dir[pdi] & PAGE_PRESENT)) {
            uint32_t *pt = alloc_page_table();
            if (!pt) {
                kprintf("identity_map: OOM at va=%x\n", va);
                return;
            }
            page_dir[pdi] = ((uint32_t)pt) | PAGE_PRESENT | PAGE_RW;
        }

        uint32_t *pt = (uint32_t *)(page_dir[pdi] & ~0xFFF);
        pt[pti] = va | PAGE_PRESENT | PAGE_RW;
    }
}

void init_paging() {
    kprintf("init_paging: allocating page dir\n");
    kernel_page_dir = alloc_page_table();
    if (!kernel_page_dir) {
        kprintf("init_paging: failed to alloc page dir\n");
        return;
    }
    kprintf("init_paging: page dir at %x\n", (uint32_t)kernel_page_dir);

    // Identity map first 8MB (be generous — covers kernel + pool)
    identity_map(kernel_page_dir, 0x0, 0x800000);

    kprintf("init_paging: identity map done, loading CR3\n");

    asm volatile (
        "mov %0, %%cr3\n\t"
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        : : "r"(kernel_page_dir) : "eax"
    );

    kprintf("Paging enabled!\n");
}