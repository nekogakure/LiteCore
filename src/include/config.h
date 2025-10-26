#ifndef _CONFIG_H
#define _CONFIG_H

#define VERSION "0.1"

#ifndef NULL
#define NULL ((void*)0)
#endif

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

/**
 * @typedef uint16_t
 * @brief 16ビット符号なし整数型
 */
typedef unsigned short uint16_t;

/**
 * @typedef uint32_t
 * @brief 32bit符号なし整数型
 */
typedef unsigned int uint32_t;

#endif /* _CONFIG_H */