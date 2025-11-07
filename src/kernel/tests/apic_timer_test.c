#include <tests/define.h>

#ifdef APIC_TIMER_TEST

#include <config.h>
#include <driver/timer/apic.h>
#include <util/console.h>

/**
 * @brief 簡易的な遅延関数（ビジーループ）
 */
static void busy_wait(uint32_t loops) {
	extern void interrupt_dispatch_all(void);
	for (volatile uint32_t i = 0; i < loops; i++) {
		__asm__ volatile("nop");
		/* 100回に1回、割り込みイベントをディスパッチ */
		if (i % 100 == 0) {
			interrupt_dispatch_all();
		}
	}
}

/**
 * @brief APIC Timerのテスト
 */
void apic_timer_test(void) {

	if (!apic_timer_available()) {
		printk("[FAIL] APIC timer is not available\n");
		printk("  (CPU may not support APIC or initialization failed)\n");
		return;
	}
	printk("[PASS] APIC timer is available\n");

	printk("\n--- Timer Frequency ---\n");
	uint32_t freq = apic_timer_get_frequency();
	printk("APIC Timer frequency: %u Hz (%u MHz)\n", freq, freq / 1000000);

	printk("\n--- Uptime Test ---\n");
	uint64_t uptime_us1 = apic_get_uptime_us();
	uint64_t uptime_ms1 = apic_get_uptime_ms();

	printk("Uptime #1: %u ms (%u us)\n", (uint32_t)uptime_ms1,
	       (uint32_t)uptime_us1);

	busy_wait(5000000); // 待機

	uint64_t uptime_us2 = apic_get_uptime_us();
	uint64_t uptime_ms2 = apic_get_uptime_ms();

	printk("Uptime #2: %u ms (%u us)\n", (uint32_t)uptime_ms2,
	       (uint32_t)uptime_us2);

	uint32_t delta_us = (uint32_t)(uptime_us2 - uptime_us1);
	uint32_t delta_ms = (uint32_t)(uptime_ms2 - uptime_ms1);

	printk("Delta: %u ms (%u us)\n", delta_ms, delta_us);

	if (uptime_us2 > uptime_us1 && uptime_ms2 >= uptime_ms1) {
		printk("[PASS] Uptime is incrementing correctly\n");
	} else {
		printk("[FAIL] Uptime is not incrementing properly\n");
	}

	printk("\n--- Continuous Read Test ---\n");
	printk("Reading timer 10 times:\n");
	uint64_t prev_us = apic_get_uptime_us();
	int monotonic = 1;

	for (int i = 0; i < 10; i++) {
		busy_wait(500000);
		uint64_t curr_us = apic_get_uptime_us();
		uint32_t delta = (uint32_t)(curr_us - prev_us);
		
		printk("  [%d] %u us (delta: %u us)\n", i, (uint32_t)curr_us, delta);

		if (curr_us <= prev_us) {
			monotonic = 0;
		}
		prev_us = curr_us;
	}

	if (monotonic) {
		printk("[PASS] Timer values are monotonically increasing\n");
	} else {
		printk("[FAIL] Non-monotonic timer values detected\n");
	}

	printk("\n--- Tick Frequency Test ---\n");
	printk("Measuring tick frequency over 1 second (busy_wait)...\n");

	uint64_t start_ms = apic_get_uptime_ms();
	busy_wait(10000000); // 約1秒相当
	uint64_t end_ms = apic_get_uptime_ms();

	uint32_t elapsed_ms = (uint32_t)(end_ms - start_ms);
	printk("Elapsed time: %u ms\n", elapsed_ms);

	if (elapsed_ms > 0 && elapsed_ms < 10000) {
		printk("[PASS] Timer is measuring time correctly\n");
		printk("  (Expected ~500-2000ms depending on CPU speed)\n");
	} else {
		printk("[WARN] Unexpected elapsed time\n");
	}

	printk("\n--- Consistency Check ---\n");
	uint64_t us = apic_get_uptime_us();
	uint64_t ms = apic_get_uptime_ms();
	
	// 32bitにキャスト
	uint32_t us_low = (uint32_t)us;
	uint32_t ms_from_us = us_low / 1000UL;
	uint32_t ms_direct = (uint32_t)ms;
	
	printk("Uptime: %u ms (direct), %u ms (from us)\n", ms_direct, ms_from_us);
	
	int32_t diff = ms_direct - ms_from_us;
	if (diff < 0) diff = -diff;
	
	if (diff <= 1) {
		printk("[PASS] us and ms values are consistent (diff=%d ms)\n", diff);
	} else {
		printk("[FAIL] us and ms values are inconsistent (diff=%d ms)\n", diff);
	}
}

#endif /* APIC_TIMER_TEST */
