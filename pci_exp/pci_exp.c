#include<linux/module.h>
#include<asm/types.h>
#include<linux/kernel.h>
#include<linux/pci.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kawaharasouta <kawahara6514@gmail.com>");
MODULE_DESCRIPTION("network module");

static char *pci_exp_msg = "module [pci_exp]:";
char *pci_exp_name = "pci_exp";



static int 
pci_exp_init_one(struct pci_dev *pdev, const struct pci_device_id *ent) {


}



#if 1
//struct pci_device_id pci_device_id;
struct pci_driver pci_exp_driver = {
	.name = pci_exp_name,
	.id_table =  // The device list this driver support.
	.probe = pci_exp_init_one // It is called at device detection if it is a prescribed driver, or at (possibly) modprobe otherwise.
	.remove =  // It is called at device unload.
#ifdef CONFIG_PM //When the power control is ON.
	.suspend =
	.resume = 
#endif /* CONFIG_PM */
}
#endif



static const struct net_device_ops pci_exp_ops = {
	.ndo_open =
	.ndo_stop = 
	.ndo_start_xmit = 
	.ndo_get_stats =
	.ndo_set_rx_mode = 
//	.ndo_set_mac_address = 
	.ndo_tx_timeout = 
//	.ndo_change_mtu = 
	.ndo_do_ioctl = 
	.ndo_validate_addr = 
//	.ndo_vlan_rx_add_vid =
//	.ndo_vlan_rx_kill_vid =
	.ndo_poll_controller =
	.ndo_fix_features =
	.ndo_set_features =
}



static int 
pci_exp_init(void) {
	printk(KERN_ALERT "%s loading pci_exp.\n", pci_exp_msg);
	return 0;

	int ret;
	ret = pci_register_driver(&pci_exp_driver);

	return ret;
}
static void 
pci_exp_exit(void) {
	printk(KERN_ALERT "%s pci_exp bye.\n", pci_exp_msg);

	int ret;
	ret = pci_unregister_driver(&pci_exp_driver);

	return;
}

module_init(pci_exp_init);
module_exit(pci_exp_exit);
