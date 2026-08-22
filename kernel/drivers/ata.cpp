#include "ata.hpp"
#include "serial.hpp"
#include <minstd/types.hpp>

uint16_t identify[256];

Disk disk = {};

inline int ata_wait(uint8_t mask, uint8_t value) {
  uint8_t status;

  while (1) {
    status = inb(ATA_STATUS);
    if (status & ATA_SR_ERR) return -1;
    if (status & ATA_SR_DF) return -1;
    if ((status & mask) == value) return 0;
  }

  return 0;
}

int ata_init() {
  // Select primary/master
  outb(ATA_DRIVE, 0xA0);

  // Clear ATA registers
  outb(ATA_SECCOUNT, 0);
  outb(ATA_LBA_LOW, 0);
  outb(ATA_LBA_MID, 0);
  outb(ATA_LBA_HIGH, 0);

  // IDENTIFY DEVICE
  outb(ATA_COMMAND, ATA_CMD_IDENTIFY);
  if (inb(ATA_STATUS) == 0) return -1; // No device responded
  if (ata_wait(ATA_SR_BSY, 0) < 0) return -1;
  if (inb(ATA_LBA_MID) != 0 || inb(ATA_LBA_HIGH) != 0) return -1; // Check that this is the ATA device type we support
  if (ata_wait(ATA_SR_DRQ, ATA_SR_DRQ) < 0) return -1;
  for (int i = 0; i < 256; i++) identify[i] = inw(ATA_DATA);
  if (!(identify[49] & (1 << 9))) return -1; // Require 28-bit LBA support

  disk.sector_count = ((uint32_t)identify[61] << 16) | (uint32_t)identify[60];
  if (disk.sector_count == 0) return -1;

  disk.present = 1;
  return 0;
}

static inline int ata_prepare_lba(uint32_t lba) {
  if (lba >= 0x10000000) return -1; // LBA28 can address at most 2^28 sectors
  if (!disk.present) return -1;
  if (lba >= disk.sector_count) return -1;

  // Select primary master and enable LBA mode
  outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
  outb(ATA_SECCOUNT, 1); // One sector

  // Send the lower 24 bits of the LBA
  outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
  outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
  outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

  return 0;
}

int ata_read_sector(uint32_t lba, void *buffer) {
  uint16_t *buf = (uint16_t *)buffer;

  if (ata_prepare_lba(lba) != 0) return -1;

  outb(ATA_COMMAND, ATA_CMD_READ_SECTORS);
  if (ata_wait(ATA_SR_DRQ, ATA_SR_DRQ) < 0) return -1;

  for (int i = 0; i < 256; i++) buf[i] = inw(ATA_DATA);

  return 0;
}

int ata_write_sector(uint32_t lba, void *buffer) {
  uint16_t *buf = (uint16_t *)buffer;

  if (ata_prepare_lba(lba) != 0) return -1;

  outb(ATA_COMMAND, ATA_CMD_WRITE_SECTORS);
  if (ata_wait(ATA_SR_DRQ, ATA_SR_DRQ) < 0) return -1;

  for (int i = 0; i < 256; i++) outw(ATA_DATA, buf[i]);
  if (ata_wait(ATA_SR_BSY, 0) < 0) return -1; // write doesn't synchronously complete

  return 0;
}
