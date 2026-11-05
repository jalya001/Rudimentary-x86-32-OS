#include "fs.hpp"
#include "drivers/ata.hpp"
#include "print.hpp"
#include <minstd/string.hpp>
#include <minstd/types.hpp>

static uint8_t sector_buf[512];

// ── in-memory state ────────────────────────────────────────────────────────
static FSuperblock sb;
static FInode      inodes[FS_MAX_INODES];
static uint8_t     bitmap[1024];   // 2 sectors × 512 bytes = 8192 bits
static FD          fds[FS_MAX_FDS];
static int         fs_ready = 0;

// ── bitmap helpers ─────────────────────────────────────────────────────────
static int  bm_test(uint32_t b) { return (bitmap[b/8] >> (b%8)) & 1; }
static void bm_set (uint32_t b) { bitmap[b/8] |=  (1u << (b%8)); }
static void bm_clr (uint32_t b) { bitmap[b/8] &= ~(1u << (b%8)); }

// ── disk helpers ───────────────────────────────────────────────────────────
static int write_superblock() {
    uint8_t buf[512] = {};
    buffer_copy((const char *)&sb, (char *)buf, sizeof(FSuperblock));
    return ata_write_sector(FS_SUPERBLOCK_LBA, buf);
}

static int write_inodes() {
    // 32 inodes × 32 bytes = 1024 bytes = 2 sectors
    uint8_t buf[512];
    buffer_copy((const char *)&inodes[0],  (char *)buf, 512);
    if (ata_write_sector(FS_INODE_LBA,   buf) < 0) return -1;
    buffer_copy((const char *)&inodes[16], (char *)buf, 512);
    if (ata_write_sector(FS_INODE_LBA+1, buf) < 0) return -1;
    return 0;
}

static int write_bitmap() {
    if (ata_write_sector(FS_BITMAP_LBA,   bitmap)       < 0) return -1;
    if (ata_write_sector(FS_BITMAP_LBA+1, bitmap + 512) < 0) return -1;
    return 0;
}

// ── allocate N contiguous blocks; return first block index or -1 ──────────
static int alloc_blocks(uint32_t n) {
    uint32_t total = sb.block_count;
    for (uint32_t start = 0; start + n <= total; start++) {
        bool ok = true;
        for (uint32_t i = 0; i < n; i++) {
            if (bm_test(start + i)) { ok = false; break; }
        }
        if (ok) {
            for (uint32_t i = 0; i < n; i++) bm_set(start + i);
            return (int)start;
        }
    }
    return -1;
}

static void free_blocks(uint32_t start, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) bm_clr(start + i);
}

// ── fs_init ────────────────────────────────────────────────────────────────
int fs_init() {
    for (int i = 0; i < FS_MAX_FDS; i++) fds[i].inode_idx = -1;

    // Try to read superblock
    if (ata_read_sector(FS_SUPERBLOCK_LBA, sector_buf) < 0) {
        kprintf("fs: ATA read failed\n");
        return -1;
    }
    buffer_copy((const char *)sector_buf, (char *)&sb, sizeof(FSuperblock));

    if (sb.magic == FS_MAGIC) {
        // Existing FS — load inodes and bitmap
        if (ata_read_sector(FS_INODE_LBA,   sector_buf) < 0) return -1;
        buffer_copy((const char *)sector_buf, (char *)&inodes[0],  512);
        if (ata_read_sector(FS_INODE_LBA+1, sector_buf) < 0) return -1;
        buffer_copy((const char *)sector_buf, (char *)&inodes[16], 512);

        if (ata_read_sector(FS_BITMAP_LBA,   bitmap)       < 0) return -1;
        if (ata_read_sector(FS_BITMAP_LBA+1, bitmap + 512) < 0) return -1;

        kprintf("fs: mounted (%u inodes used)\n", sb.inode_count);
    } else {
        // Format fresh FS
        kprintf("fs: no filesystem found, formatting...\n");
        sb.magic       = FS_MAGIC;
        sb.inode_count = 0;
        sb.data_start  = FS_DATA_LBA;
        sb.block_count = 4096;   // 4096 × 512 = 2 MB of data space

        for (int i = 0; i < FS_MAX_INODES; i++) {
            inodes[i] = {};
            inodes[i].flags = 0;
        }
        for (int i = 0; i < 1024; i++) bitmap[i] = 0;

        if (write_superblock() < 0) return -1;
        if (write_inodes()     < 0) return -1;
        if (write_bitmap()     < 0) return -1;
        kprintf("fs: formatted OK\n");
    }

    fs_ready = 1;
    return 0;
}

