この章では、LiteCoreカーネルに組み込まれたシンプルなコマンドシステムとシェルについて説明します。このシステムは、ユーザーがキーボードからコマンドを入力し、登録された関数を実行できるように設計されています。

## アーキテクチャ

### コンポーネント

1. **コマンドレジストリ** (`commands.c`)
   - コマンドの登録・検索・実行
   - コマンドライン解析
   - 最大32個のコマンドをサポート

2. **シェルインターフェース** (`shell.c`)
   - ユーザー入力の処理
   - プロンプト表示
   - バッファ管理

3. **キーボードバッファ** (`keyboard.c`)
   - リングバッファによる文字入力管理
   - ブロッキング/ノンブロッキング取得

## 使い方

### 初期化

```c
#include <shell/shell.h>
#include <shell/commands.h>

// カーネルのmain関数などで
init_shell();  // シェルとコマンドシステムを初期化
```

## 新しいコマンドの追加

### 1. コマンド関数の実装

```c
#include <shell/commands.h>
#include <util/console.h>

static int cmd_mycommand(int argc, char **argv) {
    // 引数チェック
    if (argc < 2) {
        printk("Usage: mycommand <arg>\n");
        return -1;
    }
    
    // コマンドの処理
    printk("Hello from mycommand: %s\n", argv[1]);
    
    return 0;  // 成功時は0
}
```

### 2. コマンドの登録

```c
void register_my_commands(void) {
    register_command("mycommand", "My custom command", cmd_mycommand);
}
```

### 3. 初期化時に登録

```c
init_shell();
register_my_commands();
```

## 組み込みコマンド

### 基本コマンド

- **help** - 利用可能なコマンドを表示
- **echo** - 引数を表示
- **clear** - 画面をクリア

### 拡張コマンド

- **ver** - バージョン情報を表示
- **mem** - メモリ使用状況を表示（未実装）
- **ls** - ファイル一覧を表示（未実装）
- **cat** - ファイルの内容を表示（未実装）
- **uptime** - 起動時間を表示（未実装）

## 例: FSコマンドの実装

```c
static int cmd_ls(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "/";
    
    fs_handle *fh;
    dir_handle *dh;
    
    // FSをマウント（既にマウント済みの場合は取得）
    extern fs_handle *g_fs;
    fh = g_fs;
    
    if (!fh) {
        printk("Error: Filesystem not mounted\n");
        return -1;
    }
    
    // ディレクトリを開く
    if (fs_opendir(fh, path, &dh) != 0) {
        printk("Error: Cannot open directory '%s'\n", path);
        return -1;
    }
    
    // ファイルを列挙
    char name[64];
    int is_dir;
    uint32_t size;
    uint16_t cluster;
    
    printk("Directory listing of '%s':\n", path);
    while (fs_readdir(dh, name, sizeof(name), &is_dir, &size, &cluster) == 0) {
        printk("  %s %-20s %8u bytes\n", 
               is_dir ? "[DIR] " : "[FILE]", 
               name, size);
    }
    
    fs_closedir(dh);
    return 0;
}
```

## APIリファレンス

### コマンド管理

```c
// コマンドシステムの初期化
void init_commands(void);

// コマンドを登録
int register_command(const char *name, 
                     const char *description,
                     command_func_t function);

// コマンドを実行
int execute_command(char *line);

// コマンド一覧を表示
void list_commands(void);
```

### シェル管理

```c
// シェルの初期化
void init_shell(void);

// シェルのメインループ（ブロッキング）
void shell_run(void);

// 1行処理（ポーリング）
int shell_readline_and_execute(void);
```

### キーボード入力

```c
// ブロッキング取得
char keyboard_getchar(void);

// ノンブロッキング取得
char keyboard_getchar_poll(void);
```

## コマンドライン解析

コマンドラインは自動的に空白で分割されます：

```
command arg1 arg2 arg3
```

↓

```
argc = 4
argv[0] = "command"
argv[1] = "arg1"
argv[2] = "arg2"
argv[3] = "arg3"
```

## 制限事項

- 最大コマンド数: 32
- 最大引数数: 16
- コマンドライン長: 256文字
- キーバッファサイズ: 256文字

## 今後の拡張

- [ ] パイプ（`|`）のサポート
- [ ] リダイレクト（`>`, `<`）のサポート
- [ ] 引用符（`"..."`, `'...'`）のサポート
- [ ] コマンド履歴
- [ ] タブ補完
- [ ] エイリアス機能
- [ ] スクリプト実行