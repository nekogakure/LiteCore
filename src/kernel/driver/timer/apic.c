#include <util/config.h>
#include <driver/timer/apic.h>
#include <util/console.h>

/* APIC レジスタアクセス用のベースアドレス */
static volatile uint32_t *apic_base = NULL;

/* タイマー周波数 (Hz) - キャリブレーション後に設定 */
static uint32_t apic_timer_frequency = 0;

/* システム起動時のティック数 */
static volatile uint64_t boot_tick_count = 0;

/* 現在のティック数 */
static volatile uint64_t current_tick_count = 0;

/* 初期化フラグ */
static int apic_initialized = 0;

/**
 * @brief APICレジスタを読み取る
 */
static inline uint32_t apic_read(uint32_t reg) {
	if (!apic_base)
		return 0;
	return *(volatile uint32_t *)((uint8_t *)apic_base + reg);
}

/**
 * @brief APICレジスタに書き込む
 */
static inline void apic_write(uint32_t reg, uint32_t value) {
	if (!apic_base)
		return;
	*(volatile uint32_t *)((uint8_t *)apic_base + reg) = value;
}

/**
 * @brief CPUIDを実行してAPICサポートを確認
 */
static int check_apic_support(void) {
	uint32_t eax, edx;

	/* CPUID 機能 1: プロセッサ情報と機能ビット */
	__asm__ volatile("movl $1, %%eax\n"
			 "cpuid\n"
			 : "=a"(eax), "=d"(edx)
			 :
			 : "ebx", "ecx");

	/* EDXのビット9がAPICサポートを示す */
	return (edx & (1 << 9)) ? 1 : 0;
}

/**
 * @brief APICベースアドレスを取得
 */
static uint32_t get_apic_base(void) {
	uint32_t eax, edx;

	/* MSR 0x1B (IA32_APIC_BASE) を読み取る */
	__asm__ volatile("movl $0x1B, %%ecx\n"
			 "rdmsr\n"
			 : "=a"(eax), "=d"(edx)
			 :
			 : "ecx");

	/* 下位12bitをマスクしてベースアドレスを取得 */
	return eax & 0xFFFFF000;
}

/**
 * @brief ACPI PM Timerのポート（通常0x608または0xb008）
 */
#define ACPI_PM_TIMER_PORT 0x608

/**
 * @brief ACPI PM Timerの周波数（固定）
 */
#define ACPI_PM_TIMER_FREQ 3579545 /* 3.579545 MHz */

/**
 * @brief ACPI PM Timerから現在のカウント値を読み取る
 */
static inline uint32_t read_acpi_pm_timer(void) {
	uint32_t value;
	__asm__ volatile("inl %1, %0"
			 : "=a"(value)
			 : "d"((uint16_t)ACPI_PM_TIMER_PORT));
	return value & 0x00FFFFFF;
}

/**
 * @brief ACPI PM Timerを使ってAPICタイマーをキャリブレーション
 */
static uint32_t calibrate_apic_timer(void) {
	uint32_t target_ticks = ACPI_PM_TIMER_FREQ / 100; /* 10ms */

	/* APIC Timer を分周比16、最大値で開始 */
	apic_write(APIC_TIMER_DIV, APIC_TIMER_DIV_16);
	apic_write(APIC_TIMER_INIT, 0xFFFFFFFF);

	/* ACPI PM Timerの開始値を読み取る */
	uint32_t pm_start = read_acpi_pm_timer();
	uint32_t pm_end = (pm_start + target_ticks) &
			  0x00FFFFFF; /* 24bitでラップアラウンド */

	/* ACPI PM Timerが目標値に達するまで待機 */
	uint32_t pm_current;
	if (pm_end > pm_start) {
		/* ラップアラウンドなし */
		do {
			pm_current = read_acpi_pm_timer();
		} while (pm_current < pm_end && pm_current >= pm_start);
	} else {
		/* ラップアラウンドあり */
		do {
			pm_current = read_acpi_pm_timer();
		} while (pm_current >= pm_start || pm_current < pm_end);
	}

	/* APICタイマーのカウントを読み取り */
	uint32_t apic_elapsed = 0xFFFFFFFF - apic_read(APIC_TIMER_CURRENT);

	uint32_t freq = apic_elapsed * 100;

	if (freq < 100000 || freq > 100000000) {
		/* 異常値の場合はデフォ値 */
		printk("APIC Timer: Calibration failed (freq=%u), using default\n",
		       freq);
		return 2000000; /* 2 MHz */
	}

	return freq;
}

