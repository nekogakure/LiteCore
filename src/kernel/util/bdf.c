#include <fs/ext/ext2.h>
#include <mem/manager.h>
#include <util/bdf.h>
#include <util/io.h>

// グローバルext2ハンドル
extern struct ext2_super *g_ext2_sb;

static bdf_font_t font;
static int initialized = 0;

/**
 * @brief 文字列を整数に変換
 */
static int str_to_int(const char *str) {
	int result = 0;
	int sign = 1;

	if (*str == '-') {
		sign = -1;
		str++;
	}

	while (*str >= '0' && *str <= '9') {
		result = result * 10 + (*str - '0');
		str++;
	}

	return result * sign;
}

/**
 * @brief 16進数文字列を整数に変換
 */
static uint32_t hex_to_int(const char *str) {
	uint32_t result = 0;

	while (*str) {
		result *= 16;
		if (*str >= '0' && *str <= '9') {
			result += *str - '0';
		} else if (*str >= 'A' && *str <= 'F') {
			result += *str - 'A' + 10;
		} else if (*str >= 'a' && *str <= 'f') {
			result += *str - 'a' + 10;
		}
		str++;
	}

	return result;
}

/**
 * @brief 文字列の先頭から指定文字列と一致するか確認
 */
static int starts_with(const char *str, const char *prefix) {
	while (*prefix) {
		if (*str != *prefix) {
			return 0;
		}
		str++;
		prefix++;
	}
	return 1;
}

/**
 * @brief 空白をスキップして次の単語を取得
 */
static const char *skip_spaces(const char *str) {
	while (*str == ' ' || *str == '\t') {
		str++;
	}
	return str;
}

/**
 * @brief 次の行を探す
 */
static const char *next_line(const char *str) {
	while (*str && *str != '\n') {
		str++;
	}
	if (*str == '\n') {
		str++;
	}
	return str;
}

/**
 * @brief BDFファイルをパース
 */
static int parse_bdf(const char *data, size_t size) {
	const char *p = data;
	const char *end = data + size;

	// デフォルト値を設定
	font.width = 8;
	font.height = 16;
	font.ascent = 10;
	font.descent = 2;
	font.default_char = 32;
	font.num_glyphs = 0;

	bdf_glyph_t *current_glyph = NULL;
	int bitmap_line = 0;
	int in_bitmap = 0;

	while (p < end && *p) {
		// フォント情報のパース
		if (starts_with(p, "FONTBOUNDINGBOX")) {
			p += 15; // "FONTBOUNDINGBOX"の長さ
			p = skip_spaces(p);
			font.width = str_to_int(p);
			while (*p && *p != ' ' && *p != '\t' && *p != '\n')
				p++;
			p = skip_spaces(p);
			font.height = str_to_int(p);
		} else if (starts_with(p, "FONT_ASCENT")) {
			p += 11;
			p = skip_spaces(p);
			font.ascent = str_to_int(p);
		} else if (starts_with(p, "FONT_DESCENT")) {
			p += 12;
			p = skip_spaces(p);
			font.descent = str_to_int(p);
		} else if (starts_with(p, "DEFAULT_CHAR")) {
			p += 12;
			p = skip_spaces(p);
			font.default_char = str_to_int(p);
		}
		// グリフのパース
		else if (starts_with(p, "STARTCHAR")) {
			if (font.num_glyphs >= MAX_GLYPHS) {
				printk("BDF: Too many glyphs\n");
				return 0;
			}

			current_glyph = &font.glyphs[font.num_glyphs];
			current_glyph->width = font.width;
			current_glyph->height = font.height;
			current_glyph->x_offset = 0;
			current_glyph->y_offset = 0;
			current_glyph->encoding = 0;
			bitmap_line = 0;

			for (int zi = 0; zi < MAX_GLYPH_HEIGHT; zi++) {
				current_glyph->bitmap[zi] = 0;
			}

			in_bitmap = 0;
		} else if (starts_with(p, "ENCODING")) {
			if (current_glyph) {
				p += 8;
				p = skip_spaces(p);
				current_glyph->encoding = str_to_int(p);
			}
		} else if (starts_with(p, "BBX")) {
			if (current_glyph) {
				p += 3;
				p = skip_spaces(p);
				current_glyph->width = str_to_int(p);
				while (*p && *p != ' ' && *p != '\t' &&
				       *p != '\n')
					p++;
				p = skip_spaces(p);
				current_glyph->height = str_to_int(p);
				while (*p && *p != ' ' && *p != '\t' &&
				       *p != '\n')
					p++;
				p = skip_spaces(p);
				current_glyph->x_offset = str_to_int(p);
				while (*p && *p != ' ' && *p != '\t' &&
				       *p != '\n')
					p++;
				p = skip_spaces(p);
				current_glyph->y_offset = str_to_int(p);
			}
		} else if (starts_with(p, "BITMAP")) {
			in_bitmap = 1;
			bitmap_line = 0;
		} else if (starts_with(p, "ENDCHAR")) {
			if (current_glyph) {
				font.num_glyphs++;
				current_glyph = NULL;
			}
			in_bitmap = 0;
		} else if (in_bitmap && current_glyph) {
			p = skip_spaces(p);
			if ((*p >= '0' && *p <= '9') ||
			    (*p >= 'A' && *p <= 'F') ||
			    (*p >= 'a' && *p <= 'f')) {
				if (bitmap_line < MAX_GLYPH_HEIGHT) {
					char hex_str[9];
					int i = 0;
					while (i < (int)(sizeof(hex_str) - 1) &&
					       ((*p >= '0' && *p <= '9') ||
						(*p >= 'A' && *p <= 'F') ||
						(*p >= 'a' && *p <= 'f'))) {
						hex_str[i++] = *p++;
					}
					hex_str[i] = '\0';
					uint32_t val = hex_to_int(hex_str);
					int hex_len = i;
					int total_bits = hex_len * 4;
					int width =
						current_glyph->width ?
							current_glyph->width :
							font.width;
					if (width <= 0)
						width = font.width;
					if (total_bits > width) {
						val >>= (total_bits - width);
					}
					uint32_t mask =
						(width >= 32) ?
							0xFFFFFFFFu :
							((1u << width) - 1u);
					current_glyph->bitmap[bitmap_line++] =
						(uint16_t)(val & mask);
				}
			}
		}

		// 次の行へ
		p = next_line(p);
	}

	return 1;
}

