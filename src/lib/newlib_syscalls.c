#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

enum {
    SYS_write = 1,
    SYS_exit = 2,
    SYS_sbrk = 3,
    SYS_read = 4,
    SYS_close = 5,
    SYS_fstat = 6,
    SYS_lseek = 7,
    SYS_open = 8,
    SYS_isatty = 9,
    SYS_get_reent = 10,
};

static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    register long rax asm("rax") = n;
    register long rdi asm("rdi") = a1;
    register long rsi asm("rsi") = a2;
    register long rdx asm("rdx") = a3;
    register long r10 asm("r10") = a4;
    register long r8 asm("r8") = a5;
    register long r9 asm("r9") = a6;
    asm volatile("int $0x80"
                 : "+a"(rax)
                 : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8), "r"(r9)
                 : "rcx", "r11", "memory");
    return rax;
}

ssize_t _write(int fd, const void *buf, size_t count) {
    long r = syscall6(SYS_write, fd, (long)buf, (long)count, 0, 0, 0);
    if (r < 0) { errno = (int)-r; return -1; }
    return (ssize_t)r;
}

ssize_t _read(int fd, void *buf, size_t count) {
    long r = syscall6(SYS_read, fd, (long)buf, (long)count, 0, 0, 0);
    if (r < 0) { errno = (int)-r; return -1; }
    return (ssize_t)r;
}

int _close(int fd) {
    long r = syscall6(SYS_close, fd, 0, 0, 0, 0, 0);
    if (r < 0) { errno = (int)-r; return -1; }
    return (int)r;
}

int _fstat(int fd, struct stat *st) {
    long r = syscall6(SYS_fstat, fd, (long)st, 0, 0, 0, 0);
    if (r < 0) { errno = (int)-r; return -1; }
    return 0;
}

int _isatty(int fd) {
    long r = syscall6(SYS_isatty, fd, 0, 0, 0, 0, 0);
    return (int)r;
}

off_t _lseek(int fd, off_t offset, int whence) {
    long r = syscall6(SYS_lseek, fd, (long)offset, (long)whence, 0, 0, 0);
    if (r < 0) { errno = (int)-r; return (off_t)-1; }
    return (off_t)r;
}

int _open(const char *pathname, int flags, int mode) {
    long r = syscall6(SYS_open, (long)pathname, flags, mode, 0, 0, 0);
    if (r < 0) { errno = (int)-r; return -1; }
    return (int)r;
}

void _exit(int status) {
    syscall6(SYS_exit, status, 0, 0, 0, 0, 0);
    for (;;)
        ;
}

void * _sbrk(ptrdiff_t increment) {
    long r = syscall6(SYS_sbrk, increment, 0, 0, 0, 0, 0);
    if (r == -1) { errno = ENOMEM; return (void *)-1; }
    return (void *)r;
}


__attribute__((constructor)) static void _newlib_reent_init(void) {
    const long alloc_size = 4096; /* one page */
    long r = syscall6(SYS_get_reent, alloc_size, 0, 0, 0, 0, 0);
    if (r == -1)
        return; /* allocation failed; fall back to global _impure_ptr if any */

    extern char _impure_ptr;
    *(struct _reent **)&_impure_ptr = (struct _reent *)(uintptr_t)r;
    /* kernel already zeroed the page; basic errno usage will work */
}

int _getpid(void) { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; return -1; }