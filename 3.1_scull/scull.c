#include "scull.h"

// global var
struct scull_dev *scull_devices;

// free all kernel mem for dev
static int scull_trim(struct scull_dev *dev)
{
	int i = 0;
	struct scull_qset *data;

	assert(dev, err);

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

int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		scull_trim(dev);
	}

	printk(KERN_ALERT "Call open dev: %p\n", dev);

	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "Call release dev: %p\n", filp->private_data);

	return 0;
}

struct scull_qset *scull_follow(struct scull_dev *dev, int item)
{
	struct scull_qset *qset;

	if (!dev->data) {
		// if fail, do nothing, deal in write
		dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		memset(dev->data, 0, sizeof(struct scull_qset));
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
	int retval = 0;
	int item;
	struct scull_dev *dev;
	int quantum, qset, itemsize, s_pos, q_pos, rest;
	struct scull_qset *dptr;

	assert(filp, err);

	dev = filp->private_data;
	quantum = dev->quantum;
	qset = dev->qset;
	itemsize = quantum * qset;

	printk (KERN_ALERT "begin read count: %zu, offp: %lld\n", count, *offp);
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
		printk(KERN_ALERT "There is a hole %lld\n", *offp);
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
	printk (KERN_ALERT "finish read count: %zu, offp: %lld\n", count, *offp);
	return retval;

err:
	return -EINVAL;
}

ssize_t scull_write(struct file *filp, const char __user *buff,
	      size_t count, loff_t *offp)
{
	int item;
	// default for no mem
	int retval = -ENOMEM;
	struct scull_dev *dev;
	int quantum, qset, itemsize, s_pos, q_pos, rest;
	struct scull_qset *dptr;

	assert(filp, err);
	printk(KERN_ALERT "begin write count: %zu, offp: %lld\n", count, *offp);

	dev = filp->private_data;
	quantum = dev->quantum;
	qset = dev->qset;
	itemsize = quantum * qset;

	if (down_interruptible(&dev->sem)) {
		return -ERESTARTSYS;
	}

	item = (long)*offp / itemsize;
	rest = (long)*offp % itemsize;

	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (!dptr) {
		printk(KERN_ALERT "There is No mem for write %lld\n", *offp);
		goto out;
	}

	if (!dptr->data) {
		dptr->data = kmalloc(sizeof(char *) * qset, GFP_KERNEL);	
		if (!dptr->data) {
			goto out;
		}

		memset(dptr->data, 0, sizeof(char *) * qset);
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

	printk(KERN_ALERT "finish write, count: %zu, offp: %lld\n", count, *offp);
out:
	up(&dev->sem);
	return retval;

err:
	return -EINVAL;
}

struct file_operations scull_fops = {
	.owner  = THIS_MODULE,
//	.llseek = scull_llseek,
	.read   = scull_read,
	.write  = scull_write,
//	.ioctl  = scull_ioctl,
	.open   = scull_open,
	.release = scull_release,
};

static int scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err;
	int devno;

	cdev_init(&dev->cdev, &scull_fops);
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

static void scull_cleanup(void)
{
	int i = 0;
	dev_t dev;

	if (scull_devices) {
		for (i = 0; i < scull_nr; i++) {
			scull_trim(&scull_devices[i]);
			cdev_del(&scull_devices[i].cdev);
		}

		kfree(scull_devices);
	}

	dev = MKDEV(scull_major, scull_minor);
	unregister_chrdev_region(dev, scull_nr);
}

static int scull_init(void)
{
	int i = 0;
	int res = 0;
	dev_t dev = 0;

	printk(KERN_ALERT "scull init\n");
	S_LOG("test cfalgs log\n");

	// alloc region major
	if (scull_major) {
		dev = MKDEV(scull_major, scull_minor);
		res = register_chrdev_region(dev, scull_nr, "scull");
	} else {
		res = alloc_chrdev_region(&dev, scull_minor, scull_nr, "scull");
		scull_major = MAJOR(dev);
	}

	if (res < 0) {
		printk(KERN_ALERT "alloc region fail: ma(%u), mi(%u)\n",
				  scull_major, scull_minor);
		return res;
	}

	scull_devices = kmalloc(sizeof(struct scull_dev) * scull_nr, GFP_KERNEL);
	if (!scull_devices) {
		res = -ENOMEM;
		goto fail;
	}

	memset(scull_devices, 0, scull_nr * sizeof(struct scull_dev));

	for (i = 0; i < scull_nr; i++) {
		scull_devices[i].quantum = s_quantum;
		scull_devices[i].qset    = s_qset;
		sema_init(&scull_devices[i].sem, 1);
		scull_setup_cdev(&scull_devices[i], i);
	}


	return 0;

fail:
	scull_cleanup();
	return res;
}

static void scull_exit(void)
{
	scull_cleanup();

	printk(KERN_ALERT "Goodbly, scull device\n");
}

module_init(scull_init);
module_exit(scull_exit);

