/* softlockup_test.c - Test module code to generate CPU soft lockup/panic
 *
 * Copyright (C) 2015  Saiyam Doshi
 *
 * 24/Jun/2016 - Updated by Alberto Pires de Oliveira Neto <alberto.pires.on@gmail.com>
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>

struct task_struct *task0;
static spinlock_t spinlock;
int val;

int task(void *arg)
{
	struct timeval tv;
	time_t start;
	time_t elapsed = 0;
	int loop;

	printk(KERN_INFO "%s:%d\n",__func__,__LINE__);
	/* To generate panic uncomment following */
	/* panic("softlockup: hung tasks"); */

	do_gettimeofday(&tv);
	start = tv.tv_sec;
	while(!kthread_should_stop()) {
		printk(KERN_INFO "%s:%d\n",__func__,__LINE__);
		spin_lock(&spinlock);
		/* busy loop in critical section */
		printk(KERN_INFO "%s:%d - Before\n",__func__,__LINE__);


		for (;;) {
			do_gettimeofday(&tv);
			elapsed = tv.tv_sec - start;
			if (elapsed >= 25) break;
		}

		msleep(0);
		printk(KERN_INFO "%s:%d - Elapsed: %ld\n",__func__,__LINE__, tv.tv_sec - start);

		printk(KERN_INFO "%s:%d - After\n",__func__,__LINE__);

		spin_unlock(&spinlock);
		msleep(30000);
	}

	return val;
}

static int softlockup_init(void)
{
	printk(KERN_INFO "%s:%d\n",__func__,__LINE__);

	val = 1;
	spin_lock_init(&spinlock);
	task0 = kthread_run(&task,(void *)&val,"softlockup_thread");
	set_cpus_allowed(task0, *cpumask_of(0));

	return 0;
}

static void softlockup_exit(void)
{
	printk(KERN_INFO "%s:%d\n",__func__,__LINE__);
	kthread_stop(task0);
}

module_init(softlockup_init);
module_exit(softlockup_exit);

MODULE_AUTHOR("Saiyam Doshi");
MODULE_DESCRIPTION("Test module to generate CPU soft lock-up/panic");
MODULE_LICENSE("GPL v2");
