#pragma once
#include <minstd/types.hpp>

#define FS_SUPERBLOCK_LBA   128
#define FS_INODE_LBA        129   // 2 sectors → 32 inodes (32 bytes each)
#define FS_BITMAP_LBA       131   // 2 sectors → 8192 bits → 8192 data blocks
#define FS_DATA_LBA         133   // data blocks start here

#define FS_MAX_INODES       32
#define FS_MAX_FDS          16
#define FS_NAME_LEN         16
#define FS_MAGIC            0x52464653u  // "RFSO"

// Flags
#define FS_FLAG_USED        (1u << 0)

// Open modes
#define FS_O_READ           0x01
#define FS_O_WRITE          0x02
#define FS_O_CREATE         0x04

// lseek whence
#define FS_SEEK_SET         0
#define FS_SEEK_CUR         1
#define FS_SEEK_END         2

struct FSuperblock {
    uint32_t magic;
    uint32_t inode_count;   // total inode slots
    uint32_t data_start;    // LBA of first data block (== FS_DATA_LBA)
    uint32_t block_count;   // total data blocks available
    uint8_t  _pad[496];     // pad to 512 bytes
} __attribute__((packed));

struct FInode {
    char     name[FS_NAME_LEN];
    uint32_t size;          // bytes written
    uint32_t start_block;   // index into data block space (block 0 = FS_DATA_LBA)
    uint32_t block_count;   // number of contiguous blocks allocated
    uint32_t flags;
} __attribute__((packed));  // 32 bytes

struct FD {
    int      inode_idx;     // -1 = free
    uint32_t offset;        // current read/write position
    int      mode;
};

int  fs_init();
int  fs_open(const char *name, int mode);
int  fs_close(int fd);
int  fs_read(int fd, char *buf, int size);
int  fs_write(int fd, char *buf, int size);
int  fs_lseek(int fd, int offset, int whence);
int  fs_unlink(const char *name);
int  fs_stat(int fd, char *buf);   // writes name\0size (uint32) into buf
