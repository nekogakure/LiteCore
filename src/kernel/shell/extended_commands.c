#include <shell/commands.h>
#include <util/console.h>
#include <mem/manager.h>
#include <mem/map.h>
#include <fs/ext/ext2.h>
#include <driver/timer/apic.h>
#include <device/pci.h>
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

	printk("Memory information:\n");

	const memmap_t *mm = memmap_get();
	if (!mm || mm->frames == 0) {
		printk("Physical frame map: not initialized\n");
	} else {
		uint32_t total_frames = mm->frames;
		uint32_t used_frames = 0;
		for (uint32_t i = 0; i < total_frames; ++i) {
			uint32_t word = mm->bitmap[i / 32];
			uint32_t bit = (word >> (i % 32)) & 1u;
			if (bit)
				used_frames++;
		}
		uint32_t free_frames = total_frames - used_frames;

		uint32_t total_bytes = total_frames * FRAME_SIZE;
		uint32_t used_bytes = used_frames * FRAME_SIZE;
		uint32_t free_bytes = free_frames * FRAME_SIZE;

		printk("Physical frames: total=%u (%.2fMB) used=%u (%.2fMB) free=%u (%.2fMB)\n",
			   total_frames, (double)total_bytes / (1024.0 * 1024.0),
			   used_frames, (double)used_bytes / (1024.0 * 1024.0),
			   free_frames, (double)free_bytes / (1024.0 * 1024.0));
	}

	/* Heap statistics */
	uint32_t heap_total = heap_total_bytes();
	uint32_t heap_free = heap_free_bytes();
	uint32_t heap_largest = heap_largest_free_block();

	printk("total=%u bytes (%.2fKB) free=%u bytes (%.2fKB) largest_free=%u bytes\n",
		   heap_total, (double)heap_total / 1024.0,
		   heap_free, (double)heap_free / 1024.0,
		   heap_largest);

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
 * @brief PCIクラスコードから説明文字列を取得
 */
static const char *pci_get_class_name(uint8_t base_class, uint8_t sub_class) {
	switch (base_class) {
	case 0x00:
		return "Unclassified";
	case 0x01:
		switch (sub_class) {
		case 0x01: return "IDE Controller";
		case 0x05: return "ATA Controller";
		case 0x06: return "SATA Controller";
		case 0x08: return "NVME Controller";
		default: return "Mass Storage Controller";
		}
	case 0x02:
		return "Network Controller";
	case 0x03:
		switch (sub_class) {
		case 0x00: return "VGA Controller";
		case 0x01: return "XGA Controller";
		default: return "Display Controller";
		}
	case 0x04:
		return "Multimedia Controller";
	case 0x05:
		return "Memory Controller";
	case 0x06:
		switch (sub_class) {
		case 0x00: return "Host Bridge";
		case 0x01: return "ISA Bridge";
		case 0x04: return "PCI-to-PCI Bridge";
		default: return "Bridge Device";
		}
	case 0x07:
		return "Communication Controller";
	case 0x08:
		return "System Peripheral";
	case 0x09:
		return "Input Device";
	case 0x0A:
		return "Docking Station";
	case 0x0B:
		return "Processor";
	case 0x0C:
		switch (sub_class) {
		case 0x00: return "FireWire Controller";
		case 0x03: return "USB Controller";
		default: return "Serial Bus Controller";
		}
	case 0x0D:
		return "Wireless Controller";
	case 0x0E:
		return "Intelligent I/O Controller";
	case 0x0F:
		return "Satellite Controller";
	case 0x10:
		return "Encryption/Decryption Controller";
	case 0x11:
		return "Data Acquisition Controller";
	default:
		return "Unknown Device";
	}
}

/**
 * @brief ベンダーIDから名前を取得（主要なベンダーのみ）
 */
static const char *pci_get_vendor_name(uint16_t vendor_id) {
	switch (vendor_id) {
	case 0x8086: return "Intel";
	case 0x1234: return "QEMU";
	case 0x1b36: return "Red Hat";
	case 0x1022: return "AMD";
	case 0x10de: return "NVIDIA";
	case 0x1002: return "ATI/AMD";
	default: return "Unknown";
	}
}

/**
 * @brief devicesコマンド - 接続されているデバイス一覧を表示
 */
static int cmd_devices(int argc, char **argv) {
	int verbose = 0;
	
	// -v オプションで詳細表示
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'v') {
		verbose = 1;
	}

	printk("Scanning PCI devices...\n");
	printk("================================================================================\n");
	if (verbose) {
		printk("Bus:Dev.Fn  Vendor:Device  Class  Description\n");
	} else {
		printk("Bus  Dev  Func  Vendor  Device  Class  Description\n");
	}
	printk("================================================================================\n");

	int device_count = 0;

	for (uint16_t bus = 0; bus < 256; ++bus) {
		for (uint8_t device = 0; device < 32; ++device) {
			for (uint8_t func = 0; func < 8; ++func) {
				uint32_t data = pci_read_config_dword(
					(uint8_t)bus, device, func, 0);
				uint16_t vendor = (uint16_t)(data & 0xFFFF);
				if (vendor == 0xFFFF) {
					continue; // デバイスなし
				}

				uint16_t device_id =
					(uint16_t)((data >> 16) & 0xFFFF);
				uint32_t class_rev = pci_read_config_dword(
					(uint8_t)bus, device, func, 0x08);
				uint8_t base_class = (class_rev >> 24) & 0xFF;
				uint8_t sub_class = (class_rev >> 16) & 0xFF;

				const char *class_name = pci_get_class_name(base_class, sub_class);

				if (verbose) {
					const char *vendor_name = pci_get_vendor_name(vendor);
					printk("%02x:%02x.%x     %s [%04x:%04x]  0x%02x   %s\n",
					       bus, device, func, vendor_name,
					       vendor, device_id, base_class, class_name);
				} else {
					printk("%3d  %3d  %4d  0x%04x  0x%04x  0x%02x   %s\n",
					       bus, device, func, vendor, device_id,
					       base_class, class_name);
				}

				device_count++;

				// マルチファンクションデバイスでなければ funcループを抜ける
				if (func == 0) {
					uint32_t hdr0 = pci_read_config_dword(
						(uint8_t)bus, device, func,
						0x0C);
					if (((hdr0 >> 16) & 0x80) == 0) {
						break; // single function device
					}
				}
			}
		}
	}

	printk("================================================================================\n");
	printk("Total devices found: %d\n", device_count);
	if (!verbose) {
		printk("Tip: Use 'devices -v' for verbose output\n");
	}

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
	register_command("cd", "Change directory", cmd_change_dir);
	register_command("pwd", "Print working directory", cmd_pwd);
	register_command("devices", "List connected devices", cmd_devices);
}
