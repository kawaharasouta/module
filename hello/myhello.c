#include<linux/module.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kawaharasouta <kawahara6514@gmail.com>");
MODULE_DESCRIPTION("my first module. Hello world kernel module.");

static int mymodule_init(void) {
	printk(KERN_ALERT "Hello world!!!\n");
	return 0;
}
static void mymodule_exit(void) {
	printk(KERN_ALERT "bye.\n");
	return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);
