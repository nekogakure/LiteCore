この章では、開発に便利なツールについて記述します。

## fmt.sh

### 概要
コードフォーマッターツールです。`clang-format`を使用してソースコードを整形します。

### 使い方
```bash
./tools/fmt.sh
```

### 機能
- `src/kernel`と`src/include`ディレクトリ内のすべてのC/Cヘッダファイルを検索
- `clang-format`を使用して一括整形（-iオプションで直接ファイルを更新）

## mk_ext2_image.py

### 概要
ext2ファイルシステムイメージを作成するPythonスクリプトです。指定されたファイルやディレクトリをext2イメージに含めることができます。

### 使い方
```bash
python3 tools/mk_ext2_image.py <output_image> <size_kb> [files_or_dir...]
```

### 例
```bash
# 1MBのイメージを作成し、ファイルを追加
python3 tools/mk_ext2_image.py ext2.img 1024 test.txt readme.md

# 2MBのイメージを作成し、ディレクトリ全体をコピー
python3 tools/mk_ext2_image.py ext2.img 2048 test_data/
```

### 引数
- `output_image`: 出力イメージファイルのパス
- `size_kb`: イメージサイズ（KB単位）
- `files_or_dir`: コピーするファイルまたはディレクトリのリスト（複数指定可）

### 機能
1. 指定されたサイズの空イメージファイルを作成
2. `mkfs.ext2`を使用してext2ファイルシステムを作成
3. イメージをマウント（`sudo mount`を使用、root権限が必要）
4. 指定されたファイルまたはディレクトリをイメージにコピー
5. イメージをアンマウント
6. 出力パスにイメージをコピー

### 必要な依存関係
- Python 3
- `mkfs.ext2`コマンド
- `mount`コマンド（sudoアクセス）
- `cp`コマンド

## mk_fat12_image.py

### 概要
FAT12ファイルシステムイメージを作成するPythonスクリプトです。軽量で、外部ツールに依存せずにFAT12イメージを生成できます。

### 使い方
```bash
python3 tools/mk_fat12_image.py <output.img> <inputfile1:OUTNAME1> <inputfile2:OUTNAME2> ...
```

### 例
```bash
# test.txtをTEST.TXTとして、readme.mdをREADME.MDとしてイメージに追加
python3 tools/mk_fat12_image.py fat12.img test.txt:TEST.TXT readme.md:README.MD
```

### 引数
- `output.img`: 出力FAT12イメージファイル
- `inputfile:OUTNAME`: 入力ファイルとイメージ内のファイル名のペア（8.3形式）

### 機能
1. ブートセクタを作成（512バイトセクタ、1セクタ/クラスタ）
2. FAT領域を初期化（2つのFATテーブル）
3. ルートディレクトリエントリを作成（最大16エントリ）
4. ファイルデータをデータ領域にコピー
5. FATチェーンを適切に設定

### FAT12イメージ構造
- セクタ0: ブートセクタ
- セクタ1: FAT1
- セクタ2: FAT2（FAT1のコピー）
- セクタ3: ルートディレクトリ
- セクタ4以降: データ領域（クラスタ2から開始）

### 制限事項
- シンプルな実装（1クラスタ=1セクタ）
- ルートディレクトリのみサポート
- ファイル名は8.3形式
- 小容量イメージ向け

### 必要な依存関係
- Python 3のみ（外部ツール不要）
