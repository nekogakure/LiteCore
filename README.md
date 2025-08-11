# LiteCore
中学生によって作成された、x86_64で動作するシンプルなカーネル

## ドキュメント
[GithubWiki](https://github.com/nekogakure/LiteCore/wiki)を参照してください

## ビルド
### Linux
**必要なツール**

    1. GCC
    2. make
    3. nasm
    4. Python3
    5. QEMU-System

- インストール方法（Debian / Ubuntu系）

以下のコマンドで必要なツールをインストールします
```sh
$ sudo apt update
$ sudo apt install build-essential make nasm python3 qemu-system
```
- インストール方法（pacman）

以下のコマンドで必要なツールをインストールします
```sh
$ sudo pacman -Syu base-devel make nasm python qemu
```

**ビルド手順**
1. このリポジトリをクローンします
```sh
$ git clone https://github.com/nekogakure/LiteCore.git
$ cd LiteCore
```

2. ビルドツールに実行権限を付与します
```sh
$ chmod +x ./run.sh
$ chmod +x ./make.sh
$ chmod +x ./debug.sh
```
3. それぞれのスクリプトの役割
    1. `run.sh`: プロジェクトをビルドし、QEMUで実行します
    2. `make.sh`: プロジェクトをビルドします
    3. `debug.sh`: プロジェクトをビルドし、QEMUで実行して、ログをすべて`qemu.log`に出力します

## ライセンス
[ライセンスファイル](./LICENSE)を参照してください

## 貢献
バグレポート、機能提案、プルリクエストを歓迎します
ディスカッションは[こちら](https://github.com/nekogakure/LiteCore/discussions)で行われています

## 免責事項
このOSは学習目的で作成されており、実用性や安全性は保証されません
実機での実行は自己責任で行ってください