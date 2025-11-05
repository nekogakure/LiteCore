#include <config.h>
#include <stdint.h>
#include <stddef.h>
#include <mem/manager.h>
#include <util/console.h>
#include <fs/fs.h>
#include <fs/fat/fat12.h>

// ファイルシステムの種類
typedef enum { FS_TYPE_UNKNOWN = 0, FS_TYPE_FAT12 } fs_type_t;

// 内部ハンドル定義
struct fs_handle {
	fs_type_t type;
	void *backend; // 例えば struct fat12_super*
};

struct dir_handle {
	struct fat12_super *sb;
	uint32_t sector_index;
	uint32_t entry_index;
	uint32_t entries_per_sector;
	uint32_t total_sectors;
	uint32_t root_sector;
};

struct file_handle {
	struct fat12_super *sb;
	char name[64];
	uint32_t size;
};

/**
 * @brief イメージが FAT12 っぽいかを簡易判定する
 * @param img イメージ先頭
 * @param size イメージサイズ
 * @return 1 FAT12 っぽい、0 そうでない
 */
static int detect_fat12(const uint8_t *img, size_t size) {
	if (!img || size < 512)
		return 0;
	// ブートセクタのシグネチャ 0x55AA をチェック
	if (img[510] != 0x55 || img[511] != 0xAA)
		return 0;
	// bytes per sector はオフセット11..12
	uint16_t bps = (uint16_t)img[11] | ((uint16_t)img[12] << 8);
	if (bps != 512)
		return 0; // 当面 512B 固定を期待
	return 1;
}

/**
 * @brief ファイルシステムをマウントする（現在は FAT12 のみサポート）
 * @param device_image イメージ先頭へのポインタ
 * @param size イメージ長
 * @param out 成功時に fs_handle* を返すポインタ
 * @return 0 成功、負値はエラー
 */
int fs_mount(const void *device_image, size_t size, fs_handle **out) {
	// デバッグ出力: 引数の状態を表示して早期リターンの原因を特定
	printk("fs_mount called: device_image=%p size=%u out=%p\n",
	       device_image, (unsigned)size, out);
	if (!device_image || size == 0 || !out) {
		if (!device_image)
			printk("fs_mount: device_image is NULL\n");
		if (size == 0)
			printk("fs_mount: size is 0\n");
		if (!out)
			printk("fs_mount: out is NULL\n");
		return -1;
	}
	const uint8_t *img = (const uint8_t *)device_image;
	if (detect_fat12(img, size)) {
		struct fat12_super *sb = NULL;
		int r = fat12_mount(device_image, size, &sb);
		if (r != 0)
			return -2;
		struct fs_handle *fh =
			(struct fs_handle *)kmalloc(sizeof(struct fs_handle));
		if (!fh)
			return -3;
		fh->type = FS_TYPE_FAT12;
		fh->backend = (void *)sb;
		*out = fh;
		return 0;
	}
	return -4; // 不明なフォーマット
}

/**
 * @brief アンマウントしてリソースを解放する
 */
int fs_unmount(fs_handle *h) {
	if (!h)
		return -1;
	if (h->type == FS_TYPE_FAT12) {
		struct fat12_super *sb = (struct fat12_super *)h->backend;
		if (sb)
			kfree(sb);
	}
	kfree(h);
	return 0;
}

/**
 * @brief ルートディレクトリを開く（path 未対応で root のみ）
 */
int fs_opendir(fs_handle *h, const char *path, dir_handle **out) {
	(void)path;
	if (!h || !out)
		return -1;
	if (h->type != FS_TYPE_FAT12)
		return -2;
	struct fat12_super *sb = (struct fat12_super *)h->backend;
	if (!sb)
		return -3;
	struct dir_handle *d =
		(struct dir_handle *)kmalloc(sizeof(struct dir_handle));
	if (!d)
		return -4;
	d->sb = sb;
	d->root_sector = sb->root_dir_sector;
	d->entries_per_sector = sb->bytes_per_sector / 32;
	d->total_sectors = (sb->max_root_entries + d->entries_per_sector - 1) /
			   d->entries_per_sector;
	d->sector_index = 0;
	d->entry_index = 0;
	*out = d;
	return 0;
}

/**
 * @brief ルートディレクトリを列挙する（name バッファに短名を返す）
 */
int fs_readdir(dir_handle *d, char *name, int name_len, int *is_dir,
	       uint32_t *size, uint16_t *start_cluster) {
	if (!d || !name)
		return -1;
	struct fat12_super *sb = d->sb;
	while (d->sector_index < d->total_sectors) {
		const uint8_t *sec =
			sb->image + (d->root_sector + d->sector_index) *
					    sb->bytes_per_sector;
		while (d->entry_index < d->entries_per_sector) {
			const uint8_t *ent = sec + d->entry_index * 32;
			d->entry_index++;
			if (ent[0] == 0x00)
				return 1; // 終端（0 を返すより区別のため1を返す）
			if (ent[0] == 0xE5)
				continue; // 削除
			uint8_t attr = ent[11];
			if (attr & 0x08)
				continue; // ボリュームラベル
			// 名前構築（簡易）
			int ni = 0;
			for (int i = 0; i < 8 && ni < name_len - 1; i++) {
				char c = ent[i];
				if (c == ' ')
					break;
				name[ni++] = c;
			}
			if (ent[8] != ' ' && ni < name_len - 1) {
				name[ni++] = '.';
				for (int i = 0; i < 3 && ni < name_len - 1;
				     i++) {
					char c = ent[8 + i];
					if (c == ' ')
						break;
					name[ni++] = c;
				}
			}
			name[ni] = '\0';
			if (is_dir)
				*is_dir = (attr & 0x10) ? 1 : 0;
			if (size)
				*size = (uint32_t)ent[28] |
					((uint32_t)ent[29] << 8) |
					((uint32_t)ent[30] << 16) |
					((uint32_t)ent[31] << 24);
			if (start_cluster)
				*start_cluster = (uint16_t)ent[26] |
						 ((uint16_t)ent[27] << 8);
			return 0;
		}
		d->entry_index = 0;
		d->sector_index++;
	}
	return 1; // 終端
}

int fs_closedir(dir_handle *d) {
	if (!d)
		return -1;
	kfree(d);
	return 0;
}

int fs_open(fs_handle *h, const char *path, file_handle **out) {
	if (!h || !path || !out)
		return -1;
	if (h->type != FS_TYPE_FAT12)
		return -2;
	struct fat12_super *sb = (struct fat12_super *)h->backend;
	struct file_handle *fh =
		(struct file_handle *)kmalloc(sizeof(struct file_handle));
	if (!fh)
		return -3;
	// 単純に名前を保存しておく
	int i = 0;
	for (; i < (int)sizeof(fh->name) - 1 && path[i]; i++)
		fh->name[i] = path[i];
	fh->name[i] = '\0';
	fh->sb = sb;
	fh->size = 0; // サイズは fs_read の際に取得する
	*out = fh;
	return 0;
}

int fs_read(file_handle *f, void *buf, uint32_t len, uint32_t offset) {
	if (!f || !buf)
		return -1;
	// 簡易実装: offset must be 0
	if (offset != 0)
		return -2;
	size_t out_len = 0;
	int r = fat12_read_file(f->sb, f->name, buf, len, &out_len);
	if (r == 0) {
		f->size = out_len;
		return (int)out_len;
	}
	return -3;
}

int fs_close(file_handle *f) {
	if (!f)
		return -1;
	kfree(f);
	return 0;
}
