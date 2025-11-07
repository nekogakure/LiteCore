#include <shell/commands.h>
#include <util/console.h>
#include <mem/manager.h>
#include <fs/ext/ext2.h>
#include <driver/timer/apic.h>
#include <stdint.h>

// 外部宣言: main.cで定義されているグローバル変数
extern struct ext2_super *g_ext2_sb;

// 現在のディレクトリのinode番号（デフォルトはルート）
static uint32_t current_dir_inode = 2; // EXT2_ROOT_INO = 2

// 現在のディレクトリパス
static char current_path[256] = "/";

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

	// 現在のディレクトリのinodeを読み取る
	struct ext2_inode dir_inode;
	int result = ext2_read_inode(g_ext2_sb, current_dir_inode, &dir_inode);
	if (result != 0) {
		printk("Error: Failed to read directory inode (error=%d)\n",
		       result);
		return -1;
	}

	// ディレクトリの内容を一覧表示
	result = ext2_list_dir(g_ext2_sb, &dir_inode);

	if (result < 0) {
		printk("Error: Failed to list directory (error=%d)\n", result);
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
	uint32_t uptime_ms_low = (uint32_t)uptime_ms; /* 下位32bitを取得 */
	uint32_t total_seconds = uptime_ms_low / 1000UL; /* ミリ秒を秒に変換 */

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

static int cmd_change_dir(int argc, char **argv) {
	if (argc < 2) {
		printk("Usage: cd <directory>\n");
		return -1;
	}

	if (g_ext2_sb == NULL) {
		printk("Error: ext2 filesystem not mounted\n");
		return -1;
	}

	const char *path = argv[1];
	uint32_t target_inode = 0;
	char new_path[256];

	// パスを解決
	if (path[0] == '/') {
		// 絶対パス
		int result = ext2_resolve_path(g_ext2_sb, path, &target_inode);
		if (result != 0) {
			printk("cd: %s: No such directory\n", path);
			return -1;
		}
		// パスをコピー
		int i = 0;
		while (path[i] && i < 255) {
			new_path[i] = path[i];
			i++;
		}
		new_path[i] = '\0';
	} else {
		// 相対パス
		// 現在のパスと結合
		int i = 0, j = 0;

		// ".." の処理
		if (path[0] == '.' && path[1] == '.' &&
		    (path[2] == '\0' || path[2] == '/')) {
			// 親ディレクトリへ移動
			if (current_path[0] == '/' && current_path[1] == '\0') {
				// すでにルートにいる場合
				printk("already at root directory :P\n");
				return 0;
			}

			// 現在のパスから最後の '/' を見つける
			int last_slash = -1;
			for (i = 0; current_path[i]; i++) {
				if (current_path[i] == '/') {
					last_slash = i;
				}
			}

			if (last_slash <= 0) {
				// ルートに戻る
				new_path[0] = '/';
				new_path[1] = '\0';
			} else {
				// 最後の '/' までコピー
				for (i = 0; i < last_slash; i++) {
					new_path[i] = current_path[i];
				}
				new_path[i] = '\0';
			}
		} else if (path[0] == '.' &&
			   (path[1] == '\0' || path[1] == '/')) {
			// "." - 現在のディレクトリ（何もしない）
			return 0;
		} else {
			// 通常のディレクトリ名
			// 現在のパスをコピー
			for (i = 0; current_path[i]; i++) {
				new_path[i] = current_path[i];
			}

			// 末尾が '/' でない場合は追加
			if (i > 0 && new_path[i - 1] != '/') {
				new_path[i++] = '/';
			}

			// 新しいディレクトリ名を追加
			for (j = 0; path[j] && i < 255; j++, i++) {
				new_path[i] = path[j];
			}
			new_path[i] = '\0';
		}

		// 新しいパスを解決
		int result =
			ext2_resolve_path(g_ext2_sb, new_path, &target_inode);
		if (result != 0) {
			printk("cd: %s: No such directory\n", path);
			return -1;
		}
	}

	// inodeを読み取ってディレクトリかどうか確認
	struct ext2_inode target_inode_data;
	int result =
		ext2_read_inode(g_ext2_sb, target_inode, &target_inode_data);
	if (result != 0) {
		printk("cd: Failed to read inode\n");
		return -1;
	}

	// ディレクトリかどうか確認
	if ((target_inode_data.i_mode & 0xF000) != 0x4000) { // S_IFDIR = 0x4000
		printk("cd: %s: Not a directory\n", path);
		return -1;
	}

	// カレントディレクトリを更新
	current_dir_inode = target_inode;

	// パスを更新
	int i = 0;
	while (new_path[i] && i < 255) {
		current_path[i] = new_path[i];
		i++;
	}
	current_path[i] = '\0';

	return 0;
}

/**
 * @brief pwdコマンド - 現在のディレクトリを表示
 */
static int cmd_pwd(int argc, char **argv) {
	(void)argc;
	(void)argv;

	printk("%s\n", current_path);
	return 0;
}

/**
 * @brief 現在のディレクトリパスを取得
 */
const char *get_current_directory(void) {
	return current_path;
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
	register_command("cd", "Change directory", cmd_change_dir);
	register_command("pwd", "Print working directory", cmd_pwd);
}
