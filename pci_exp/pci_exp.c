#include<linux/module.h>
#include<asm/types.h>
#include<linux/kernel.h>
#include<linux/pci.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kawaharasouta <kawahara6514@gmail.com>");
MODULE_DESCRIPTION("my first module. Hello world kernel module.");

char pci_exp_name[] = "pci_exp";


#if 1
//struct pci_device_id pci_device_id;
struct pci_driver pci_exp_driver = {
	.name = pci_exp_name,
	.id_table = 
	.probe = 
	.remove = 
}
#endif


static int 
pci_exp_init(void) {
	printk(KERN_ALERT "loading pci_exp.\n");
	return 0;

	int ret;
	ret = pci_register_driver(&pci_exp_driver);

	return ret;
}
static void 
pci_exp_exit(void) {
	printk(KERN_ALERT "pci_exp bye.\n");

	int ret;
	ret = pci_unregister_driver(&pci_exp_driver);

	return;
}

module_init(pci_exp_init);
module_exit(pci_exp_exit);
