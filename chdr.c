#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/cdev.h>
#include <linux/errno.h>	/* error codes */
#include <linux/fcntl.h>
#include <linux/wait.h>


MODULE_AUTHOR("Andrey Yunak");
MODULE_LICENSE("Dual MIT/GPL");

const char * bp_dev_name = "buffer_pipe";
uint buffer_size =  64;	/* buffer size */

module_param(buffer_size, uint, S_IRUGO);

dev_t device_num; // = MKDEV(<major>, <minor>)
int major = 0;
int minor = 0;

struct bp_dev{
	struct cdev cdev;		/* Char device structure */
	char *buffer, *end;		/* begin of buf, end of buf */
	int buffersize;			/* used in pointer arithmetic */
	char *rp, *wp;			/* where to read, where to write */
	struct mutex lock;		/* mutual exclusion mutex */
	wait_queue_head_t inq, outq;		/* read and write queues */
	int nreaders, nwriters;	/* number of openings for r/w */
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

	if (filp->f_mode & FMODE_READ)
		dev->nreaders++;
	if (filp->f_mode & FMODE_WRITE)
		dev->nwriters++;
	mutex_unlock(&dev->lock);

	return nonseekable_open(inode, filp);
};

static int bp_device_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "%s: release\n", bp_dev_name);

	struct bp_dev *dev = filp->private_data;

	mutex_lock(&dev->lock);
	if (filp->f_mode & FMODE_READ)
		dev->nreaders--;
	if (filp->f_mode & FMODE_WRITE)
		dev->nwriters--;
	if (dev->nreaders + dev->nwriters == 0) {
		kfree(dev->buffer);
		dev->buffer = NULL; /* the other fields are not checked on open */
		printk(KERN_INFO "%s: release and buffer = NULL\n", bp_dev_name);
	}
	mutex_unlock(&dev->lock);
	return 0;
};

static ssize_t bp_device_read(struct file *filp, char __user *buf, size_t count, 
								loff_t *f_pos)
{
	printk(KERN_INFO "%s: READ\n", bp_dev_name);
	struct bp_dev *dev = filp->private_data;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	while (dev->rp == dev->wp) {	/* nothing to read */
		mutex_unlock(&dev->lock);	/* release the lock */
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		printk(KERN_INFO "\"%s\" reading: going to sleep\n", current->comm);
		if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp)))
			return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
		/* otherwise loop, but first reacquire the lock */
		if (mutex_lock_interruptible(&dev->lock))
			return -ERESTARTSYS;
	}
	/* ok, data is there, return something */
	if (dev->wp > dev->rp)
		count = min(count, (size_t)(dev->wp - dev->rp));
	else /* the write pointer has wrapped, return data up to dev->end */
		count = min(count, (size_t)(dev->end - dev->rp));
	if (copy_to_user(buf, dev->rp, count)) {
		mutex_unlock (&dev->lock);
		return -EFAULT;
	}
	dev->rp += count;
	if (dev->rp == dev->end)
		dev->rp = dev->buffer; /* wrapped */
	mutex_unlock (&dev->lock);

	/* finally, awake any writers and return */
	wake_up_interruptible(&dev->outq);
	printk(KERN_INFO "\"%s\" did read %li bytes\n",current->comm, (long)count);
	return count;
};


/* How much space is free? */
static int spacefree(struct bp_dev *dev)
{
	if (dev->rp == dev->wp)
		return dev->buffersize - 1;
	return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) - 1;
}


static int bp_device_getwritespace(struct bp_dev *dev, struct file *filp)
{
	while (spacefree(dev) == 0) { /* full */
		DEFINE_WAIT(wait);
		
		mutex_unlock(&dev->lock);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		printk(KERN_INFO "\"%s\" writing: going to sleep\n",current->comm);
		prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
		if (spacefree(dev) == 0)
			schedule();
		finish_wait(&dev->outq, &wait);
		if (signal_pending(current))
			return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
		if (mutex_lock_interruptible(&dev->lock))
			return -ERESTARTSYS;
	}
	return 0;
}	


static ssize_t bp_device_write(struct file *filp, const char __user *buf, size_t count,
								loff_t *f_pos)
{
	printk(KERN_INFO "%s: WRITE\n", bp_dev_name);
	struct bp_dev *dev = filp->private_data;
	int result;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	/* Make sure there's space to write */
	result = bp_device_getwritespace(dev, filp);
	if (result)
		return result; /* bp_device_getwritespace called up(&dev->lock) */

	/* ok, space is there, accept something */
	count = min(count, (size_t)spacefree(dev));
	if (dev->wp >= dev->rp)
		count = min(count, (size_t)(dev->end - dev->wp)); /* to end-of-buf */
	else /* the write pointer has wrapped, fill up to rp-1 */
		count = min(count, (size_t)(dev->rp - dev->wp - 1));
	printk(KERN_INFO "Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);
	if (copy_from_user(dev->wp, buf, count)) {
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	dev->wp += count;
	if (dev->wp == dev->end)
		dev->wp = dev->buffer; /* wrapped */
	mutex_unlock(&dev->lock);

	/* finally, awake any reader */
	wake_up_interruptible(&dev->inq);  /* blocked in read() and select() */

	printk(KERN_INFO "\"%s\" did write %li bytes\n",current->comm, (long)count);
	return count;
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
	int err;
    
	cdev_init(&dev->cdev, &bp_dev_fops);
	dev->cdev.owner = THIS_MODULE;
	//dev->cdev.ops = &bp_dev_fops;
	err = cdev_add(&dev->cdev, device_num, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding %s", err, bp_dev_name);
};

static int __init bp_driver_init(void)
{
	int ret = 0;
	
	printk(KERN_INFO "%s: Driver init\n", bp_dev_name);
	ret = alloc_chrdev_region( &device_num, minor, 1, bp_dev_name);
	if(ret < 0)
	{
		printk(KERN_WARNING "%s: was NOT initialised. Error code: %d\n", bp_dev_name, ret);
		return ret;
	}
	major = MAJOR(device_num);
	minor = MINOR(device_num);
	
	bp_device = kmalloc(sizeof(struct bp_dev), GFP_KERNEL);
	if (bp_device == NULL) {
		unregister_chrdev_region(device_num, 1);
		printk(KERN_WARNING "%s: was NOT initialised. NOMEM\n", bp_dev_name);
		return -1;
	}
	memset(bp_device, 0,sizeof(struct bp_dev));
	init_waitqueue_head(&bp_device->inq);
	init_waitqueue_head(&bp_device->outq);
	mutex_init(&bp_device->lock);
	bp_device_setup_cdev(bp_device);
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
	unregister_chrdev_region(device_num, 1);
	bp_device = NULL; 
	printk(KERN_INFO "%s: Driver exit\n", bp_dev_name);
	
}

module_init(bp_driver_init);
module_exit(bp_driver_exit);