この章では、拡張コマンドについて記述します。
実装されているファイルは[extended_commands.c](../../src/kernel/util/extended_commands.c)です。

## 概要
拡張コマンドモジュールは、ファイルシステム操作やシステム情報表示などの高度なシェルコマンドを提供します。ext2ファイルシステムの操作、メモリ情報の表示、PCIデバイスの列挙などが含まれます。

## 関数 / API

### コマンド関数

#### `cmd_mem`
メモリ使用状況を表示します。

コマンド: `mem`

#### `cmd_ls`
ext2ファイルシステムのルートディレクトリのファイル一覧を表示します。

コマンド: `ls`

#### `cmd_cat`
ファイルの内容を表示します。

コマンド: `cat <filename>`

引数:
  - filename: 表示するファイル名

#### `cmd_pci`
PCIデバイスを列挙して情報を表示します。

コマンド: `pci`

### 登録関数

#### `void register_extended_commands(void)`
すべての拡張コマンドをコマンドシステムに登録します。

引数: なし

## 定数 / 定義

このファイルには定数定義はありません。

## 構造体

このファイルには構造体の定義はありません。

### 提供されるコマンド

- **mem**: メモリ使用状況を表示（未実装）
- **ls**: ファイル一覧を表示（ext2ファイルシステム）
- **cat**: ファイルの内容を表示
- **pci**: PCIデバイスの列挙

### ext2ファイルシステム依存

`ls`と`cat`コマンドは、グローバル変数`g_ext2_sb`を通じてマウントされたext2ファイルシステムにアクセスします。ファイルシステムがマウントされていない場合、エラーを返します。

### 使用例

```
LiteCore> ls
Directory listing of '/':
NAME                 SIZE  TYPE
----------------------------------------
test.txt               123  FILE
readme.md              456  FILE

Total: 2 entries

LiteCore> cat test.txt
Hello, LiteCore!
This is a test file.

LiteCore> pci
Scanning PCI bus...
PCI device found: bus=0 dev=0 func=0 vendor=0x8086 device=0x1237
...
PCI scan complete

LiteCore> mem
Memory information:
  (Memory statistics not yet implemented)
```
