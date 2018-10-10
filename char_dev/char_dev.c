#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/types.h>
#include<linux/cdev.h>
#include<linux/slab.h>

#include<asm/uaccess.h>

#define CHAR_MAJOR 0
#define DEV_NAME "char_dev"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kawaharasouta <kawahara6514@gmail.com>");
MODULE_DESCRIPTION("simple character device.");

static int major_num = 0;

static struct cdev *cdev = NULL;


struct chardev_data {
	int val;
	struct cdev cdev;
};

static int chardev_open(struct inode *inode, struct file *fp) {
#if 1
	struct chardev_data *data;
#if 0
	unsigned int minor = iminor(inode);
	data = container_of(inode->i_cdev, struct chardev_data, cdev);
	fp->private_data = data;
	printk(KERN_INFO "char_dev: %s", __FUNCTION__);
  printk(KERN_INFO "&inode->i_cdev = %p\n", &inode->i_cdev);
  printk(KERN_INFO "  data = %p\n", data);
  printk(KERN_INFO "  cdev = %p\n", cdev);
#else
	printk("%s: major %d, minor %d (pid %d) \n", __func__, imajor(inode), iminor(inode), current->pid);
	data = (struct chardev_data *)kmalloc(sizeof(struct chardev_data), GFP_KERNEL);
	if(!data) {
		printk(KERN_ALERT "allocate error\n");
	}
	fp->private_data = data;
#endif

#else 
	printk("chardev_open\n");
#endif
	return 0;
}
static int chardev_close(struct inode *inode, struct file *fp) {
	printk("%s: major %d, minor %d (pid %d) \n", __func__, imajor(inode), iminor(inode), current->pid);
	if(fp->private_data) {
		kfree(fp->private_data);
			fp->private_data = NULL;
	}
	printk("chardev_close\n");
	return 0;
}
static ssize_t chardev_read(struct file *fp, char __user *buf, size_t count, loff_t *fpos) {
	//printk("chardev_read\n");
	printk("%s: count %lu pos %lld\n", __func__, count, *fpos);
	struct chardev_data *data = (struct chardev_data *)fp->private_data;
	int val = data->val;
	int ret = count;
	int i;
	for (i = 0; i < count; i++) {
		if(copy_to_user(&buf[i], &val, 1)) {
			ret = -EFAULT;
		}
	}

	return ret;
}
static ssize_t chardev_write(struct file *fp,const char __user *buf, size_t count, loff_t *fpos) {
	//printk("chardev_write\n");
	printk("%s: count %lu pos %lld\n", __func__, count, *fpos);
	struct chardev_data *data = (struct chardev_data *)fp->private_data;
	int val;
	if (count >= 1) {
		if(copy_from_user(&val, buf, 1)) {
			return -EFAULT;
		}
	}
	data->val = val;
	return count;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = chardev_open,
	.release = chardev_close,
	.read = chardev_read,
	.write = chardev_write,
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

	
	cdev = (struct cdev *)kmalloc(sizeof(struct cdev), GFP_KERNEL);
	if(!cdev) {
		printk(KERN_ALERT "kmalloc failed\n");
	}
	int devno = MKDEV(major_num, 0);
	cdev_init(cdev, &fops);
	//cdev->owner = THIS_MODULE;
	ret = cdev_add(cdev, devno, 1);
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

	cdev_del(cdev);
	kfree(cdev);
	return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);
