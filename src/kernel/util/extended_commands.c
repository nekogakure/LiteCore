#include <util/commands.h>
#include <util/console.h>
#include <mem/manager.h>
#include <fs/ext/ext2.h>
#include <driver/timer/apic.h>
#include <stdint.h>

// 外部宣言: main.cで定義されているグローバル変数
extern struct ext2_super *g_ext2_sb;

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
	(void)argc;
	(void)argv;

	if (g_ext2_sb == NULL) {
		printk("Error: ext2 filesystem not mounted\n");
		return -1;
	}

	// ルートディレクトリのファイル一覧を表示
	int count = ext2_list_root(g_ext2_sb);

	if (count < 0) {
		printk("Error: Failed to list directory\n");
		return -1;
	}

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

	if (g_ext2_sb == NULL) {
		printk("Error: ext2 filesystem not mounted\n");
		return -1;
	}

	const char *filename = argv[1];

	// 最大8KBのファイルを読み込む（スタック上）
	char buffer[8192];
	size_t bytes_read = 0;

	// ルートディレクトリからファイルを読み込む
	int result = ext2_read_file(g_ext2_sb, filename, buffer, sizeof(buffer),
				    &bytes_read);

	if (result != 0) {
		printk("Error: Failed to read file '%s' (error code: %d)\n",
		       filename, result);
		return -1;
	}

	if (bytes_read == 0) {
		printk("(empty file)\n");
		return 0;
	}

	// ファイル内容を表示
	for (size_t i = 0; i < bytes_read; i++) {
		printk("%c", buffer[i]);
	}

	// 改行で終わっていない場合は改行を追加
	if (buffer[bytes_read - 1] != '\n') {
		printk("\n");
	}

	return 0;
}

/**
 * @brief verコマンド - バージョン情報を表示
 */
static int cmd_ver(int argc, char **argv) {
	(void)argc;
	(void)argv;

	printk("LiteCore Kernel\n");
	printk("Version: %s\n", VERSION);
	printk("Build: %s %s\n", __DATE__, __TIME__);
	printk("Author: nekogakure\n");

	return 0;
}

/**
 * @brief uptimeコマンド - 起動時間を表示
 */
static int cmd_uptime(int argc, char **argv) {
	(void)argc;
	(void)argv;

	if (!apic_timer_available()) {
		printk("Uptime: APIC timer not available\n");
		return 0;
	}

	uint64_t uptime_ms = apic_get_uptime_ms();
	/* 32bit演算で時間を計算 */
	uint32_t uptime_ms_low = (uint32_t)uptime_ms;  /* 下位32bitを取得 */
	uint32_t total_seconds = uptime_ms_low / 1000UL;  /* ミリ秒を秒に変換 */
	
	uint32_t days = total_seconds / 86400UL;
	uint32_t hours = (total_seconds % 86400UL) / 3600UL;
	uint32_t minutes = (total_seconds % 3600UL) / 60UL;
	uint32_t seconds = total_seconds % 60UL;

	printk("System uptime: ");
	if (days > 0) {
		printk("%u days, ", days);
	}
	printk("%02u:%02u:%02u\n", hours, minutes, seconds);

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
