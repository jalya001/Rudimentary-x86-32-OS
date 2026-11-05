#pragma once

#include <minstd/types.hpp>

void call_yield(void);
void call_exit(void);
int getpid(void);
/*
int mbox_open(int key);
int mbox_close(int q);
int mbox_stat(int q, int *count, int *space);
int mbox_recv(int q, char *m);
int mbox_send(int q, char *m);
*/
int getchar(int *c);

void call_write(int fd, const char *fmt, uint32_t *args);

int readdir(unsigned char *buf);
void loadproc(int location, int size);
int call_getchar(); // returns char or -1 if buffer empty
void call_exec(const char *name);

int call_fs_open(const char *filename, int mode);
int call_fs_close(int fd);
int call_fs_read(int fd, char *buffer, int size);
int call_fs_write(int fd, char *buffer, int size);
int call_fs_lseek(int fd, int offset, int whence);
int call_fs_unlink(const char *linkname);
int call_fs_stat(int fd, char *buffer);
