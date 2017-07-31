#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

#define assert(x, label) \
	do { \
		if (!x) { \
			printk(KERN_ALERT "assert fail"); \
			goto label; \
	} while(0);

unsigned int scull_major, scull_minor;
unsigned int s_quantum, s_qset;
module_param(s_quantum, uint, S_IRUGO);
module_param(s_qset,    uint, S_IRUGO);

struct file_operations scull_fops = {
	.owner  = THIS_MODULE,
//	.llseek = scull_llseek,
	.read   = scull_read,
	.write  = scull_write,
//	.ioctl  = scull_ioctl,
	.open   = scull_open,
	.release = scull_release,
};

struct scull_qset {
	void **data;
	struct scull_qset *next;
};

struct scull_dev {
	unsigned int qset;
	unsigned int quantum;
	struct scull_qset *data;
	unsigned long size; // total size

	struct semaphore sem;
	struct cdev cdev;
};

