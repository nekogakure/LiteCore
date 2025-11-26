#ifndef _FAT16_H
#define _FAT16_H

#include <stdint.h>
#include <stddef.h>
#include <fs/block_cache.h>

struct fat16_super {
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t num_fats;
	uint16_t max_root_entries;
	uint32_t total_sectors;
	uint32_t fat_size_sectors;
	uint32_t first_data_sector;
	uint32_t root_dir_sector;
	uint8_t *image;
	size_t image_size;
};

int fat16_mount(void *image, size_t size, struct fat16_super **out);
int fat16_mount_with_cache(struct block_cache *cache, struct fat16_super **out);
int fat16_list_root(struct fat16_super *sb);
int fat16_read_file(struct fat16_super *sb, const char *name, void *buf,
		    size_t len, size_t *out_len);
int fat16_get_file_size(struct fat16_super *sb, const char *name,
			uint32_t *out_size);
int fat16_write_file(struct fat16_super *sb, const char *name, const void *buf,
		     size_t len);
int fat16_create_file(struct fat16_super *sb, const char *name);

#endif
