#include<linux/module.h>
#include<asm/types.h>
#include<linux/kernel.h>
#include<linux/pci.h>

#include<errno.h>
#include<ioport.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kawaharasouta <kawahara6514@gmail.com>");
MODULE_DESCRIPTION("my first module. Hello world kernel module.");

static char *msg="module [pci.o]";

#if 0
//struct pci_device_id pci_device_id;
struct pci_driver pci_exp_driver = {
	.name = pci_exp_name,
	.id_table = 
	.probe = 
	.remove = 
}
#endif


static void 
print_pci_info(void) {
	struct pci_dev *device = NULL;

	while (1) {
		device = pci_find_device(PCI_ANY_ID, PCI_ANY_ID, device);

		if (device == NULL) 
			break;

#define BASE_ADDRESS_NUM 6
		u16 vendor_id, device_id;
		u16 class;
		u16 sub_vendor_id, sub_device_id;
		u8 irq;

		unsigned long resource_start, resource_end, resource_flag;
		int i;


		pci_read_config_word(device, PCI_VENDOR_ID, &vendor_id);
		pci_read_config_word(device, PCI_DEVICE_ID, &device_id);
		pci_read_config_word(device, PCI_CLASS_DEVICE, &class);
		pci_read_config_word(device, PCI_SUBSYSTEM_VENDOR_ID, &sub_vendor_id);
		pci_read_config_word(device, PCI_SUBSYSTEM_ID, &sub_deivce_id);
		pci_read_config_word(device, PCI_INTERRUPT_LINE, &irq);

		printk(KERN_INFO "%s: vendor id = %i, device_id = %i\n", msg, vendor_id, device_id);
		printk(KERN_INFO "%s: class = %i\n", msg, class);
		printk(KERN_INFO "%s: ***subsystem***\n");
		printk(KERN_INFO "%s: vendor id = %i, device_id = %i\n", msg, sub_vendor_id, sub_device_id);
		printk(KERN_INFO "%s: IRQ = %i\n", msg, irq);


		for (i = 0; i < BASE_ADDRESS_NUM; i++) {
			resource_start = pci_resource_start(device, i);
			resource_end = pci_resource_end(device, i);
			resource_flag = pci_resource_flag(device, i);

			if (resource_start != 0 || resource_end != 0) {
				printk(KERN_INFO "%s: Base address 0x%0x - 0x%0x\n", msg, resource_start, resource_end);

				if (resource_flag & IORESOURCE_IO)
					printk(KERN_INFO "%s: IO port\n", msg);

				if (resource_flag & IORESOURCE_MEM)
					printk(KERN_INFO "%s: Memoey\n", msg);

				if (resource_flag & IORESOURCE_PREFETCH)
					printk(KERN_INFO "%s: Prefetchable\n", msg);

				if (resource_flag & IORESOURCE_READONLY)
					printk(KERN_INFO "%s: Read Only\n", msg);

				printk(KERN_INFO "%s: \n",msg);
			}
		}

		printk(KERN_INFO "%s: \n",msg);
	}
}


static int 
pci_exp_init(void) {
	printk(KERN_ALERT "%s loading.\n", msg);

	print_pci_info();

	return -ENODEV;
}
static void 
pci_exp_exit(void) {
	printk(KERN_ALERT "%s bye.\n", msg);

	return;
}

module_init(pci_exp_init);
module_exit(pci_exp_exit);
