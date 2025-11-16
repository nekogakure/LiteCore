#ifndef _KEYBOARD_H
#define _KEYBOARD_H

/**
 * @brief キーボードの初期化
 */
void keyboard_init(void);

/**
 * @brief キーボード入力をポーリング（非ブロッキング）
 */
void keyboard_poll(void);

/**
 * @brief キーボードから1文字取得（ブロッキング）
 * @return 取得した文字
 */
char keyboard_getchar(void);

/**
 * @brief キーボードから1文字取得（ポーリング）
 * @return 取得した文字、バッファが空の場合0
 */
char keyboard_getchar_poll(void);

#endif /* _KEYBOARD_H */