// ── fs_open ────────────────────────────────────────────────────────────────
int fs_open(const char *name, int mode) {
    if (!fs_ready) return -1;

    // Find free FD
    int fd = -1;
    for (int i = 0; i < FS_MAX_FDS; i++) {
        if (fds[i].inode_idx == -1) { fd = i; break; }
    }
    if (fd < 0) return -1;

    // Look up inode
    int inode_idx = -1;
    for (int i = 0; i < FS_MAX_INODES; i++) {
        if ((inodes[i].flags & FS_FLAG_USED) && strcmp(inodes[i].name, name) == 0) {
            inode_idx = i;
            break;
        }
    }

    if (inode_idx < 0) {
        // File not found
        if (!(mode & FS_O_CREATE)) return -1;

        // Find free inode slot
        for (int i = 0; i < FS_MAX_INODES; i++) {
            if (!(inodes[i].flags & FS_FLAG_USED)) { inode_idx = i; break; }
        }
        if (inode_idx < 0) return -1;

        // Allocate 1 initial block
        int blk = alloc_blocks(1);
        if (blk < 0) return -1;

        inodes[inode_idx] = {};
        int ni = 0;
        while (ni < FS_NAME_LEN - 1 && name[ni]) {
            inodes[inode_idx].name[ni] = name[ni];
            ni++;
        }
        inodes[inode_idx].name[ni] = '\0';
        inodes[inode_idx].size        = 0;
        inodes[inode_idx].start_block = (uint32_t)blk;
        inodes[inode_idx].block_count = 1;
        inodes[inode_idx].flags       = FS_FLAG_USED;
        sb.inode_count++;

        write_inodes();
        write_bitmap();
        write_superblock();
    }

    fds[fd].inode_idx = inode_idx;
    fds[fd].offset    = (mode & FS_O_WRITE) ? inodes[inode_idx].size : 0;
    fds[fd].mode      = mode;
    return fd;
}

// ── fs_close ───────────────────────────────────────────────────────────────
int fs_close(int fd) {
    if (fd < 0 || fd >= FS_MAX_FDS || fds[fd].inode_idx < 0) return -1;
    fds[fd].inode_idx = -1;
    return 0;
}

// ── fs_read ────────────────────────────────────────────────────────────────
int fs_read(int fd, char *buf, int size) {
    if (fd < 0 || fd >= FS_MAX_FDS || fds[fd].inode_idx < 0) return -1;
    if (!(fds[fd].mode & FS_O_READ)) return -1;

    FInode *in = &inodes[fds[fd].inode_idx];
    uint32_t remaining = in->size - fds[fd].offset;
    if ((uint32_t)size > remaining) size = (int)remaining;
    if (size <= 0) return 0;

    int read = 0;
    while (read < size) {
        uint32_t abs_offset = fds[fd].offset + read;
        uint32_t block_idx  = abs_offset / 512;
        uint32_t block_off  = abs_offset % 512;
        uint32_t lba        = FS_DATA_LBA + in->start_block + block_idx;
        uint32_t chunk      = 512 - block_off;
        if (chunk > (uint32_t)(size - read)) chunk = (uint32_t)(size - read);

        if (ata_read_sector(lba, sector_buf) < 0) return -1;
        buffer_copy((const char *)sector_buf + block_off, buf + read, chunk);
        read += (int)chunk;
    }

    fds[fd].offset += (uint32_t)read;
    return read;
}

