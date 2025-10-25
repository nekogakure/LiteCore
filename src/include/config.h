#ifndef _CONFIG_H
#define _CONFIG_H

/**
 * @def VIDEO_MEMORY
 * @brief VGAの開始アドレス
 */
#define VIDEO_MEMORY    (0xB8000)

/**
 * @def COLOR
 * @brief 色属性（黒背景・白文字）
 */
#define COLOR           (0x07)

/**
 * @typedef uint8_t
 * @brief 8ビット符号なし整数型
 */
typedef unsigned char uint8_t;

#endif /* _CONFIG_H */