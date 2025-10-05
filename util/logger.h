/**
 * @file logger.h
 * @brief カーネル用ログ出力機能
 * @details シリアルポートを使用したデバッグ出力
 */

#ifndef LITECORE_UTIL_LOGGER_H
#define LITECORE_UTIL_LOGGER_H

#include <stdint.h>
#include <stdarg.h>

/**
 * @brief シリアルポートを初期化
 */
void logger_init(void);

/**
 * @brief シリアルポートに1文字出力
 * @param ch 出力する文字
 */
void logger_putchar(char ch);

/**
 * @brief シリアルポートに文字列出力
 * @param str 出力する文字列
 */
void logger_puts(const char* str);

/**
 * @brief printf風のフォーマット付き出力
 * @param format フォーマット文字列
 * @param ... 可変引数
 */
void logger_print(const char* format, ...);

/**
 * @brief デバッグマクロ
 */
#ifdef DEBUG
#define debug(fmt, ...) logger_print("[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...) ((void)0)
#endif

#endif /* LITECORE_UTIL_LOGGER_H */