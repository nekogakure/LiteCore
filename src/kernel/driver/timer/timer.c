#include <driver/timer/apic.h>
#include <interrupt/irq.h>
#include <util/console.h>

/**
 * @brief 指定されたマイクロ秒だけ待つ
 * @param us 待機時間（マイクロ秒）
 * @return 0 成功, -1 APICタイマー未初期化
 */
int kwait(uint32_t us) {
	if (!apic_timer_available()) {
		return -1;
	}

	uint64_t start = apic_get_uptime_us();
	uint64_t target = start + us;

	while (apic_get_uptime_us() < target) {
		interrupt_dispatch_all();
	}

	return 0;
}