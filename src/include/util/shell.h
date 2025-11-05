#ifndef _UTIL_SHELL_H
#define _UTIL_SHELL_H

/**
 * @brief シェルの初期化
 */
void init_shell(void);

/**
 * @brief シェルのメインループを実行
 */
void shell_run(void);

/**
 * @brief 1行のコマンドを読み取って実行
 * @return 読み取った文字数
 */
int shell_readline_and_execute(void);

#endif // _UTIL_SHELL_H
