// Very small VFS/FD shim used by kernel syscalls.
#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <stdint.h>
#include <stddef.h>

/* forward declarations for backend types used in API */
struct vfs_backend;
struct block_cache;

void vfs_init(void);
int vfs_write(int fd, const void *buf, size_t len);
int vfs_read(int fd, void *buf, size_t len);
int vfs_close(int fd);
int vfs_open(const char *pathname, int flags, int mode);
int vfs_lseek(int fd, int64_t offset, int whence);
int vfs_fstat(int fd, void *buf);
int vfs_isatty(int fd);

/* Backend registration and mounting helpers */
int vfs_register_backend(struct vfs_backend *b);
void vfs_register_builtin_backends(void);
int vfs_mount_with_cache(struct block_cache *cache);
int vfs_read_file_all(const char *path, void **out_buf, uint32_t *out_size);
int vfs_list_root(void);
int vfs_resolve_path(const char *path, int *is_dir, uint32_t *out_size);

#endif
