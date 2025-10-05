## LiteCore Coding Guide Line
LiteCore Kernelをコーディングする際に、できるだけ守ってほしいガイドラインです。これは、私の個人的な意見も含みます。

### 基本
#### インデントは8です。
4じゃないと見づらい、という人がいるかもしれませんが、それはコードの問題です。私の思う限り、インデントはコードブロックを見やすくするためのものなはずです。インデントが多くて見づらいコードは、つまりブロックが多すぎます。減らしましょう。折返しがされないくらいが程よいと思います。

#### コメントは日本語か英語で書いてください
私は日本語話者ですが、英語でのコメントも良いと思います。ただし、一つのファイル内では統合してください。

#### コンマを使用したブロックの使用を避けてください。
ただ見づらくなり、コードの書き換えが難しくなるだけです。
```c
if (foo == boo) do_1(), do_2();
```

よりも

```c
if (foo == boo) {
        do_1();
        do_2();
}
```
のほうが見やすいのは一目瞭然です。

#### つまるところ
かんたんに書いてください。なにも複雑なコードを書く必要はありません。見やすく、かんたんで、わかりやすければどんなコードでも構いません。

### 命名規則

#### 変数名は snake_case を使用してください
```c
int file_count = 0;
char source_dir[256];
const char* config_value = NULL;
```


#### 関数名も snake_case を使用してください
```c
int load_config(void);
const char* get_config_value(const char* key);
void build_compile_command(void);
```
Rustのようにかっこよさを求める必要はありません。質

#### 定数（#define）は UPPER_SNAKE_CASE を使用してください
```c
#define MAX_LINE_LENGTH 256
#define CONFIG_FILE "../.config"
#define DEFAULT_COMPILER "gcc"
```

#### グローバル変数は避けてください
どうしても必要な場合は、static を使用してファイルスコープに限定してください。
はい。**どうしても必要な場合は**です。

### エラーハンドリング

#### 関数の戻り値でエラーを示してください
```c
// 良い例
int load_file(const char* filename) {
        FILE* fp = fopen(filename, "r");
        if (fp == NULL) {
                return -1;  // エラーを示す
        }
        // ...処理...
        fclose(fp);
        return 0;  // 成功を示す
}

// 使用例
if (load_file("config.txt") != 0) {
        fprintf(stderr, "Failed to load config\n");
        return 1;
}
```

#### NULLポインタのチェックを忘れないでください
```c
const char* value = get_config_value("CC");
if (value == NULL) {
        value = "gcc";  // デフォルト値を使用
}
```

### コメント

#### Doxygenコメント
各ファイル、関数、定数、構造体にはDoxygenコメントをつけてください。

#### その他のコメント
コメントを書くのはとてもいいことですが、書きすぎないでください。

### メモリ管理

#### malloc/freeは必ずペアで使用してください
```c
char* buffer = malloc(1024);
if (buffer == NULL) {
        return -1;
}
// ...使用...
free(buffer);
buffer = NULL;  // ダングリングポインタを避ける
```

#### 配列のサイズを明確にしてください
```c
#define MAX_FILES 100
char files[MAX_FILES][256];  // サイズが明確
```

### ファイル構成

#### ヘッダーファイルにはインクルードガードを使用してください
```c
#ifndef LITECORE_CONFIG_H
#define LITECORE_CONFIG_H

// 宣言...

#endif /* LITECORE_CONFIG_H */
```

#### 各ファイルの先頭にはファイルの説明を書いてください
```c
/**
 * @file config.c
 * @brief 設定ファイルの読み込み機能
 * @details .configファイルを解析してビルド設定を取得します
 */
```

### 関数の設計

#### 関数は一つのことだけを行うようにしてください
```c
// 良い例：単一責任
int count_c_files(const char* dir_path);
int load_file_list(const char* dir_path, char files[][256]);

// 悪い例：複数の責任
int count_and_load_files(const char* dir_path, char files[][256]);
```

#### 関数の引数は5個以下にしてください
引数が多い場合は、構造体を使用することを検討してください。

```c
// 構造体を使用した例
typedef struct {
        char cc[64];
        char cflags[256];
        char output_dir[256];
        int debug_symbols;
} build_config_t;

void build_command(const build_config_t* config, char* command);
```

### テストとデバッグ

#### デバッグ用のprintfは本番コードから削除してください
デバッグが必要な場合は、`debug()`関数を使用してください

### コードレビュー

#### コミット前に自分でコードを見直してください
- 未使用の変数はないか
- メモリリークはないか
- エラーハンドリングは適切か
- コメントは適切か

#### 他の人が読みやすいコードを心がけてください
あなたが夜寝て、スッキリした後にそのコードを見たとき、すぐに理解できるかどうかを考えてください。

## 最後に
このガイドラインは絶対的なものではありません。プロジェクトの要求や状況に応じて、適切に判断して使用してください。重要なのは、一貫性とチーム全体での合意です。