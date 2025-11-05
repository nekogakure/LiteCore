#include <stddef.h>
#include <util/console.h>
#include <fs/fs.h>

void fs_test() {
	fs_handle *fh = NULL;
	extern const unsigned char _binary_src_file_img_start[];
	extern const unsigned char _binary_src_file_img_end[];
	const unsigned char *img = _binary_src_file_img_start;
	size_t img_len = (size_t)((uintptr_t)_binary_src_file_img_end -
				  (uintptr_t)_binary_src_file_img_start);

	printk("fs_test: Embedded FAT12 image at %p, size=%u bytes\n", img,
	       (unsigned)img_len);

	int r = fs_mount(img, img_len, &fh);
	if (r != 0) {
		printk("fs_mount failed: %d\n", r);
		return;
	}
	printk("fs_mount: SUCCESS\n");

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