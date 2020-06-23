/* softlockup_test.c - Test module code to generate CPU soft lockup/panic
 *
 * Copyright (C) 2015  Saiyam Doshi
 *
 * 25/Jun/2016 - Updated by Alberto Pires de Oliveira Neto <alberto.pires.on@gmail.com>
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

#define PROCFS_PARENT    "softlockup_test"
#define PROCFS_BUSY_LOOP "softlockup_test_busy"
#define PROCFS_MAIN_LOOP "softlockup_test_main"

static ssize_t procfile_read(struct file *file, char *buffer, size_t length, loff_t *offset);
static ssize_t procfile_write(struct file *file, const char *buffer, size_t length, loff_t *offset);
static ssize_t procfile_write_main(struct file *file, const char *buffer, size_t length, loff_t *offset);
struct proc_dir_entry *proc_parent;
struct proc_dir_entry *proc_busy;
struct proc_dir_entry *proc_main;

struct task_struct *task0;
static spinlock_t spinlock;
static char msg[128];
int val;
int loop_seconds;
int main_seconds;

static struct file_operations file_busy_ops = {  
	.owner = THIS_MODULE,
	.read  = procfile_read,
	.write = procfile_write,
};

static struct file_operations file_main_ops = {  
	.owner = THIS_MODULE,
	.write  = procfile_write_main,
};


int task(void *arg)
{
	struct timespec64 tv;
	time_t start;
	time_t elapsed = 0;

	printk(KERN_INFO "%s:%d\n",__func__,__LINE__);
	/* To generate panic uncomment following */
	/* panic("softlockup: hung tasks"); */

	ktime_get_real_ts64(&tv);
	start = tv.tv_sec;
	while(!kthread_should_stop()) {
		printk(KERN_INFO "%s:%d\n",__func__,__LINE__);
		spin_lock(&spinlock);
		/* busy loop in critical section */
		printk(KERN_INFO "%s:%d - Before\n",__func__,__LINE__);

		for (;;) {
			ktime_get_real_ts64(&tv);
			elapsed = tv.tv_sec - start;
			if (elapsed >= loop_seconds) {
				printk(KERN_INFO "%s:%d - Timer Value (%d)\n",__func__,__LINE__, loop_seconds);
				ktime_get_real_ts64(&tv);
				start = tv.tv_sec;
				printk(KERN_INFO "%s:%d - Reset Timer\n",__func__,__LINE__);
				break;
			}
		}

		msleep(0);
		printk(KERN_INFO "%s:%d - Elapsed: %ld\n",__func__,__LINE__, tv.tv_sec - start);

		printk(KERN_INFO "%s:%d - After\n",__func__,__LINE__);

		spin_unlock(&spinlock);
		msleep(main_seconds);
	}

	return val;
}

static int softlockup_init(void)
{
	printk(KERN_INFO "%s:%d\n",__func__,__LINE__);

	val = 1;
	loop_seconds = 5;
	main_seconds = 5000;
	spin_lock_init(&spinlock);
	task0 = kthread_run(&task,(void *)&val,"softlockup_thread");
	set_cpus_allowed_ptr(task0, cpumask_of(0));

	// Proc file system
	proc_parent = proc_mkdir(PROCFS_PARENT, NULL);

	proc_busy = proc_create(PROCFS_BUSY_LOOP, S_IFREG | S_IRUGO | O_RDWR, proc_parent, &file_busy_ops);
	if (proc_busy == NULL) {
		remove_proc_entry(PROCFS_BUSY_LOOP, NULL);

		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", PROCFS_BUSY_LOOP);
		return -ENOMEM;
	}

	proc_main = proc_create(PROCFS_MAIN_LOOP, S_IFREG | S_IRUGO | O_RDWR, proc_parent, &file_main_ops);
	if (proc_main == NULL) {
		remove_proc_entry(PROCFS_MAIN_LOOP, NULL);

		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", PROCFS_MAIN_LOOP);
		return -ENOMEM;
	}
	return 0;
}

static void softlockup_exit(void)
{
	printk(KERN_INFO "%s:%d\n",__func__,__LINE__);
	kthread_stop(task0);
	remove_proc_entry(PROCFS_BUSY_LOOP, proc_parent);
	remove_proc_entry(PROCFS_MAIN_LOOP, proc_parent);
	remove_proc_entry(PROCFS_PARENT   , NULL);
}

static ssize_t procfile_read(struct file *file, char *buffer, size_t length, loff_t *offset)  
{
	static int finished = 0;
	int ret = 0;

	printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_BUSY_LOOP);

	if (finished) {
		printk(KERN_INFO "procfs_read: END\n");
		finished = 0;
		return 0;
	}   

	finished = 1;
	ret = sprintf(buffer, "Hello,world!\n");
	return ret;
}

static ssize_t procfile_write(struct file *file, const char *buffer, size_t length, loff_t *offset)  
{
	int ignore;

	printk(KERN_INFO "procfile_write (/proc/%s) called\n", PROCFS_BUSY_LOOP);
	printk(KERN_INFO "procfs_write.length : %ld\n", length);

	ignore = copy_from_user(msg, buffer, length);
	ignore = kstrtoint(msg, 10, &loop_seconds);
	printk(KERN_INFO "Value : %d\n", loop_seconds);

	return length;
}

static ssize_t procfile_write_main(struct file *file, const char *buffer, size_t length, loff_t *offset)  
{
	int aux;
	int ignore;

	printk(KERN_INFO "procfile_write (/proc/%s) called\n", PROCFS_MAIN_LOOP);
	printk(KERN_INFO "procfs_write.length : %ld\n", length);

	ignore = copy_from_user(msg, buffer, length);
	ignore = kstrtoint(msg, 10, &aux);
	main_seconds = aux * 1000;
	printk(KERN_INFO "Value : %d\n", main_seconds);

	return length;
}

module_init(softlockup_init);
module_exit(softlockup_exit);

MODULE_AUTHOR("Saiyam Doshi");
MODULE_DESCRIPTION("Test module to generate CPU soft lock-up/panic");
MODULE_LICENSE("GPL v2");
