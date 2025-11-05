#ifndef _UTIL_COMMANDS_H
#define _UTIL_COMMANDS_H

/**
 * @typedef command_func_t
 * @brief シェルコマンドの関数ポインタ型
 * @param argc 引数の数
 * @param argv 引数の配列
 * @return コマンドの実行結果（0: 成功、0以外: エラー）
 */
typedef int (*command_func_t)(int argc, char **argv);

/**
 * @struct shell_command_t
 * @brief シェルコマンドの情報を格納する構造体
 */
typedef struct {
        const char *name;           /// コマンド名
        const char *description;    /// コマンドの説明
        command_func_t function;    /// 実行する関数
} shell_command_t;

// コマンドシステムの初期化・管理関数

/**
 * @brief コマンドシステムの初期化
 */
void init_commands(void);

/**
 * @brief コマンドを登録
 * @param name コマンド名
 * @param description コマンドの説明
 * @param function コマンドの実行関数
 * @return 成功時0、失敗時-1
 */
int register_command(const char *name, const char *description,
                     command_func_t function);

/**
 * @brief コマンドを実行
 * @param line コマンドライン文字列
 * @return コマンドの実行結果
 */
int execute_command(char *line);

/**
 * @brief 登録されている全コマンドを表示
 */
void list_commands(void);

/**
 * @brief 組み込みコマンドを登録
 */
void register_builtin_commands(void);

/**
 * @brief 拡張コマンドを登録
 */
void register_extended_commands(void);

#endif // _UTIL_COMMANDS_H