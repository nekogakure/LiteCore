#include <fs/vfs.h>
#include <device/keyboard.h>
#include <util/console.h>
#include <mem/usercopy.h>
#include <task/multi_task.h>
#include <stdint.h>
#include <stddef.h>
#include <mem/manager.h>
#include <fs/ext/ext2.h>

extern struct ext2_super *g_ext2_sb;

#define MAX_OPEN_FILES 256

typedef enum { VFS_TYPE_UNKNOWN = 0, VFS_TYPE_EXT2 } vfs_type_t;

struct vfs_file {
    vfs_type_t type;
    union {
        struct {
            struct ext2_super *sb;
            struct ext2_inode inode;
            char path[256];
        } ext2;
    } u;
    uint32_t offset;
};

static struct vfs_file *open_files[MAX_OPEN_FILES];

static int allocate_global_handle(struct vfs_file *f) {
    for (int i = 0; i < MAX_OPEN_FILES; ++i) {
        if (open_files[i] == NULL) {
            open_files[i] = f;
            return i;
        }
    }
    return -1;
}

static void free_global_handle(int idx) {
    if (idx < 0 || idx >= MAX_OPEN_FILES)
        return;
    if (open_files[idx]) {
        kfree(open_files[idx]);
        open_files[idx] = NULL;
    }
}

void vfs_init(void) {
    // 現状は何もしない
}

/* Per-task fd allocation helpers */
static int vfs_fd_alloc_for_current(void) {
    task_t *t = task_current();
    if (!t)
        return -1;
    for (int i = 3; i < 32; ++i) {
        if (t->fds[i] == -1) {
            return i;
        }
    }
    return -1;
}

static int vfs_fd_release_for_current(int fd) {
    task_t *t = task_current();
    if (!t)
        return -1;
    if (fd < 3 || fd >= 32)
        return -1;
    if (t->fds[fd] == -1)
        return -1;
    int global_idx = t->fds[fd];
    if (global_idx >= 0) {
        /* free global handle */
        free_global_handle(global_idx);
    }
    t->fds[fd] = -1;
    return 0;
}

int vfs_write(int fd, const void *buf, size_t len) {
    if (buf == NULL)
        return -1;
    if (fd == 1 || fd == 2) {
        /* print in chunks to avoid very large stack usage */
        const char *p = (const char *)buf;
        size_t remaining = len;
        while (remaining > 0) {
            size_t chunk = remaining > 1024 ? 1024 : remaining;
            char tmp[1025];
            for (size_t i = 0; i < chunk; ++i)
                tmp[i] = p[len - remaining + i];
            tmp[chunk] = '\0';
            printk("%s", tmp);
            remaining -= chunk;
        }
        return (int)len;
    }
    /* If fd maps to an open file (global handle), writing not supported yet */
    task_t *t = task_current();
    if (!t)
        return -1;
    if (fd >= 3 && fd < 32) {
        int global_idx = t->fds[fd];
        if (global_idx >= 0 && open_files[global_idx]) {
            /* write not implemented for ext2 yet */
            return -1;
        }
    }
    return -1; /* not implemented */
}

int vfs_read(int fd, void *buf, size_t len) {
    if (buf == NULL)
        return -1;
    if (fd == 0) {
        char *out = (char *)buf;
        size_t i;
        for (i = 0; i < len; ++i) {
            char c = keyboard_getchar();
            out[i] = c;
            if (c == '\n') {
                i++;
                break;
            }
        }
        return (int)i;
    }
    /* handle opened files via global handle table */
    task_t *t = task_current();
    if (!t)
        return -1;
    if (fd >= 3 && fd < 32) {
        int global_idx = t->fds[fd];
        if (global_idx < 0 || global_idx >= MAX_OPEN_FILES)
            return -1;
        struct vfs_file *vf = open_files[global_idx];
        if (!vf)
            return -1;
        if (vf->type == VFS_TYPE_EXT2) {
            size_t out_len = 0;
            int r = ext2_read_inode_data(vf->u.ext2.sb, &vf->u.ext2.inode,
                                         buf, len, (uint32_t)vf->offset,
                                         &out_len);
            if (r != 0)
                return -1;
            vf->offset += out_len;
            return (int)out_len;
        }
    }
    return -1;
}

int vfs_close(int fd) {
    return vfs_fd_release_for_current(fd);
}

