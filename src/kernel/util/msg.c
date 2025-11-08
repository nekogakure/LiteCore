#include <util/config.h>
#include <util/console.h>

#define MSG_NO_ERROR 0
#define MSG_INTER_ERROR 1
#define MSG_MEM_ERROR 2
#define MSG_FS_ERROR 3
#define MSG_IO_ERROR 4

/**
 * @brief エラーメッセージをエラーコードから表示する
 * @param error_code エラーコード
 */
void msg_from_code(int error_code) {
	switch (error_code) {
	case MSG_NO_ERROR:
		printk("No error.\n");
		break;
	case MSG_INTER_ERROR:
		printk("Error: Interrupt handling error occurred.\n");
		break;
	case MSG_MEM_ERROR:
		printk("Error: Memory management error occurred.\n");
		break;
	case MSG_FS_ERROR:
		printk("Error: Filesystem error occurred.\n");
		break;
	case MSG_IO_ERROR:
		printk("Error: I/O operation error occurred.\n");
		break;
	default:
		printk("Error: Unknown error code %d\n", error_code);
		break;
	}
}