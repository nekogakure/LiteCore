この章では、シェルシステムの統合初期化について記述します。
実装されているファイルは[shell_integration.c](https://github.com/nekogakure/LiteCore/blob/main/src/kernel/shell/shell_integration.c), [shell_integration.h](https://github.com/nekogakure/LiteCore/blob/main/src/include/shell/shell_integration.h)です。

## 概要
シェルシステム全体の初期化を統合的に管理するモジュールです。シェル本体の初期化と、すべてのコマンド（組み込みと拡張）の登録を一括で行います。

## 関数 / API

#### `void init_full_shell(void)`
シェルシステム全体を初期化します。以下の処理を順に実行します：

1. シェル本体の初期化（`init_shell()`）
   - ウェルカムメッセージの表示
   - コマンドシステムの初期化
   - 組み込みコマンドの登録（help, echo, clear）
   - プロンプトの表示
2. 拡張コマンドの登録（`register_extended_commands()`）
   - mem, ls, cat, ver, uptime, cd, pwd, devices
3. カスタムコマンドの登録（必要に応じて追加）

引数: なし

## 定数 / 定義

このファイルには定数定義はありません。

## 構造体

このファイルには構造体の定義はありません。

## 実装の詳細

### 統合初期化の利点
`init_full_shell()`を使用することで、シェルシステム全体を1つの関数呼び出しで初期化できます。これにより、メインカーネル（`kmain`）から簡単にシェルを起動できます。

### 拡張性
カスタムコマンドを追加する場合、`init_full_shell()`内で`register_command()`を呼び出すだけで済みます。これにより、新しいコマンドを簡単に統合できます。

### 初期化順序
1. まずシェル本体と組み込みコマンドを初期化
2. 次に拡張コマンドを登録
3. 最後にカスタムコマンドを登録

この順序により、基本的なシェル機能が確実に利用可能になってから、拡張機能が追加されます。
