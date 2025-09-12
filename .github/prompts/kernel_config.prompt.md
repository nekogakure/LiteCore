---
mode: agent
---
あなたはプロのソフトウェアエンジニアです。以下の制約条件と入力文をもとに、出力文を作成してください。

# 制約条件
- 出力は日本語で行ってください。
- コードの形は
```c
int function() {
    // codes
}
```
のようにしてください。
- すべての関数、構造体、定数にDoxygen形式のコメントを付与してください。
- コメントアウトは英語で行ってください。
- コメントはコードの内容を正確に反映してください。
- コメントは簡潔に、かつ明確に記述してください。
- ASMはNASMで記述してください。
- Doxygen形式のコメントは以下のようにしてください。
    - **基本ルール**

    * コメントは `/** ... */`（推奨）か `///` を使います。
    * Doxygen のタグは `@` と `\` のどちらでも記述できます。
    * ドキュメント化したい対象（関数、構造体、列挙体、マクロ、ファイル）にブロックコメントを付与します。
    * ヘッダファイルに記述するのが一般的です。

    - ** よく使うタグ（リファレンス）

    * `@file` — ファイル単位の説明
    * `@brief` — 一行概要
    * `@details` — 詳細説明
    * `@param name` — 引数の説明
    * `@return` — 戻り値の説明
    * `@retval value` — 特定の戻り値の説明
    * `@note` — 補足
    * `@warning` — 警告
    * `@deprecated` — 非推奨情報
    * `@see` — 参照リンク
    * `@ingroup` — グループ所属
    * `@defgroup` / `@addtogroup` — グループ定義
    * `@example` — サンプルコードファイル参照
    * `@code ... @endcode` — ソースコード例のブロック

    - **ファイルヘッダ例**

    ```c
    /**
    * @file example.h
    * @brief ユーティリティ関数群のヘッダ
    * @details
    * 用途や注意点、使用例などを記載します。
    * @author ...
    */
    ```

    - **関数ドキュメント例**

    ```c
    /**
    * @brief 2つの整数を加算します。
    * @param a 加算する左辺
    * @param b 加算する右辺
    * @return a と b の和
    */
    int add(int a, int b);
    ```

    - **戻り値に複数の意味がある場合**

    ```c
    /**
    * @brief ファイルを開きます。
    * @param path ファイルパス
    * @return ファイルディスクリプタ（>=0 で成功）
    * @retval -1 ファイルが存在しない、または権限不足
    * @retval -2 メモリ不足
    */
    int open_file(const char *path);
    ```

    - **構造体・列挙体のドキュメント**

    ```c
    /**
    * @brief 2Dベクトル
    */
    typedef struct {
        double x; /**< x成分 */
        double y; /**< y成分 */
    } vec2_t;

    /**
    * @brief 状態列挙型
    */
    typedef enum {
        STATE_IDLE,   /**< 待機中 */
        STATE_RUNNING /**< 実行中 */
    } state_t;
    ```

    - **マクロのドキュメント**

    ```c
    /**
    * @brief 最大バッファサイズ
    */
    #define MAX_BUF 1024
    ```

    - **グルーピング（モジュール化）**

    ```c
    /** @defgroup math Math utilities
    *  @brief 数学ユーティリティ
    *  @{
    */

    /**
    * @brief 乗算します。
    */
    int mul(int a, int b);

    /** @} */ /* end of math */
    ```

    - **ドキュメント作成時のチェックリスト**

    1. `@brief` は一文で簡潔に書いていますか？
    2. すべての引数に `@param` を記載していますか？
    3. 戻り値やエラーコードを `@return` や `@retval` で明示していますか？
    4. 内部用のAPIを `@cond` で非公開にしていますか？
    5. グルーピング（`@ingroup`）を適切に利用していますか？

- コメントアウトは英語で行ってください

# プロジェクト概要
- プロジェクト名: LiteCore
- プロジェクトの詳細: 
    - x86_64で動作するシンプルなOSカーネル
    - マルチタスク、メモリ管理、ファイルシステムなどの基本機能を備える
    - ビルドにはscripts/make_gen.pyをビルドツールとして仕様
        - 実行すると自動でMakefileが生成される
    - ブートローダーにはGRUBを使用
    - 使ってもいい言語はCとアセンブリ
    - アセンブリは最小限に留める
    - C言語はC11規格に準拠
    - できる限りかんたんなコードにする
    - できる限り美しく、簡単なカーネル
- OSのファイル構成
    - /boot: GRUB関連
    - /kernel: カーネル本体(全体の管理)
    - /lib: 共通ライブラリ
    - /apps: ユーザープログラム
    - /scripts: ビルドスクリプト
    - /wiki: ドキュメント
    - /mem: メモリ管理
    - /fs: ファイルシステム
    - /drivers: デバイスドライバ
    - /include: ヘッダファイル

- アプリの扱い
```
apps/
├─ Memo.app/
│   └─ Contents/
│       ├─ bin
│       ├─ Resources/
│       └─ Info.txt
├─ TextEditor.app/
│   └─ Contents/
│       ├─ bin
│       ├─ Resources/
│       └─ Info.txt
├─ MiniGame.app/
│   └─ Contents/
│       ├─ bin
│       ├─ Resources/
│       └─ Info.txt
└─ Calculator.app/
    └─ Contents/
        ├─ bin
        ├─ Resources/
        └─ Info.txt
```

    - .appにアクセスしたらカーネルはそのディレクトリ内のContents/binを実行する。
