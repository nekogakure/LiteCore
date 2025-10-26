#include <config.h>
#include <interrupt/irq.h>
#include <util/io.h>

typedef void (*irq_handler_t)(uint32_t irq, void* ctx);

/* 最大IRQハンドラ数（シンプルに16個にしてみる） */
#define MAX_IRQS 16

static irq_handler_t irq_table[MAX_IRQS] = {0};
static void* irq_ctx[MAX_IRQS] = {0};

/* FIFO */
#define FIFO_CAPACITY 64
typedef struct {
        uint32_t buf[FIFO_CAPACITY];
        uint32_t head;
        uint32_t tail;
        uint32_t count;
} fifo_t;

static fifo_t irq_fifo = { .head = 0, .tail = 0, .count = 0 };

static inline void fifo_init(fifo_t* f) {
        f->head = f->tail = f->count = 0;
}

static inline int fifo_is_full(fifo_t* f) { return f->count >= FIFO_CAPACITY; }
static inline int fifo_is_empty(fifo_t* f) { return f->count == 0; }

static inline int fifo_push(fifo_t* f, uint32_t v) {
        if (fifo_is_full(f)) return 0;
        f->buf[f->tail] = v;
        f->tail = (f->tail + 1) % FIFO_CAPACITY;
        f->count++;
        return 1;
}

static inline int fifo_pop(fifo_t* f, uint32_t* out) {
        if (fifo_is_empty(f)) return 0;
        *out = f->buf[f->head];
        f->head = (f->head + 1) % FIFO_CAPACITY;
        f->count--;
        return 1;
}

/**
 * @fn interrupt_register
 * @brief IRQ 番号に対してハンドラを登録する（簡易）
 * @param irq 0..(MAX_IRQS-1)
 * @param handler ハンドラ関数
 * @param ctx ハンドラに渡すコンテキストポインタ
 * @return 0=成功, -1=範囲外
 */
int interrupt_register(uint32_t irq, irq_handler_t handler, void* ctx) {
        if (irq >= MAX_IRQS) return -1;
        uint32_t flags = irq_save();
        irq_table[irq] = handler;
        irq_ctx[irq] = ctx;
        irq_restore(flags);
        return 0;
}

/**
 * @fn interrupt_unregister
 */
int interrupt_unregister(uint32_t irq) {
        if (irq >= MAX_IRQS) return -1;
        uint32_t flags = irq_save();
        irq_table[irq] = NULL;
        irq_ctx[irq] = NULL;
        irq_restore(flags);
        return 0;
}

/**
 * @fn interrupt_raise
 * @brief 割り込みイベントを FIFO に入れる（割り込みハンドラから呼べる）
 */
int interrupt_raise(uint32_t event) {
    /* 割り込み中でも呼べるようにロックは使わない（非ブロッキング） */
        return fifo_push(&irq_fifo, event);
}

/**
 * @fn interrupt_dispatch_one
 * @brief FIFO からイベントを取り出して対応する IRQ ハンドラを呼ぶ（同期コンテキスト）
 */
int interrupt_dispatch_one(void) {
        uint32_t evt;
        if (!fifo_pop(&irq_fifo, &evt)) return 0;

        /* evt のレイアウト: 上位16bit = irq番号, 下位16bit = payload */
        uint32_t irq = (evt >> 16) & 0xFFFF;
        uint32_t payload = evt & 0xFFFF;

        if (irq < MAX_IRQS && irq_table[irq]) {
                irq_table[irq](payload, irq_ctx[irq]);
        } else {
                printk("Unhandled IRQ event: irq=%u payload=%u\n", (unsigned)irq, (unsigned)payload);
        }
        return 1;
}

/**
 * @fn interrupt_dispatch_all
 * @brief FIFO のイベントを全て処理する（通常 main ループで呼ぶ）
 */
void interrupt_dispatch_all(void) {
        while (interrupt_dispatch_one()) {}
}

/**
 * @fn interrupt_init
 * @brief 初期化
 */
void interrupt_init(void) {
        fifo_init(&irq_fifo);
        for (uint32_t i = 0; i < MAX_IRQS; ++i) {
                irq_table[i] = NULL;
                irq_ctx[i] = NULL;
        }
}
