/* 古い、古い。消す */

#include <stddef.h>
#include <stdint.h>
#include <fs/fs.h>
#include <fs/vfs.h>

int fs_mount(const void *device_image, size_t size, fs_handle **out) {
	(void)device_image;
	(void)size;
	(void)out;
	return -1;
}

int fs_unmount(fs_handle *h) {
	(void)h;
	return -1;
}

int fs_opendir(fs_handle *h, const char *path, dir_handle **out) {
	(void)h;
	(void)path;
	(void)out;
	return -1;
}

int fs_readdir(dir_handle *d, char *name, int name_len, int *is_dir,
	       uint32_t *size, uint16_t *start_cluster) {
	(void)d;
	(void)name;
	(void)name_len;
	(void)is_dir;
	(void)size;
	(void)start_cluster;
	return -1;
}

int fs_closedir(dir_handle *d) {
	(void)d;
	return -1;
}

int fs_open(fs_handle *h, const char *path, file_handle **out) {
	(void)h;
	(void)path;
	(void)out;
	return -1;
}

int fs_read(file_handle *f, void *buf, uint32_t len, uint32_t offset) {
	(void)f;
	(void)buf;
	(void)len;
	(void)offset;
	return -1;
}

int fs_close(file_handle *f) {
	(void)f;
	return -1;
}
