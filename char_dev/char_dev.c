#include<linux/module.h>
#include<linux/fs.h>
#include<linux/types.h>
#include <linux/cdev.h>
//#include<linux/kdev_t.h>

#define CHAR_MAJOR 0
#define DEV_NAME "char_dev"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kawaharasouta <kawahara6514@gmail.com>");
MODULE_DESCRIPTION("simple character device.");

static int major_num = 0;



struct file_operations fops = {
	//.open = chardev_open;
};


static int mymodule_init(void) {
	printk(KERN_INFO "char_dev loaded.\n");

	dev_t dev;
	int ret;
	ret = alloc_chrdev_region(&dev, 0, 1, DEV_NAME);
	if (ret) {
		printk(KERN_ALERT "alloc_chardev_region failed.\n");
		return ret;
	}
	major_num = MAJOR(dev);
	printk(KERN_INFO "char_dev major_num:%d.\n", major_num);


	struct cdev cdev;
	int devno = MKDEV(major_num, 0);
	cdev_init(&cdev, &fops);
	cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdev, devno, 1);
	if(ret) {
		printk(KERN_ALERT "cdev_add failed.\n");
		return ret;
	}


	return 0;
}
static void mymodule_exit(void) {
	printk(KERN_INFO "char_dev exit.\n");

	dev_t dev = MKDEV(major_num, 0);
	unregister_chrdev_region(dev, 1);
	return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);
