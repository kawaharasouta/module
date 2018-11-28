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

#define SNULL_RX_INTR 0x0001
#define SNULL_TX_INTR 0x0002

struct net_device *snull_devs[2];

struct snull_packet {
	struct snull_packet *next;
	struct net_device *dev;
	int datalen;
	u8 data[ETH_DSTS_LEN];
}

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

int pool_size = 8;
module_param(pool_size, int, 0);

//!! Set up a device's packet pool.
void snull_setup_pool(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;

	priv->ppool = NULL;
	for (int i = 0; i < pool_size; i++) {
		pkt = kmalloc (sizeof (struct snull_packet), GFP_KERNEL);
		if (pkt == NULL) {
			printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
			return;
		}
		//make ring buffer
		pkt->dev = dev;
		pkt->next = priv->ppool;
		priv->ppool = pkt;
	}
}

static void snull_teardown_pool(struct net_device *dev) {
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;
	while ((pkt = priv->ppool)) {
		priv->ppool = pkt->next;
		kfree (pkt);
		/* FIXME - in-flight packets ? */
	}	
}

struct snull_packet *snull_get_tx_buffer(struct net_device *dev) //detach the head of ppool
{
	struct snull_priv *priv = netdev_priv(dev);
	unsigned long flags;
	struct snull_packet *pkt;
    
	//spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->ppool;
	priv->ppool = pkt->next;
	if (priv->ppool == NULL) {
		printk (KERN_INFO "Pool empty\n");
		netif_stop_queue(dev);
	}
	//spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}

void snull_enqueue_buf(struct net_device *dev, struct snull_packet *pkt) // Put the specified pkt in rx_queue.
{
	unsigned long flags;
	struct snull_priv *priv = netdev_priv(dev);

	//spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->rx_queue;  /* FIXME - misorders packets */
	priv->rx_queue = pkt;
	//spin_unlock_irqrestore(&priv->lock, flags);
}


// Enable and disable receive interrupts.
static void snull_rx_ints(struct net_device *dev, int enable)
{
	struct snull_priv *priv = netdev_priv(dev);
	priv->rx_int_enabled = enable;
}



// open/close
static int snull_open(struct net_device *dev) {
	memcpy(dev->dev_addr, "\0SNUL0", ETH_ALEN);
	if (dev == snull_devs[1])
		dev->dev_addr[ETH_ALEN - 1]++; /* \0SNUL1 */
	netif_start_queue(dev);
	return 0;
}
static int snull_release(struct net_device *dev) {
	netif_stop_queue(dev);
	return 0;
}

static int snull_tx(struct sk_buff *skb, struct net_device *dev) {
	int len;
	char *data, shortpkt[ETH_ZLEN]; // ETH_ZLEN defined at if_ether meaning minumum packet
	struct snull_priv *priv = netdev_priv(dev);

	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memset(shortpkt, skb->data, skb->len);
		len = ETH_ZLEN;
		data = shortpkt;
	}
	dev->trans_start = jiffies;

	priv->skb = skb;

	snull_hw_tx(data, len, dev);

	return 0;
}

void snull_rx(struct net_device *dev, struct snull_packet *pkt)
{
	struct sk_buff *skb;
	struct snull_priv *priv = netdev_priv(dev);

	/*
	 * The packet has been retrieved from the transmission
	 * medium. Build an skb around it, so upper layers can handle it
	 */
	skb = dev_alloc_skb(pkt->datalen + 2);
	if (!skb) {
		if (printk_ratelimit())
			printk(KERN_NOTICE "snull rx: low on mem - packet dropped\n");
		priv->stats.rx_dropped++;
		goto out;
	}
	skb_reserve(skb, 2); /* align IP on 16B boundary */  
	memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);

	/* Write metadata, and then pass to the receive level */
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += pkt->datalen;
	netif_rx(skb);
  out:
	return;
}

static void snull_regular_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int statusword;
	struct snull_priv *priv;
	struct snull_packet *pkt = NULL;
	/*
	 * As usual, check the "device" pointer to be sure it is
	 * really interrupting.
	 * Then assign "struct device *dev"
	 */
	struct net_device *dev = (struct net_device *)dev_id;
	/* ... and check with hw if it's really ours */

	if (!dev)
		return;

	priv = netdev_priv(dev);
	//spin_lock(&priv->lock);

	/* retrieve statusword: real netdevices use I/O instructions */
	statusword = priv->status;
	priv->status = 0;
	if (statusword & SNULL_RX_INTR) {
		/* send it to snull_rx for handling */
		pkt = priv->rx_queue;
		if (pkt) {
			priv->rx_queue = pkt->next;
			snull_rx(dev, pkt);
		}
	}
	if (statusword & SNULL_TX_INTR) {
		/* a transmission is over: free the skb */
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += priv->tx_packetlen;
		dev_kfree_skb(priv->skb);
	}

	/* Unlock the device and we are done */
	//spin_unlock(&priv->lock);
	if (pkt) snull_release_buffer(pkt); /* Do this outside the lock! */
	return;
}


struct net_device_stats*
snull_stats(struct net_device *dev) {
	struct snull_priv = netdev_priv(dev);
	return &priv->stats;	
}

static void snull_setup(struct net_device *dev) {
	struct snull_priv *priv;
	ether_setup(dev);

	//! maybe equal net_device_ops
	dev->open = snull_open;
	dev->stop = snull_release;
	//dev->set_config = snull_config;
	dev->hard_start_xmit = snull_tx;
	//dev->do_ioctl = snull_ioctl;
	//dev->get_stats = snull_stats;
	//dev->change_mtu = snull_change_mtu;  
	//dev->rebuild_header  = snull_rebuild_header;
	//dev->hard_header = snull_header;
	//dev->tx_timeout = snull_tx_timeout;
	//dev->watchdog_timeo = timeout;

	// flags, features, hard_header_cache ???
	dev->flags |= IFF_NOARP;
	dev->features |= NETIF_F_NOCSUM; // no checksum in tx.
	dev->hard_header_cache = NULL; // disable caching.

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
	int ret = -ENOMEM;
	printk(KERN_INFO "snull_init\n");

	snull_devs[0] = alloc_netdev(sizeof(struct snull_priv), sn%d, snull_setup);
	snull_devs[1] = alloc_netdev(sizeof(struct snull_priv), sn%d, snull_setup);
	if (snull_dev[0] == NULL || snull_dev[1] == NULL)
		goto out;

	ret = -ENODEV;
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
