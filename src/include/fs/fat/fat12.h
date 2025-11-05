#ifndef _FAT12_H
#define _FAT12_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief FAT12 のマウント情報を保持する構造体
 *
 * image は埋め込みイメージへのポインタを保持する。
 */
struct fat12_super {
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t num_fats;
	uint16_t max_root_entries;
	uint32_t total_sectors;
	uint32_t fat_size_sectors;
	uint32_t first_data_sector;
	uint32_t root_dir_sector;
	const uint8_t *image;
	size_t image_size;
};

/**
 * @brief FAT12 イメージをマウントする
 * @param image イメージ先頭へのポインタ
 * @param size イメージ長（バイト）
 * @param out マウント結果を返すポインタ
 * @return 0 成功、負値はエラー
 */
int fat12_mount(const void *image, size_t size, struct fat12_super **out);

/**
 * @brief ルートディレクトリの内容をデバッグ表示する
 * @param sb マウント済み super
 * @return 0 成功、負値はエラー
 */
int fat12_list_root(struct fat12_super *sb);

/**
 * @brief ルート内の短名で指定したファイルを読み出す（簡易実装）
 * @param sb マウント済み super
 * @param name 読み出すファイル名（例: "EXAMPLE.TXT"）
 * @param buf データ格納先
 * @param len バッファ長
 * @param out_len 実際に読み取ったバイト数を返す（NULL 可）
 * @return 0 成功、負値はエラー
 */
int fat12_read_file(struct fat12_super *sb, const char *name, void *buf,
		    size_t len, size_t *out_len);

#endif
