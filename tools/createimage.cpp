#include "elf32.hpp"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define OUTPUT_FILE "image"
#define SECTOR_SIZE 512 
#define OS_SIZE_LOC 2
#define KERNEL_ENTRY_LOC 503

typedef struct {
  int fd;
  off_t start_offset;
  off_t stripped_size;
  off_t entry;
} FileInfo;

int fill_metadata(FileInfo* info) { // Assumes it is at the start of the file
  off_t min_offset = INT64_MAX;
  off_t max_offset = 0;
  bool found = false;

  if (!info || info->fd < 0) return -1; // invalid file

  off_t old_pos = lseek(info->fd, 0, SEEK_CUR);
  if (old_pos == -1L) return -1;

  Elf32_Ehdr ehdr32;
  if (read(info->fd, &ehdr32, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) goto fail; // invalid read or size

  if (ehdr32.e_ident[EI_MAG0] != ELFMAG0 || ehdr32.e_ident[EI_MAG1] != ELFMAG1 ||
    ehdr32.e_ident[EI_MAG2] != ELFMAG2 || ehdr32.e_ident[EI_MAG3] != ELFMAG3) {
    printf("File is not ELF\n");
    goto fail;
  }

  if (ehdr32.e_ident[EI_CLASS] != ELFCLASS32) {
    printf("File is not 32bit\n");
    goto fail;
  }

  if (ehdr32.e_ident[EI_DATA] != ELFDATA2LSB) {
    printf("File is not little endian\n");
    goto fail;
  }

  info->entry = ehdr32.e_entry;

  lseek(info->fd, ehdr32.e_phoff, SEEK_SET);

  for (int i = 0; i < ehdr32.e_phnum; i++) {
    Elf32_Phdr phdr;
    if (read(info->fd, &phdr, sizeof(phdr)) != sizeof(phdr)) goto fail;
    if (phdr.p_type != PT_LOAD) continue;
    found = true;

    if (phdr.p_offset < min_offset) min_offset = phdr.p_offset;
    off_t end = phdr.p_offset + phdr.p_filesz;
    if (end > max_offset) max_offset = end;
  }

  if (!found) goto fail;

  info->start_offset = min_offset;
  info->stripped_size = max_offset - min_offset;

  if (info->stripped_size < info->start_offset) goto fail;

  lseek(info->fd, old_pos, SEEK_SET);
  return 1;

fail:
  lseek(info->fd, old_pos, SEEK_SET);
  return -1;
}

off_t round_up_to_sector(off_t size) {
  return ((size + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
}

uint32_t count_sectors(uint32_t size) { // Assumes it has already been rounded up
  return size / SECTOR_SIZE;
}

// Portable replacement for Linux's sendfile(): copies `size` bytes starting
// at `offset` in `in_fd` to the current position of `out_fd`. sendfile()'s
// signature differs between Linux and macOS/BSD, so a plain read/write loop
// is used instead — simpler and works everywhere.
int copy_range(int out_fd, int in_fd, off_t offset, off_t size) {
  if (lseek(in_fd, offset, SEEK_SET) == (off_t)-1) return -1;
  char buf[8192];
  off_t remaining = size;
  while (remaining > 0) {
    ssize_t chunk = remaining < (off_t)sizeof(buf) ? remaining : (off_t)sizeof(buf);
    ssize_t n = read(in_fd, buf, chunk);
    if (n <= 0) return -1;
    ssize_t written = write(out_fd, buf, n);
    if (written != n) return -1;
    remaining -= n;
  }
  return 0;
}

int main(int argc, char **argv)
{
  uint32_t kernel_size = 0;
  uint32_t kernel_entry = 0;
  int rc;

  int file_count = argc - 2;
  FileInfo* file_infos = new FileInfo[file_count];

  off_t disk_size = 0;
  for (int i = 2; i < argc; i++) {
    file_infos[i-2].fd = open(argv[i], O_RDONLY);
    
    off_t file_size;
    fill_metadata(&file_infos[i-2]);

    if (i == 2) { // bootblock always first
      file_infos[i-2].stripped_size = (off_t) SECTOR_SIZE;
    } else if (i == 3) { // kernel is always second
        kernel_size = (uint32_t) round_up_to_sector(file_infos[i-2].stripped_size);
        kernel_entry = file_infos[i-2].entry;
    }
    
    disk_size += round_up_to_sector(file_infos[i-2].stripped_size);
  }

  int image = open(OUTPUT_FILE, O_CREAT | O_WRONLY, 0644);
  if (image == -1) { perror("open failed"); return 1; }
  if (ftruncate(image, disk_size) == -1) { perror("ftruncate failed"); close(image); return 1; }

  rc = lseek(image, 0, SEEK_SET); // maybe unnecessary
    
  for (int i = 0; i < file_count; i++) {
    rc = copy_range(image, file_infos[i].fd, file_infos[i].start_offset, file_infos[i].stripped_size);
    rc = close(file_infos[i].fd);
  }

  uint32_t kernel_sectors = count_sectors(kernel_size);
  rc = lseek(image, OS_SIZE_LOC, SEEK_SET);
  rc = write(image, &kernel_sectors, sizeof(kernel_sectors)); // Not endian safe
  rc = lseek(image, KERNEL_ENTRY_LOC, SEEK_SET);
  rc = write(image, &kernel_entry, sizeof(kernel_entry));

// rc unused, needs check perhaps
  
  printf("OS_SIZE: %x\n", kernel_sectors);
  printf("KERNEL_ENTRY: %x\n", kernel_entry);

  close(image);
  delete[] file_infos;

  return 0;
}
