#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define SCULL_IOC_MAGIC 'k'

#define SCULL_IOCRESET    _IO(SCULL_IOC_MAGIC,  7)
#define SCULL_IOCSQUANTUM _IOW(SCULL_IOC_MAGIC, 8, int)
#define SCULL_IOCTQUANTUM _IOW(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCGQUANTUM _IOR(SCULL_IOC_MAGIC, 10, int)
#define SCULL_IOCQQUANTUM _IOR(SCULL_IOC_MAGIC, 11, int)

int main(int argc, char *argv[])
{
	int fd, quantum, old_quantum;
	const char *dev = NULL;

	if (argc < 2) {
		puts("Usage: ./s_ioctl_test $dev_name");
		exit(1);
	}

	dev = argv[1];
	printf("test %s\n", dev);
	
	fd = open(dev, O_RDWR);
	if (fd < 0) {
		perror("open fail");
		exit(1);
	}
	
	quantum = 888;
	ioctl(fd, SCULL_IOCGQUANTUM, &old_quantum);

	ioctl(fd, SCULL_IOCSQUANTUM, &quantum);
	quantum = ioctl(fd, SCULL_IOCQQUANTUM);
	printf("dev: %s, old: %d, after set quantum: %d\n", dev, old_quantum, quantum);

	return 0;
}

