#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/arp.h>
#include <linux/interrupt.h>
#include <linux/skbuff.h>

#define MODULE_NAME "SNULL"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kawaharasouta <kawahara6514@gmail.com>");
MODULE_DESCRIPTION("snull network device.");


struct net_device *snull_devs[2];

//private date having each device
struct snull_priv {

};


static void snull_setup() {

}

static void snull_exit(void) {

}

static int snull_init(void) {
	int ret = ENOMEM;
	printk(KERN_INFO "snull_init\n");

	snull_devs[0] = allc_netdev(sizeof(struct snull_priv), sn%d, snull_setup);
	snull_devs[1] = allc_netdev(sizeof(struct snull_priv), sn%d, snull_setup);
	if (snull_dev[0] == NULL || snull_dev[1] == NULL)
		goto out;

	ret = ENODEV;
	int result;

	for (int i = 0; i < 2; i++) {
		if ((result = register_netdev(snull_devs[i]))) {
			printk(KERN_INFO MODULE_NAME ": error %i registering device \"%s\"\n", result, snull_devs[i]->name);
		}
		else {
			ret = 0;
		}
	}

out:	if (ret)
		snull_exit();
	return ret;
}


module_init(snull_init);
module_exit(snull_exit);
