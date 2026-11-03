#include "elf32.hpp"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define OUTPUT_FILE      "image"
#define SECTOR_SIZE      512
#define OS_SIZE_LOC      2
#define KERNEL_ENTRY_LOC 503
#define PROG_DIR_LOC     6

struct ProgramDirEntry {
    char     name[16];
    uint32_t lba;
    uint32_t sectors;
    uint32_t entry;
    uint8_t  _pad[4];
};

typedef struct {
    int    fd;
    off_t  start_offset;
    off_t  stripped_size;
    off_t  entry;
    char   name[16];
} FileInfo;

int fill_metadata(FileInfo *info) {
    off_t min_offset = (off_t)0x7fffffffffffffffLL;
    off_t max_offset = 0;
    bool found = false;

    if (!info || info->fd < 0) return -1;

    off_t old_pos = lseek(info->fd, 0, SEEK_CUR);
    if (old_pos == (off_t)-1) return -1;

    Elf32_Ehdr ehdr;
    if (read(info->fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) goto fail;
    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        printf("File is not ELF\n"); goto fail;
    }
    if (ehdr.e_ident[EI_CLASS] != ELFCLASS32)  { printf("File is not 32-bit\n");        goto fail; }
    if (ehdr.e_ident[EI_DATA]  != ELFDATA2LSB) { printf("File is not little-endian\n"); goto fail; }

    info->entry = ehdr.e_entry;
    lseek(info->fd, ehdr.e_phoff, SEEK_SET);

    for (int i = 0; i < ehdr.e_phnum; i++) {
        Elf32_Phdr phdr;
        if (read(info->fd, &phdr, sizeof(phdr)) != sizeof(phdr)) goto fail;
        if (phdr.p_type != PT_LOAD) continue;
        found = true;
        if ((off_t)phdr.p_offset < min_offset) min_offset = phdr.p_offset;
        off_t end = phdr.p_offset + phdr.p_filesz;
        if (end > max_offset) max_offset = end;
    }
    if (!found) goto fail;

    info->start_offset  = min_offset;
    info->stripped_size = max_offset - min_offset;

    lseek(info->fd, old_pos, SEEK_SET);
    return 0;

fail:
    lseek(info->fd, old_pos, SEEK_SET);
    return -1;
}

