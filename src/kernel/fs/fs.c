#include <config.h>
#include <stdint.h>
#include <stddef.h>
#include <mem/manager.h>
#include <util/console.h>
#include <fs/fs.h>
#include <fs/fat/fat12.h>
#include <fs/ext/ext2.h>

// ファイルシステムの種類
typedef enum { FS_TYPE_UNKNOWN = 0, FS_TYPE_FAT12, FS_TYPE_EXT2 } fs_type_t;

// 内部ハンドル定義
struct fs_handle {
	fs_type_t type;
	void *backend; // struct fat12_super* または struct ext2_super*
};

struct dir_handle {
	fs_type_t type;
	union {
		struct {
			struct fat12_super *sb;
			uint32_t sector_index;
			uint32_t entry_index;
			uint32_t entries_per_sector;
			uint32_t total_sectors;
			uint32_t root_sector;
		} fat12;
		struct {
			struct ext2_super *sb;
			struct ext2_inode dir_inode;
			uint32_t current_block_idx;
			uint32_t current_offset;
			uint32_t read_offset;
		} ext2;
	} u;
};

struct file_handle {
	fs_type_t type;
	union {
		struct {
			struct fat12_super *sb;
			char name[64];
			uint32_t size;
		} fat12;
		struct {
			struct ext2_super *sb;
			char name[64];
			uint32_t size;
		} ext2;
	} u;
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
 * @brief イメージが ext2 っぽいかを簡易判定する
 * @param img イメージ先頭
 * @param size イメージサイズ
 * @return 1 ext2 っぽい、0 そうでない
 */
static int detect_ext2(const uint8_t *img, size_t size) {
	if (!img || size < 2048)
		return 0;
	// スーパーブロックは1024バイトオフセット
	// マジックナンバーはオフセット56 (1024 + 56 = 1080)
	uint16_t magic = (uint16_t)img[1080] | ((uint16_t)img[1081] << 8);
	if (magic == 0xEF53)
		return 1;
	return 0;
}

/**
 * @brief ファイルシステムをマウントする（FAT12とext2をサポート）
 * @param device_image イメージ先頭へのポインタ
 * @param size イメージ長
 * @param out 成功時に fs_handle* を返すポインタ
 * @return 0 成功、負値はエラー
 */
int fs_mount(const void *device_image, size_t size, fs_handle **out) {
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

	/* ext2を先にチェック */
	if (detect_ext2(img, size)) {
		struct ext2_super *sb = NULL;
		int r = ext2_mount(device_image, size, &sb);
		if (r != 0)
			return -2;
		struct fs_handle *fh =
			(struct fs_handle *)kmalloc(sizeof(struct fs_handle));
		if (!fh)
			return -3;
		fh->type = FS_TYPE_EXT2;
		fh->backend = (void *)sb;
		*out = fh;
		return 0;
	}

	/* FAT12をチェック */
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
	} else if (h->type == FS_TYPE_EXT2) {
		struct ext2_super *sb = (struct ext2_super *)h->backend;
		if (sb)
			kfree(sb);
	}
	kfree(h);
	return 0;
}

/**
 * @brief ディレクトリを開く（パス対応）
 */
