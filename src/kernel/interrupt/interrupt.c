#include <util/config.h>
#include <interrupt/irq.h>
#include <util/io.h>

typedef void (*irq_handler_t)(uint32_t irq, void *ctx);

/* 最大IRQ数 (IRQ 0-15: PIC, IRQ 32+: APIC/その他) */
#define MAX_IRQS 256
/* 1 IRQ に登録できるハンドラ数（簡易実装） */
#define MAX_HANDLERS_PER_IRQ 4

static irq_handler_t irq_table[MAX_IRQS][MAX_HANDLERS_PER_IRQ];
static void *irq_ctx[MAX_IRQS][MAX_HANDLERS_PER_IRQ];
static uint8_t irq_count[MAX_IRQS] = { 0 };

/* FIFO */
#define FIFO_CAPACITY 64
typedef struct {
	uint32_t buf[FIFO_CAPACITY];
	uint32_t head;
	uint32_t tail;
	uint32_t count;
} fifo_t;

static fifo_t irq_fifo = { .head = 0, .tail = 0, .count = 0 };

static inline void fifo_init(fifo_t *f) {
	f->head = f->tail = f->count = 0;
}

static inline int fifo_is_full(fifo_t *f) {
	return f->count >= FIFO_CAPACITY;
}
static inline int fifo_is_empty(fifo_t *f) {
	return f->count == 0;
}

static inline int fifo_push(fifo_t *f, uint32_t v) {
	if (fifo_is_full(f))
		return 0;
	f->buf[f->tail] = v;
	f->tail = (f->tail + 1) % FIFO_CAPACITY;
	f->count++;
	return 1;
}

static inline int fifo_pop(fifo_t *f, uint32_t *out) {
	if (fifo_is_empty(f))
		return 0;
	*out = f->buf[f->head];
	f->head = (f->head + 1) % FIFO_CAPACITY;
	f->count--;
	return 1;
}

/**
 * @fn interrupt_register
 * @brief ベクタ番号に対してハンドラを登録する（簡易）
 * @param irq ベクタ番号 0..(MAX_IRQS-1)
 * @param handler ハンドラ関数
 * @param ctx ハンドラに渡すコンテキストポインタ
 * @return 0=成功, -1=範囲外
 */
int interrupt_register(uint32_t irq, irq_handler_t handler, void *ctx) {
	if (irq >= MAX_IRQS)
		return -1;
	uint32_t flags = irq_save();
	// 重複登録を防ぎ、空きスロットに追加
	for (uint32_t i = 0; i < irq_count[irq]; ++i) {
		if (irq_table[irq][i] == handler && irq_ctx[irq][i] == ctx) {
			irq_restore(flags);
			return 0; // 既に登録済
		}
	}
	if (irq_count[irq] >= MAX_HANDLERS_PER_IRQ) {
		irq_restore(flags);
		return -1; // 空きねぇよﾊｯ
	}
	irq_table[irq][irq_count[irq]] = handler;
	irq_ctx[irq][irq_count[irq]] = ctx;
	irq_count[irq]++;
	irq_restore(flags);
	return 0;
}

/**
 * @fn interrupt_unregister
 */
int interrupt_unregister(uint32_t irq) {
	if (irq >= MAX_IRQS)
		return -1;
	uint32_t flags = irq_save();
	/* 全ハンドラを解除 */
	for (uint32_t i = 0; i < irq_count[irq]; ++i) {
		irq_table[irq][i] = NULL;
		irq_ctx[irq][i] = NULL;
	}
	irq_count[irq] = 0;
	irq_restore(flags);
	return 0;
}

/**
 * @fn interrupt_raise
 * @brief 割り込みイベントを FIFO に入れる（割り込みハンドラから呼べる）
 * @param event 上位16bit: ベクタ番号, 下位16bit: payload
 */
int interrupt_raise(uint32_t event) {
	// 割り込み中でも呼べるように短時間だけ割り込みを無効化してpush
	uint32_t flags = irq_save();
	int res = fifo_push(&irq_fifo, event);
	irq_restore(flags);

	if (!res) {
		// FIFOが満杯でイベントが入らなかった。ログできるなら通知する。
		printk("interrupt raise: fifo full, dropping event irq=%u payload=%u\n",
		       (unsigned)((event >> 16) & 0xFFFF),
		       (unsigned)(event & 0xFFFF));
	}
	return res;
}

/**
 * @fn interrupt_dispatch_one
 * @brief FIFO からイベントを取り出して対応する IRQ ハンドラを呼ぶ（同期コンテキスト）
 */
int interrupt_dispatch_one(void) {
	uint32_t evt;
	// popはディスパッチ側（通常同期コンテキスト）で短時間割り込みを無効化して行う
	uint32_t flags = irq_save();
	if (!fifo_pop(&irq_fifo, &evt)) {
		irq_restore(flags);
		return 0;
	}
	irq_restore(flags);

	/* evt のレイアウト: 上位16bit = ベクタ番号, 下位16bit = payload */
	uint32_t vec = (evt >> 16) & 0xFFFF;
	uint32_t payload = evt & 0xFFFF;

	if (vec < MAX_IRQS && irq_count[vec] > 0) {
		for (uint32_t i = 0; i < irq_count[vec]; ++i) {
			if (irq_table[vec][i]) {
				// ハンドラに渡す第1引数はpayload（キーボードの場合はscancode等）
				// ベクタ番号はハンドラが必要なら ctx 経由で渡す
				irq_table[vec][i](payload, irq_ctx[vec][i]);
			}
		}
	} else {
		// デバッグ出力は最小限に（大量の未処理割り込みでログが埋まるのを防ぐ）
		// printk("Unhandled IRQ event: vec=%u payload=%u\n",
		//        (unsigned)vec, (unsigned)payload);
	}
	return 1;
}

/**
 * @fn interrupt_dispatch_all
 * @brief FIFO のイベントを全て処理する（通常 main ループで呼ぶ）
 */
void interrupt_dispatch_all(void) {
	while (interrupt_dispatch_one()) {
	}
}

/**
 * @fn interrupt_init
 * @brief 初期化
 */
void interrupt_init(void) {
	fifo_init(&irq_fifo);
	for (uint32_t i = 0; i < MAX_IRQS; ++i) {
		irq_count[i] = 0;
		for (uint32_t j = 0; j < MAX_HANDLERS_PER_IRQ; ++j) {
			irq_table[i][j] = NULL;
			irq_ctx[i][j] = NULL;
		}
	}
}