off_t round_up_sector(off_t sz) { return ((sz + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE; }
uint32_t sector_count(off_t sz)  { return (uint32_t)(round_up_sector(sz) / SECTOR_SIZE); }

int copy_range(int out_fd, int in_fd, off_t offset, off_t size) {
    if (lseek(in_fd, offset, SEEK_SET) == (off_t)-1) return -1;
    char buf[8192];
    off_t rem = size;
    while (rem > 0) {
        ssize_t chunk = rem < (off_t)sizeof(buf) ? rem : (off_t)sizeof(buf);
        ssize_t n = read(in_fd, buf, chunk);
        if (n <= 0) return -1;
        if (write(out_fd, buf, n) != n) return -1;
        rem -= n;
    }
    return 0;
}

int pad_to_sector(int out_fd, off_t written) {
    off_t remainder = written % SECTOR_SIZE;
    if (remainder == 0) return 0;
    off_t pad = SECTOR_SIZE - remainder;
    char zeros[SECTOR_SIZE] = {};
    return (write(out_fd, zeros, (size_t)pad) == pad) ? 0 : -1;
}

static const char *basename_simple(const char *path) {
    const char *s = strrchr(path, '/');
    return s ? s + 1 : path;
}

int main(int argc, char **argv) {
    // argv: createimage [--extended] bootblock kernel [user1 user2 ...]
    int arg_start = 1;
    if (argc > 1 && strcmp(argv[1], "--extended") == 0) arg_start = 2;

    if (argc - arg_start < 2) {
        printf("Usage: createimage [--extended] bootblock kernel [userprog ...]\n");
        return 1;
    }

    int bootblock_idx = arg_start;
    int kernel_idx    = arg_start + 1;
    int user_start    = arg_start + 2;
    int user_count    = argc - user_start;

    FileInfo bb = {}, kern = {};
    bb.fd   = open(argv[bootblock_idx], O_RDONLY);
    kern.fd = open(argv[kernel_idx],    O_RDONLY);
    if (bb.fd < 0 || kern.fd < 0) { perror("open"); return 1; }

    // bootblock is a raw binary produced by objcopy, NOT an ELF — hardcode it
    bb.start_offset  = 0;
    bb.stripped_size = SECTOR_SIZE;
    bb.entry         = 0;

    if (fill_metadata(&kern) < 0) { printf("failed to parse kernel ELF\n"); return 1; }

    FileInfo *users = new FileInfo[user_count + 1]();
    for (int i = 0; i < user_count; i++) {
        users[i].fd = open(argv[user_start + i], O_RDONLY);
        if (users[i].fd < 0) { perror("open user"); return 1; }
        if (fill_metadata(&users[i]) < 0) {
            printf("failed to parse user ELF: %s\n", argv[user_start + i]);
            return 1;
        }
        const char *bn = basename_simple(argv[user_start + i]);
        strncpy(users[i].name, bn, 15);
        users[i].name[15] = '\0';
    }

    // Disk layout:
    //   sector 0          : bootblock (1 sector, raw binary)
    //   sectors 1..N      : kernel    (kernel_sectors sectors)
    //   sector N+1        : program directory (1 sector)
    //   sector N+2..      : user programs (stripped PT_LOAD data)

    uint32_t kernel_sectors = sector_count(kern.stripped_size);
    uint32_t prog_dir_lba   = 1 + kernel_sectors;
    uint32_t next_user_lba  = prog_dir_lba + 1;

    // Build program directory sector
    uint8_t dir_sector[SECTOR_SIZE] = {};
    ProgramDirEntry *dir = (ProgramDirEntry *)dir_sector;
    int max_entries = (int)(SECTOR_SIZE / sizeof(ProgramDirEntry));

    for (int i = 0; i < user_count && i < max_entries; i++) {
        uint32_t usecs = sector_count(users[i].stripped_size);
        strncpy(dir[i].name, users[i].name, 15);
        dir[i].name[15] = '\0';
        dir[i].lba     = next_user_lba;
        dir[i].sectors = usecs;
        dir[i].entry   = (uint32_t)users[i].entry;
        for (int p = 0; p < 4; p++) dir[i]._pad[p] = 0;
        next_user_lba += usecs;
        printf("USER: %-16s  lba=%-4u  sectors=%-4u  entry=0x%x\n",
               dir[i].name, dir[i].lba, dir[i].sectors, dir[i].entry);
    }

    // Write image
    int image = open(OUTPUT_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (image < 0) { perror("open image"); return 1; }

    // Bootblock (1 sector, raw)
    copy_range(image, bb.fd, 0, SECTOR_SIZE);

    // Kernel
    copy_range(image, kern.fd, kern.start_offset, kern.stripped_size);
    pad_to_sector(image, kern.stripped_size);

    // Program directory
    write(image, dir_sector, SECTOR_SIZE);

    // User programs
    for (int i = 0; i < user_count; i++) {
        copy_range(image, users[i].fd, users[i].start_offset, users[i].stripped_size);
        pad_to_sector(image, users[i].stripped_size);
    }

    // Patch bootblock
    uint32_t kernel_entry = (uint32_t)kern.entry;

    lseek(image, OS_SIZE_LOC, SEEK_SET);
    write(image, &kernel_sectors, 4);     // offset 2: kernel sector count

    lseek(image, PROG_DIR_LOC, SEEK_SET);
    write(image, &prog_dir_lba, 4);       // offset 6: program directory LBA

    lseek(image, KERNEL_ENTRY_LOC, SEEK_SET);
    write(image, &kernel_entry, 4);       // offset 503: kernel entry point

    printf("OS_SIZE:      %u sectors\n", kernel_sectors);
    printf("KERNEL_ENTRY: 0x%x\n",       kernel_entry);
    printf("PROG_DIR_LBA: %u\n",         prog_dir_lba);

    close(image);
    for (int i = 0; i < user_count; i++) close(users[i].fd);
    close(kern.fd);
    close(bb.fd);
    delete[] users;
    return 0;
}
