#include "scull.h"

// free all kernel mem for dev
static int scull_trim(struct scull_dev *dev)
{
	assert(dev, err);
	
	struct scull_qset *data;

	for (data = dev->data; data; data = data->next) {
		if (data->data) {
			for (i = 0; i < dev->qset; i++) {
				kfree(data->data[i]);
			}

			kfree(data->data);
			data->data = NULL;
		}

		kfree(data);
	}

	dev->size = 0;
	dev->quantum = s_quantum;
	dev->qset = s_qset;
	dev->data = NULL;

	return 0;

err:
	return -1;
}

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

	printk(KERN_ALERT "Call open dev: %p", dev);

	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "Call release dev: %p", filp->private_data);

	return 0;
}

struct scull_qset *scull_follow(struct scull_dev *dev, int item)
{
	struct scull_qset *qset;

	if (!dev->data) {
		// if fail, do nothing, deal in write
		dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
	}

	qset = dev->data;
	while (item > 0) {
		--item;
		if (!qset->next) {
			qset->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		}

		qset = qset->next;
		if (!qset) {
			return NULL;
		}

		memset(qset, 0, sizeof(struct scull_qset));
	}

	return qset;
}

ssize_t scull_read(struct file *filp, char __user *buff, 
	     size_t count, loff_t *offp)
{
	assert(filp, err);

	int retval = 0;
	struct scull_dev *dev;
	int quantum, qset, itemsize, s_pos, q_pos, rest;
	struct scull_qset *dptr;

	dev = filp->private_data;
	quantum = dev->quantum;
	qset = dev->qset;
	itemsize = quantum * qset;

	if (down_interruptible(&dev->sem)) {
		return -ERESTARTSYS;
	}

	if (*offp >= dev->size) {
		// end of file
		goto out;
	}

	if (*offp + count >= dev->size) {
		count = dev->size - *offp;
	}

	item = (long)*offp / itemsize;
	rest = (long)*offp % itemsize;

	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (dptr == NULL || !dptr->data || !dptr->data[s_pos]) {
		printk(KERN_ALERT "There is a hole %lu", *offp);
		goto out;
	}

	if (count > quantum - q_pos) {
		count = quantum - q_pos;
	}

	if (copy_to_user(buff, dptr->data[s_pos] + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}

	*offp += count;
	retval = count;

out:
	up(&dev->sem);
	return retval;

err:
	return -EINVAL;
}

ssize_t scull_write(struct file *filp, const char __user *buff,
	      size_t count, loff_t *offp)
{
	assert(filp, err);

	// default for no mem
	int retval = -ENOMEM;
	struct scull_dev *dev;
	int quantum, qset, itemsize, s_pos, q_pos, rest;
	struct scull_qset *dptr;

	dev = filp->private_data;
	quantum = dev->quantum;
	qset = dev->qset;
	itemsize = quantum * qset;

	if (down_interruptible(&dev->sem)) {
		return -ERESTARTSYS;
	}

	if (*offp + count >= dev->size) {
		count = dev->size - *offp;
	}

	item = (long)*offp / itemsize;
	rest = (long)*offp % itemsize;

	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (dptr == NULL) {
		printk(KERN_ALERT "There is No mem for write %lu", *offp);
		goto out;
	}

	if (!dptr->data) {
		dptr->data = kmalloc(sizeof(char) * qset, GFP_KERNEL);	
		if (!dptr->data) {
			goto out;
		}

		memset(dptr->data, 0, sizeof(char) * qset);
	}

	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(sizeof(char) * quantum, GFP_KERNEL);
		if (!dptr->data[s_pos]) {
			goto out;
		}

		memset(dptr->data[s_pos], 0, sizeof(char) * quantum);
	}

	if (count > quantum - q_pos) {
		count = quantum - q_pos;
	}

	if (copy_from_user(dptr->data[s_pos] + q_pos, buff, count)) {
		retval = -EFAULT;
		goto out;
	}

	*offp += count;
	retval = count;

	if (dev->size < *offp) {
		dev->size = *offp;
	}

out:
	up(&dev->sem);
	return retval;

err:
	return -EINVAL;
}

static int scull_init(void)
{
	printk(KERN_ALERT "scull init\n");

	return 0;
}

static void scull_exit(void)
{
	printk(KERN_ALERT "Goodbly, scull device\n");
}

module_init(scull_init);
module_exit(scull_exit);

