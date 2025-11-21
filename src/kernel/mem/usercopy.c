#include <mem/usercopy.h>
#include <mem/vmem.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int validate_user_range(const void *addr, size_t n) {
	if (addr == NULL)
		return -1;
	if (n == 0)
		return 0;

	uintptr_t a = (uintptr_t)addr;
	uintptr_t end = a + n - 1;

	const uintptr_t page_size = 0x1000;
	uintptr_t page = a & ~(page_size - 1);
	while (page <= end) {
		uint64_t phys = vmem_virt_to_phys64((uint64_t)page);
		if (phys == 0 || phys == UINT64_MAX)
			return -1;
		page += page_size;
	}
	return 0;
}

int copy_to_user(void *user_dest, const void *kern_src, size_t n) {
	if (user_dest == NULL || kern_src == NULL)
		return -1;
	if (validate_user_range(user_dest, n) != 0)
		return -1;

	uint8_t *d = (uint8_t *)user_dest;
	const uint8_t *s = (const uint8_t *)kern_src;

	/* Copy by page to be more efficient and avoid per-byte overhead. */
	const uintptr_t page_size = 0x1000;
	uintptr_t dst = (uintptr_t)d;
	size_t remaining = n;
	while (remaining > 0) {
		uintptr_t page_end = ((dst & ~(page_size - 1)) + page_size);
		size_t chunk = (size_t)(page_end - dst);
		if (chunk > remaining)
			chunk = remaining;
		/* use small kernel memcpy to avoid libc dependency */
		uint8_t *kd = (uint8_t *)dst;
		const uint8_t *ks = (const uint8_t *)s;
		for (size_t i = 0; i < chunk; ++i)
			kd[i] = ks[i];
		dst += chunk;
		s += chunk;
		remaining -= chunk;
	}
	return 0;
}

int copy_from_user(void *kern_dest, const void *user_src, size_t n) {
	if (kern_dest == NULL || user_src == NULL)
		return -1;
	if (validate_user_range(user_src, n) != 0)
		return -1;

	uint8_t *d = (uint8_t *)kern_dest;
	const uint8_t *s = (const uint8_t *)user_src;

	/* Copy by page for efficiency */
	const uintptr_t page_size = 0x1000;
	uintptr_t src = (uintptr_t)s;
	size_t remaining = n;
	while (remaining > 0) {
		uintptr_t page_end = ((src & ~(page_size - 1)) + page_size);
		size_t chunk = (size_t)(page_end - src);
		if (chunk > remaining)
			chunk = remaining;
		uint8_t *kd = d;
		const uint8_t *ks = (const uint8_t *)src;
		for (size_t i = 0; i < chunk; ++i)
			kd[i] = ks[i];
		d += chunk;
		src += chunk;
		remaining -= chunk;
	}
	return 0;
}
