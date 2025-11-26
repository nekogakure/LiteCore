#ifndef _EXT2_H
#define _EXT2_H

#include <stdint.h>
#include <stddef.h>

/* ext2 Superblock (オフセット 1024, サイズ 1024バイト) */
struct ext2_superblock {
	uint32_t s_inodes_count; /* inodeの総数 */
	uint32_t s_blocks_count; /* ブロックの総数 */
	uint32_t s_r_blocks_count; /* 予約ブロック数 */
	uint32_t s_free_blocks_count; /* 空きブロック数 */
	uint32_t s_free_inodes_count; /* 空きinode数 */
	uint32_t s_first_data_block; /* 最初のデータブロック */
	uint32_t s_log_block_size; /* ブロックサイズ (1024 << s_log_block_size) */
	uint32_t s_log_frag_size; /* フラグメントサイズ */
	uint32_t s_blocks_per_group; /* ブロックグループあたりのブロック数 */
	uint32_t s_frags_per_group; /* ブロックグループあたりのフラグメント数 */
	uint32_t s_inodes_per_group; /* ブロックグループあたりのinode数 */
	uint32_t s_mtime; /* マウント時刻 */
	uint32_t s_wtime; /* 書き込み時刻 */
	uint16_t s_mnt_count; /* マウント回数 */
	uint16_t s_max_mnt_count; /* 最大マウント回数 */
	uint16_t s_magic; /* マジックシグネチャ (0xEF53) */
	uint16_t s_state; /* ファイルシステムの状態 */
	uint16_t s_errors; /* エラー発生時の動作 */
	uint16_t s_minor_rev_level; /* マイナーリビジョン */
	uint32_t s_lastcheck; /* 最終チェック時刻 */
	uint32_t s_checkinterval; /* チェック間隔 */
	uint32_t s_creator_os; /* 作成OS */
	uint32_t s_rev_level; /* リビジョンレベル */
	uint16_t s_def_resuid; /* デフォルト予約UID */
	uint16_t s_def_resgid; /* デフォルト予約GID */
	/* EXT2_DYNAMIC_REV用の拡張フィールド */
	uint32_t s_first_ino; /* 最初の利用可能inode */
	uint16_t s_inode_size; /* inodeサイズ */
	uint16_t s_block_group_nr; /* このスーパーブロックのブロックグループ番号 */
	uint32_t s_feature_compat; /* 互換性のある機能 */
	uint32_t s_feature_incompat; /* 非互換な機能 */
	uint32_t s_feature_ro_compat; /* 読み取り専用互換機能 */
	uint8_t s_uuid[16]; /* ボリュームID */
	char s_volume_name[16]; /* ボリューム名 */
	char s_last_mounted[64]; /* 最後にマウントされたパス */
	uint32_t s_algo_bitmap; /* 圧縮アルゴリズム */
	/* 残りのフィールドは省略（簡易実装用） */
};

/* ext2 Block Group Descriptor */
struct ext2_group_desc {
	uint32_t bg_block_bitmap; /* ブロックビットマップのブロック番号 */
	uint32_t bg_inode_bitmap; /* inodeビットマップのブロック番号 */
	uint32_t bg_inode_table; /* inodeテーブルの開始ブロック番号 */
	uint16_t bg_free_blocks_count; /* 空きブロック数 */
	uint16_t bg_free_inodes_count; /* 空きinode数 */
	uint16_t bg_used_dirs_count; /* ディレクトリ数 */
	uint16_t bg_pad; /* パディング */
	uint8_t bg_reserved[12]; /* 予約領域 */
};

/* ext2 Inode */
struct ext2_inode {
	uint16_t i_mode; /* ファイルモード */
	uint16_t i_uid; /* 所有者UID */
	uint32_t i_size; /* サイズ（バイト） */
	uint32_t i_atime; /* アクセス時刻 */
	uint32_t i_ctime; /* 作成時刻 */
	uint32_t i_mtime; /* 変更時刻 */
	uint32_t i_dtime; /* 削除時刻 */
	uint16_t i_gid; /* グループGID */
	uint16_t i_links_count; /* ハードリンク数 */
	uint32_t i_blocks; /* ブロック数 */
	uint32_t i_flags; /* ファイルフラグ */
	uint32_t i_osd1; /* OS依存1 */
	uint32_t i_block[15]; /* ブロックポインタ */
	uint32_t i_generation; /* ファイルバージョン */
	uint32_t i_file_acl; /* ファイルACL */
	uint32_t i_dir_acl; /* ディレクトリACL */
	uint32_t i_faddr; /* フラグメントアドレス */
	uint8_t i_osd2[12]; /* OS依存2 */
};

