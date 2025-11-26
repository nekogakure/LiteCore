#include <stdint.h>
#include <stddef.h>
#include <util/console.h>
#include <mem/manager.h>
#include <fs/fat/fat16.h>

/* 前方参照: ルート内検索 */
static uint8_t *find_root_entry(struct fat16_super *sb, const char *name,
				uint8_t **ent_out, uint8_t **free_out);

/* 小さな helper 関数群 */
static void mem_copy(void *dst, const void *src, size_t n) {
	uint8_t *d = (uint8_t *)dst;
	const uint8_t *s = (const uint8_t *)src;
	for (size_t i = 0; i < n; ++i)
		d[i] = s[i];
}
static void mem_set(void *dst, int v, size_t n) {
	uint8_t *d = (uint8_t *)dst;
	for (size_t i = 0; i < n; ++i)
		d[i] = (uint8_t)v;
}

static uint16_t le16(const uint8_t *p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t le32(const uint8_t *p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

/* FAT テーブルの先頭バイトオフセットを返す */
static uint32_t fat_offset_bytes(struct fat16_super *sb) {
	return (uint32_t)sb->reserved_sectors * sb->bytes_per_sector;
}

/* 指定クラスタの FAT エントリ (16bit) を読み書きする */
static uint16_t fat_read_entry(struct fat16_super *sb, uint16_t cluster) {
	uint32_t off = fat_offset_bytes(sb) + cluster * 2;
	uint8_t *p = sb->image + off;
	return le16(p);
}

static void fat_write_entry(struct fat16_super *sb, uint16_t cluster,
			    uint16_t val) {
	uint32_t off = fat_offset_bytes(sb) + cluster * 2;
	/* FAT コピーが複数あるので全て更新する */
	for (uint8_t f = 0; f < sb->num_fats; ++f) {
		uint32_t fat_off = off + (uint32_t)f * sb->fat_size_sectors *
						 sb->bytes_per_sector;
		uint8_t *p = sb->image + fat_off;
		p[0] = (uint8_t)(val & 0xff);
		p[1] = (uint8_t)((val >> 8) & 0xff);
	}
}

static int allocate_chain(struct fat16_super *sb, uint16_t n,
			  uint16_t *out_start) {
	if (n == 0)
		return -1;
	uint32_t total_clusters = (sb->total_sectors - sb->first_data_sector) /
				  sb->sectors_per_cluster;
	uint16_t *list = (uint16_t *)kmalloc(sizeof(uint16_t) * n);
	if (!list)
		return -3;
	uint16_t found = 0;
	for (uint16_t c = 2; c < (uint16_t)(2 + total_clusters) && found < n;
	     ++c) {
		uint16_t e = fat_read_entry(sb, c);
		if (e == 0) {
			list[found++] = c;
		}
	}
	if (found < n) {
		kfree(list);
		return -2; /* 空き不足 */
	}
	/* 見つかったクラスタを順にリンクして書き込む */
	for (uint16_t i = 0; i < n; ++i) {
		uint16_t cur = list[i];
		uint16_t val = (i + 1 == n) ? 0xFFFF : list[i + 1];
		fat_write_entry(sb, cur, val);
	}
	*out_start = list[0];
	kfree(list);
	return 0;
}

static void make_shortname(const char *name, char out[11]) {
	/* 空白で埋める */
	for (int i = 0; i < 11; ++i)
		out[i] = ' ';
	int ni = 0;
	int si = 0;
	/* ベース名 8 バイト */
	while (name[si] && name[si] != '.' && ni < 8) {
		char c = name[si++];
		if (c >= 'a' && c <= 'z')
			c = c - 'a' + 'A';
		out[ni++] = c;
	}
	/* 拡張子 */
	if (name[si] == '.')
		si++;
	ni = 8;
	while (name[si] && ni < 11) {
		char c = name[si++];
		if (c >= 'a' && c <= 'z')
			c = c - 'a' + 'A';
		out[ni++] = c;
	}
}

static uint8_t *find_entry_in_dir(struct fat16_super *sb,
				  uint16_t start_cluster, const char *name,
				  uint8_t **ent_out, uint8_t **free_out) {
	char shortname[11];
	make_shortname(name, shortname);
	uint32_t entries_per_sector = sb->bytes_per_sector / 32;
	uint8_t *first_free = NULL;

	if (start_cluster == 0) {
		uint32_t root_sector = sb->root_dir_sector;
		uint32_t sectors =
			((sb->max_root_entries + entries_per_sector - 1) /
			 entries_per_sector);
		for (uint32_t s = 0; s < sectors; ++s) {
			uint8_t *sec = sb->image +
				       (root_sector + s) * sb->bytes_per_sector;
			for (uint32_t e = 0; e < entries_per_sector; ++e) {
				uint8_t *ent = sec + e * 32;
				if (ent[0] == 0x00) {
					if (free_out)
						*free_out = first_free ?
								    first_free :
								    ent;
					return NULL; /* end */
				}
				if (ent[0] == 0xE5) {
					if (!first_free)
						first_free = ent;
					continue;
				}
				uint8_t attr = ent[11];
				if (attr & 0x08)
					continue;
				int match = 1;
				for (int i = 0; i < 11; ++i) {
					if (ent[i] != (uint8_t)shortname[i]) {
						match = 0;
						break;
					}
				}
				if (match) {
					if (ent_out)
						*ent_out = ent;
					return ent;
				}
			}
		}
		if (free_out)
			*free_out = first_free;
		return NULL;
	} else {
		uint16_t cur = start_cluster;
		while (cur >= 2 && cur < 0xFFF8) {
			uint32_t sector = sb->first_data_sector +
					  (cur - 2) * sb->sectors_per_cluster;
			for (uint8_t sc = 0; sc < sb->sectors_per_cluster;
			     ++sc) {
				uint8_t *sec =
					sb->image +
					(sector + sc) * sb->bytes_per_sector;
				for (uint32_t e = 0; e < entries_per_sector;
				     ++e) {
					uint8_t *ent = sec + e * 32;
					if (ent[0] == 0x00) {
						if (free_out)
							*free_out =
								first_free ?
									first_free :
									ent;
						return NULL; /* end */
					}
					if (ent[0] == 0xE5) {
						if (!first_free)
							first_free = ent;
						continue;
					}
					uint8_t attr = ent[11];
					if (attr & 0x08)
						continue;
					int match = 1;
					for (int i = 0; i < 11; ++i) {
						if (ent[i] !=
						    (uint8_t)shortname[i]) {
							match = 0;
							break;
						}
					}
					if (match) {
						if (ent_out)
							*ent_out = ent;
						return ent;
					}
				}
			}
			uint16_t next = fat_read_entry(sb, cur);
			if (next == 0 || next >= 0xFFF8)
				break;
			cur = next;
		}
		if (free_out)
			*free_out = first_free;
		return NULL;
	}
}

static int dir_append_cluster(struct fat16_super *sb, uint16_t start_cluster,
			      uint8_t **new_slot_out) {
	if (start_cluster == 0)
		return -1; /* root は固定領域で拡張不可 */
	/* 最後のクラスタを見つける */
	uint16_t cur = start_cluster;
	while (cur >= 2 && cur < 0xFFF8) {
		uint16_t next = fat_read_entry(sb, cur);
		if (next == 0 || next >= 0xFFF8)
			break;
		cur = next;
	}
	uint16_t newc;
	if (allocate_chain(sb, 1, &newc) != 0)
		return -2; /* 空き不足 */
	/* last -> newc, newc -> EOF */
	fat_write_entry(sb, cur, newc);
	fat_write_entry(sb, newc, 0xFFFF);
	/* 新しいクラスタ領域をクリア */
	uint32_t sector =
		sb->first_data_sector + (newc - 2) * sb->sectors_per_cluster;
	uint32_t bytes = sb->bytes_per_sector * sb->sectors_per_cluster;
	uint8_t *ptr = sb->image + sector * sb->bytes_per_sector;
	mem_set(ptr, 0, bytes);
	if (new_slot_out)
		*new_slot_out = ptr; /* 最初のエントリ位置 */
	return 0;
}

static uint8_t *resolve_path(struct fat16_super *sb, const char *path,
			     uint8_t **ent_out, uint8_t **free_out,
			     uint16_t *parent_start_cluster_out) {
	if (!path || path[0] != '/')
		return NULL; /* 簡易: 絶対パスのみサポート */
	/* root から開始 */
	uint16_t dir_cluster = 0; /* 0 == root */
	const char *p = path;
	/* skip leading slash */
	while (*p == '/')
		p++;
	char comp[13];
	while (*p) {
		/* extract component */
		int ci = 0;
		while (*p && *p != '/' && ci < (int)sizeof(comp) - 1)
			comp[ci++] = *p++;
		comp[ci] = '\0';
		while (*p == '/')
			p++;
		int is_last = (*p == '\0');
		uint8_t *found = NULL;
		uint8_t *free_slot = NULL;
		find_entry_in_dir(sb, dir_cluster, comp, &found, &free_slot);
		if (!found) {
			/* 見つからなければ、最後の要素なら free_slot を返す */
			if (is_last) {
				if (parent_start_cluster_out)
					*parent_start_cluster_out = dir_cluster;
				if (ent_out)
					*ent_out = NULL;
				if (free_out)
					*free_out = free_slot;
				return NULL;
			}
			/* 中間のディレクトリが見つからない -> 解決失敗 */
			return NULL;
		}
		/* 見つかった場合、次へ進むか最終要素として返す */
		if (is_last) {
			if (parent_start_cluster_out)
				*parent_start_cluster_out = dir_cluster;
			if (ent_out)
				*ent_out = found;
			if (free_out)
				*free_out = free_slot;
			return found;
		}
		/* 中間要素: ディレクトリであることを確認して次のクラスタへ */
		uint8_t attr = found[11];
		if (!(attr & 0x10))
			return NULL; /* 中間がディレクトリでない */
		uint16_t next_cluster = le16(found + 26);
		if (next_cluster < 2)
			return NULL; /* ディレクトリのクラスタが無効 */
		dir_cluster = next_cluster;
	}
	return NULL;
}

/* マウント */
int fat16_mount(void *image, size_t size, struct fat16_super **out) {
	if (!image || size < 512)
		return -1;
	uint8_t *img = (uint8_t *)image;
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

	if (bytes_per_sector != 512)
		return -2; /* 簡易: 512のみサポート */

	struct fat16_super *sb =
		(struct fat16_super *)kmalloc(sizeof(struct fat16_super));
	if (!sb)
		return -3;
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

/* ブロックキャッシュ経由で FAT16 をマウントする（キャッシュから全イメージを読み込み、
   メモリ上に展開する簡易実装）。 */
int fat16_mount_with_cache(struct block_cache *cache,
			   struct fat16_super **out) {
	if (!cache || !out)
		return -1;
	uint32_t bs = cache->block_size;
	uint8_t *tmp = (uint8_t *)kmalloc(bs);
	if (!tmp)
		return -2;
	/* 読み取り: ブロック0 を読み BPB を解析 */
	if (block_cache_read(cache, 0, tmp) != 0) {
		kfree(tmp);
		return -3;
	}
	uint16_t bytes_per_sector = le16(tmp + 11);
	uint8_t sectors_per_cluster = tmp[13];
	uint16_t reserved = le16(tmp + 14);
	uint8_t num_fats = tmp[16];
	uint16_t max_root = le16(tmp + 17);
	uint16_t total_sectors_short = le16(tmp + 19);
	uint32_t total_sectors = total_sectors_short ? total_sectors_short :
						       le32(tmp + 32);
	uint16_t fat_size_sectors = le16(tmp + 22);
	if (!fat_size_sectors)
		fat_size_sectors = (uint16_t)le32(tmp + 36);

	/* 簡易チェック */
	if (bytes_per_sector != 512) {
		kfree(tmp);
		return -4;
	}

	uint64_t img_size = (uint64_t)total_sectors * bytes_per_sector;
	uint8_t *img = (uint8_t *)kmalloc((size_t)img_size);
	if (!img) {
		kfree(tmp);
		return -5;
	}
	/* ブロック単位で読み出してメモリにコピー */
	uint32_t nblocks = (uint32_t)((img_size + bs - 1) / bs);
	for (uint32_t b = 0; b < nblocks; ++b) {
		if (block_cache_read(cache, b, tmp) != 0) {
			kfree(tmp);
			kfree(img);
			return -6;
		}
		uint32_t copy = (uint32_t)bs;
		uint64_t off = (uint64_t)b * bs;
		if (off + copy > (uint64_t)img_size)
			copy = (uint32_t)((uint64_t)img_size - off);
		for (uint32_t i = 0; i < copy; ++i)
			img[off + i] = tmp[i];
	}
	kfree(tmp);

	/* 通常のメモリマウントを再利用 */
	struct fat16_super *sb = NULL;
	int r = fat16_mount((void *)img, (size_t)img_size, &sb);
	if (r != 0) {
		kfree(img);
		return -7;
	}
	/* sb->image は img に設定されている */
	*out = sb;
	return 0;
}

/* ルート一覧 */
int fat16_list_root(struct fat16_super *sb) {
	if (!sb)
		return -1;
	uint32_t root_sector = sb->root_dir_sector;
	uint32_t entries_per_sector = sb->bytes_per_sector / 32;
	uint32_t sectors = ((sb->max_root_entries + entries_per_sector - 1) /
			    entries_per_sector);
	char name[13];
	for (uint32_t s = 0; s < sectors; ++s) {
		uint8_t *sec =
			sb->image + (root_sector + s) * sb->bytes_per_sector;
		for (uint32_t e = 0; e < entries_per_sector; ++e) {
			uint8_t *ent = sec + e * 32;
			if (ent[0] == 0x00)
				return 0;
			if (ent[0] == 0xE5)
				continue;
			uint8_t attr = ent[11];
			if (attr & 0x08)
				continue;
			mem_set(name, 0, sizeof(name));
			int ni = 0;
			for (int i = 0; i < 8; ++i) {
				char c = ent[i];
				if (c == ' ')
					break;
				name[ni++] = c;
			}
			if (ent[8] != ' ') {
				name[ni++] = '.';
				for (int i = 0; i < 3; ++i) {
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

int fat16_get_file_size(struct fat16_super *sb, const char *name,
			uint32_t *out_size) {
	if (!sb || !name || !out_size)
		return -1;
	uint8_t *ent = NULL;
	uint8_t *free_slot = NULL;
	uint16_t parent = 0;
	if (name[0] == '/') {
		resolve_path(sb, name, &ent, &free_slot, &parent);
	} else {
		find_root_entry(sb, name, &ent, &free_slot);
	}
	if (!ent)
		return -2;
	uint32_t size = le32(ent + 28);
	*out_size = size;
	return 0;
}

int fat16_is_dir(struct fat16_super *sb, const char *path) {
	if (!sb || !path)
		return 0;
	if (path[0] == '/' && (path[1] == '\0' || path[1] == '/'))
		return 1; /* root */
	uint8_t *ent = NULL;
	uint8_t *free_slot = NULL;
	uint16_t parent = 0;
	if (path[0] == '/') {
		resolve_path(sb, path, &ent, &free_slot, &parent);
	} else {
		find_root_entry(sb, path, &ent, &free_slot);
	}
	if (!ent)
		return 0;
	uint8_t attr = ent[11];
	return (attr & 0x10) ? 1 : 0;
}

/* ルートディレクトリ内のエントリ位置を探す (見つかれば ent_out にポインタを返す) */
static uint8_t *find_root_entry(struct fat16_super *sb, const char *name,
				uint8_t **ent_out, uint8_t **free_out) {
	char shortname[11];
	make_shortname(name, shortname);
	uint32_t root_sector = sb->root_dir_sector;
	uint32_t entries_per_sector = sb->bytes_per_sector / 32;
	uint32_t sectors = ((sb->max_root_entries + entries_per_sector - 1) /
			    entries_per_sector);
	uint8_t *first_free = NULL;
	for (uint32_t s = 0; s < sectors; ++s) {
		uint8_t *sec =
			sb->image + (root_sector + s) * sb->bytes_per_sector;
		for (uint32_t e = 0; e < entries_per_sector; ++e) {
			uint8_t *ent = sec + e * 32;
			if (ent[0] == 0x00) {
				if (free_out)
					*free_out = first_free ? first_free :
								 ent;
				return NULL; /* end => not found */
			}
			if (ent[0] == 0xE5) {
				if (!first_free)
					first_free = ent;
				continue;
			}
			uint8_t attr = ent[11];
			if (attr & 0x08)
				continue;
			int match = 1;
			for (int i = 0; i < 11; ++i) {
				char a = ent[i];
				char b = shortname[i];
				if (a != b) {
					match = 0;
					break;
				}
			}
			if (match) {
				if (ent_out)
					*ent_out = ent;
				return ent;
			}
		}
	}
	if (free_out)
		*free_out = first_free;
	return NULL;
}

/* ファイルを読み出す (最初のクラスタのみを想定) */
int fat16_read_file(struct fat16_super *sb, const char *name, void *buf,
		    size_t len, size_t *out_len) {
	if (!sb || !name)
		return -1;
	uint8_t *ent = NULL;
	uint8_t *free_slot = NULL;
	uint16_t parent_cluster = 0;
	if (name[0] == '/') {
		/* 絶対パス解決 */
		resolve_path(sb, name, &ent, &free_slot, &parent_cluster);
	} else {
		find_root_entry(sb, name, &ent, NULL);
	}
	if (!ent)
		return -2; /* 見つからない */
	uint16_t start_cluster = le16(ent + 26);
	uint32_t file_size = le32(ent + 28);
	if (file_size == 0) {
		if (out_len)
			*out_len = 0;
		return 0; /* 空ファイル */
	}
	if (start_cluster < 2)
		return -3;
	uint32_t bytes_to_read = file_size;
	if (bytes_to_read > len)
		bytes_to_read = len;
	uint32_t cluster_bytes = sb->bytes_per_sector * sb->sectors_per_cluster;
	uint32_t bytes_read = 0;
	uint16_t cur = start_cluster;
	while (cur >= 2 && cur < 0xFFF8 && bytes_read < bytes_to_read) {
		uint32_t sector = sb->first_data_sector +
				  (cur - 2) * sb->sectors_per_cluster;
		uint8_t *src = sb->image + sector * sb->bytes_per_sector;
		uint32_t copy = bytes_to_read - bytes_read;
		if (copy > cluster_bytes)
			copy = cluster_bytes;
		mem_copy((uint8_t *)buf + bytes_read, src, copy);
		bytes_read += copy;
		uint16_t next = fat_read_entry(sb, cur);
		if (next == 0 || next >= 0xFFF8)
			break;
		cur = next;
	}
	if (out_len)
		*out_len = bytes_read;
	return 0;
}

/* 連続割当の古い実装は削除し、allocate_chain を使用します */

/* 既存チェーンを解放 (FAT エントリを 0 に戻す) */
static void free_chain(struct fat16_super *sb, uint16_t start) {
	uint16_t cur = start;
	while (cur >= 2 && cur != 0 && cur < 0xFFF8) {
		uint16_t next = fat_read_entry(sb, cur);
		fat_write_entry(sb, cur, 0);
		if (next == 0 || next >= 0xFFF8)
			break;
		cur = next;
	}
}

/* ファイル作成: 空ファイル */
int fat16_create_file(struct fat16_super *sb, const char *name) {
	if (!sb || !name)
		return -1;
	uint8_t *existing = NULL;
	uint8_t *free_slot = NULL;
	uint16_t parent_cluster = 0;
	if (name[0] == '/') {
		resolve_path(sb, name, &existing, &free_slot, &parent_cluster);
	} else {
		find_root_entry(sb, name, &existing, &free_slot);
		parent_cluster = 0;
	}
	if (existing) {
		/* 既に存在する場合はサイズを 0 にしてクラスタを解放 */
		uint16_t start = le16(existing + 26);
		if (start >= 2)
			free_chain(sb, start);
		/* start cluster とサイズをクリア */
		existing[26] = existing[27] = 0;
		existing[28] = existing[29] = existing[30] = existing[31] = 0;
		return 0;
	}
	if (!free_slot) {
		/* 親ディレクトリがサブディレクトリならクラスタ追加を試みる */
		/* parent_cluster は resolve_path で設定されている（root 呼び出し時は 0） */
		if (parent_cluster != 0) {
			uint8_t *new_slot = NULL;
			if (dir_append_cluster(sb, parent_cluster, &new_slot) ==
			    0) {
				free_slot = new_slot;
			}
		}
		if (!free_slot)
			return -2; /* 空きエントリ無し */
	}
	/* 新しいエントリを作る: 簡易的にファイルを空で作成 */
	char shortname[11];
	make_shortname(name, shortname);
	for (int i = 0; i < 11; ++i)
		free_slot[i] = (uint8_t)shortname[i];
	free_slot[11] = 0x20; /* attribute: archive */
	/* reserved */ free_slot[12] = free_slot[13] = 0;
	/* time/date set to zero */
	for (int i = 14; i < 26; ++i)
		free_slot[i] = 0;
	/* start cluster */ free_slot[26] = free_slot[27] = 0;
	/* size */ for (int i = 28; i < 32; ++i)
		free_slot[i] = 0;
	return 0;
}

/* ファイル書き込み: 上書き (既存は解放、連続クラスタを割当して書く) */
int fat16_write_file(struct fat16_super *sb, const char *name, const void *buf,
		     size_t len) {
	if (!sb || !name || !buf)
		return -1;
	uint8_t *ent = NULL;
	uint8_t *free_slot = NULL;
	uint16_t parent_cluster = 0;
	if (name[0] == '/') {
		resolve_path(sb, name, &ent, &free_slot, &parent_cluster);
	} else {
		find_root_entry(sb, name, &ent, &free_slot);
	}
	if (!ent) {
		if (!free_slot) {
			/* 親ディレクトリがサブディレクトリならクラスタ追加を試みる */
			if (parent_cluster != 0) {
				uint8_t *new_slot = NULL;
				if (dir_append_cluster(sb, parent_cluster,
						       &new_slot) == 0) {
					free_slot = new_slot;
				}
			}
			if (!free_slot)
				return -2;
		}
		ent = free_slot;
		/* 新規エントリの初期化 */
		char shortname[11];
		make_shortname(name, shortname);
		for (int i = 0; i < 11; ++i)
			ent[i] = (uint8_t)shortname[i];
		ent[11] = 0x20; /* archive */
		for (int i = 12; i < 26; ++i)
			ent[i] = 0;
	} else {
		/* 既存ファイルなら既存クラスタを解放 */
		uint16_t old_start = le16(ent + 26);
		if (old_start >= 2)
			free_chain(sb, old_start);
	}
	/* 必要クラスタ数を計算 */
	uint32_t cluster_bytes = sb->bytes_per_sector * sb->sectors_per_cluster;
	uint16_t need = (uint16_t)((len + cluster_bytes - 1) / cluster_bytes);
	/* len == 0 の場合はクラスタ割当を行わずエントリを空にする */
	if (len == 0) {
		ent[26] = ent[27] = 0;
		ent[28] = ent[29] = ent[30] = ent[31] = 0;
		return 0;
	}
	if (need == 0)
		need = 1;
	uint16_t start_cluster;
	/* 非連続クラスタでも割当可能な allocate_chain を使う */
	if (allocate_chain(sb, need, &start_cluster) != 0)
		return -3;
	/* データ領域へ書き込み (チェーンに従い順次書き込む) */
	uint32_t remaining = (uint32_t)len;
	uint16_t cur = start_cluster;
	uint32_t written = 0;
	while (cur >= 2 && cur < 0xFFF8 && remaining > 0) {
		uint32_t sector = sb->first_data_sector +
				  (cur - 2) * sb->sectors_per_cluster;
		uint8_t *dst = sb->image + sector * sb->bytes_per_sector;
		uint32_t to_copy = remaining > cluster_bytes ? cluster_bytes :
							       remaining;
		mem_copy(dst, (const uint8_t *)buf + written, to_copy);
		written += to_copy;
		remaining -= to_copy;
		uint16_t next = fat_read_entry(sb, cur);
		if (next == 0 || next >= 0xFFF8)
			break;
		cur = next;
	}
	/* ディレクトリエントリの start cluster と size を更新 */
	ent[26] = (uint8_t)(start_cluster & 0xff);
	ent[27] = (uint8_t)((start_cluster >> 8) & 0xff);
	uint32_t size = (uint32_t)len;
	ent[28] = (uint8_t)(size & 0xff);
	ent[29] = (uint8_t)((size >> 8) & 0xff);
	ent[30] = (uint8_t)((size >> 16) & 0xff);
	ent[31] = (uint8_t)((size >> 24) & 0xff);
	return 0;
}
