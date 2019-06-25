#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <asm/atomic.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

//4
#define DEV_NAME "cyx_mmap"
static unsigned long address;

static void cyx_vma_open(struct vm_area_struct* vma){
	printk("cyx_vma_open,virt %lx,phs %lx\n",vma->vm_start,vma->vm_pgoff<<PAGE_SHIFT);
}

static void cyx_vma_close(struct vm_area_struct* vma){
	printk("cyx_vma_close\n");
}

static int cyx_vma_fault(struct vm_area_struct* vma,struct vm_fault* vmf){
	printk("====cyx_vma_fault====\n");
	struct page* pageptr;
	unsigned long offset = address;
	unsigned long virtaddr = vmf->virtual_address-vma->vm_start+offset;

	pageptr = virt_to_page(virtaddr);
	if(!pageptr){
		return VM_FAULT_SIGBUS;
	}
	get_page(pageptr);
	vmf->page = pageptr;
	return 0;
}

static struct vm_operations_struct cyx_fault_vm_ops = {
	.open = cyx_vma_open,
	.close = cyx_vma_close,
	.fault = cyx_vma_fault,
};

static int cyx_fault_mmap(struct file* file,struct vm_area_struct* vma){
	printk("====cyx_fault_mmap====\n");
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	if(offset >= __pa(high_memory) || (file->f_flags & O_SYNC)){
		vma->vm_flags |= VM_IO;
	}
	vma->vm_flags |= VM_RESERVED;
	vma->vm_ops = &cyx_fault_vm_ops;
	cyx_vma_open(vma);
	return 0;
}

static int cyx_open(struct inode* inode,struct file* file){
	printk("====cyx_open====\n");
	return 0;
}

static int cyx_release(struct inode* inode,struct file* file){
	printk("====cyx_release====\n");
	return 0;
}

static struct file_operations cyx_fault_ops = {
	.open = cyx_open,
	.release = cyx_release,
	.mmap = cyx_fault_mmap,
};

static struct miscdevice nopage_dev = 
{
	    MISC_DYNAMIC_MINOR,
		DEV_NAME,
		&cyx_fault_ops,
};

//3
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

//2
#define PROC_NAME "test_proc"
static struct my_02_data{
	char* buf;
	size_t max_size;
	size_t current_count;
	};

static struct my_02_data* my_02_buf = NULL;
static uint buf_size = 0;

module_param(buf_size,uint,S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(buf_size,"the size of my_02_buf\n");
MODULE_AUTHOR("CYX");
MODULE_LICENSE("GPL");

static void* my_seq_start(struct seq_file* file,loff_t* pos){
	printk("[--kernel--]start\n");
	return (*pos < my_02_buf->current_count) ? pos : NULL;
}

static void* my_seq_next(struct seq_file* file,void* v,loff_t* pos){
	printk("[--kernel--]next,pos = %d\n",*pos);
	(*pos)++;
	if(*(pos) >= my_02_buf->current_count)
		return NULL;
	return pos;
}

static void my_seq_stop(struct seq_file* file,void* v){

}

static int my_seq_show(struct seq_file* s,void* v){
	loff_t index = *(loff_t*)v;
	seq_printf(s,"%c",my_02_buf->buf[index]);
	if(index == my_02_buf->current_count-1){
		seq_printf(s,"\n");
	}
	return 0;
	}

static const struct seq_operations my_seq_ops = {
	.start = my_seq_start,
	.next = my_seq_next,
	.stop = my_seq_stop,
	.show = my_seq_show,
};

static int my_open(struct inode* inode,struct file* file){
	printk("[--kernel--]open\n");
	return seq_open(file,&my_seq_ops);
	}

static ssize_t my_read(struct file* file,char __user* buf,size_t count,loff_t* offset){
	if(count < my_02_buf->current_count){
		if(copy_to_user(buf,my_02_buf->buf + *offset,count) != 0)
			return -EFAULT;
		*offset += count;
		return count;
	}
	else{
		if(copy_to_user(buf,my_02_buf->buf + *offset,my_02_buf->current_count) != 0){
			return -EFAULT;
		}
		*offset += my_02_buf->current_count;
		return my_02_buf->current_count;
	}
}

static ssize_t my_write(struct file* file,const char __user* buf,size_t count,loff_t* offset){
	memset(my_02_buf->buf,0,buf_size);
	if(count < my_02_buf->max_size){
		if(copy_from_user(my_02_buf->buf + *offset,buf,count) != 0)
			return -EFAULT;
		my_02_buf->current_count = count;
		*offset += count;
		return count;
	}
	else{
		if(copy_from_user(my_02_buf->buf + *offset,buf,my_02_buf->max_size) != 0)
			return -EFAULT;
		my_02_buf->current_count = my_02_buf->max_size;
		*offset += my_02_buf->max_size;
		return my_02_buf->max_size;
	}
}

static struct file_operations my_file_ops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.read = seq_read,
	.write = my_write,
	.llseek = seq_lseek,
	.release = seq_release,
	};

