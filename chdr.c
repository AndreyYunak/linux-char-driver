#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/cdev.h>


MODULE_AUTHOR("Andrey Yunak");
MODULE_LICENSE("Dual MIT/GPL");

const char * bp_dev_name = "buffer_pipe";
dev_t major;
dev_t minor = 0;

struct bp_dev{
        struct cdev cdev;                  /* Char device structure */
};

struct bp_dev *bp_device;

static void bp_device_setup(struct bp_dev *dev)
{
};

static int __init bp_driver_init(void)
{
	int ret = 0;
	
	printk(KERN_INFO "%s: Driver init\n", bp_dev_name);
	ret = alloc_chrdev_region( &major, minor, 1, bp_dev_name);
	if(ret < 0)
	{
		printk(KERN_WARNING "%s: was NOT initialised. Error code: %d\n", bp_dev_name, ret);
		return ret;
	}
	major = MAJOR(major);
	
	bp_device = kmalloc(sizeof(struct bp_dev), GFP_KERNEL);
	if (bp_device == NULL) {
		unregister_chrdev_region(major, 1);
		printk(KERN_WARNING "%s: was NOT initialised. NOMEM\n", bp_dev_name);
		return -1;
	}
	memset(bp_device, 0,sizeof(struct bp_dev));
	bp_device_setup(bp_device);
	printk(KERN_INFO "%s: was initialised. major: %d, minor: %d\n", bp_dev_name, major, minor);
	return 0;
}

static void __exit bp_driver_exit(void)
{
	if (!bp_device)
		return; /* nothing else to release */

	cdev_del(&bp_device->cdev);
	//kfree(bp_device->buffer);
	kfree(bp_device);
	unregister_chrdev_region(major, 1);
	bp_device = NULL; 
	printk(KERN_INFO "%s: Driver exit\n", bp_dev_name);
	
}

module_init(bp_driver_init);
module_exit(bp_driver_exit);