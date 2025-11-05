#ifndef _DRIVER_ATA_H
#define _DRIVER_ATA_H

#include <stdint.h>
#include <stddef.h>

/* ATAレジスタ (Primary Bus) */
#define ATA_PRIMARY_DATA       0x1F0
#define ATA_PRIMARY_ERROR      0x1F1
#define ATA_PRIMARY_FEATURES   0x1F1
#define ATA_PRIMARY_SECCOUNT   0x1F2
#define ATA_PRIMARY_LBALOW     0x1F3
#define ATA_PRIMARY_LBAMID     0x1F4
#define ATA_PRIMARY_LBAHIGH    0x1F5
#define ATA_PRIMARY_DRIVE      0x1F6
#define ATA_PRIMARY_STATUS     0x1F7
#define ATA_PRIMARY_COMMAND    0x1F7

/* ATAレジスタ (Secondary Bus) */
#define ATA_SECONDARY_DATA     0x170
#define ATA_SECONDARY_ERROR    0x171
#define ATA_SECONDARY_FEATURES 0x171
#define ATA_SECONDARY_SECCOUNT 0x172
#define ATA_SECONDARY_LBALOW   0x173
#define ATA_SECONDARY_LBAMID   0x174
#define ATA_SECONDARY_LBAHIGH  0x175
#define ATA_SECONDARY_DRIVE    0x176
#define ATA_SECONDARY_STATUS   0x177
#define ATA_SECONDARY_COMMAND  0x177

/* ATAコマンド */
#define ATA_CMD_READ_PIO       0x20
#define ATA_CMD_WRITE_PIO      0x30
#define ATA_CMD_IDENTIFY       0xEC

/* ATAステータスビット */
#define ATA_SR_BSY   0x80  /* Busy */
#define ATA_SR_DRDY  0x40  /* Drive ready */
#define ATA_SR_DF    0x20  /* Drive write fault */
#define ATA_SR_DSC   0x10  /* Drive seek complete */
#define ATA_SR_DRQ   0x08  /* Data request ready */
#define ATA_SR_CORR  0x04  /* Corrected data */
#define ATA_SR_IDX   0x02  /* Index */
#define ATA_SR_ERR   0x01  /* Error */

/* ATAエラービット */
#define ATA_ER_BBK   0x80  /* Bad block */
#define ATA_ER_UNC   0x40  /* Uncorrectable data */
#define ATA_ER_MC    0x20  /* Media changed */
#define ATA_ER_IDNF  0x10  /* ID not found */
#define ATA_ER_MCR   0x08  /* Media change request */
#define ATA_ER_ABRT  0x04  /* Command aborted */
#define ATA_ER_TK0NF 0x02  /* Track 0 not found */
#define ATA_ER_AMNF  0x01  /* Address mark not found */

/* ATA デバイス */
#define ATA_MASTER 0xE0
#define ATA_SLAVE  0xF0

/**
 * @brief ATAドライバを初期化する
 * 
 * @return 成功時0、失敗時-1
 */
int ata_init(void);

/**
 * @brief ATAデバイスからセクタを読み取る
 * 
 * @param drive ドライブ番号 (0=Primary Master, 1=Primary Slave, 2=Secondary Master, 3=Secondary Slave)
 * @param lba 読み取り開始LBA (Logical Block Address)
 * @param sectors 読み取るセクタ数
 * @param buffer 読み取ったデータを格納するバッファ
 * @return 成功時0、失敗時-1
 */
int ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sectors, void *buffer);

/**
 * @brief ATAデバイスにセクタを書き込む
 * 
 * @param drive ドライブ番号
 * @param lba 書き込み開始LBA
 * @param sectors 書き込むセクタ数
 * @param buffer 書き込むデータのバッファ
 * @return 成功時0、失敗時-1
 */
int ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sectors, const void *buffer);

#endif /* _DRIVER_ATA_H */
