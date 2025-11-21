#include <task/multi_task.h>
#include <util/console.h>
#include <tests/define.h>

static void task_a(void) {
	for (int i = 0; i < 10; i++) {
		printk("A\n");
		task_yield();
	}
}

static void task_b(void) {
	for (int i = 0; i < 10; i++) {
		printk("B\n");
		task_yield();
	}
}

void multi_task_test(void) {
	task_t *ta = task_create(task_a, "task_a", 1);
	task_t *tb = task_create(task_b, "task_b", 1);
	if (ta)
		task_ready(ta);
	if (tb)
		task_ready(tb);

	task_schedule();
}