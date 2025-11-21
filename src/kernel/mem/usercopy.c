#include <mem/usercopy.h>
#include <stdint.h>

int copy_to_user(void *user_dest, const void *kern_src, size_t n) {
	if (user_dest == NULL || kern_src == NULL)
		return -1;
	uint8_t *d = (uint8_t *)user_dest;
	const uint8_t *s = (const uint8_t *)kern_src;
	for (size_t i = 0; i < n; ++i)
		d[i] = s[i];
	return 0;
}

int copy_from_user(void *kern_dest, const void *user_src, size_t n) {
	if (kern_dest == NULL || user_src == NULL)
		return -1;
	uint8_t *d = (uint8_t *)kern_dest;
	const uint8_t *s = (const uint8_t *)user_src;
	for (size_t i = 0; i < n; ++i)
		d[i] = s[i];
	return 0;
}
