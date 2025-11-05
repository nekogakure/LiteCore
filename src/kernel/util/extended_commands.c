#include <util/commands.h>
#include <util/console.h>
#include <mem/manager.h>
#include <fs/fs.h>
#include <stdint.h>

// 埋め込みファイルイメージのシンボル
extern const unsigned char _binary_src_file_img_start[];
extern const unsigned char _binary_src_file_img_end[];

// グローバルFSハンドル（初回マウント時に設定）
static fs_handle *g_fs = NULL;

/**
 * @brief FSをマウントする（初回のみ）
 */
static int ensure_fs_mounted(void) {
	if (g_fs != NULL) {
		return 0; // 既にマウント済み
	}
	
	const unsigned char *img = _binary_src_file_img_start;
	size_t img_len = (size_t)((uintptr_t)_binary_src_file_img_end -
				  (uintptr_t)_binary_src_file_img_start);
	
	int r = fs_mount(img, img_len, &g_fs);
	if (r != 0) {
		printk("Error: Failed to mount filesystem (code %d)\n", r);
		return -1;
	}
	
	return 0;
}

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
	
	// FSをマウント
	if (ensure_fs_mounted() != 0) {
		return -1;
	}
	
	// ディレクトリを開く
	dir_handle *dir = NULL;
	int r = fs_opendir(g_fs, path, &dir);
	if (r != 0) {
		printk("Error: Cannot open directory '%s' (code %d)\n", path, r);
		return -1;
	}
	
	printk("Directory listing of '%s':\n", path);
	printk("%-16s %8s  %s\n", "NAME", "SIZE", "TYPE");
	printk("----------------------------------------\n");
	
	// エントリを列挙
	char name[64];
	int is_dir;
	uint32_t size;
	uint16_t cluster;
	int count = 0;
	
	while (1) {
		r = fs_readdir(dir, name, sizeof(name), &is_dir, &size, &cluster);
		if (r != 0) {
			break; // 終端またはエラー
		}
		
		const char *type = is_dir ? "DIR" : "FILE";
		if (is_dir) {
			printk("%-16s %8s  %s\n", name, "", type);
		} else {
			printk("%-16s %8u  %s\n", name, size, type);
		}
		count++;
	}
	
	fs_closedir(dir);
	printk("\nTotal: %d entries\n", count);
	
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
	
	// FSをマウント
	if (ensure_fs_mounted() != 0) {
		return -1;
	}
	
	const char *filename = argv[1];
	
	// ファイルを開く
	file_handle *file = NULL;
	int r = fs_open(g_fs, filename, &file);
	if (r != 0) {
		printk("Error: Cannot open file '%s' (code %d)\n", filename, r);
		return -1;
	}
	
	// ファイル内容を読み込む（最大16KB）
	const size_t max_size = 16 * 1024;
	char *buffer = (char *)kmalloc(max_size);
	if (!buffer) {
		printk("Error: Out of memory\n");
		fs_close(file);
		return -1;
	}
	
	int bytes_read = fs_read(file, buffer, max_size - 1, 0);
	if (bytes_read < 0) {
		printk("Error: Cannot read file '%s' (code %d)\n", filename, bytes_read);
		kfree(buffer);
		fs_close(file);
		return -1;
	}
	
	// NULL終端して表示
	buffer[bytes_read] = '\0';
	printk("%s", buffer);
	
	// 改行がない場合は追加
	if (bytes_read > 0 && buffer[bytes_read - 1] != '\n') {
		printk("\n");
	}
	
	kfree(buffer);
	fs_close(file);
	
	return 0;
}

/**
 * @brief verコマンド - バージョン情報を表示
 */
static int cmd_ver(int argc, char **argv) {
	(void)argc;
	(void)argv;
	
	printk("LiteCore Operating System\n");
	printk("Version: %s\n", VERSION);
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
