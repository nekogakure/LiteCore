/**
 * @file types.h
 * @brief カーネル基本型定義
 * @details カーネル全体で使用する基本的な型定義
 */

#ifndef LITECORE_KERNEL_TYPES_H
#define LITECORE_KERNEL_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/** @brief カーネル関数の戻り値型 */
typedef int kernel_result_t;

/** @brief 成功を示す戻り値 */
#define KERNEL_SUCCESS 0

/** @brief 一般的なエラーを示す戻り値 */
#define KERNEL_ERROR -1

/** @brief メモリ不足エラーを示す戻り値 */
#define KERNEL_ERROR_NO_MEMORY -2

/** @brief 無効な引数エラーを示す戻り値 */
#define KERNEL_ERROR_INVALID_ARG -3

#endif /* LITECORE_KERNEL_TYPES_H */