/**
 * @brief APIC Timerを初期化
 */
int apic_timer_init(void) {
#ifdef INIT_MSG
	printk("APIC Timer: Initializing...\n");
#endif
	/* CPUのAPICサポートを確認 */
	if (!check_apic_support()) {
		printk("APIC Timer: CPU does not support APIC\n");
		return -1;
	}

	/* APICベースアドレスを取得 */
	uint32_t base_addr = get_apic_base();
	apic_base = (volatile uint32_t *)base_addr;
#ifdef INIT_MSG
	printk("APIC Timer: Base address = 0x%08x\n", base_addr);
#endif

	/* APICを有効化 (Spurious Interrupt Vector Register) */
	uint32_t spurious = apic_read(APIC_SPURIOUS);
	spurious |= (1 << 8); /* APIC Software Enable */
	spurious |= 0xFF; /* Spurious Vector = 0xFF */
	apic_write(APIC_SPURIOUS, spurious);

#ifdef INIT_MSG
	/* タイマー周波数をキャリブレーション */
	printk("APIC Timer: Calibrating frequency...\n");
#endif
	apic_timer_frequency = calibrate_apic_timer();
#ifdef INIT_MSG
	printk("APIC Timer: Frequency = %u Hz (%u MHz)\n", apic_timer_frequency,
	       apic_timer_frequency / 1000000);
#endif

	/* タイマーをPeriodic モードで設定 */
	apic_write(APIC_TIMER_DIV, APIC_TIMER_DIV_16); /* 分周比16 */

	/* ベクタ番号48、Periodicモード、割り込みマスクなし */
	uint32_t lvt = APIC_TIMER_MODE_PERIODIC | 48;
	apic_write(APIC_TIMER_LVT, lvt);
#ifdef INIT_MSG
	printk("APIC Timer: LVT configured = 0x%08x\n", lvt);
#endif
	/* 1000Hz (1ms間隔) で動作するように設定 */
	uint32_t init_count = apic_timer_frequency / 1000; /* 1000Hz */
	apic_write(APIC_TIMER_INIT, init_count);

#ifdef INIT_MSG
	printk("APIC Timer: Running at 1000 Hz (initial count = %u)\n",
	       init_count);
#endif
	boot_tick_count = 0;
	current_tick_count = 0;
	apic_initialized = 1;

#ifdef INIT_MSG
	printk("APIC Timer: Initialization complete\n");
#endif
	return 0;
}

/**
 * @brief APIC Timerが利用可能かチェック
 */
int apic_timer_available(void) {
	return apic_initialized;
}

/**
 * @brief タイマー割り込みハンドラ (1ms毎に呼ばれる)
 * 
 * @param irq 割り込み番号
 * @param context コンテキスト（未使用）
 */
void apic_timer_tick(uint32_t irq, void *context) {
	(void)irq;
	(void)context;

	if (!apic_initialized)
		return;

	current_tick_count++;

	/* EOI (End of Interrupt) を送信 */
	if (apic_base) {
		apic_write(APIC_EOI, 0);
	}
}

/**
 * @brief システム起動からの経過時間をマイクロ秒で取得
 */
uint64_t apic_get_uptime_us(void) {
	if (!apic_initialized) {
		return 0;
	}

	/* current_tick_count は1msごとに増加 */
	uint64_t ticks = current_tick_count - boot_tick_count;

	/* ティック数をマイクロ秒に変換 (1 tick = 1ms = 1000us) */
	return ticks * 1000ULL;
}

/**
 * @brief システム起動からの経過時間をミリ秒で取得
 */
uint64_t apic_get_uptime_ms(void) {
	if (!apic_initialized) {
		return 0;
	}

	/* current_tick_count は1msごとに増加 */
	return current_tick_count - boot_tick_count;
}

/**
 * @brief APIC Timerの周波数を取得
 */
uint32_t apic_timer_get_frequency(void) {
	return apic_timer_frequency;
}

/**
 * @brief 指定マイクロ秒だけ遅延
 */
void apic_timer_delay_us(uint32_t us) {
	if (!apic_initialized)
		return;

	uint64_t start = apic_get_uptime_us();
	uint64_t target = start + us;

	while (apic_get_uptime_us() < target) {
		__asm__ volatile("pause");
	}
}

/**
 * @brief 指定ミリ秒だけ遅延
 */
void apic_timer_delay_ms(uint32_t ms) {
	apic_timer_delay_us(ms * 1000);
}