/* ext2 Directory Entry */
struct ext2_dir_entry {
	uint32_t inode; /* inode番号 */
	uint16_t rec_len; /* このエントリの長さ */
	uint8_t name_len; /* ファイル名の長さ */
	uint8_t file_type; /* ファイルタイプ */
	char name[]; /* ファイル名（可変長） */
};

/* ファイルタイプ */
#define EXT2_FT_UNKNOWN 0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2
#define EXT2_FT_CHRDEV 3
#define EXT2_FT_BLKDEV 4
#define EXT2_FT_FIFO 5
#define EXT2_FT_SOCK 6
#define EXT2_FT_SYMLINK 7

/* i_mode のフラグ */
#define EXT2_S_IFREG 0x8000 /* 通常ファイル */
#define EXT2_S_IFDIR 0x4000 /* ディレクトリ */
#define EXT2_S_IFLNK 0xA000 /* シンボリックリンク */

/* マジックナンバー */
#define EXT2_SUPER_MAGIC 0xEF53

/* ルートinode番号 */
#define EXT2_ROOT_INO 2

/* 前方宣言 */
struct block_cache;

/**
 * @brief ext2マウント情報を保持する構造体
 */
struct ext2_super {
	const uint8_t *
		image; /* イメージ先頭へのポインタ（廃止予定、互換性のため残す） */
	size_t image_size; /* イメージサイズ */
	struct ext2_superblock sb; /* スーパーブロック */
	uint32_t block_size; /* ブロックサイズ（バイト） */
	uint32_t num_groups; /* ブロックグループ数 */
	uint32_t group_desc_offset; /* グループディスクリプタのオフセット */
	struct block_cache *cache; /* ブロックキャッシュ */
};

/**
 * @brief ext2イメージをマウントする（レガシー、埋め込みイメージ用）
 * @param image イメージ先頭へのポインタ
 * @param size イメージサイズ
 * @param out マウント結果を返すポインタ
 * @return 0 成功、負値はエラー
 */
int ext2_mount(const void *image, size_t size, struct ext2_super **out);

/**
 * @brief ext2ファイルシステムをブロックキャッシュ経由でマウントする
 * @param cache ブロックキャッシュ
 * @param out マウント結果を返すポインタ
 * @return 0 成功、負値はエラー
 */
int ext2_mount_with_cache(struct block_cache *cache, struct ext2_super **out);

/**
 * @brief ルートディレクトリの内容をデバッグ表示する
 * @param sb マウント済みsuper
 * @return 0 成功、負値はエラー
 */
int ext2_list_root(struct ext2_super *sb);

/**
 * @brief inode番号からinodeを読み取る
 * @param sb マウント済みsuper
 * @param inode_num inode番号
 * @param inode 読み取ったinodeを格納する構造体
 * @return 0 成功、負値はエラー
 */
int ext2_read_inode(struct ext2_super *sb, uint32_t inode_num,
		    struct ext2_inode *inode);

/**
 * @brief ディレクトリinodeからファイル名を検索する
 * @param sb マウント済みsuper
 * @param dir_inode ディレクトリのinode
 * @param name 検索するファイル名
 * @param out_inode 見つかったファイルのinode番号を返す
 * @return 0 成功、負値はエラー
 */
int ext2_find_file_in_dir(struct ext2_super *sb, struct ext2_inode *dir_inode,
			  const char *name, uint32_t *out_inode);

/**
 * @brief ファイル名からファイルを読み出す（ルートディレクトリのみ対応）
 * @param sb マウント済みsuper
 * @param name 読み出すファイル名
 * @param buf データ格納先
 * @param len バッファ長
 * @param out_len 実際に読み取ったバイト数を返す
 * @return 0 成功、負値はエラー
 */
int ext2_read_file(struct ext2_super *sb, const char *name, void *buf,
		   size_t len, size_t *out_len);

