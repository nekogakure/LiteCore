この賞ではファイルシステムについての設計ドキュメントを記述します。

## FAT12
- まずは読み取り専用でディスクイメージをマウントし、ルートディレクトリの一覧取得とファイル読み出しができるようにする。
- シンプルなAPIをカーネルに提供し、テストディスクイメージと統合テストを追加する。
- 後で書き込みや他のFSに移行しやすいよう、抽象化レイヤを設計する。

### ファイルシステムAPI（カーネル内部用）
- fs_mount(device, mount_point) -> fs_handle*
- fs_unmount(fs_handle)
- fs_opendir(fs_handle, path) -> dir_handle*
- fs_readdir(dir_handle) -> fs_dirent (name, is_dir, size, cluster)
- fs_open(fs_handle, path) -> file_handle*
- fs_read(file_handle, buf, len, offset) -> read_bytes
- fs_close(file_handle)

当面は同期・ブロック操作のみ。

### FAT12 のオンディスク概観（要点）
- ブートセクタ (ブートレコード): BPB（BIOS Parameter Block）を参照して、
	- バイト/セクタ, セクタ/クラスタ, 予約領域セクタ数, FAT 数, 最大ルートエントリ数, 総セクタ数 等を読み取る。
- FAT 領域: FAT エントリ (12-bit) が並ぶ。クラスタの次を示すチェーンが存在する。
- ルートディレクトリ領域: 固定サイズ領域（FAT12 固有）。
- データ領域: クラスタ番号 2 から始まる実ファイルデータ。

重要なマッピング:
- クラスタ -> セクタ: data_region_first_sector + (cluster - 2) * sectors_per_cluster

### マウントと読み取りフロー
1. fs_mount が呼ばれると、ブロックデバイスからセクタ 0 を読み取り BPB を解析して `fat12_super` を初期化する。
2. ルートディレクトリ領域を読み取りメタ情報（ファイル名, 属性, start_cluster, file_size）を取得できる。
3. fs_readdir は root のセクタチェーンを逐次読む（短いので一括読みで OK）。サブディレクトリはクラスター連鎖をたどる。
4. fs_open -> start_cluster がわかったら `fat12_file` を返す。fs_read はオフセットに対応するクラスタを FAT で追跡してブロックデバイスから読み出す。