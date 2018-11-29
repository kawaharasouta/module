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

#include<uapi/linux/netdevice.h>

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
	u8 data[ETH_DATA_LEN];
};

//private date having each device
struct snull_priv {
	struct net_device_stats stats;
	int status;
	struct snull_packet *ppool;
	struct snull_packet *rx_queue;
	int rx_int_enabled;
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	//spinlock_t lock;
};

static int lockup = 0;
module_param(lockup, int, 0);

int pool_size = 8;
module_param(pool_size, int, 0);

//!! Set up a device's packet pool.
void 
snull_setup_pool(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;
	int i;
	priv->ppool = NULL;
	for (i = 0; i < pool_size; i++) {
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

static void 
snull_teardown_pool(struct net_device *dev) {
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;
	while ((pkt = priv->ppool)) {
		priv->ppool = pkt->next;
		kfree (pkt);
		/* FIXME - in-flight packets ? */
	}	
}

struct snull_packet* 
snull_get_tx_buffer(struct net_device *dev) //detach the head of ppool
{
	struct snull_priv *priv = netdev_priv(dev);
	//unsigned long flags;
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

void 
snull_release_buffer(struct snull_packet *pkt)
{
	//unsigned long flags;
	struct snull_priv *priv = netdev_priv(pkt->dev);
	
	//spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->ppool;
	priv->ppool = pkt;
	//pin_unlock_irqrestore(&priv->lock, flags);
	if (netif_queue_stopped(pkt->dev) && pkt->next == NULL)
		netif_wake_queue(pkt->dev);
}

void 
snull_enqueue_buf(struct net_device *dev, struct snull_packet *pkt) // Put the specified pkt in rx_queue.
{
	//unsigned long flags;
	struct snull_priv *priv = netdev_priv(dev);

	//spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->rx_queue;  /* FIXME - misorders packets */
	priv->rx_queue = pkt;
	//spin_unlock_irqrestore(&priv->lock, flags);
}

struct 
snull_packet *snull_dequeue_buf(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;
	//unsigned long flags;

	//spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->rx_queue;
	if (pkt != NULL)
		priv->rx_queue = pkt->next;
	//spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}

// Enable and disable receive interrupts.
static void 
snull_rx_ints(struct net_device *dev, int enable)
{
	struct snull_priv *priv = netdev_priv(dev);
	priv->rx_int_enabled = enable;
}



// open/close
static int 
snull_open(struct net_device *dev) {
	memcpy(dev->dev_addr, "\0SNUL0", ETH_ALEN);
	if (dev == snull_devs[1])
		dev->dev_addr[ETH_ALEN - 1]++; /* \0SNUL1 */
	netif_start_queue(dev);
	return 0;
}
static int 
snull_release(struct net_device *dev) {
	netif_stop_queue(dev);
	return 0;
}

void 
snull_rx(struct net_device *dev, struct snull_packet *pkt)
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

static void 
snull_regular_interrupt(int irq, void *dev_id, struct pt_regs *regs)
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

/*
 * Transmit a packet (low level interface)
 */
static void 
snull_hw_tx(char *buf, int len, struct net_device *dev)
{
	/*
	 * This function deals with hw details. This interface loops
	 * back the packet to the other snull interface (if any).
	 * In other words, this function implements the snull behaviour,
	 * while all other procedures are rather device-independent
	 */
	struct iphdr *ih;
	struct net_device *dest;
	struct snull_priv *priv;
	u32 *saddr, *daddr;
	struct snull_packet *tx_buffer;
    
	/* I am paranoid. Ain't I? */
	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		printk("snull: Hmm... packet too short (%i octets)\n",
				len);
		return;
	}

	if (0) { /* enable this conditional to look at the data */
		int i;
		//PDEBUG("len is %i\n" KERN_DEBUG "data:",len);
		for (i=14 ; i<len; i++)
			printk(" %02x",buf[i]&0xff);
		printk("\n");
	}
	/*
	 * Ethhdr is 14 bytes, but the kernel arranges for iphdr
	 * to be aligned (i.e., ethhdr is unaligned)
	 */
	ih = (struct iphdr *)(buf+sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;

	((u8 *)saddr)[2] ^= 1; /* change the third octet (class C) */
	((u8 *)daddr)[2] ^= 1;

	ih->check = 0;         /* and rebuild the checksum (ip needs it) */
	ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);

//	if (dev == snull_devs[0])
//		PDEBUGG("%08x:%05i --> %08x:%05i\n",
//				ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source),
//				ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest));
//	else
//		PDEBUGG("%08x:%05i <-- %08x:%05i\n",
//				ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest),
//				ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source));

	/*
	 * Ok, now the packet is ready for transmission: first simulate a
	 * receive interrupt on the twin device, then  a
	 * transmission-done on the transmitting device
	 */
	dest = snull_devs[dev == snull_devs[0] ? 1 : 0];
	priv = netdev_priv(dest);
	tx_buffer = snull_get_tx_buffer(dev);
	tx_buffer->datalen = len;
	memcpy(tx_buffer->data, buf, len);
	snull_enqueue_buf(dest, tx_buffer);
	if (priv->rx_int_enabled) {
		priv->status |= SNULL_RX_INTR;
		snull_regular_interrupt(0, dest, NULL);
	}

	priv = netdev_priv(dev);
	priv->tx_packetlen = len;
	priv->tx_packetdata = buf;
	priv->status |= SNULL_TX_INTR;
	if (lockup && ((priv->stats.tx_packets + 1) % lockup) == 0) {
        	/* Simulate a dropped transmit interrupt */
		netif_stop_queue(dev);
		//PDEBUG("Simulate lockup at %ld, txp %ld\n", jiffies,
		//		(unsigned long) priv->stats.tx_packets);
	}
	else
		snull_regular_interrupt(0, dev, NULL);
}

/*
 * Transmit a packet (called by the kernel)
 */
int 
snull_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	char *data, shortpkt[ETH_ZLEN];
	struct snull_priv *priv = netdev_priv(dev);
	
	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memcpy(shortpkt, skb->data, skb->len);
		len = ETH_ZLEN;
		data = shortpkt;
	}
	//dev->trans_start = jiffies; /* save the timestamp */

	/* Remember the skb, so we can free it at interrupt time */
	priv->skb = skb;

	/* actual deliver of data is device-specific, and not shown here */
	snull_hw_tx(data, len, dev);

	return 0; /* Our simple device can not fail */
}

struct net_device_stats*
snull_stats(struct net_device *dev) {
	struct snull_priv *priv = netdev_priv(dev);
	return &priv->stats;	
}

int 
snull_header(struct sk_buff *skb, struct net_device *dev, unsigned short type, void *daddr, void *saddr, unsigned int len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest,   daddr ? daddr : dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1]   ^= 0x01;   /* dest is us xor 1 */
	return (dev->hard_header_len);
}

