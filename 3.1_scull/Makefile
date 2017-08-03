
# Comment/Uncomment log
DEBUG = y

ifeq ($(DEBUG), y)
	DFLAGS = -O -g -DS_DEBUG
else
	DFLAGS = -O2
endif

EXTRA_CFLAGS += $(DFLAGS)

ifneq ($(KERNELRELEASE), )
	obj-m := scull.o


else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)


default:
	#@echo "i am def, $(EXTRA_CFLAGS)"
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules


endif

clean:
	rm -rf *.o *core *.depend *.cmd *.ko *.mod.c .tmp_versions  *odule* .scull* .Make*

