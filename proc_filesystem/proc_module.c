#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>

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

int init(void){
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
	return 0;
	}

void cleanup(void){
	if(my_02_buf != NULL){
		kfree(my_02_buf->buf);
		kfree(my_02_buf);
	}
	remove_proc_entry(PROC_NAME,NULL);
	}

module_init(init);
module_exit(cleanup);
