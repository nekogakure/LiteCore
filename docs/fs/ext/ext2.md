この章では、LiteCoreカーネルに実装されたext2ファイルシステムの概要、設計、使用方法について説明します。

- `src/include/fs/ext/ext2.h` - ext2構造体とAPI定義
- `src/kernel/fs/ext/ext2.c` - ext2実装
- `src/kernel/fs/fs.c` - VFSレイヤー（ext2対応）
- `src/kernel/tests/ext2_test.c` - ext2テスト
- `tools/mk_ext2_image.py` - ext2イメージ作成ツール

### 実装機能

1. **マウント**: ext2イメージの認識とマウント
2. **inode読み取り**: inode番号からinodeを読み取る
3. **ディレクトリ列挙**: ルートディレクトリのファイル一覧
4. **ファイル読み取り**: 通常ファイルの内容を読み取る
5. **VFS統合**: FAT12と同じインターフェースで使用可能

### 制限事項

- **読み取り専用**: 書き込み操作は未実装
- **直接ブロックのみ**: 間接ブロックポインタは未対応（最大12ブロック = 48KB @ 4KB/block）
- **ルートディレクトリのみ**: サブディレクトリの探索は未実装
- **シンプルなパス**: パス解析は基本的な機能のみ

## テスト方法

### 1. ext2イメージの作成

```bash
# テスト用ファイルを用意
cd src
echo "Hello ext2!" > test.txt

# ext2イメージを作成（1MBサイズ）
cd ..
python3 tools/mk_ext2_image.py src/ext2.img 1024 src/test.txt src/readme.md

# 注意: このスクリプトは sudo 権限が必要です
```

### 2. Makefileの更新

Makefileに以下を追加して、ext2イメージを埋め込みます：

```makefile
# ext2テスト用にイメージを埋め込む
bin/src_ext2_img.o: src/ext2.img
	objcopy -I binary -O elf32-i386 -B i386 \
		--rename-section .data=.rodata,alloc,load,readonly,data,contents \
		src/ext2.img bin/src_ext2_img.o

# リンク時に追加
bin/kernel.elf: ... bin/src_ext2_img.o
```

### 3. テストの有効化

`src/include/tests/define.h`で以下を有効化：

```c
#define TEST_TRUE
#define EXT2_TEST
```

### 4. ビルドと実行

```bash
make clean && make all
make run
```

## ext2構造

### スーパーブロック

- オフセット: 1024バイト
- サイズ: 1024バイト
- マジックナンバー: 0xEF53

### ブロックグループディスクリプタ

スーパーブロックの次のブロックに配置されます。

### inode構造

- サイズ: 128バイト（デフォルト）
- ブロックポインタ: 15個
  - 0-11: 直接ブロックポインタ
  - 12: 間接ブロックポインタ（未実装）
  - 13: 二重間接ブロックポインタ（未実装）
  - 14: 三重間接ブロックポインタ（未実装）

### ディレクトリエントリ

可変長構造：
- inode番号: 4バイト
- レコード長: 2バイト
- 名前長: 1バイト
- ファイルタイプ: 1バイト
- ファイル名: 可変長

## 使用例

### コマンドラインから

```
LiteCore> ls
Directory listing of '/':
NAME                 SIZE  TYPE
----------------------------------------
test.txt               123  FILE
readme.md              456  FILE

Total: 2 entries

LiteCore> cat test.txt
Hello ext2!
This is a test file for the ext2 filesystem implementation.
```

### プログラムから

```c
#include <fs/fs.h>

// マウント
fs_handle *fh;
fs_mount(ext2_image, image_size, &fh);

// ディレクトリ一覧
dir_handle *dir;
fs_opendir(fh, "/", &dir);

char name[64];
int is_dir;
uint32_t size;
while (fs_readdir(dir, name, sizeof(name), &is_dir, &size, NULL) == 0) {
    printk("%s: %u bytes\n", name, size);
}
fs_closedir(dir);

// ファイル読み取り
file_handle *file;
fs_open(fh, "test.txt", &file);

char buf[256];
int n = fs_read(file, buf, sizeof(buf), 0);
buf[n] = '\0';
printk("%s\n", buf);

fs_close(file);
fs_unmount(fh);
```

## API仕様

### ext2_mount

```c
int ext2_mount(const void *image, size_t size, struct ext2_super **out);
```

ext2イメージをマウントします。

- 戻り値: 0=成功、負値=エラー

### ext2_read_inode

```c
int ext2_read_inode(struct ext2_super *sb, uint32_t inode_num,
                    struct ext2_inode *inode);
```

inode番号からinodeを読み取ります。

### ext2_find_file_in_dir

```c
int ext2_find_file_in_dir(struct ext2_super *sb, struct ext2_inode *dir_inode,
                          const char *name, uint32_t *out_inode);
```

ディレクトリ内からファイル名を検索します。

### ext2_read_file

```c
int ext2_read_file(struct ext2_super *sb, const char *name, void *buf,
                   size_t len, size_t *out_len);
```

ルートディレクトリからファイルを読み取ります。

### ext2_list_root

```c
int ext2_list_root(struct ext2_super *sb);
```

ルートディレクトリの内容をデバッグ出力します。

## 今後の拡張

1. **間接ブロックポインタ**: 大きなファイル対応
2. **サブディレクトリ**: パス探索の完全実装
3. **シンボリックリンク**: リンク解決
4. **書き込み対応**: ファイル作成・変更・削除
5. **拡張属性**: xattr対応
6. **ジャーナリング**: ext3/ext4互換性