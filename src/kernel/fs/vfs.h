// Very small VFS/FD shim used by kernel syscalls.
#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <stdint.h>
#include <stddef.h>

void vfs_init(void);
int vfs_write(int fd, const void *buf, size_t len);
int vfs_read(int fd, void *buf, size_t len);
int vfs_close(int fd);
int vfs_open(const char *pathname, int flags, int mode);
int vfs_lseek(int fd, int64_t offset, int whence);
int vfs_fstat(int fd, void *buf);
int vfs_isatty(int fd);

#endif
