#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/cdev.h>
#include <linux/errno.h>	/* error codes */


MODULE_AUTHOR("Andrey Yunak");
MODULE_LICENSE("Dual MIT/GPL");

const char * bp_dev_name = "buffer_pipe";
int buffer_size =  64;	/* buffer size */
dev_t major;
dev_t minor = 0;

struct bp_dev{
	struct cdev cdev;		/* Char device structure */
	char *buffer, *end;		/* begin of buf, end of buf */
	int buffersize;			/* used in pointer arithmetic */
	char *rp, *wp;			/* where to read, where to write */
	struct mutex lock;		/* mutual exclusion mutex */
};
struct bp_dev *bp_device;


static int bp_device_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "%s: open\n", bp_dev_name);
	struct bp_dev *dev;

	dev = container_of(inode->i_cdev, struct bp_dev, cdev);
	filp->private_data = dev;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;
	if (!dev->buffer) {
		/* allocate the buffer */
		dev->buffer = kmalloc(buffer_size, GFP_KERNEL);
		if (!dev->buffer) {
			mutex_unlock(&dev->lock);
			return -ENOMEM;
		}
	}
	dev->buffersize = buffer_size;
	dev->end = dev->buffer + dev->buffersize;
	dev->rp = dev->wp = dev->buffer;	/* rd and wr from the beginning */

	mutex_unlock(&dev->lock);

	return nonseekable_open(inode, filp);
};

static int bp_device_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "%s: release\n", bp_dev_name);

	struct bp_dev *dev = filp->private_data;

	mutex_lock(&dev->lock);
	kfree(dev->buffer);
	dev->buffer = NULL; /* the other fields are not checked on open */
	mutex_unlock(&dev->lock);
	return 0;
};

static ssize_t bp_device_read(struct file *filp, char __user *buf, size_t count, 
								loff_t *f_pos)
{
	printk(KERN_INFO "%s: READ\n", bp_dev_name);
	return 0;
};

static ssize_t bp_device_write(struct file *filp, const char __user *buf, size_t count,
								loff_t *f_pos)
{
	printk(KERN_INFO "%s: WRITE\n", bp_dev_name);
	return 0;
};


struct file_operations bp_dev_fops = {
	.owner =	THIS_MODULE,
	.llseek =	no_llseek,		//no_llseek (<linux/fs.h>)
	.read =		bp_device_read,
	.write =	bp_device_write,
	.open =		bp_device_open,
	.release =	bp_device_release
};

static void bp_device_setup_cdev(struct bp_dev *dev)
{
	printk(KERN_INFO "%s: setup_cdev\n", bp_dev_name);
	int err,  devno = MKDEV(major, minor);
    
	cdev_init(&dev->cdev, &bp_dev_fops);
	dev->cdev.owner = THIS_MODULE;
	//dev->cdev.ops = &bp_dev_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding %s", err, bp_dev_name);
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
	//major = MAJOR(major);
	
	bp_device = kmalloc(sizeof(struct bp_dev), GFP_KERNEL);
	if (bp_device == NULL) {
		unregister_chrdev_region(major, 1);
		printk(KERN_WARNING "%s: was NOT initialised. NOMEM\n", bp_dev_name);
		return -1;
	}
	memset(bp_device, 0,sizeof(struct bp_dev));
	bp_device_setup_cdev(bp_device);
	printk(KERN_INFO "%s: was initialised. major: %d, minor: %d\n", bp_dev_name, MAJOR(major), minor);
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