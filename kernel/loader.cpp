#include "loader.hpp"
#include "drivers/ata.hpp"
#include "paging.hpp"
#include "kernel.hpp"
#include <minstd/string.hpp>
#include "print.hpp"

// Must match the layout written by tools/createimage.cpp
struct ProgramDirEntry {
    char     name[16];
    uint32_t lba;
    uint32_t sectors;
    uint32_t entry;     // e_entry: byte offset from the start of the stripped binary
    uint8_t  _pad[4];
};

#define SECTOR_SIZE      512
#define PROG_DIR_LOC     6           // byte offset in bootblock where prog_dir_lba is stored
#define PROG_LOAD_BASE   0x08048000  // virtual address where user programs are mapped
#define SECTORS_PER_PAGE (PAGE_SIZE / SECTOR_SIZE)  // 8

static uint32_t read_prog_dir_lba() {
    static uint8_t sector[SECTOR_SIZE];
    if (ata_read_sector(0, sector) < 0) return 0;
    uint32_t lba;
    buffer_copy((const char *)(sector + PROG_DIR_LOC), (char *)&lba, sizeof(lba));
    return lba;
}

int load_program(const char *name) {
    // Copy name to kernel buffer - caller may be in user space
    char kname[16];
    int i = 0;
    while (i < 15 && name[i]) { 
        kname[i] = name[i];
        i++;
    }
    kname[i] = '\0';
    name = kname; // use kernel buffer from here on
    
    // 1. Read the program directory sector
    uint32_t dir_lba = read_prog_dir_lba();
    if (dir_lba == 0) {
        kprintf("loader: could not read prog_dir_lba from bootblock\n");
        return -1;
    }

    static uint8_t dir_buf[SECTOR_SIZE];
    if (ata_read_sector(dir_lba, dir_buf) < 0) {
        kprintf("loader: could not read program directory at lba %u\n", dir_lba);
        return -1;
    }
    
    // 2. Find the entry matching `name`
    ProgramDirEntry *dir = (ProgramDirEntry *)dir_buf;
    int max_entries = SECTOR_SIZE / (int)sizeof(ProgramDirEntry);
    ProgramDirEntry *found = nullptr;
    for (int i = 0; i < max_entries; i++) {
        if (dir[i].name[0] == '\0') break;
        if (strcmp(dir[i].name, name) == 0) {
            found = &dir[i];
            break;
        }
    }
    if (!found) {
        kprintf("loader: program '%s' not found\n", name);
        return -1;
    }

    // 3. Create a user thread.
    // With -Ttext 0x08048000, e_entry is already an absolute VA (0x08048000+offset).
    // Do NOT add PROG_LOAD_BASE again.
    void (*entry_vaddr)() = (void (*)())(found->entry);
    tcb_t *t = thread_create_user(entry_vaddr);
    if (!t) {
        kprintf("loader: thread_create failed for '%s'\n", name);
        return -1;
    }

    // 4. Map program pages and read sectors into physical frames
    uint32_t total_bytes = (uint32_t)found->sectors * SECTOR_SIZE;
    uint32_t num_pages = (total_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t page = 0; page < num_pages; page++) {
        uint32_t paddr = frame_alloc();
        if (!paddr) {
            kprintf("loader: out of frames loading '%s'\n", name);
            return -1;
        }

        uint32_t vaddr = PROG_LOAD_BASE + page * PAGE_SIZE;
        map_page(t->page_directory, vaddr, paddr, PAGE_RW | PAGE_USER);

        uint32_t lba = found->lba + page * SECTORS_PER_PAGE;
        for (uint32_t s = 0; s < SECTORS_PER_PAGE; s++) {
            if (page * SECTORS_PER_PAGE + s >= found->sectors) break;  // don't read past the end of the program
            void *dst = (void *)(paddr + s * SECTOR_SIZE);
            if (ata_read_sector(lba + s, dst) < 0) {
                kprintf("loader: disk read error at lba %u\n", lba + s);
                return -1;
            }
        }
    }

    kprintf("loader: loaded '%s' (%u sectors, entry=0x%x)\n",
            found->name, found->sectors, (uint32_t)entry_vaddr);
    return 0;
}