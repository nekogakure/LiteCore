#include <stddef.h>
#include <util/console.h>
#include <fs/fs.h>

void fs_test() {
	fs_handle *fh = NULL;
	extern const unsigned char _binary_src_file_img_start[];
	extern const unsigned char _binary_src_file_img_end[];
	extern const unsigned char _binary_src_file_img_size;
	const unsigned char *img = _binary_src_file_img_start;
	size_t img_len = (size_t)((uintptr_t)_binary_src_file_img_end -
				  (uintptr_t)_binary_src_file_img_start);

	printk("DEBUG: _binary_src_file_img_start=%p\n",
	       _binary_src_file_img_start);
	printk("DEBUG: _binary_src_file_img_end=%p\n",
	       _binary_src_file_img_end);
	printk("DEBUG: _binary_src_file_img_size=%u\n",
	       (unsigned)(uintptr_t)&_binary_src_file_img_size);

	printk("img=%p len=%u\n", img, (unsigned)img_len);
	{
		int dump = (img_len >= 64) ? 64 : (int)img_len;
		printk("img data:");
		for (int i = 0; i < dump; i++) {
			printk(" %02x", (unsigned)img[i]);
		}
		printk("\n");

		// FAT12ブートセクタの期待値
		const unsigned char expected[] = { 0xeb, 0x3c, 0x90, 0x4d,
						   0x53, 0x44, 0x4f, 0x53,
						   0x35, 0x2e, 0x30, 0x00,
						   0x02, 0x01, 0x01, 0x00 };
		int match = 1;
		for (int i = 0; i < 16 && i < (int)img_len; i++) {
			if (img[i] != expected[i]) {
				match = 0;
				break;
			}
		}
		printk("DEBUG: Expected FAT12 boot sector: %s\n",
		       match ? "MATCH" : "MISMATCH");

		if (img_len >= 512) {
			unsigned bps = img[11] | (img[12] << 8);
			printk("bytes/sector=%u sig=%02x%02x\n", bps, img[510],
			       img[511]);
		}
	}
	int r = fs_mount(img, img_len, &fh);
	if (r != 0) {
		printk("fs_mount failed: %d\n", r);
		return;
	}
	dir_handle *d;
	if (fs_opendir(fh, "/", &d) == 0) {
		char name[64];
		int is_dir;
		uint32_t size;
		uint16_t cluster;
		while (fs_readdir(d, name, sizeof(name), &is_dir, &size,
				  &cluster) == 0) {
			printk("  %s %s (%u bytes)\n",
			       is_dir ? "[DIR]" : "[FILE]", name, size);
		}
		fs_closedir(d);
	}
	// ファイル読み取りテスト
	file_handle *fhnd;
	if (fs_open(fh, "EXAMPLE.TXT", &fhnd) == 0) {
		char buf[128];
		int n = fs_read(fhnd, buf, sizeof(buf), 0);
		if (n > 0) {
			buf[n < (int)sizeof(buf) ? n : (int)sizeof(buf) - 1] =
				'\0';
			printk("FILE CONTENT:\n%s\n", buf);
		}
		fs_close(fhnd);
	}
	fs_unmount(fh);
}

void fat12_test() {
	fs_test();
}