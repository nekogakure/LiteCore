#ifndef UEFI_TIMER_H
#define UEFI_TIMER_H

#include <stdint.h>

/**
 * @brief UEFI環境用のタイマー初期化
 * @return 0: 成功, -1: 失敗
 */
int uefi_timer_init(void);

/**
 * @brief タイマー割り込みハンドラ
 */
void uefi_timer_tick(uint32_t irq, void *context);

/**
 * @brief ミリ秒単位のスリープ
 * @param ms スリープ時間（ミリ秒）
 */
void uefi_sleep_ms(uint32_t ms);

/**
 * @brief 起動からの経過時間を取得（ミリ秒）
 * @return 経過時間（ミリ秒）
 */
uint64_t uefi_get_uptime_ms(void);

/**
 * @brief マイクロ秒単位の待機
 * @param us 待機時間（マイクロ秒）
 */
void uefi_wait_us(uint32_t us);

#endif // UEFI_TIMER_H