int fs_opendir(fs_handle *h, const char *path, dir_handle **out) {
	if (!h || !out)
		return -1;

	struct dir_handle *d =
		(struct dir_handle *)kmalloc(sizeof(struct dir_handle));
	if (!d)
		return -4;

	d->type = h->type;

	if (h->type == FS_TYPE_FAT12) {
		/* FAT12はルートディレクトリのみ対応 */
		struct fat12_super *sb = (struct fat12_super *)h->backend;
		if (!sb) {
			kfree(d);
			return -3;
		}
		d->u.fat12.sb = sb;
		d->u.fat12.root_sector = sb->root_dir_sector;
		d->u.fat12.entries_per_sector = sb->bytes_per_sector / 32;
		d->u.fat12.total_sectors = (sb->max_root_entries +
					    d->u.fat12.entries_per_sector - 1) /
					   d->u.fat12.entries_per_sector;
		d->u.fat12.sector_index = 0;
		d->u.fat12.entry_index = 0;
	} else if (h->type == FS_TYPE_EXT2) {
		/* ext2は完全なパス対応 */
		struct ext2_super *sb = (struct ext2_super *)h->backend;
		if (!sb) {
			kfree(d);
			return -3;
		}
		d->u.ext2.sb = sb;

		/* パスが指定されていない、または "/" の場合はルート */
		uint32_t dir_inode_num = EXT2_ROOT_INO;
		if (path && path[0] != '\0' &&
		    !(path[0] == '/' && path[1] == '\0')) {
			/* パスからinode番号を解決 */
			int r = ext2_resolve_path(sb, path, &dir_inode_num);
			if (r != 0) {
				kfree(d);
				return r;
			}
		}

		/* ディレクトリinodeを読み取る */
		int r = ext2_read_inode(sb, dir_inode_num,
					&d->u.ext2.dir_inode);
		if (r != 0) {
			kfree(d);
			return r;
		}

		/* ディレクトリかどうか確認 */
		if ((d->u.ext2.dir_inode.i_mode & 0xF000) != EXT2_S_IFDIR) {
			kfree(d);
			return -5; /* ディレクトリではない */
		}

		d->u.ext2.current_block_idx = 0;
		d->u.ext2.current_offset = 0;
		d->u.ext2.read_offset = 0;
	} else {
		kfree(d);
		return -2;
	}

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

	if (d->type == FS_TYPE_FAT12) {
		struct fat12_super *sb = d->u.fat12.sb;
		while (d->u.fat12.sector_index < d->u.fat12.total_sectors) {
			const uint8_t *sec =
				sb->image + (d->u.fat12.root_sector +
					     d->u.fat12.sector_index) *
						    sb->bytes_per_sector;
			while (d->u.fat12.entry_index <
			       d->u.fat12.entries_per_sector) {
				const uint8_t *ent =
					sec + d->u.fat12.entry_index * 32;
				d->u.fat12.entry_index++;
				if (ent[0] == 0x00)
					return 1; // 終端
				if (ent[0] == 0xE5)
					continue; // 削除
				uint8_t attr = ent[11];
				if (attr & 0x08)
					continue; // ボリュームラベル
				// 名前構築（簡易）
				int ni = 0;
				for (int i = 0; i < 8 && ni < name_len - 1;
				     i++) {
					char c = ent[i];
					if (c == ' ')
						break;
					name[ni++] = c;
				}
				if (ent[8] != ' ' && ni < name_len - 1) {
					name[ni++] = '.';
					for (int i = 0;
					     i < 3 && ni < name_len - 1; i++) {
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
					*start_cluster =
						(uint16_t)ent[26] |
						((uint16_t)ent[27] << 8);
				return 0;
			}
			d->u.fat12.entry_index = 0;
			d->u.fat12.sector_index++;
		}
		return 1; // 終端
	} else if (d->type == FS_TYPE_EXT2) {
		struct ext2_super *sb = d->u.ext2.sb;
		struct ext2_inode *dir_inode = &d->u.ext2.dir_inode;
		uint32_t dir_size = dir_inode->i_size;

		/* すべてのエントリを読み終わった */
		if (d->u.ext2.read_offset >= dir_size)
			return 1;

		/* 間接ブロックポインタ対応 */
		while (d->u.ext2.read_offset < dir_size) {
			/* ブロック番号を取得（ext2_get_block_numを使用） */
			uint32_t block_num;
			int r = ext2_get_block_num(sb, dir_inode,
						   d->u.ext2.current_block_idx,
						   &block_num);
			if (r != 0 || block_num == 0)
				return 1; /* 終端 */

			uint32_t block_offset = block_num * sb->block_size;

			if (block_offset + sb->block_size > sb->image_size)
				return -2;

			const uint8_t *block_data = sb->image + block_offset;

			while (d->u.ext2.current_offset < sb->block_size &&
			       d->u.ext2.read_offset < dir_size) {
				const uint8_t *entry =
					block_data + d->u.ext2.current_offset;
				uint32_t inode = (uint32_t)entry[0] |
						 ((uint32_t)entry[1] << 8) |
						 ((uint32_t)entry[2] << 16) |
						 ((uint32_t)entry[3] << 24);
				uint16_t rec_len = (uint16_t)entry[4] |
						   ((uint16_t)entry[5] << 8);
				uint8_t name_len_val = entry[6];
				uint8_t file_type = entry[7];

				if (rec_len == 0)
					return 1; // 終端

				d->u.ext2.current_offset += rec_len;
				d->u.ext2.read_offset += rec_len;

				if (inode != 0 && name_len_val > 0) {
					/* 名前をコピー */
					int copy_len =
						(name_len_val <
						 (uint8_t)(name_len - 1)) ?
							name_len_val :
							(name_len - 1);
					for (int i = 0; i < copy_len; i++) {
						name[i] = entry[8 + i];
					}
					name[copy_len] = '\0';

					if (is_dir)
						*is_dir = (file_type ==
							   EXT2_FT_DIR) ?
								  1 :
								  0;

					if (size || start_cluster) {
						/* inodeを読み取ってサイズを取得 */
						struct ext2_inode file_inode;
						if (ext2_read_inode(
							    sb, inode,
							    &file_inode) == 0) {
							if (size)
								*size = file_inode
										.i_size;
							if (start_cluster)
								*start_cluster =
									0; /* ext2では使用しない */
						}
					}

					return 0;
				}
			}

			/* 次のブロックへ */
			d->u.ext2.current_block_idx++;
			d->u.ext2.current_offset = 0;
		}

		return 1; // 終端
	}

	return -1;
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

	struct file_handle *fh =
		(struct file_handle *)kmalloc(sizeof(struct file_handle));
	if (!fh)
		return -3;

	fh->type = h->type;

	if (h->type == FS_TYPE_FAT12) {
		struct fat12_super *sb = (struct fat12_super *)h->backend;
		int i = 0;
		for (; i < (int)sizeof(fh->u.fat12.name) - 1 && path[i]; i++)
			fh->u.fat12.name[i] = path[i];
		fh->u.fat12.name[i] = '\0';
		fh->u.fat12.sb = sb;
		fh->u.fat12.size = 0;
	} else if (h->type == FS_TYPE_EXT2) {
		struct ext2_super *sb = (struct ext2_super *)h->backend;
		int i = 0;
		for (; i < (int)sizeof(fh->u.ext2.name) - 1 && path[i]; i++)
			fh->u.ext2.name[i] = path[i];
		fh->u.ext2.name[i] = '\0';
		fh->u.ext2.sb = sb;
		fh->u.ext2.size = 0;
	} else {
		kfree(fh);
		return -2;
	}

	*out = fh;
	return 0;
}

int fs_read(file_handle *f, void *buf, uint32_t len, uint32_t offset) {
	if (!f || !buf)
		return -1;

	size_t out_len = 0;
	int r = -1;

	if (f->type == FS_TYPE_FAT12) {
		/* FAT12はオフセット0のみ対応 */
		if (offset != 0)
			return -2;
		r = fat12_read_file(f->u.fat12.sb, f->u.fat12.name, buf, len,
				    &out_len);
		if (r == 0) {
			f->u.fat12.size = out_len;
			return (int)out_len;
		}
	} else if (f->type == FS_TYPE_EXT2) {
		/* ext2は完全なオフセット対応 */
		r = ext2_read_file_by_path(f->u.ext2.sb, f->u.ext2.name, buf,
					   len, offset, &out_len);
		if (r == 0) {
			f->u.ext2.size = out_len;
			return (int)out_len;
		}
	}

	return -3;
}

int fs_close(file_handle *f) {
	if (!f)
		return -1;
	kfree(f);
	return 0;
}
