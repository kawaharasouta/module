#include<linux/module.h>
//#include<linux/timer.h>
#include<linux/debugfs.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kawaharasouta <kawahara6514@gmail.com>");
MODULE_DESCRIPTION("A simple example of debugfs");

static struct dentry *topdir;
static struct dentry *testfile;
static char testbuf[128];

struct timer_list mytimer;
#define MYTIMER_TIMEOUT_SECS	((unsigned long)1000)

static void 
mytimer_fn(unsigned long arg)
{
	printk(KERN_ALERT "%lu secs passed.\n", MYTIMER_TIMEOUT_SECS);
}

/***************************/
static ssize_t 
mytimer_remain_msecs_read(struct file *f, char __user *buf, size_t len, loff_t *ppos)
{
	unsigned long diff_msecs, now = jiffies;
	
	if (time_after(mytimer.expires, now))
		diff_msecs = (mytimer.expires - now) * 1000 / HZ;
	else
		diff_msecs = 0;
	
	snprintf(testbuf, sizeof(testbuf), "%lu\n", diff_msecs);
	return simple_read_from_buffer(buf, len, ppos, testbuf, strlen(testbuf));
}

static struct file_operations test_fops = {
	.owner = THIS_MODULE,
	.read = mytimer_remain_msecs_read,
};
/***************************/


static int 
mymodule_init(void) {
	printk(KERN_ALERT "debugfs example.\n");

	/* timer init */
	init_timer(&mytimer);
	mytimer.expires = jiffies + MYTIMER_TIMEOUT_SECS*HZ;
	mytimer.data = 0;
	mytimer.function = mytimer_fn;
	add_timer(&mytimer);

	/* debugfs */
	topdir = debugfs_create_dir("mytimer", NULL);
	if (!topdir)
		return -ENOMEM;
	testfile = debugfs_create_file("mytimer_remain_msecs", 0400, topdir, NULL, &test_fops);
	if (!testfile)
		return -ENOMEM;

	return 0;
}
static void 
mymodule_exit(void) {
	debugfs_remove_recursive(topdir);
	del_timer(&mytimer);

	printk(KERN_ALERT "debugfs example bye.\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