int vfs_open(const char *pathname, int flags, int mode) {
    (void)flags; (void)mode;
    if (!pathname)
        return -1;
    if (g_ext2_sb == NULL)
        return -1;

    /* resolve path to inode */
    uint32_t inode_num;
    int r = ext2_resolve_path(g_ext2_sb, pathname, &inode_num);
    if (r != 0)
        return -1;

    /* read inode */
    struct ext2_inode inode;
    r = ext2_read_inode(g_ext2_sb, inode_num, &inode);
    if (r != 0)
        return -1;

    /* allocate vfs_file and global handle */
    struct vfs_file *vf = (struct vfs_file *)kmalloc(sizeof(struct vfs_file));
    if (!vf)
        return -1;
    vf->type = VFS_TYPE_EXT2;
    vf->u.ext2.sb = g_ext2_sb;
    vf->u.ext2.inode = inode;
    /* copy path */
    int i = 0;
    for (; i < (int)sizeof(vf->u.ext2.path) - 1 && pathname[i]; ++i)
        vf->u.ext2.path[i] = pathname[i];
    vf->u.ext2.path[i] = '\0';
    vf->offset = 0;

    int global_idx = allocate_global_handle(vf);
    if (global_idx < 0) {
        kfree(vf);
        return -1;
    }

    /* allocate per-task fd and store global index */
    task_t *t = task_current();
    if (!t) {
        free_global_handle(global_idx);
        return -1;
    }
    int local_fd = vfs_fd_alloc_for_current();
    if (local_fd < 0) {
        free_global_handle(global_idx);
        return -1;
    }
    t->fds[local_fd] = global_idx;
    return local_fd;
}

int vfs_lseek(int fd, int64_t offset, int whence) {
    task_t *t = task_current();
    if (!t)
        return -1;
    if (fd < 3 || fd >= 32)
        return -1;
    int global_idx = t->fds[fd];
    if (global_idx < 0 || global_idx >= MAX_OPEN_FILES)
        return -1;
    struct vfs_file *vf = open_files[global_idx];
    if (!vf)
        return -1;
    uint64_t newoff = 0;
    if (whence == 0) { /* SEEK_SET */
        newoff = offset;
    } else if (whence == 1) { /* SEEK_CUR */
        newoff = vf->offset + offset;
    } else if (whence == 2) { /* SEEK_END */
        /* need file size */
        newoff = (uint64_t)vf->u.ext2.inode.i_size + offset;
    } else {
        return -1;
    }
    vf->offset = (uint32_t)newoff;
    return (int64_t)vf->offset;
}

int vfs_fstat(int fd, void *buf) {
    if (buf == NULL)
        return -1;
    if (fd == 1 || fd == 2 || fd == 0) {
        uint32_t mode = 0020000; /* S_IFCHR */
        /* common fallback: write mode at offset 0 (older layouts) */
        (void)copy_to_user(buf, &mode, sizeof(mode));
        /* try also at offset 16 (glibc/newlib-like placements) */
        (void)copy_to_user((void *)((uintptr_t)buf + 16), &mode, sizeof(mode));
        /* set st_size (64-bit) to 0 at offset 48 and 40 as possible places */
        uint64_t z64 = 0;
        (void)copy_to_user((void *)((uintptr_t)buf + 48), &z64, sizeof(z64));
        (void)copy_to_user((void *)((uintptr_t)buf + 40), &z64, sizeof(z64));
        return 0;
    }
    /* check opened files */
    task_t *t = task_current();
    if (!t)
        return -1;
    if (fd >= 3 && fd < 32) {
        int global_idx = t->fds[fd];
        if (global_idx < 0 || global_idx >= MAX_OPEN_FILES)
            return -1;
        struct vfs_file *vf = open_files[global_idx];
        if (!vf)
            return -1;
        /* write mode */
        uint32_t mode = (vf->type == VFS_TYPE_EXT2) ? 0100000 /* regular */ : 0;
        (void)copy_to_user(buf, &mode, sizeof(mode));
        (void)copy_to_user((void *)((uintptr_t)buf + 16), &mode,
                           sizeof(mode));
        /* write size */
        uint64_t sz = 0;
        if (vf->type == VFS_TYPE_EXT2)
            sz = (uint64_t)vf->u.ext2.inode.i_size;
        (void)copy_to_user((void *)((uintptr_t)buf + 48), &sz, sizeof(sz));
        (void)copy_to_user((void *)((uintptr_t)buf + 40), &sz, sizeof(sz));
        return 0;
    }
    return -1;
}

int vfs_isatty(int fd) {
    return (fd == 0 || fd == 1 || fd == 2) ? 1 : 0;
}
