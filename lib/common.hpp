#pragma once

#define SYS_YIELD       0
#define SYS_EXIT        1
#define SYS_EXEC        2
#define SYS_GETCHAR     3
#define SYS_WRITE       4
#define SYS_FS_OPEN     5
#define SYS_FS_CLOSE    6
#define SYS_FS_READ     7
#define SYS_FS_WRITE    8
#define SYS_FS_LSEEK    9
#define SYS_FS_UNLINK   10
#define SYS_FS_STAT     11

#define FS_O_READ   0x01
#define FS_O_WRITE  0x02
#define FS_O_CREATE 0x04

#define FS_SEEK_SET 0
#define FS_SEEK_CUR 1
#define FS_SEEK_END 2