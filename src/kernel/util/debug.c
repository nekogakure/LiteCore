#include <util/config.h>
#include <util/console.h>
#include <util/debug.h>

static int current_log_level = INFO;

/**
 * @brief ログレベルを設定
 * @param level ログレベル(INFO, WARN, ERR, ALL)
 */
void set_log_level(int level) {
	current_log_level = level;
}

/**
 * @brief デバッグメッセージを出力
 * @param msg メッセージ
 * @param level ログレベル(INFO, WARN, ERR, ALL)
 */
void debug(const char *msg, int level) {
	if (current_log_level >= level) {
		printk("[DEBUG] %s\n", msg);
	}
}