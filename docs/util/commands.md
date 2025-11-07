この章では、コマンドシステムについて記述します。
実装されているファイルは[commands.c](../../src/kernel/util/commands.c), [commands.h](../../src/include/shell/commands.h)です。

## 概要
コマンドシステムは、シェルで実行可能なコマンドの登録と実行を管理します。コマンドラインの解析、引数の分割、コマンドハンドラの呼び出しを行います。最大32個のコマンドを登録でき、各コマンドには名前、説明、実行関数が関連付けられます。

## 関数 / API

#### `void init_commands(void)`
コマンドシステムを初期化します。コマンドカウンタをリセットします。

引数: なし

#### `int register_command(const char *name, const char *description, command_func_t function)`
コマンドを登録します。

引数:
  - name(const char*): コマンド名
  - description(const char*): コマンドの説明
  - function(command_func_t): コマンドの実行関数

戻り値: 成功時0、失敗時-1（コマンド数が上限に達した場合）

#### `int execute_command(char *line)`
コマンドラインを解析し、対応するコマンドを実行します。

引数:
  - line(char*): コマンドライン文字列

戻り値: コマンドの実行結果（0=成功、負値=エラー）

#### `void list_commands(void)`
登録されている全コマンドとその説明を表示します。

引数: なし

#### `void register_builtin_commands(void)`
組み込みコマンド（help、clear、echo等）を登録します。

引数: なし

#### `void register_extended_commands(void)`
拡張コマンド（ls、cat、pci等）を登録します。

引数: なし

## 定数 / 定義

- `MAX_COMMANDS`: 32 - 登録可能な最大コマンド数
- `MAX_ARGS`: 16 - 1つのコマンドラインに含められる最大引数数
- `CMD_BUFFER_SIZE`: 256 - コマンドラインバッファの最大サイズ

## 構造体

#### `typedef int (*command_func_t)(int argc, char **argv)`
シェルコマンドの関数ポインタ型です。

引数:
  - argc(int): 引数の数
  - argv(char**): 引数の配列

戻り値: コマンドの実行結果（0=成功、0以外=エラー）

#### `struct shell_command_t`
シェルコマンドの情報を格納する構造体です。

- `name(const char*)`: コマンド名
- `description(const char*)`: コマンドの説明
- `function(command_func_t)`: 実行する関数

### コマンドライン解析

コマンドラインは空白文字（スペース、タブ、改行）で区切られたトークンに分割されます。最初のトークンがコマンド名で、残りが引数として渡されます。

### 使用例

```c
// カスタムコマンドを定義
int my_command(int argc, char **argv) {
    printk("My command with %d arguments\n", argc);
    return 0;
}

// コマンドを登録
register_command("mycommand", "My custom command", my_command);

// シェルから実行
// LiteCore> mycommand arg1 arg2
```

### 組み込みコマンド

- `help`: 利用可能なコマンドの一覧を表示
- `clear`: 画面をクリア
- `echo`: 引数をそのまま表示
