この章では、仮想ファイルシステム（VFS）レイヤーについて記述します。
実装されているファイルは[fs.c](../../src/kernel/fs/fs.c), [fs.h](../../src/include/fs/fs.h)です。

## 概要
VFS（Virtual File System）レイヤーは、複数のファイルシステム実装（FAT12、ext2）に対する統一されたインターフェースを提供します。ファイルシステムの種類を自動検出し、適切なバックエンドにディスパッチします。ディレクトリの列挙、ファイルの読み取りなど、基本的なファイル操作をサポートします。

## 関数 / API

#### `int fs_mount(const void *device_image, size_t size, fs_handle **out)`
イメージからファイルシステムをマウントします。FAT12またはext2を自動検出します。

引数:
  - device_image(const void*): イメージ先頭へのポインタ（埋め込みバッファ等）
  - size(size_t): イメージ長（バイト）
  - out(fs_handle**): 成功時にfs_handleポインタを返す

戻り値: 0=成功、負値=エラー

#### `int fs_unmount(fs_handle *h)`
ファイルシステムをアンマウントし、リソースを解放します。

引数:
  - h(fs_handle*): ファイルシステムハンドル

戻り値: 0=成功、負値=エラー

#### `int fs_opendir(fs_handle *h, const char *path, dir_handle **out)`
ディレクトリを開きます。現在はルートディレクトリ（"/"）のみをサポートします。

引数:
  - h(fs_handle*): ファイルシステムハンドル
  - path(const char*): ディレクトリパス（"/" のみサポート）
  - out(dir_handle**): 成功時にdir_handleポインタを返す

戻り値: 0=成功、負値=エラー

#### `int fs_readdir(dir_handle *d, char *name, int name_len, int *is_dir, uint32_t *size, uint16_t *start_cluster)`
ディレクトリエントリを列挙します。呼び出すたびに次のエントリを返します。

引数:
  - d(dir_handle*): ディレクトリハンドル
  - name(char*): ファイル名を格納する出力バッファ
  - name_len(int): バッファ長
  - is_dir(int*): ディレクトリかどうかを返す（出力、1=ディレクトリ、0=ファイル）
  - size(uint32_t*): ファイルサイズを返す（出力）
  - start_cluster(uint16_t*): 開始クラスタを返す（出力、FAT12のみ）

戻り値: 0=成功、負値=エラー（エントリがない場合も負値）

#### `int fs_closedir(dir_handle *d)`
ディレクトリハンドルを閉じます。

引数:
  - d(dir_handle*): ディレクトリハンドル

戻り値: 0=成功、負値=エラー

#### `int fs_open(fs_handle *h, const char *path, file_handle **out)`
ファイルを開きます。現在はルートディレクトリ内のファイルのみをサポートします。

引数:
  - h(fs_handle*): ファイルシステムハンドル
  - path(const char*): ファイルパス
  - out(file_handle**): 成功時にfile_handleポインタを返す

戻り値: 0=成功、負値=エラー

#### `int fs_read(file_handle *f, void *buf, uint32_t len, uint32_t offset)`
ファイルからデータを読み取ります（同期読み取り）。

引数:
  - f(file_handle*): ファイルハンドル
  - buf(void*): データを格納するバッファ
  - len(uint32_t): 読み取るバイト数
  - offset(uint32_t): ファイル内のオフセット

戻り値: 読み取ったバイト数、エラー時は負値

#### `int fs_close(file_handle *f)`
ファイルハンドルを閉じます。

引数:
  - f(file_handle*): ファイルハンドル

戻り値: 0=成功、負値=エラー

## 定数 / 定義

このファイルには定数定義はありません。内部でファイルシステムタイプを管理しています。

## 構造体

#### `fs_handle`（不透明型）
ファイルシステムハンドルを表す不透明な構造体です。内部でファイルシステムの種類とバックエンドポインタを保持します。

#### `dir_handle`（不透明型）
ディレクトリハンドルを表す不透明な構造体です。ディレクトリの列挙状態を保持します。

#### `file_handle`（不透明型）
ファイルハンドルを表す不透明な構造体です。ファイル名とサイズを保持します。

### ファイルシステムの検出

VFSレイヤーは以下の順序でファイルシステムを検出します：

1. **FAT12検出**: ブートセクタのシグネチャ（0x55AA）とbytes per sectorをチェック
2. **ext2検出**: スーパーブロックのマジックナンバー（0xEF53）をチェック

### サポートされているファイルシステム

- **FAT12**: MS-DOS互換のFAT12ファイルシステム
- **ext2**: Linux ext2ファイルシステム（読み取り専用）

### 制限事項

- ルートディレクトリのみサポート（サブディレクトリは未実装）
- 読み取り専用（書き込み機能は未実装）
- パス解析は基本的な機能のみ
