#include <syscall.h>
#include <util/console.h>
#include <task/multi_task.h>
#include <mem/manager.h>
#include <device/keyboard.h>
#include <mem/usercopy.h>
#include <fs/vfs.h>
#include <stdint.h>

static uint64_t sys_write(uint64_t fd, const void *buf, uint64_t len) {
	return (uint64_t)vfs_write((int)fd, buf, (size_t)len);
}

static void sys_exit(int code) {
	(void)code;
	task_exit();
}

static uint64_t sys_sbrk(intptr_t inc) {
	if (inc == 0) {
		return 0;
	}
	if (inc < 0) {
		return (uint64_t)-1;
	}
	void *p = kmalloc((uint32_t)inc);
	if (!p)
		return (uint64_t)-1;
	return (uint64_t)p;
}

static uint64_t sys_read(uint64_t fd, void *buf, uint64_t len) {
	return (uint64_t)vfs_read((int)fd, buf, (size_t)len);
}

static uint64_t sys_close(uint64_t fd) {
	return (uint64_t)vfs_close((int)fd);
}

static uint64_t sys_open(const char *pathname, uint64_t flags, uint64_t mode) {
	return (uint64_t)vfs_open(pathname, (int)flags, (int)mode);
}

static uint64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence) {
	return (uint64_t)vfs_lseek((int)fd, (int64_t)offset, (int)whence);
}

static uint64_t sys_isatty(uint64_t fd) {
	return (uint64_t)vfs_isatty((int)fd);
}

static uint64_t sys_fstat(uint64_t fd, void *buf) {
	return (uint64_t)vfs_fstat((int)fd, buf);
}

static uint64_t dispatch_syscall(uint64_t num, uint64_t a0, uint64_t a1,
				 uint64_t a2, uint64_t a3, uint64_t a4,
				 uint64_t a5) {
	switch (num) {
	case SYS_write:
		return sys_write(a0, (const void *)a1, a2);
	case SYS_read:
		return sys_read(a0, (void *)a1, a2);
	case SYS_close:
		return sys_close(a0);
	case SYS_open:
		return sys_open((const char *)a0, a1, a2);
	case SYS_lseek:
		return sys_lseek(a0, a1, a2);
	case SYS_isatty:
		return sys_isatty(a0);
	case SYS_fstat:
		return sys_fstat(a0, (void *)a1);
	case SYS_exit:
		sys_exit((int)a0);
		return 0;
	case SYS_sbrk:
		return sys_sbrk((intptr_t)a0);
	default:
		return (uint64_t)-1; /* ENOSYS */
	}
}

void syscall_entry_c(uint64_t *regs_stack, uint32_t vec) {
	(void)vec;
	uint64_t num = regs_stack[0];
	uint64_t rax = regs_stack[0];
	uint64_t rcx = regs_stack[1];
	uint64_t rdx = regs_stack[2];
	uint64_t rbx = regs_stack[3];
	uint64_t rbp = regs_stack[4];
	uint64_t rsi = regs_stack[5];
	uint64_t rdi = regs_stack[6];
	uint64_t r8 = regs_stack[7];
	uint64_t r9 = regs_stack[8];
	uint64_t r10 = regs_stack[9];

	uint64_t a0 = rdi;
	uint64_t a1 = rsi;
	uint64_t a2 = rdx;
	uint64_t a3 = r10;
	uint64_t a4 = r8;
	uint64_t a5 = r9;

	uint64_t ret = dispatch_syscall(num, a0, a1, a2, a3, a4, a5);

	regs_stack[0] = ret;
}
