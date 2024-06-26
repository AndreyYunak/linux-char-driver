#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/init.h>


MODULE_AUTHOR("Andrey Yunak");
MODULE_VERSION("0.1");
MODULE_LICENSE("Dual MIT/GPL");

static int __init driver_init(void)
{
	printk(KERN_INFO "TEST Driver init\n");
	return 0;
}

static void __exit driver_exit(void)
{
	printk(KERN_INFO "TEST Driver exit\n");
}

module_init(driver_init);
module_exit(driver_exit);