#include <driver/timer/uefi_timer.h>
#include <util/io.h>
#include <interrupt/irq.h>

// PITタイマー
#define PIT_FREQUENCY 1193182
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43
#define PIT_MODE 0x36 // Mode 3: Square wave generator

static volatile uint64_t timer_ticks = 0;
static uint32_t timer_frequency_hz = 100; // 100Hz (10ms間隔)

/**
 * @brief タイマー割り込みハンドラ
 */
void uefi_timer_tick(uint32_t irq, void *context) {
	(void)irq;
	(void)context;
	timer_ticks++;
}

/**
 * @brief UEFI環境用のタイマー初期化
 * PIT（Programmable Interval Timer）を使用
 */
int uefi_timer_init(void) {
	// PITの分周値を計算
	uint16_t divisor = (uint16_t)(PIT_FREQUENCY / timer_frequency_hz);

	// PITをプログラム
	outb(PIT_COMMAND, PIT_MODE);
	outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
	outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

	// PIC Master (IRQ 0-7) のマスクを読み取り
	uint8_t mask = inb(0x21);
	// IRQ 0 (PITタイマー) のマスクを解除
	mask &= ~0x01;
	outb(0x21, mask);

#ifdef INIT_MSG
	printk("UEFI Timer: Initialized with PIT at %u Hz\n",
	       timer_frequency_hz);
	printk("UEFI Timer: IRQ 0 unmasked (PIC mask = 0x%02x)\n", mask);
#endif
	return 0;
}

/**
 * @brief ミリ秒単位のスリープ
 */
void uefi_sleep_ms(uint32_t ms) {
	// タイマーベースのスリープ
	uint64_t target = timer_ticks + (ms * timer_frequency_hz / 1000);

	while (timer_ticks < target) {
		// メインループで割り込みディスパッチが行われるので、ここでは何もしない
		__asm__ volatile("hlt"); // CPUを休止
	}
}

/**
 * @brief 起動からの経過時間を取得（ミリ秒）
 */
uint64_t uefi_get_uptime_ms(void) {
	return (timer_ticks * 1000) / timer_frequency_hz;
}

/**
 * @brief マイクロ秒単位の待機（ビジーウェイト）
 * タイマーに依存せず、単純なループで待機
 */
void uefi_wait_us(uint32_t us) {
	// 簡易的なビジーウェイト
	// 1マイクロ秒あたり約100回のループ（調整済み）
	// 大きすぎる値は防ぐ（最大10ms）
	if (us > 10000) {
		us = 10000;
	}

	volatile uint32_t loops = us * 100;
	while (loops--) {
		__asm__ volatile("pause");
	}
}
