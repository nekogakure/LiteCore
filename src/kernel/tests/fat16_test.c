#include <util/console.h>
#include <mem/manager.h>
#include <fs/fat/fat16.h>
#include <stddef.h>

void fat16_test() {
	uint32_t total_sectors = 128;
	uint32_t img_size = 512 * total_sectors;
	uint8_t *img = (uint8_t *)kmalloc(img_size);
	if (!img) {
		printk("fat16_test: kmalloc failed\n");
		return;
	}
	/* zero clear */
	for (uint32_t i = 0; i < img_size; ++i)
		img[i] = 0;
	/* BPB minimal fields */
	img[11] = 0x00;
	img[12] = 0x02; /* bytes per sector = 512 */
	img[13] = 1; /* sectors per cluster */
	img[14] = 1;
	img[15] = 0; /* reserved sectors = 1 */
	img[16] = 2; /* num fats */
	img[17] = 0x00;
	img[18] = 0x02; /* max root entries = 512 */
	img[19] = (uint8_t)(total_sectors & 0xff);
	img[20] = (uint8_t)((total_sectors >> 8) & 0xff);
	img[22] = 1;
	img[23] = 0; /* fat size sectors = 1 */

	struct fat16_super *sb = NULL;
	if (fat16_mount(img, img_size, &sb) != 0) {
		printk("fat16_test: mount failed\n");
		return;
	}

	const char *name = "T.TXT";
	const char *msg = "hello fat16";
	if (fat16_create_file(sb, name) != 0) {
		printk("fat16_test: create failed\n");
		return;
	}
	if (fat16_write_file(sb, name, msg, 11) != 0) {
		printk("fat16_test: write failed\n");
		return;
	}
	char buf[32];
	size_t out_len = 0;
	if (fat16_read_file(sb, name, buf, sizeof(buf), &out_len) != 0) {
		printk("fat16_test: read failed\n");
		return;
	}
	if (out_len != 11) {
		printk("fat16_test: size mismatch %u\n", (unsigned)out_len);
		return;
	}
	buf[out_len] = '\0';
	if (out_len == 11 && buf[0] == 'h' && buf[10] == '6') {
		printk("fat16_test: OK (%s)\n", buf);
	} else {
		printk("fat16_test: FAIL content=%.11s\n", buf);
	}
}
