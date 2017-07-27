#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");

static char *whom = "word";
static int   howm = 2;

module_param(whom, charp, S_IRUGO);
module_param(howm, int,   S_IRUGO);

static int hello_init(void)
{
	int i = 0;

	printk(KERN_ALERT "Hello world\n");

	for (; i<howm; i++) {
		printk(KERN_ALERT "%s\n", whom);
	}
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "Goodbly, curel world\n");
}

module_init(hello_init);
module_exit(hello_exit);

