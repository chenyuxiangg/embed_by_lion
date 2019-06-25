#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <asm/atomic.h>

MODULE_AUTHOR("xiaoxiangge");
MODULE_LICENSE("GPL");
typedef struct Timer_data{
	struct timer_list timer;
	unsigned long prev_jiffies;
	unsigned int loops;
}timer_data;

timer_data test_data1;
timer_data test_data2;
static void do_work(void*);
static DECLARE_WORK(test_work1,do_work);
static struct workqueue_struct* test_workqueue;
static struct tasklet_struct my_tasklet; 
atomic_t wq_run_times;
static int failed_cnt = 0;

static int work_delay = 2*HZ;

static void tasklet_handler(unsigned long data){
	printk("====tasklet run times:%u====\n",atomic_read(&wq_run_times));
}

void test_timer_fn1(unsigned long arg){
	timer_data* data = (timer_data*)arg;
	unsigned long j = jiffies;
	data->timer.expires += work_delay;
	data->prev_jiffies = j;
	add_timer(&data->timer);
	if(queue_work(test_workqueue,&test_work1) == 0){
		printk("Timer(0)add work queue failed.\n");
		(*(&failed_cnt))++;
	}
	data->loops++;
	printk("timer-0 loops:%u\n",data->loops);
}

void test_timer_fn2(unsigned long arg){
	timer_data* data = (timer_data*)arg;
	unsigned long j = jiffies;
	data->timer.expires += 3*HZ;
	data->prev_jiffies = j;
	add_timer(&data->timer);
	tasklet_schedule(&my_tasklet);
	data->loops++;
	printk("timer-1 loops:%u\n",data->loops);
}

void do_work(void* arg){
	atomic_inc(&wq_run_times);
	printk("=====work queue run times:%u=====\n",atomic_read(&wq_run_times));
	printk("====failed_cnt:%u\n",*(&failed_cnt));
}

int wq_init(void){
	tasklet_init(&my_tasklet,tasklet_handler,0);

	unsigned long j = jiffies;
	printk("jiffies:%u\n",jiffies);
	atomic_set(&wq_run_times,0);
	init_timer(&test_data1.timer);
	test_data1.loops = 0;
	test_data1.prev_jiffies = j;
	test_data1.timer.function = test_timer_fn1;
	test_data1.timer.data = (unsigned long)(&test_data1);
	test_data1.timer.expires = j+2*HZ;
	add_timer(&test_data1.timer);

	init_timer(&test_data2.timer);
	test_data2.loops = 0;
	test_data2.prev_jiffies = j;
	test_data2.timer.function = test_timer_fn2;
	test_data2.timer.data = (unsigned long)(&test_data2);
	test_data2.timer.expires = j+3*HZ;
	add_timer(&test_data2.timer);
	test_workqueue = create_singlethread_workqueue("test-wq");
	printk("test-wq init success\n");
	printk("jiffies:%lu\n",jiffies);
	return 0;
}

void wq_exit(void){
	del_timer(&test_data1.timer);
	del_timer(&test_data2.timer);
	tasklet_kill(&my_tasklet);
	destroy_workqueue(test_workqueue);
	printk("wq_exit success\n");
	printk("tasklet exit success.\n");
}

module_init(wq_init);
module_exit(wq_exit);
