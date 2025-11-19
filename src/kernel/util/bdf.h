#ifndef _BDF_H
#define _BDF_H

#include <stdint.h>

#define MAX_GLYPHS 1400
#define MAX_GLYPH_HEIGHT 16

/**
 * @brief BDFフォントのグリフ情報
 */
typedef struct {
	uint32_t encoding; // 文字コード
	uint8_t width; // グリフの幅
	uint8_t height; // グリフの高さ
	int8_t x_offset; // X方向のオフセット
	int8_t y_offset; // Y方向のオフセット
	uint16_t bitmap[MAX_GLYPH_HEIGHT]; // ビットマップデータ
} bdf_glyph_t;

/**
 * @brief BDFフォント情報
 */
typedef struct {
	uint8_t width; // フォントの幅
	uint8_t height; // フォントの高さ
	uint8_t ascent; // ベースラインからの上昇
	uint8_t descent; // ベースラインからの下降
	uint32_t default_char; // デフォルト文字
	uint32_t num_glyphs; // グリフの数
	bdf_glyph_t glyphs[MAX_GLYPHS]; // グリフ配列
} bdf_font_t;

int bdf_init(const char *path);
const bdf_glyph_t *bdf_get_glyph(uint32_t c);
const bdf_font_t *bdf_get_font(void);

#endif /* _BDF_H */
