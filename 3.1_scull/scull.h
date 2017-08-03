#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
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
		} \
	} while(0);

#ifdef S_DEBUG 
#define S_LOG(fmt, args...) printk(KERN_ALERT "scull: " fmt, ##args)
#else 
#define S_LOG(fmt, args...)	/* do nothing */
#endif

#define SCULL_NR  8
#define S_QUANTUM 4000
#define S_QSET    1000

unsigned int scull_major, scull_minor;
unsigned int s_quantum = S_QUANTUM;
unsigned int s_qset = S_QSET;
int scull_nr = SCULL_NR;
module_param(s_quantum, uint, S_IRUGO);
module_param(s_qset,    uint, S_IRUGO);
module_param(scull_major, uint, S_IRUGO);
module_param(scull_minor, uint, S_IRUGO);

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

