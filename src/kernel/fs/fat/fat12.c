
#include <stdint.h>
#include <stddef.h>
#include <util/console.h>
#include <mem/manager.h>
#include <fs/fat/fat12.h>

// カーネル環境向け
static void mem_copy(void *dst, const void *src, size_t n) {
	uint8_t *d = (uint8_t *)dst;
	const uint8_t *s = (const uint8_t *)src;
	for (size_t i = 0; i < n; i++)
		d[i] = s[i];
}
static void mem_set(void *dst, int v, size_t n) {
	uint8_t *d = (uint8_t *)dst;
	for (size_t i = 0; i < n; i++)
		d[i] = (uint8_t)v;
}

// リトルエンディアン値を読み取るヘルパー関数
static uint16_t le16(const uint8_t *p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t le32(const uint8_t *p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

/**
 * @brief FAT12 イメージをマウントして super 情報を初期化する
 *
 * @param image マウントするイメージの先頭アドレス
 * @param size イメージのバイト長
 * @param out 成功時に初期化された struct fat12_super* を返すポインタ
 * @return 0 成功
 * @return 負値 エラー（短いイメージ、メモリ不足、未対応フォーマット等）
 */
int fat12_mount(const void *image, size_t size, struct fat12_super **out) {
	if (!image || size < 512)
		return -1; // イメージが小さすぎる
	const uint8_t *img = (const uint8_t *)image;
	struct fat12_super *sb = (struct fat12_super *)img; // 一時的に使用

	uint16_t bytes_per_sector = le16(img + 11);
	uint8_t sectors_per_cluster = img[13];
	uint16_t reserved = le16(img + 14);
	uint8_t num_fats = img[16];
	uint16_t max_root = le16(img + 17);
	uint16_t total_sectors_short = le16(img + 19);
	uint32_t total_sectors = total_sectors_short ? total_sectors_short :
						       le32(img + 32);
	uint16_t fat_size_sectors = le16(img + 22);
	if (!fat_size_sectors)
		fat_size_sectors = (uint16_t)le32(img + 36);

	// 最小限の検証: セクタサイズは 512 固定を期待
	if (bytes_per_sector != 512)
		return -2;
	sb = (struct fat12_super *)kmalloc(sizeof(struct fat12_super));
	if (!sb)
		return -3; // メモリ確保失敗
	sb->bytes_per_sector = bytes_per_sector;
	sb->sectors_per_cluster = sectors_per_cluster;
	sb->reserved_sectors = reserved;
	sb->num_fats = num_fats;
	sb->max_root_entries = max_root;
	sb->total_sectors = total_sectors;
	sb->fat_size_sectors = fat_size_sectors;
	sb->image = img;
	sb->image_size = size;

	uint32_t root_dir_sectors =
		((max_root * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
	uint32_t first_data_sector =
		reserved + (num_fats * fat_size_sectors) + root_dir_sectors;
	uint32_t root_dir_sector = reserved + (num_fats * fat_size_sectors);
	sb->first_data_sector = first_data_sector;
	sb->root_dir_sector = root_dir_sector;

	*out = sb;
	return 0;
}

/**
 * @brief ルートディレクトリのエントリをコンソールに出力する（簡易デバッグ用）
 *
 * @param sb マウント済みの super 構造体
 * @return 0 成功、負値はエラー
 */
int fat12_list_root(struct fat12_super *sb) {
	if (!sb)
		return -1;
	const uint8_t *img = sb->image;
	uint32_t root_sector = sb->root_dir_sector;
	uint32_t max_entries = sb->max_root_entries;
	uint32_t entries_per_sector = sb->bytes_per_sector / 32;
	uint32_t sectors =
		(max_entries + entries_per_sector - 1) / entries_per_sector;
	char name[13];
	for (uint32_t s = 0; s < sectors; s++) {
		const uint8_t *sec =
			img + (root_sector + s) * sb->bytes_per_sector;
		for (uint32_t e = 0; e < entries_per_sector; e++) {
			const uint8_t *ent = sec + e * 32;
			if (ent[0] == 0x00)
				return 0; // 以降エントリ無し
			if (ent[0] == 0xE5)
				continue; // 削除済みエントリ
			uint8_t attr = ent[11];
			if (attr & 0x08)
				continue; // ボリュームラベルは無視
			// 8.3 名の組み立て
			mem_set(name, 0, sizeof(name));
			int ni = 0;
			for (int i = 0; i < 8; i++) {
				char c = ent[i];
				if (c == ' ')
					break;
				name[ni++] = c;
			}
			if (ent[8] != ' ') {
				name[ni++] = '.';
				for (int i = 0; i < 3; i++) {
					char c = ent[8 + i];
					if (c == ' ')
						break;
					name[ni++] = c;
				}
			}
			uint32_t file_size = le32(ent + 28);
			printk("%s  (%u bytes)\n", name, file_size);
		}
	}
	return 0;
}

/**
 * @brief ルートディレクトリ内の短名（8.3）で指定したファイルを読み出す
 *
 * 注: 簡易実装のため、断片化や複数クラスタにまたがるファイルの追跡は未実装。
 * 最初のクラスタのみを読み取る想定。
 *
 * @param sb マウント済みの super 構造体
 * @param name 読み出すファイル名（例: "EXAMPLE.TXT" または "example.txt"）
 * @param buf データ格納先バッファ
 * @param len バッファ長
 * @param out_len 実際に読み取ったバイト数を返すポインタ（NULL 可）
 * @return 0 成功、負値はエラー
 */
int fat12_read_file(struct fat12_super *sb, const char *name, void *buf,
		    size_t len, size_t *out_len) {
	if (!sb || !name)
		return -1;
	const uint8_t *img = sb->image;
	uint32_t root_sector = sb->root_dir_sector;
	uint32_t max_entries = sb->max_root_entries;
	uint32_t entries_per_sector = sb->bytes_per_sector / 32;
	uint32_t sectors =
		(max_entries + entries_per_sector - 1) / entries_per_sector;
	char shortname[12];
	mem_set(shortname, ' ', 11);
	shortname[11] = '\0';
	// 入力名を 8.3 の大文字形式に変換（ドットは無視）
	int si = 0;
	for (int i = 0; i < 11 && name[si]; i++) {
		char c = name[si++];
		if (c == '.')
			continue;
		if (c >= 'a' && c <= 'z')
			c = c - 'a' + 'A';
		shortname[i] = c;
	}

	for (uint32_t s = 0; s < sectors; s++) {
		const uint8_t *sec =
			img + (root_sector + s) * sb->bytes_per_sector;
		for (uint32_t e = 0; e < entries_per_sector; e++) {
			const uint8_t *ent = sec + e * 32;
			if (ent[0] == 0x00)
				return -2; // 見つからない
			if (ent[0] == 0xE5)
				continue; // 削除済み
			uint8_t attr = ent[11];
			if (attr & 0x08)
				continue; // ボリュームラベルは無視
			// 名前比較
			int match = 1;
			for (int i = 0; i < 11; i++) {
				char a = ent[i];
				char b = shortname[i];
				if (a != b) {
					match = 0;
					break;
				}
			}
			if (!match)
				continue;
			uint16_t start_cluster = le16(ent + 26);
			uint32_t file_size = le32(ent + 28);
			// FAT チェーン追跡は行わず、最初のクラスタのみ読む簡易実装
			uint32_t bytes_to_read = file_size;
			if (bytes_to_read > len)
				bytes_to_read = len;
			uint32_t first_data_sector = sb->first_data_sector;
			uint32_t sectors_per_cluster = sb->sectors_per_cluster;
			uint32_t sector =
				first_data_sector +
				(start_cluster - 2) * sectors_per_cluster;
			uint8_t *dest = (uint8_t *)buf;
			const uint8_t *src =
				img + sector * sb->bytes_per_sector;
			uint32_t copy = (bytes_to_read > sb->bytes_per_sector) ?
						sb->bytes_per_sector :
						bytes_to_read;
			mem_copy(dest, src, copy);
			if (out_len)
				*out_len = copy;
			return 0;
		}
	}
	return -3;
}