/**
 * @brief パスからinode番号を解決する（完全なパス対応）
 * @param sb マウント済みsuper
 * @param path パス（絶対パスまたは相対パス）
 * @param out_inode 見つかったinode番号を返す
 * @return 0 成功、負値はエラー
 */
int ext2_resolve_path(struct ext2_super *sb, const char *path,
		      uint32_t *out_inode);

/**
 * @brief inodeのブロックを読み取る（間接ブロック対応）
 * @param sb マウント済みsuper
 * @param inode 読み取り元のinode
 * @param block_index ファイル内のブロックインデックス
 * @param out_block_num 物理ブロック番号を返す
 * @return 0 成功、負値はエラー
 */
int ext2_get_block_num(struct ext2_super *sb, struct ext2_inode *inode,
		       uint32_t block_index, uint32_t *out_block_num);

/**
 * @brief inodeのデータを読み取る（完全版、間接ブロック対応）
 * @param sb マウント済みsuper
 * @param inode 読み取り元のinode
 * @param buf データ格納先
 * @param len 読み取るバイト数
 * @param offset 読み取り開始オフセット
 * @param out_len 実際に読み取ったバイト数を返す
 * @return 0 成功、負値はエラー
 */
int ext2_read_inode_data(struct ext2_super *sb, struct ext2_inode *inode,
			 void *buf, size_t len, uint32_t offset,
			 size_t *out_len);

/**
 * @brief シンボリックリンクを解決する
 * @param sb マウント済みsuper
 * @param link_inode シンボリックリンクのinode
 * @param out_target リンク先のinode番号を返す
 * @return 0 成功、負値はエラー
 */
int ext2_resolve_symlink(struct ext2_super *sb, struct ext2_inode *link_inode,
			 uint32_t *out_target);

/**
 * @brief パスからファイルを読み取る（完全版）
 * @param sb マウント済みsuper
 * @param path ファイルパス
 * @param buf データ格納先
 * @param len バッファ長
 * @param offset 読み取り開始オフセット
 * @param out_len 実際に読み取ったバイト数を返す
 * @return 0 成功、負値はエラー
 */
int ext2_read_file_by_path(struct ext2_super *sb, const char *path, void *buf,
			   size_t len, uint32_t offset, size_t *out_len);

/**
 * @brief 空きブロックを割り当てる（簡易実装）
 * @param sb マウント済みsuper
 * @param out_block 割り当てられたブロック番号を返す
 * @return 0 成功、負値はエラー
 */
int ext2_allocate_block(struct ext2_super *sb, uint32_t *out_block);

/**
 * @brief inodeの内容をディスクに書き込む（簡易実装）
 * @param sb マウント済みsuper
 * @param inode_num 書き込むinode番号
 * @param inode 書き込むinodeデータ
 * @return 0 成功、負値はエラー
 */
int ext2_write_inode(struct ext2_super *sb, uint32_t inode_num,
		     struct ext2_inode *inode);

/**
 * @brief inodeのデータをファイルに書き込む（簡易実装、直接ブロックのみ）
 * @param sb マウント済みsuper
 * @param inode 書き込むinode（変更されたサイズ等はこの構造体に反映される）
 * @param buf 書き込むデータ
 * @param len 書き込むバイト数
 * @param offset 書き込み開始オフセット
 * @param out_len 実際に書き込んだバイト数を返す
 * @return 0 成功、負値はエラー
 */
int ext2_write_inode_data(struct ext2_super *sb, struct ext2_inode *inode,
			  const void *buf, size_t len, uint32_t offset,
			  size_t *out_len);

/**
 * @brief パスでファイルを作成する（簡易実装: ルート直下のみ対応）
 * @param sb マウント済みsuper
 * @param path 作成するファイルパス（"/name" 形式のみサポート）
 * @param mode ファイルモード
 * @param out_inode 作成したinode番号を返す
 * @return 0 成功、負値はエラー
 */
int ext2_create_file(struct ext2_super *sb, const char *path, uint16_t mode,
		     uint32_t *out_inode_num);

/**
 * @brief ディレクトリの内容を一覧表示する（デバッグ用）
 * @param sb マウント済みsuper
 * @param dir_inode ディレクトリのinode
 * @return 0 成功、負値はエラー
 */
int ext2_list_dir(struct ext2_super *sb, struct ext2_inode *dir_inode);

#endif /* _EXT2_H */
