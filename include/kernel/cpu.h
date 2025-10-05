/**
 * @file cpu.h
 * @brief CPU制御関数の宣言
 * @details x86-64 CPU制御機能（インラインアセンブリ使用）
 */

#ifndef LITECORE_KERNEL_CPU_H
#define LITECORE_KERNEL_CPU_H

#include "types.h"

/**
 * @brief CPUの割り込みを無効化
 */
void cpu_disable_interrupts(void);

/**
 * @brief CPUの割り込みを有効化
 */
void cpu_enable_interrupts(void);

/**
 * @brief CPUを停止状態にする
 */
void cpu_halt(void);

/**
 * @brief 指定したポートから8ビット値を読み取り
 * @param port ポート番号
 * @return 読み取った値
 */
uint8_t cpu_inb(uint16_t port);

/**
 * @brief 指定したポートに8ビット値を書き込み
 * @param port ポート番号
 * @param value 書き込む値
 */
void cpu_outb(uint16_t port, uint8_t value);

#endif /* LITECORE_KERNEL_CPU_H */