static const struct net_device_ops snull_ops = {
	.ndo_open = snull_open,
	.ndo_stop = snull_release,
	.ndo_start_xmit = snull_tx,
	.ndo_get_stats = snull_stats,
};

static void 
snull_setup(struct net_device *dev) {
	struct snull_priv *priv;
	ether_setup(dev);

//	//! maybe equal net_device_ops
//	dev->open = snull_open;
//	dev->stop = snull_release;
//	//dev->set_config = snull_config;
//	dev->hard_start_xmit = snull_tx;
//	//dev->do_ioctl = snull_ioctl;
//	dev->get_stats = snull_stats;
//	//dev->change_mtu = snull_change_mtu;  
//	//dev->rebuild_header  = snull_rebuild_header;
//	dev->hard_header = snull_header;
//	//dev->tx_timeout = snull_tx_timeout;
//	//dev->watchdog_timeo = timeout;

	// flags, features, hard_header_cache ???
	dev->flags |= IFF_NOARP;
	//dev->features |= NETIF_F_NO_CSUM; // no checksum in tx.
	//dev->hard_header_cache = NULL; // disable caching.

	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct snull_priv));
	//spin_loxk_init(&priv->lock);
	snull_rx_ints(dev, 1);
}

static void 
snull_exit(void) {
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

static int 
snull_init(void) {
	int ret = -ENOMEM;
	int result, i;
	printk(KERN_INFO "snull_init\n");

	snull_devs[0] = alloc_netdev(sizeof(struct snull_priv), "sn%d", NET_NAME_UNKNOWN, snull_setup);
	snull_devs[1] = alloc_netdev(sizeof(struct snull_priv), "sn%d", NET_NAME_UNKNOWN, snull_setup);
	if (snull_devs[0] == NULL || snull_devs[1] == NULL)
		goto out;

	ret = -ENODEV;

	for (i = 0; i < 2; i++) {
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
