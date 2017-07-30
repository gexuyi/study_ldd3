#include "scull.h"

static int scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err;
	int devno;

	cdev_init(&dev->cdev, scull_fops);
	dev->cdev.owner = THIS_MODULE;

	devno = MKDEV(scull_major, scull_minor + index);
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_NOTICE "Error %d adding scull(%d)\n",
		       err, index);
		return err;
	}

	return 0;
}

int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		scull_trim(dev);
	}

	return 0;
}

static int scull_init(void)
{
	int i = 0;

	printk(KERN_ALERT "scull init\n");

	return 0;
}

static void scull_exit(void)
{
	printk(KERN_ALERT "Goodbly, scull device\n");
}

module_init(scull_init);
module_exit(scull_exit);

