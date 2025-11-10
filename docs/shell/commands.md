この章では、シェルコマンドシステムについて記述します。
実装されているファイルは[commands.c](../../../src/kernel/shell/commands.c), [commands.h](../../../src/include/shell/commands.h)です。

## 概要
シェルコマンドシステムは、ユーザーがテキストベースのコマンドを入力し実行するための基盤機能を提供します。コマンドの登録、パース、実行を管理し、組み込みコマンド（help, echo, clear）を含みます。

## 関数 / API

#### `void init_commands(void)`
コマンドシステムを初期化します。コマンドカウンタをリセットします。

引数: なし

#### `int register_command(const char *name, const char *description, command_func_t function)`
新しいコマンドを登録します。最大32個のコマンドを登録できます。

引数:
  - name(const char*): コマンド名
  - description(const char*): コマンドの説明
  - function(command_func_t): コマンドの実行関数

戻り値: 0=成功、-1=失敗（コマンドリスト満杯または無効な引数）

#### `int execute_command(char *line)`
コマンドライン文字列をパースして実行します。空白で区切られた引数を解析し、該当するコマンドを検索して実行します。

引数:
  - line(char*): 実行するコマンドライン文字列

戻り値: コマンドの実行結果（0=成功、負値=エラー）

#### `void list_commands(void)`
登録されている全コマンドを表示します。コマンド名と説明を一覧表示します。

引数: なし

#### `void register_builtin_commands(void)`
組み込みコマンド（help, echo, clear）を登録します。

引数: なし

#### `const char *get_current_directory(void)`
現在のディレクトリパスを取得します。

引数: なし

戻り値: 現在のディレクトリパス

## 定数 / 定義

- `MAX_COMMANDS`: 32 - 登録可能な最大コマンド数
- `MAX_ARGS`: 16 - コマンドラインの最大引数数
- `CMD_BUFFER_SIZE`: 256 - コマンドラインバッファの最大サイズ

## 構造体

#### `typedef int (*command_func_t)(int argc, char **argv)`
シェルコマンドの関数ポインタ型です。

- argc(int): 引数の数
- argv(char**): 引数の配列

戻り値: コマンドの実行結果（0=成功、0以外=エラー）

#### `struct shell_command_t`
シェルコマンドの情報を格納する構造体です。

- `name(const char*)`: コマンド名
- `description(const char*)`: コマンドの説明
- `function(command_func_t)`: 実行する関数

## 実装の詳細

### コマンドパース
`parse_command_line()`関数は、コマンドライン文字列を空白文字（スペース、タブ、改行、復帰）で区切り、引数配列に分割します。

### コマンド検索
`find_command()`関数は、登録されているコマンドリストから指定された名前のコマンドを線形探索します。

### 組み込みコマンド

- **help**: 登録されている全コマンドを表示
- **echo**: 引数をコンソールに表示
- **clear**: コンソール画面をクリア（25行の改行とANSIエスケープシーケンス）

### エラーハンドリング
- コマンドラインが長すぎる場合（256バイト以上）
- 未知のコマンドが指定された場合
- コマンドリストが満杯の場合（32個以上）