//1
struct my_01_data{
	char buf[1024];
	size_t count;
	size_t busy_count;
	};

static struct my_01_data* my_01_buf = NULL;			//(1)
static struct cdev cdev;
static dev_t  devno;

static char tmp[128] = "hello from kernel!";

static int
hello_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "[--kernel--]open\n");
	//(2)
	if(my_01_buf == NULL){
		my_01_buf = (struct my_01_data*)kmalloc(sizeof(struct my_01_data),GFP_KERNEL);
		memset(my_01_buf->buf,0,sizeof(my_01_buf->buf));
		my_01_buf->count = 0;
		my_01_buf->busy_count = 0;
		}
	my_01_buf->count++;
    return 0;
}

static int
hello_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "[--kernel--]release\n");
	//(3)
	if(my_01_buf != NULL){
		my_01_buf->count--;
		if(my_01_buf->count == 0){
			kfree(my_01_buf);
			}
		}
    return 0;
}

static ssize_t
hello_read(struct file *filep, char __user *buf, size_t count, loff_t *offset)
{
    size_t  avail;

    printk(KERN_INFO "[--kernel--]read\n");

    avail = sizeof(tmp) - *offset;

    if (count <= avail) {
        if (copy_to_user(buf, tmp + *offset, count) != 0) {
            return -EFAULT;
        }

        *offset += count;
        return count;

    } else {
        if (copy_to_user(buf, tmp + *offset, avail) != 0) {
            return -EFAULT;
        }

        *offset += avail;
        return avail;
    }
}

static ssize_t
cyx_read(struct file* file,char __user* buf,size_t count,loff_t* offset)
{
	size_t avail = 0;
	size_t busy_count = 0;
	printk(KERN_INFO "[--kernel--]read\n");
	avail = sizeof(my_01_buf->buf) - *offset;
	busy_count = my_01_buf->busy_count;
	if(count <= avail){
		if(count <= busy_count){
			if(copy_to_user(buf,my_01_buf->buf + *offset,count) != 0){
				return -EFAULT;
				}
			*offset += count;
			return count;
			}
		else{
			if(copy_to_user(buf,my_01_buf->buf + *offset,busy_count) != 0){
				return -EFAULT;
				}
			*offset += busy_count;
			return busy_count;
			}
		}
	else{
		if(copy_to_user(buf,my_01_buf->buf + *offset,busy_count) != 0){
			return -EFAULT;
			}
		*offset += busy_count;
		return busy_count;
		}
	}

static ssize_t
hello_write(struct file *filep, const char __user *buf, size_t count,
            loff_t *offset)
{
    size_t  avail;

    printk(KERN_INFO "[--kernel--]write\n");

    avail = sizeof(tmp) - *offset;

    memset(tmp + *offset, 0, avail);

    if (count > avail) {
        if (copy_from_user(tmp + *offset, buf, avail) != 0) {
            return -EFAULT;
        }
        *offset += avail;
        return avail;

    } else {
        if (copy_from_user(tmp + *offset, buf, count) != 0) {
            return -EFAULT;
        }
        *offset += count;
        return count;
    }
}

