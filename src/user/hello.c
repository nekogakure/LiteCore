#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>

extern ssize_t write(int fd, const void *buf, size_t count);
extern void _exit(int status);

int main(void) {
	const char msg[] = "Hello from user\n";
	write(1, msg, sizeof(msg) - 1);
	_exit(0);
	return 0;
}
