#ifndef _FONT_H
#define _FONT_H

#include <stdint.h>

/**
 * @brief 8x16 ASCII フォントデータ
 * 文字コード 32-126 (95文字) に対応
 * 各文字は16バイト (16行 x 8ピクセル)
 */
extern const uint8_t font_8x16[95][16];

#endif /* _FONT_H */

