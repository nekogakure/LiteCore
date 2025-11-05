#include <driver/ata.h>
#include <util/io.h>
#include <util/console.h>

/**
 * @brief ATAデバイスの準備完了を待つ
 */
static void ata_wait_ready(uint16_t base) {
	while (inb(base + 7) & ATA_SR_BSY) {
		/* Busyフラグがクリアされるまで待機 */
	}
}

/**
 * @brief ATAデバイスのDRQビットを待つ
 */
static int ata_wait_drq(uint16_t base) {
	uint32_t timeout = 100000;
	uint8_t status;
	
	while (timeout--) {
		status = inb(base + 7);
		
		if (status & ATA_SR_ERR) {
			return -1;  /* エラー */
		}
		
		if (status & ATA_SR_DRQ) {
			return 0;   /* DRQ準備完了 */
		}
	}
	
	return -1;  /* タイムアウト */
}

/**
 * @brief ドライブ番号からベースアドレスとドライブ選択を取得
 */
static void ata_get_base(uint8_t drive, uint16_t *base, uint8_t *drive_sel) {
	if (drive < 2) {
		/* Primary Bus */
		*base = ATA_PRIMARY_DATA;
		*drive_sel = (drive == 0) ? ATA_MASTER : ATA_SLAVE;
	} else {
		/* Secondary Bus */
		*base = ATA_SECONDARY_DATA;
		*drive_sel = (drive == 2) ? ATA_MASTER : ATA_SLAVE;
	}
}

/**
 * @brief ATAドライバを初期化する
 */
int ata_init(void) {
	printk("ATA: Initializing ATA driver\n");
	
	/* Secondary Masterを選択（QEMUの-hdb） */
	outb(ATA_SECONDARY_DRIVE, ATA_MASTER);
	ata_wait_ready(ATA_SECONDARY_DATA);
	
	/* IDENTIFYコマンドを送信 */
	outb(ATA_SECONDARY_COMMAND, ATA_CMD_IDENTIFY);
	ata_wait_ready(ATA_SECONDARY_DATA);
	
	uint8_t status = inb(ATA_SECONDARY_STATUS);
	
	if (status == 0) {
		printk("ATA: No drive detected on Secondary Master\n");
		printk("ATA: Trying Primary Master...\n");
		
		/* Primary Masterを試す */
		outb(ATA_PRIMARY_DRIVE, ATA_MASTER);
		ata_wait_ready(ATA_PRIMARY_DATA);
		
		outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);
		ata_wait_ready(ATA_PRIMARY_DATA);
		
		status = inb(ATA_PRIMARY_STATUS);
		
		if (status == 0) {
			printk("ATA: No drive detected on Primary Master\n");
			return -1;
		}
		
		if (status & ATA_SR_ERR) {
			printk("ATA: Error detecting Primary Master\n");
			return -1;
		}
		
		/* IDENTIFYデータを読み取る */
		for (int i = 0; i < 256; i++) {
			(void)inw(ATA_PRIMARY_DATA);
		}
		
		printk("ATA: Primary Master detected\n");
		return 0;
	}
	
	if (status & ATA_SR_ERR) {
		printk("ATA: Error detecting Secondary Master, trying Primary Master...\n");
		
		/* Primary Masterを試す */
		outb(ATA_PRIMARY_DRIVE, ATA_MASTER);
		ata_wait_ready(ATA_PRIMARY_DATA);
		
		outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);
		ata_wait_ready(ATA_PRIMARY_DATA);
		
		status = inb(ATA_PRIMARY_STATUS);
		
		if (status == 0 || (status & ATA_SR_ERR)) {
			printk("ATA: No valid ATA drive found\n");
			return -1;
		}
		
		/* IDENTIFYデータを読み取る */
		for (int i = 0; i < 256; i++) {
			(void)inw(ATA_PRIMARY_DATA);
		}
		
		printk("ATA: Primary Master detected\n");
		return 0;
	}
	
	/* IDENTIFYデータを読み取る（256ワード = 512バイト）*/
	for (int i = 0; i < 256; i++) {
		(void)inw(ATA_SECONDARY_DATA);
	}
	
	printk("ATA: Secondary Master detected\n");
	
	return 0;
}

/**
 * @brief ATAデバイスからセクタを読み取る
 */
int ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sectors, void *buffer) {
	uint16_t base;
	uint8_t drive_sel;
	uint16_t *buf = (uint16_t *)buffer;
	
	if (sectors == 0) {
		return -1;
	}
	
	/* ベースアドレスとドライブ選択を取得 */
	ata_get_base(drive, &base, &drive_sel);
	
	/* デバイスが準備完了するまで待機 */
	ata_wait_ready(base);
	
	/* LBA28モードでドライブを選択 */
	outb(base + 6, drive_sel | ((lba >> 24) & 0x0F));
	
	/* セクタ数を設定 */
	outb(base + 2, sectors);
	
	/* LBAアドレスを設定 */
	outb(base + 3, (uint8_t)(lba & 0xFF));
	outb(base + 4, (uint8_t)((lba >> 8) & 0xFF));
	outb(base + 5, (uint8_t)((lba >> 16) & 0xFF));
	
	/* READコマンドを送信 */
	outb(base + 7, ATA_CMD_READ_PIO);
	
	/* 各セクタを読み取る */
	for (int s = 0; s < sectors; s++) {
		/* DRQを待つ */
		if (ata_wait_drq(base) != 0) {
			printk("ATA: Read timeout at sector %d\n", s);
			return -1;
		}
		
		/* 256ワード（512バイト）を読み取る */
		for (int i = 0; i < 256; i++) {
			buf[s * 256 + i] = inw(base);
		}
		
		/* 400nsの遅延 */
		for (int i = 0; i < 4; i++) {
			inb(base + 7);
		}
	}
	
	return 0;
}

/**
 * @brief ATAデバイスにセクタを書き込む
 */
int ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sectors, const void *buffer) {
	uint16_t base;
	uint8_t drive_sel;
	const uint16_t *buf = (const uint16_t *)buffer;
	
	if (sectors == 0) {
		return -1;
	}
	
	/* ベースアドレスとドライブ選択を取得 */
	ata_get_base(drive, &base, &drive_sel);
	
	/* デバイスが準備完了するまで待機 */
	ata_wait_ready(base);
	
	/* LBA28モードでドライブを選択 */
	outb(base + 6, drive_sel | ((lba >> 24) & 0x0F));
	
	/* セクタ数を設定 */
	outb(base + 2, sectors);
	
	/* LBAアドレスを設定 */
	outb(base + 3, (uint8_t)(lba & 0xFF));
	outb(base + 4, (uint8_t)((lba >> 8) & 0xFF));
	outb(base + 5, (uint8_t)((lba >> 16) & 0xFF));
	
	/* WRITEコマンドを送信 */
	outb(base + 7, ATA_CMD_WRITE_PIO);
	
	/* 各セクタを書き込む */
	for (int s = 0; s < sectors; s++) {
		/* DRQを待つ */
		if (ata_wait_drq(base) != 0) {
			printk("ATA: Write timeout at sector %d\n", s);
			return -1;
		}
		
		/* 256ワード（512バイト）を書き込む */
		for (int i = 0; i < 256; i++) {
			outw(base, buf[s * 256 + i]);
		}
		
		/* 書き込み完了を待つ */
		ata_wait_ready(base);
		
		/* 400nsの遅延 */
		for (int i = 0; i < 4; i++) {
			inb(base + 7);
		}
	}
	
	return 0;
}
