#include "paging.hpp"
#include "memory.hpp"
#include "print.hpp"
#include "kernel.hpp"

uint32_t *kernel_page_directory = nullptr;   // matches paging.hpp

static uint32_t __attribute__((aligned(4096))) page_dir_storage[1024];
static uint32_t __attribute__((aligned(4096))) page_table_storage[8][1024];
static uint32_t __attribute__((aligned(4096))) page_dirs[THREAD_LIMIT][1024]; // one pool, bitmap-tracked
static bool pd_used[THREAD_LIMIT] = {};
static int pt_count = 0;

static uint32_t *alloc_page_table() {
    if (pt_count >= 8) {
        kprintf("alloc_page_table: out of static page tables\n");
        return nullptr;
    }
    return page_table_storage[pt_count++];
}

uint32_t *alloc_page_directory() {
    for (int i = 0; i < THREAD_LIMIT; i++) {
        if (!pd_used[i]) {
            pd_used[i] = true;
            uint32_t *pd = page_dirs[i];
            for (int j = 0; j < 1024; j++) pd[j] = kernel_page_directory[j];
            return pd;
        }
    }
    kprintf("alloc_page_directory: out of page directories\n");
    return nullptr;
}

void free_page_directory(uint32_t *pd) {
    for (int i = 0; i < THREAD_LIMIT; i++) {
        if (page_dirs[i] == pd) { pd_used[i] = false; return; }
    }
    kprintf("free_page_directory: unknown pd %x\n", (uint32_t)pd);
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
    kernel_page_directory = page_dir_storage;
    identity_map(kernel_page_directory, 0x0, 0x800000);
    asm volatile (
        "mov %0, %%cr3\n\t"
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        : : "r"(kernel_page_directory) : "eax"
    );

    kprintf("Paging enabled!\n");
}