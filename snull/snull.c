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
	struct net_device_stats stats;
	int status;
	struct snull_packet *ppool;
	struct snull_packet *rx_queue;
	int tx_int_enabled;
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	//spinlock_t lock;
};


static void snull_setup(struct net_device *dev) {
	struct snull_priv *priv;
	ether_setup(dev);

	// flags, features, hard_header_cache ???
	
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct snull_priv));
	//spin_loxk_init(&priv->lock);
	snull_rx_ints(dev, 1);
}

static void snull_exit(void) {
	int i;
	printk(KERN_INFO "snull_exit\n");
	for (i = 0; i < 2; i++) {
		if (snull_devs[i]) {
			unregister_netdev(snull_devs[i]);
			snull_teardown_pool(snull_devs[i]);
			free_netdev(snull_devs[i]);
		}
	}
	return;
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
