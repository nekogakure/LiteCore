#include <util/config.h>
#include <util/console.h>
#include <task/multi_task.h>
#include <tests/define.h>

// カウントアップするタスク（ひたすら数えるんだ...いいね...？）
static void test_task1(void) {
	printk("[Task1] Started! About to yield...\n");
	task_yield();
	printk("[Task1] Resumed after yield\n");
	printk("[Task1] Exiting...\n");
	task_exit();
}

// 別のメッセージを出力（するだけ）
static void test_task2(void) {
	printk("[Task2] Started! About to yield...\n");
	task_yield();
	printk("[Task2] Resumed after yield\n");
	printk("[Task2] Exiting...\n");
	task_exit();
}

/**
 * @brief マルチタスクのテスト
 */
void multi_task_test(void) {
	printk("====== SIMPLE MULTITASK TEST ======\n");
	printk("Creating 2 test tasks...\n");

	// タスク1を作成して実行キューに追加
	task_t *t1 = task_create(test_task1, "TestTask1", 1);
	if (t1) {
		task_ready(t1);
		printk("Created Task1 (TID=%u)\n", (unsigned)t1->tid);
	} else {
		printk("Failed to create Task1\n");
		return;
	}

	// タスク2を作成して実行キューに追加
	task_t *t2 = task_create(test_task2, "TestTask2", 1);
	if (t2) {
		task_ready(t2);
		printk("Created Task2 (TID=%u)\n", (unsigned)t2->tid);
	} else {
		printk("Failed to create Task2\n");
		return;
	}

	printk("Tasks created. Manually yielding to trigger first switch...\n");
	new_line();
	
	// 最初のタスクスイッチを手動でトリガー
	task_yield();
	
	printk("\n[MAIN] Back to main after tasks completed\n");
	printk("====== TEST COMPLETE ======\n");
	new_line();
}
