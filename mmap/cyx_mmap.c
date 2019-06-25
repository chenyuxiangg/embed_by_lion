#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#define DEV_NAME "cyx_mmap"

MODULE_LICENSE("GPL");

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


static int cyx_init(void){
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
	return 0;
}

static void cyx_exit(void){
	free_pages(address,get_order(65536));
	misc_deregister(&nopage_dev);
	printk("====cyx_exit====\n");
}

module_init(cyx_init);
module_exit(cyx_exit);
