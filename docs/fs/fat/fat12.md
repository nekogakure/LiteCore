この章では、FAT12ファイルシステムについて記述します。
実装されているファイルは[fat12.c](../../../src/kernel/fs/fat/fat12.c), [fat12.h](../../../src/include/fs/fat/fat12.h)です。

## 概要
FAT12ファイルシステムドライバは、MS-DOS互換のFAT12ファイルシステムをサポートします。ブートセクタの解析、FATテーブルの読み取り、ルートディレクトリのエントリ列挙、ファイルの読み取りを実装しています。主にフロッピーディスクイメージや小容量ストレージで使用されます。

## 関数 / API

#### `int fat12_mount(const void *image, size_t size, struct fat12_super **out)`
FAT12イメージをマウントして構造情報を初期化します。

引数:
  - image(const void*): マウントするイメージの先頭アドレス
  - size(size_t): イメージのバイト長
  - out(struct fat12_super**): 成功時に初期化されたfat12_superポインタを返す

戻り値: 0=成功、負値=エラー（短いイメージ、メモリ不足、未対応フォーマット等）

#### `int fat12_list_root(struct fat12_super *sb)`
ルートディレクトリの内容をデバッグ表示します。

引数:
  - sb(struct fat12_super*): マウント済みのスーパーブロック

戻り値: 0=成功、負値=エラー

#### `int fat12_read_file(struct fat12_super *sb, const char *name, void *buf, size_t len, size_t *out_len)`
ルートディレクトリ内の指定したファイルを読み出します（簡易実装）。

引数:
  - sb(struct fat12_super*): マウント済みのスーパーブロック
  - name(const char*): 読み出すファイル名（例: "EXAMPLE.TXT"）
  - buf(void*): データ格納先バッファ
  - len(size_t): バッファ長
  - out_len(size_t*): 実際に読み取ったバイト数を返す（NULL可）

戻り値: 0=成功、負値=エラー

## 定数 / 定義

このファイルには定数定義はありません。FAT12フォーマットの標準値を使用します。

## 構造体

#### `struct fat12_super`
FAT12のマウント情報を保持する構造体です。

- `bytes_per_sector(uint16_t)`: セクタあたりのバイト数（通常512）
- `sectors_per_cluster(uint8_t)`: クラスタあたりのセクタ数
- `reserved_sectors(uint16_t)`: 予約セクタ数
- `num_fats(uint8_t)`: FATテーブルの数（通常2）
- `max_root_entries(uint16_t)`: ルートディレクトリの最大エントリ数
- `total_sectors(uint32_t)`: 総セクタ数
- `fat_size_sectors(uint32_t)`: FAT1個あたりのセクタ数
- `first_data_sector(uint32_t)`: データ領域の開始セクタ
- `root_dir_sector(uint32_t)`: ルートディレクトリの開始セクタ
- `image(const uint8_t*)`: 埋め込みイメージへのポインタ
- `image_size(size_t)`: イメージサイズ

### FAT12ディスクレイアウト

1. **ブートセクタ**: セクタ0、BPB（BIOS Parameter Block）を含む
2. **FAT領域**: 予約セクタの後、通常2つのFATテーブル
3. **ルートディレクトリ**: FAT領域の後、固定サイズのディレクトリエントリ
4. **データ領域**: クラスタ2から始まるファイルデータ

### ディレクトリエントリ

各エントリは32バイト：
- オフセット0-10: ファイル名（8.3形式）
- オフセット11: 属性バイト
- オフセット26-27: 開始クラスタ番号
- オフセット28-31: ファイルサイズ

### FATチェーン

- 0x000: 未使用クラスタ
- 0x002-0xFEF: 次のクラスタ番号
- 0xFF0-0xFF6: 予約
- 0xFF7: 不良クラスタ
- 0xFF8-0xFFF: ファイル終端（EOF）
