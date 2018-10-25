#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/types.h>
//#include<linux/cdev.h>
#include<linux/slab.h>

#include<linux/netdevice.h>

#include<asm/uaccess.h>

#define CHAR_MAJOR 0
#define DEV_NAME "char_dev"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kawaharasouta <kawahara6514@gmail.com>");
MODULE_DESCRIPTION("simple network device.");

static int major_num = 0;

static struct net_device *netdev = NULL;


//struct chardev_data {
//	int val;
//	struct cdev cdev;
//};

struct netdev_priv {
	struct netdevice_stats stats;
	int status;

}

static int netdev_open(struct net_device *dev) {
	
}

struct net_device_ops net_dev_ops = {
	//.ndo_init = netdev_init,
	.ndo_open = netdev_open,
	.ndo_stop = netdev_close,
	.ndo_start_xmit = netdev_tx,
	//.ndo_do_ioctl = netdev_ioctl.
	.ndo_sget_stats = netdev_get_stats,
};

//!!!!!!!!!!!
static void netdev_setup(struct net_device *dev) {

}

//!!!!!!!!!!!!
static int netdev_init(void) {
	int ret;
	printk(KERN_INFO "netdev_init\n");
	
	netdev = alloc_netdev(sizeof(struct netdev_priv), "nd%d", NET_NAME_UNKNOWN, netdev_setup);

	if (!netdev) {
		printk(KERN_ALERT MODULE_NAME "alloc_netdev faild.\n");
		netdev_exit();
		return -ENOMEM;
	}

	ret = register_netdev(netdev);
	if (ret) {
		printk(KERN_ALERT MODULE_NAME "register_netdev faild.\n");
		netdev_exit();
		return ret;
	}
	return 0;
}
//!!!!!!!!!!!!!
static void netdev_exit(void) {
	printk(KERN_INFO MODULE_NAME "netdev_exit\n");

	if (netdev) {
		unregister_netdev(netdev);
		//pool down
		free_netdev(neteev);
	}

	return;
}

module_init(netdev_init);
module_exit(netdev_exit);
