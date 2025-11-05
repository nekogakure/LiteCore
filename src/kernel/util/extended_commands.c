#include <util/commands.h>
#include <util/console.h>
#include <mem/manager.h>
#include <fs/fs.h>

/**
 * @brief memコマンド - メモリ使用状況を表示
 */
static int cmd_mem(int argc, char **argv) {
	(void)argc;
	(void)argv;
	
	// TODO: メモリマネージャーから統計情報を取得
	printk("Memory information:\n");
	printk("  (Memory statistics not yet implemented)\n");
	
	return 0;
}

/**
 * @brief lsコマンド - ファイル一覧を表示
 */
static int cmd_ls(int argc, char **argv) {
	const char *path = "/";
	
	if (argc > 1) {
		path = argv[1];
	}
	
	// TODO: FSシステムからファイル一覧を取得
	printk("Directory listing of '%s':\n", path);
	printk("  (Filesystem listing not yet fully integrated)\n");
	
	return 0;
}

/**
 * @brief catコマンド - ファイルの内容を表示
 */
static int cmd_cat(int argc, char **argv) {
	if (argc < 2) {
		printk("Usage: cat <filename>\n");
		return -1;
	}
	
	// TODO: FSシステムからファイルを読み取る
	printk("Reading file '%s'...\n", argv[1]);
	printk("  (File reading not yet fully integrated)\n");
	
	return 0;
}

/**
 * @brief verコマンド - バージョン情報を表示
 */
static int cmd_ver(int argc, char **argv) {
	(void)argc;
	(void)argv;
	
	printk("LiteCore Operating System\n");
	printk("Version: 0.1.0 (Development)\n");
	printk("Build: %s %s\n", __DATE__, __TIME__);
	
	return 0;
}

/**
 * @brief uptimeコマンド - 起動時間を表示
 */
static int cmd_uptime(int argc, char **argv) {
	(void)argc;
	(void)argv;
	
	// TODO: タイマーから起動時間を取得
	printk("System uptime:\n");
	printk("  (Timer not yet implemented)\n");
	
	return 0;
}

/**
 * @brief 拡張コマンドを登録
 */
void register_extended_commands(void) {
	register_command("mem", "Display memory information", cmd_mem);
	register_command("ls", "List directory contents", cmd_ls);
	register_command("cat", "Display file contents", cmd_cat);
	register_command("ver", "Display version information", cmd_ver);
	register_command("uptime", "Display system uptime", cmd_uptime);
}
