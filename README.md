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

phase 2.
	scull_read/write: add read/write for scull device

phase 3.
	finish scull_init and scull_exit to init module and exit module
	now:
		bash ./scull.load   --> insmod scull.ko && mknod device
		echo/dd/fio to /dev/scull0  --> test read
		cat/dd/fio  from /dev/scull0 --> test write
		exp:
		   echo "helo" > /dev/scull0
		   cat /dev/scull0    // also can monitor dmesg by `dmesg -m -T`

phase 4.
	add S_DBEUG macro for debug or not
	#NOTICE
	   when i write makefile, statement "DEBUG = y ",
	   a space after line made "ifeq ($(DEBUG), y)" not true,
	   so never let spaces before a new line

phase 5.
	there is some differen between 2.6 and 3.10 in proc file operater
	so very simple proc entry create for scull(only support 3.10)

	add scull.load and scull.unload
	Usage:
		cd $code_path
		make
		bash scull.load/scull.unload --> only support current dir op

	add misc-progs/gdbline for gdb vmlinux add section for print some var value

