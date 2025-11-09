#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdint.h>

#define VERSION "0.1"

// UEFI環境でのビルド
#define UEFI_MODE

// 初期化メッセージを表示するかどうか
#define INIT_MSG

#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * @def VIDEO_MEMORY
 * @brief VGAの開始アドレス
 */
#define VIDEO_MEMORY (0xB8000)

/**
 * @def COLOR
 * @brief 色属性（黒背景・白文字）
 */
#define COLOR (0x07)

#endif /* _CONFIG_H */