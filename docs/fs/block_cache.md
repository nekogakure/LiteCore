この章では、ブロックキャッシュについて記述します。
実装されているファイルは[block_cache.c](https://github.com/nekogakure/LiteCore/blob/main/src/kernel/fs/block_cache.c), [block_cache.h](https://github.com/nekogakure/LiteCore/blob/main/src/include/fs/block_cache.h)です。

## 概要
ブロックキャッシュは、ディスクI/Oのパフォーマンスを向上させるためのLRU（Least Recently Used）キャッシュです。ATAドライブからのブロック読み取り/書き込みをキャッシュし、頻繁にアクセスされるブロックをメモリに保持します。ダーティブロックの管理と遅延書き込みをサポートします。

## 関数 / API

#### `struct block_cache *block_cache_init(uint8_t drive, uint32_t block_size, uint32_t num_entries)`
ブロックキャッシュを初期化します。

引数:
  - drive(uint8_t): ATAドライブ番号（例: 1 = Primary Slave）
  - block_size(uint32_t): ブロックサイズ（バイト、通常1024）
  - num_entries(uint32_t): キャッシュエントリ数（例: 32 = 32KB）

戻り値: ブロックキャッシュポインタ、失敗時はNULL

#### `int block_cache_read(struct block_cache *cache, uint32_t block_num, void *buffer)`
ブロックを読み取ります（キャッシュ経由）。キャッシュにヒットすればキャッシュから返し、ミスした場合はATAドライブから読み込んでキャッシュに格納します。

引数:
  - cache(struct block_cache*): ブロックキャッシュ
  - block_num(uint32_t): ブロック番号
  - buffer(void*): 読み取ったデータを格納するバッファ

戻り値: 0=成功、負値=エラー

#### `int block_cache_write(struct block_cache *cache, uint32_t block_num, const void *buffer)`
ブロックを書き込みます（キャッシュ経由）。キャッシュに書き込み、dirtyフラグを立てます。実際のディスクへの書き込みはflush時に行います。

引数:
  - cache(struct block_cache*): ブロックキャッシュ
  - block_num(uint32_t): ブロック番号
  - buffer(const void*): 書き込むデータ

戻り値: 0=成功、負値=エラー

#### `int block_cache_flush(struct block_cache *cache)`
dirtyブロックをすべてディスクに書き込みます。

引数:
  - cache(struct block_cache*): ブロックキャッシュ

戻り値: 0=成功、負値=エラー

#### `void block_cache_print_stats(struct block_cache *cache)`
キャッシュ統計情報（ヒット率、ミス率）を表示します。

引数:
  - cache(struct block_cache*): ブロックキャッシュ

#### `void block_cache_destroy(struct block_cache *cache)`
ブロックキャッシュを破棄し、メモリを解放します。

引数:
  - cache(struct block_cache*): ブロックキャッシュ

## 定数 / 定義

このファイルには定数定義はありません。ブロックサイズとエントリ数は初期化時に指定します。

## 構造体

#### `struct block_cache_entry`
ブロックキャッシュエントリを表す構造体です。各エントリは1ブロックのデータを保持します。

- `block_num(uint32_t)`: ブロック番号（0 = 無効）
- `data(uint8_t*)`: ブロックデータへのポインタ
- `last_used(uint32_t)`: 最終使用時刻（タイムスタンプ）
- `dirty(int)`: 書き込みが必要かどうか（1=dirty, 0=clean）
- `valid(int)`: このエントリが有効かどうか（1=有効, 0=無効）

#### `struct block_cache`
ブロックキャッシュ管理構造体です。

- `drive(uint8_t)`: ATAドライブ番号
- `block_size(uint32_t)`: ブロックサイズ（バイト）
- `num_entries(uint32_t)`: キャッシュエントリ数
- `entries(struct block_cache_entry*)`: エントリ配列へのポインタ
- `timestamp(uint32_t)`: 現在のタイムスタンプ（LRU管理用）
- `hits(uint32_t)`: キャッシュヒット数（統計情報）
- `misses(uint32_t)`: キャッシュミス数（統計情報）

### LRUアルゴリズム

- **キャッシュヒット**: ブロックが既にキャッシュに存在する場合、タイムスタンプを更新
- **キャッシュミス**: 無効なエントリがあれば使用、なければ最も古いエントリを置き換え
- **ダーティフラグ**: 書き込まれたブロックにはdirtyフラグが立ち、flush時にディスクに書き込まれる

### 性能最適化

- セクタ単位（512バイト）からブロック単位（通常1024バイト以上）への変換
- LRUアルゴリズムによる効率的なキャッシュ管理
- 遅延書き込みによるI/O回数の削減
