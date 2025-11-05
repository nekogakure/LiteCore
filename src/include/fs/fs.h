#ifndef _KERNEL_FS_H
#define _KERNEL_FS_H

#include <stdint.h>
#include <stddef.h>

typedef struct fs_handle fs_handle;
typedef struct dir_handle dir_handle;
typedef struct file_handle file_handle;

/**
 * @brief イメージからファイルシステムをマウントする
 * @param device_image イメージ先頭へのポインタ（埋め込みバッファ等）
 * @param size イメージ長（バイト）
 * @param out 成功時に fs_handle* を返すポインタ
 * @return 0 成功、負値はエラー
 */
int fs_mount(const void *device_image, size_t size, fs_handle **out);

/** @brief アンマウント */
int fs_unmount(fs_handle *h);

/** @brief ディレクトリを開く */
int fs_opendir(fs_handle *h, const char *path, dir_handle **out);

/**
 * @brief ディレクトリエントリを列挙する
 * @param name 出力バッファ
 * @param name_len バッファ長
 * @param is_dir ディレクトリかどうかを返す（出力）
 */
int fs_readdir(dir_handle *d, char *name, int name_len, int *is_dir,
	       uint32_t *size, uint16_t *start_cluster);

/** @brief ディレクトリハンドルを閉じる */
int fs_closedir(dir_handle *d);

/** @brief ファイルを開く */
int fs_open(fs_handle *h, const char *path, file_handle **out);

/** @brief ファイルを読み込む（同期） */
int fs_read(file_handle *f, void *buf, uint32_t len, uint32_t offset);

/** @brief ファイルを閉じる */
int fs_close(file_handle *f);

#endif
