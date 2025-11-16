#ifndef _BLOCK_CACHE_H
#define _BLOCK_CACHE_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief ブロックキャッシュエントリ
 * 
 * LRU（Least Recently Used）キャッシュとして実装。
 * 各エントリは1ブロック（通常1024バイト）のデータを保持。
 */
struct block_cache_entry {
	uint32_t block_num; /* ブロック番号（0 = 無効） */
	uint8_t *data; /* ブロックデータ */
	uint32_t last_used; /* 最終使用時刻（タイムスタンプ） */
	int dirty; /* 書き込みが必要かどうか */
	int valid; /* このエントリが有効かどうか */
};

/**
 * @brief ブロックキャッシュ管理構造体
 */
struct block_cache {
	uint8_t drive; /* ATAドライブ番号 */
	uint32_t block_size; /* ブロックサイズ（バイト） */
	uint32_t num_entries; /* キャッシュエントリ数 */
	struct block_cache_entry *entries; /* エントリ配列 */
	uint32_t timestamp; /* 現在のタイムスタンプ */

	/* 統計情報 */
	uint32_t hits; /* キャッシュヒット数 */
	uint32_t misses; /* キャッシュミス数 */
};

/**
 * @brief ブロックキャッシュを初期化する
 * 
 * @param drive ATAドライブ番号（例: 1 = Primary Slave）
 * @param block_size ブロックサイズ（バイト、通常1024）
 * @param num_entries キャッシュエントリ数（例: 32 = 32KB）
 * @return ブロックキャッシュポインタ、失敗時はNULL
 */
struct block_cache *block_cache_init(uint8_t drive, uint32_t block_size,
				     uint32_t num_entries);

/**
 * @brief ブロックを読み取る（キャッシュ経由）
 * 
 * キャッシュにヒットすればキャッシュから返す。
 * ミスした場合はATAドライブから読み込んでキャッシュに格納。
 * 
 * @param cache ブロックキャッシュ
 * @param block_num ブロック番号
 * @param buffer 読み取ったデータを格納するバッファ
 * @return 0 成功、負値はエラー
 */
int block_cache_read(struct block_cache *cache, uint32_t block_num,
		     void *buffer);

/**
 * @brief ブロックを書き込む（キャッシュ経由）
 * 
 * キャッシュに書き込み、dirtyフラグを立てる。
 * 実際のディスクへの書き込みはflush時に行う。
 * 
 * @param cache ブロックキャッシュ
 * @param block_num ブロック番号
 * @param buffer 書き込むデータ
 * @return 0 成功、負値はエラー
 */
int block_cache_write(struct block_cache *cache, uint32_t block_num,
		      const void *buffer);

/**
 * @brief dirtyブロックをすべてディスクに書き込む
 * 
 * @param cache ブロックキャッシュ
 * @return 0 成功、負値はエラー
 */
int block_cache_flush(struct block_cache *cache);

/**
 * @brief キャッシュ統計情報を表示する
 * 
 * @param cache ブロックキャッシュ
 */
void block_cache_print_stats(struct block_cache *cache);

/**
 * @brief ブロックキャッシュを破棄する
 * 
 * @param cache ブロックキャッシュ
 */
void block_cache_destroy(struct block_cache *cache);

#endif /* _BLOCK_CACHE_H */
