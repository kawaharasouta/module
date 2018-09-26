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

static unsigned long mytimer_timeout_msecs = 1000 * 1000;

static void 
mytimer_fn(unsigned long arg)
{
	printk(KERN_ALERT "%lu secs passed.\n", mytimer_timeout_msecs / 1000);
}

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

static ssize_t 
mytimer_remain_msecs_write(struct file *f, const char __user *buf, size_t len, loff_t *ppos)
{
	ssize_t ret;

	ret = simple_write_to_buffer(testbuf, sizeof(testbuf), ppos, buf, len);
	if (ret < 0)
		return ret;
	sscanf(testbuf, "%20lu", &mytimer_timeout_msecs);
	mod_timer(&mytimer, jiffies + mytimer_timeout_msecs * HZ / 1000);
	return ret;
}

static struct file_operations test_fops = {
	.owner = THIS_MODULE,
	.read = mytimer_remain_msecs_read,
	.write = mytimer_remain_msecs_write,
};

static int 
mymodule_init(void)
{
	printk(KERN_ALERT "debug example.\n");
	init_timer(&mytimer);
	mytimer.expires = jiffies + mytimer_timeout_msecs * HZ / 1000; 
	mytimer.data = 0;
	mytimer.function = mytimer_fn;
	add_timer(&mytimer);

	topdir = debugfs_create_dir("mytimer", NULL);
	if (!topdir)
		return -ENOMEM;
	testfile = debugfs_create_file("remain_msecs", 0600, topdir, NULL, &test_fops);
	if (!testfile)
		return -ENOMEM;

	return 0;
}
static void 
mymodule_exit(void)
{
	debugfs_remove_recursive(topdir);
	del_timer(&mytimer);

	printk(KERN_ALERT "debug example bye.\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
