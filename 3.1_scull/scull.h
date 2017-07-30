#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

unsigned int scull_major, scull_minor;

struct file_operations scull_fops = {
	.owner  = THIS_MODULE,
	.llseek = scull_llseek,
	.read   = scull_read,
	.write  = scull_write,
	.ioctl  = scull_ioctl,
	.open   = scull_open,
	.release = scull_release,
};

struct scull_dev {

	struct cdev cdev;
};