// ── fs_write ───────────────────────────────────────────────────────────────
int fs_write(int fd, char *buf, int size) {
    if (fd < 0 || fd >= FS_MAX_FDS || fds[fd].inode_idx < 0) return -1;
    if (!(fds[fd].mode & FS_O_WRITE)) return -1;

    FInode *in = &inodes[fds[fd].inode_idx];

    int written = 0;
    while (written < size) {
        uint32_t abs_offset  = fds[fd].offset + (uint32_t)written;
        uint32_t block_idx   = abs_offset / 512;
        uint32_t block_off   = abs_offset % 512;

        // Grow file if needed
        if (block_idx >= in->block_count) {
            uint32_t need = block_idx + 1;
            // Try to extend: free old, alloc contiguous new
            free_blocks(in->start_block, in->block_count);
            int new_start = alloc_blocks(need);
            if (new_start < 0) {
                // Restore old allocation
                alloc_blocks(in->block_count);  // best-effort
                break;
            }
            // Copy existing data to new location if start changed
            if ((uint32_t)new_start != in->start_block) {
                for (uint32_t b = 0; b < in->block_count; b++) {
                    uint8_t tmp[512];
                    ata_read_sector(FS_DATA_LBA + in->start_block + b, tmp);
                    ata_write_sector(FS_DATA_LBA + (uint32_t)new_start + b, tmp);
                }
            }
            in->start_block = (uint32_t)new_start;
            in->block_count = need;
            write_bitmap();
        }

        uint32_t lba   = FS_DATA_LBA + in->start_block + block_idx;
        uint32_t chunk = 512 - block_off;
        if (chunk > (uint32_t)(size - written)) chunk = (uint32_t)(size - written);

        // Read-modify-write for partial block
        if (block_off != 0 || chunk != 512) {
            if (ata_read_sector(lba, sector_buf) < 0) break;
        }
        buffer_copy(buf + written, (char *)sector_buf + block_off, chunk);
        if (ata_write_sector(lba, sector_buf) < 0) break;

        written += (int)chunk;
    }

    fds[fd].offset += (uint32_t)written;
    if (fds[fd].offset > in->size) {
        in->size = fds[fd].offset;
        write_inodes();
    }
    return written;
}

// ── fs_lseek ───────────────────────────────────────────────────────────────
int fs_lseek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= FS_MAX_FDS || fds[fd].inode_idx < 0) return -1;
    FInode *in = &inodes[fds[fd].inode_idx];
    int pos;
    if      (whence == FS_SEEK_SET) pos = offset;
    else if (whence == FS_SEEK_CUR) pos = (int)fds[fd].offset + offset;
    else if (whence == FS_SEEK_END) pos = (int)in->size + offset;
    else return -1;
    if (pos < 0) pos = 0;
    fds[fd].offset = (uint32_t)pos;
    return pos;
}

// ── fs_unlink ──────────────────────────────────────────────────────────────
int fs_unlink(const char *name) {
    if (!fs_ready) return -1;
    for (int i = 0; i < FS_MAX_INODES; i++) {
        if ((inodes[i].flags & FS_FLAG_USED) && strcmp(inodes[i].name, name) == 0) {
            free_blocks(inodes[i].start_block, inodes[i].block_count);
            inodes[i] = {};
            sb.inode_count--;
            write_inodes();
            write_bitmap();
            write_superblock();
            return 0;
        }
    }
    return -1;
}

// ── fs_stat ────────────────────────────────────────────────────────────────
// writes: name (16 bytes) + size (4 bytes uint32) = 20 bytes into buf
int fs_stat(int fd, char *buf) {
    if (fd < 0 || fd >= FS_MAX_FDS || fds[fd].inode_idx < 0) return -1;
    FInode *in = &inodes[fds[fd].inode_idx];
    buffer_copy(in->name, buf, FS_NAME_LEN);
    buffer_copy((const char *)&in->size, buf + FS_NAME_LEN, 4);
    return 0;
}
