#ifndef _DEVICE_ACPI_H
#define _DEVICE_ACPI_H

#include <config.h>
#include <stdint.h>

/**
 * ACPI RSDP (Root System Description Pointer) 構造体
 */
struct acpi_rsdp {
	char signature[8]; /* "RSD PTR " */
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address; /* RSDT物理アドレス */
} __attribute__((packed));

/**
 * ACPI RSDP 2.0+ 拡張部分
 */
struct acpi_rsdp_ext {
	struct acpi_rsdp rsdp;
	uint32_t length;
	uint64_t xsdt_address;
	uint8_t extended_checksum;
	uint8_t reserved[3];
} __attribute__((packed));

/**
 * ACPI SDT (System Description Table) ヘッダー
 */
struct acpi_sdt_header {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__((packed));

/**
 * ACPI RSDT (Root System Description Table)
 */
struct acpi_rsdt {
	struct acpi_sdt_header header;
	uint32_t sdt_pointers[]; /* 可変長の配列 */
} __attribute__((packed));

/**
 * ACPI FADT (Fixed ACPI Description Table) - PM Timer用
 */
struct acpi_fadt {
	struct acpi_sdt_header header;
	uint32_t firmware_ctrl;
	uint32_t dsdt;
	uint8_t reserved;
	uint8_t preferred_pm_profile;
	uint16_t sci_interrupt;
	uint32_t smi_command_port;
	uint8_t acpi_enable;
	uint8_t acpi_disable;
	uint8_t s4bios_req;
	uint8_t pstate_control;
	uint32_t pm1a_event_block;
	uint32_t pm1b_event_block;
	uint32_t pm1a_control_block;
	uint32_t pm1b_control_block;
	uint32_t pm2_control_block;
	uint32_t pm_timer_block; /* PM Timer I/Oポート */
	uint32_t gpe0_block;
	uint32_t gpe1_block;
	uint8_t pm1_event_length;
	uint8_t pm1_control_length;
	uint8_t pm2_control_length;
	uint8_t pm_timer_length; /* PM Timer長（通常4バイト）*/
	uint8_t gpe0_length;
	uint8_t gpe1_length;
	uint8_t gpe1_base;
	uint8_t cstate_control;
	uint16_t worst_c2_latency;
	uint16_t worst_c3_latency;
	uint16_t flush_size;
	uint16_t flush_stride;
	uint8_t duty_offset;
	uint8_t duty_width;
	uint8_t day_alarm;
	uint8_t month_alarm;
	uint8_t century;
	uint16_t boot_arch_flags;
	uint8_t reserved2;
	uint32_t flags; /* bit 8: TMR_VAL_EXT (32bit timer) */
	/* 以降は拡張フィールド（省略可能） */
} __attribute__((packed));

/**
 * ACPI初期化
 * @return 0: 成功, -1: 失敗
 */
int acpi_init(void);

/**
 * ACPI PM Timerから現在のカウント値を取得
 * @return タイマーカウント値（3.579545 MHzでカウント）
 */
uint32_t acpi_timer_read(void);

/**
 * ACPI PM Timerが利用可能かチェック
 * @return 1: 利用可能, 0: 利用不可
 */
int acpi_timer_available(void);

/**
 * システム起動からの経過時間をマイクロ秒で取得
 * @return マイクロ秒
 */
uint64_t acpi_get_uptime_us(void);

/**
 * システム起動からの経過時間をミリ秒で取得
 * @return ミリ秒
 */
uint64_t acpi_get_uptime_ms(void);

#endif /* _DEVICE_ACPI_H */
