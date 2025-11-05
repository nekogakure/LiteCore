#include <util/shell_integration.h>
#include <util/shell.h>
#include <util/commands.h>

/**
 * @brief シェルシステム全体を初期化（すべてのコマンドを登録）
 */
void init_full_shell(void) {
	// シェルの初期化
	init_shell();
	
	// 拡張コマンドの登録
	register_extended_commands();

	// 追加のカスタムコマンドをここで登録
}