static ssize_t
cyx_write(struct file* file,const char __user* buf,size_t count,loff_t* offset){
	size_t avail = 0;
	printk(KERN_INFO "[--kernel--]write\n");
	avail = sizeof(my_01_buf->buf) - *offset;
	memset(my_01_buf->buf + *offset,0,avail);

	if(count > avail){
		if(copy_from_user(my_01_buf->buf + *offset,buf,avail) != 0){
			return -EFAULT;
			}
		*offset += avail;
		my_01_buf->busy_count = avail;
		return avail;
		}
	else{
		if(copy_from_user(my_01_buf->buf + *offset,buf,count) != 0){
			return -EFAULT;
			}
		*offset += count;
		my_01_buf->busy_count = count;
		return count;
		}
	}

static loff_t
hello_llseek(struct file *filep, loff_t off, int whence)
{
    loff_t  newpos;

    switch (whence) {
    case 0: /* SEEK_SET */
        newpos = off;
        break;
    case 1: /* SEEK_CUR */
        newpos = filep->f_pos + off;
        break;
    case 2: /* SEEK_END */
        newpos = sizeof(tmp) + off;
        break;
    default:
        return -EINVAL;
    }

    if (newpos < 0) {
        return -EINVAL;
    }

    filep->f_pos = newpos;
    return newpos;
}

static const struct file_operations  fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .release = hello_release,
    .read = cyx_read,
    .llseek = hello_llseek,
    .write = cyx_write,
};


int init(void){
	//4
	int result = misc_register(&nopage_dev);
	if(result){
		return result;
	}

	address = __get_free_pages(GFP_KERNEL,get_order(65536));
	if(address == 0){
		misc_deregister(&nopage_dev);
		printk("====cyx_init failed====\n");
		return -ENOMEM;
	}

	printk("====cyx_init success====\n");
	printk("====address:%lu\n",address);

	//3
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


	//2
	struct proc_dir_entry* entry;
	entry = proc_create(PROC_NAME,S_IWUSR | S_IRUGO,NULL,&my_file_ops);
	if(entry == NULL){
		return -ENOMEM;
		}
	if(my_02_buf == NULL){
		my_02_buf = (struct my_02_data*)kmalloc(sizeof(struct my_02_data),GFP_KERNEL);
		my_02_buf->buf = (char*)kmalloc(buf_size,GFP_KERNEL);
		memset(my_02_buf->buf,0,buf_size);
		my_02_buf->max_size = buf_size;
		my_02_buf->current_count = 0;
		}
	printk(KERN_INFO "initialize /proc/test_proc success!\n");

	//1
	int    ret;

    printk(KERN_INFO "Load hello\n");

    devno = MKDEV(111, 0);
    ret = register_chrdev_region(devno, 1, "hello");

    if (ret < 0) {
        return ret;
    }

    cdev_init(&cdev, &fops);
    cdev.owner = THIS_MODULE;

    cdev_add(&cdev, devno, 1);

	return 0;
	}

void cleanup(void){
	//4
	free_pages(address,get_order(65536));
	misc_deregister(&nopage_dev);
	printk("====cyx_exit====\n");
	
	//3
	del_timer(&test_data1.timer);
	del_timer(&test_data2.timer);
	tasklet_kill(&my_tasklet);
	destroy_workqueue(test_workqueue);
	printk("wq_exit success\n");
	printk("tasklet exit success.\n");

	//2
	if(my_02_buf != NULL){
		kfree(my_02_buf->buf);
		kfree(my_02_buf);
	}
	remove_proc_entry(PROC_NAME,NULL);

	//1
	printk(KERN_INFO "cleanup hello\n");
    unregister_chrdev_region(devno, 1);
    cdev_del(&cdev);

	}

module_init(init);
module_exit(cleanup);
