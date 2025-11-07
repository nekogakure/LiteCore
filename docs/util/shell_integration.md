この章では、シェル統合機能について記述します。
実装されているファイルは[shell_integration.c](../../src/kernel/util/shell_integration.c), [shell_integration.h](../../src/include/shell/shell_integration.h)です。

## 概要
シェル統合モジュールは、シェルシステム全体の初期化を簡素化します。標準シェル、組み込みコマンド、拡張コマンドを一括で初期化します。

## 関数 / API

#### `void init_full_shell(void)`
シェルシステム全体を初期化します。以下の処理を実行します：

1. シェルの初期化（`init_shell()`）
2. 拡張コマンドの登録（`register_extended_commands()`）
3. 追加のカスタムコマンドの登録（必要に応じて）

引数: なし

## 定数 / 定義

このファイルには定数定義はありません。

## 構造体

このファイルには構造体の定義はありません。

### 初期化シーケンス

`init_full_shell()`を呼び出すことで、以下が自動的に実行されます：

1. **コンソール初期化**: ウェルカムメッセージの表示
2. **コマンドシステム初期化**: コマンドテーブルのリセット
3. **組み込みコマンド登録**: help、clear、echoなど
4. **拡張コマンド登録**: ls、cat、pci、memなど
5. **プロンプト表示**: "LiteCore> "の表示

### 使用方法

```c
// カーネルのメインループ内で
if (!shell_started) {
    char c = keyboard_getchar_poll();
    if (c != 0) {
        shell_started = 1;
        printk("\n");
        init_full_shell();  // シェル全体を初期化
    }
}
```

### 拡張性

追加のカスタムコマンドは、`init_full_shell()`内で`register_command()`を呼び出して登録できます。