int bdf_init(const char *path) {
	if (initialized) {
		return 1;
	}

	if (!g_ext2_sb) {
		printk("BDF: ext2 filesystem not mounted\n");
		return 0;
	}

	// パスからinode番号を解決
	uint32_t inode_num;
	int result = ext2_resolve_path(g_ext2_sb, path, &inode_num);
	if (result != 0) {
		printk("BDF: Failed to resolve path %s\n", path);
		return 0;
	}

	// inodeを読み取る
	struct ext2_inode inode;
	result = ext2_read_inode(g_ext2_sb, inode_num, &inode);
	if (result != 0) {
		printk("BDF: Failed to read inode\n");
		return 0;
	}

	// ファイルサイズを取得
	size_t file_size = inode.i_size;
	if (file_size == 0 ||
	    file_size > 1024 * 1024) { // 1MBを超える場合はエラー
		printk("BDF: Invalid file size: %u\n", (uint32_t)file_size);
		return 0;
	}

	// メモリを確保
	char *buffer = (char *)kmalloc((uint32_t)file_size + 1);
	if (!buffer) {
		printk("BDF: Failed to allocate memory\n");
		return 0;
	}

	// ファイルデータを読み込む
	size_t bytes_read = 0;
	result = ext2_read_inode_data(g_ext2_sb, &inode, buffer, file_size, 0,
				      &bytes_read);
	if (result != 0 || bytes_read == 0) {
		printk("BDF: Failed to read file data\n");
		kfree(buffer);
		return 0;
	}

	buffer[bytes_read] = '\0';

	// BDFファイルをパース
	result = parse_bdf(buffer, bytes_read);
	kfree(buffer);

	if (result) {
		initialized = 1;
#ifdef INIT_MSG
		printk("BDF: Loaded font with %u glyphs (%dx%d)\n",
		       font.num_glyphs, font.width, font.height);
#endif

		for (uint32_t gi = 0; gi < font.num_glyphs; gi++) {
			bdf_glyph_t *g = &font.glyphs[gi];
			if (g->width > 16)
				g->width = 16;
			if (g->height == 0)
				g->height = font.height;
			if (g->height > MAX_GLYPH_HEIGHT)
				g->height = MAX_GLYPH_HEIGHT;
			for (int r = g->height; r < MAX_GLYPH_HEIGHT; r++) {
				g->bitmap[r] = 0;
			}
		}

		int found_default = 0;
		for (uint32_t gi = 0; gi < font.num_glyphs; gi++) {
			if (font.glyphs[gi].encoding == font.default_char) {
				found_default = 1;
				break;
			}
		}
		if (!found_default) {
			font.default_char = 32;
		}
	}

	return result;
}

const bdf_glyph_t *bdf_get_glyph(uint32_t c) {
	if (!initialized) {
		return NULL;
	}

	// グリフを検索
	for (uint32_t i = 0; i < font.num_glyphs; i++) {
		if (font.glyphs[i].encoding == c) {
			return &font.glyphs[i];
		}
	}

	// デフォルト文字を返す
	for (uint32_t i = 0; i < font.num_glyphs; i++) {
		if (font.glyphs[i].encoding == font.default_char) {
			return &font.glyphs[i];
		}
	}

	return NULL;
}

const bdf_font_t *bdf_get_font(void) {
	if (!initialized) {
		return NULL;
	}
	return &font;
}
