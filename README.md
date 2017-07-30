This is a study for linux device driver

# for hello.ko
 Usage:
   make
   insmod hello.ko howm=5 whom="YES"
   rmmod  hello.ko
   rmmod
	# dmesg for log


# for scull.ko
phase 1.
	scull_open: add scull_dev to file->private_data for next read/write
 	add_cdv: for register kobject(link dev_t and kobj), operator on cdev

