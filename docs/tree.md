この章ではプロジェクトの構成と起動時の構成について説明します。

## プロジェクト構成
プロジェクトは以下のようなディレクトリ構成になっています。

```
/
├─ README.md
├─ Makefile
├─ LICENSE
├─ ...
├─ docs/
│  └─ ...            # ドキュメント
└─ src/
   ├─ kernel.ld      # リンカファイル
   ├─ kernel/        # カーネル
   │  ├─ device/     # デバイスドライバ
   │  ├─ fs/         # ファイルシステム
   │  │  ├─ ext/     # ext周りのドライバ
   │  │  └─ fat/     # fat周りのドライバ
   │  ├─ interrupt/  # 割り込みハンドラとか
   │  ├─ mem/        # メモリ管理
   │  ├─ sync/       # スピンロック
   │  ├─ tests/      # テスト
   │  ├─ util/       # IOなどのユーティリティ
   │  ├─ console.c   # コンソール
   │  ├─ entry.c     # カーネルエントリ（内部でmainを読み出しているだけ）
   │  └─ main.c      # カーネルメイン
   ├─ include/       # ヘッダ
   │  └─ ...         # kernel/と同じ
   └─ boot/          # ブートローダー（LBoot）
```
<!--
/
 README.md
 Makefile
 LICENSE
 ...
 docs/
  ...            # ドキュメント
 src/
  kernel.ld      # リンカファイル
  kernel/        # カーネル
   device/     # デバイスドライバ
   fs/         # ファイルシステム
    ext/     # ext周りのドライバ
    fat/     # fat周りのドライバ
   interrupt/  # 割り込みハンドラとか
   mem/        # メモリ管理
   sync/       # スピンロック
   tests/      # テスト
   util/       # IOなどのユーティリティ
   console.c   # コンソール
   entry.c     # カーネルエントリ（内部でmainを読み出しているだけ）
   main.c      # カーネルメイン
  include/       # ヘッダ
   ...         # kernel/と同じ
  boot/          # LBoot（ブートローダー）
-->

## 起動時の構成
LiteCoreカーネルでは以下のようなファイルツリーを想定しています。

```
/
├─ apps/        # アプリケーション群
├─ boot/        # ブートローダ関連ファイル
├─ kernel/      # カーネルバイナリおよび関連ファイル
├─ users/       # ユーザーデータ（Linuxのホームディレクトリ相当）
└─ data/        # その他データファイル群（設定ファイルなど）
```
<!--
/
 apps/
 boot/
 kernel/
 user/
 data/
-->

### アプリケーションの配置
`/apps/` 以下にアプリケーションバイナリを配置します。
アプリケーションはそれぞれディレクトリとして配置され、その中に実行ファイルや関連リソースを格納します。
アプリケーションは`appname.app/`という形式のディレクトリにまとめられます。（以降、`.app`ディレクトリと呼びます）
例えば、`/apps/calculator.app/`には電卓アプリケーションのバイナリとリソースが含まれます。

`.app`ディレクトリの中には以下のファイルが含まれます。
- `appname.bin`: アプリケーションのメイン実行バイナリ
- `resources/`: 画像や設定ファイルなどのリソースを格納するディレクトリ
- `manifest.json`: アプリケーションのメタ情報（名前、バージョン、概要、パブリッシャー）を記述したJSONファイル
manifest.txtの例:
```json
{
  "name": "Calculator",
  "version": "1.0.0",
  "description": "A simple calculator application.",
  "publisher": "LiteCore Dev Team"
}
```