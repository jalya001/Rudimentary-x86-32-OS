#include "paging.hpp"
#include "memory.hpp"
#include "print.hpp"
#include "kernel.hpp"

uint32_t *kernel_page_dir = nullptr;

// Static page table storage - outside kpool, page-aligned
static uint32_t __attribute__((aligned(4096))) page_dir_storage[1024];
static uint32_t __attribute__((aligned(4096))) page_table_storage[8][1024]; // 8 page tables = 32MB mappable
static uint32_t __attribute__((aligned(4096))) thread_page_dirs[THREAD_LIMIT][1024]; 
static int pt_count = 0; 

static uint32_t *alloc_page_table() {
    if (pt_count >= 8) {
        kprintf("alloc_page_table: out of static page tables\n");
        return nullptr;
    }
    uint32_t *pt = page_table_storage[pt_count++];
    // already zeroed (BSS)
    return pt;
}

uint32_t *alloc_page_directory() {
    // Find a free slot - for now use a simple counter
    // (A bitmap would be cleaner but this works for THREAD_LIMIT=12)
    static int next_dir = 0;
    if (next_dir >= THREAD_LIMIT) return nullptr;
    uint32_t *dir = thread_page_dirs[next_dir++];
    // Copy kernel mappings from kernel_page_dir
    for (int i = 0; i < 1024; i++) dir[i] = kernel_page_dir[i];
    return dir;
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
    kernel_page_dir = page_dir_storage;
    kprintf("init_paging: page dir at %x\n", (uint32_t)kernel_page_dir);
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