#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "dm510_dev"
#define DEVICE_COUNT 2
#define BUFFER_SIZE 1024

static int dm510_major = 0;
static struct dm510_dev {
    struct cdev cdev;
    char buffer[BUFFER_SIZE];
    unsigned int current_size; // Current size of data stored in buffer
} devices[DEVICE_COUNT];

static int dm510_open(struct inode *inode, struct file *filp) {
    struct dm510_dev *dev;
    
    dev = container_of(inode->i_cdev, struct dm510_dev, cdev);
    filp->private_data = dev; // For other methods to access the device
    
    return 0;
}

static int dm510_release(struct inode *inode, struct file *filp) {
    return 0;
}

static ssize_t dm510_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct dm510_dev *dev = filp->private_data;
    ssize_t retval = 0;

    if (*f_pos >= dev->current_size) // If read beyond current data size
        goto out;

    if (*f_pos + count > dev->current_size)
        count = dev->current_size - *f_pos;

    if (copy_to_user(buf, dev->buffer + *f_pos, count)) {
        retval = -EFAULT;
        goto out;
    }
    
    *f_pos += count;
    retval = count;

out:
    return retval;
}

static ssize_t dm510_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    struct dm510_dev *dev = filp->private_data;
    ssize_t retval = -ENOMEM; // Assume we'll run out of space

    if (*f_pos >= BUFFER_SIZE)
        goto out;

    if (*f_pos + count > BUFFER_SIZE)
        count = BUFFER_SIZE - *f_pos;

    if (copy_from_user(dev->buffer + *f_pos, buf, count)) {
        retval = -EFAULT;
        goto out;
    }

    *f_pos += count;
    retval = count;

    if (dev->current_size < *f_pos)
        dev->current_size = *f_pos;

out:
    return retval;
}

static struct file_operations dm510_fops = {
    .owner = THIS_MODULE,
    .read = dm510_read,
    .write = dm510_write,
    .open = dm510_open,
    .release = dm510_release,
};

static int __init dm510_init(void) {
    dev_t devno;
    int result, i;

    result = alloc_chrdev_region(&devno, 0, DEVICE_COUNT, DEVICE_NAME);
    dm510_major = MAJOR(devno);

    if (result < 0) {
        printk(KERN_WARNING "dm510: can't get major %d\n", dm510_major);
        return result;
    }

    for (i = 0; i < DEVICE_COUNT; i++) {
        cdev_init(&devices[i].cdev, &dm510_fops);
        devices[i].cdev.owner = THIS_MODULE;
        devices[i].current_size = 0;
        cdev_add(&devices[i].cdev, MKDEV(dm510_major, i), 1);
    }

    printk(KERN_INFO "DM510 device initialized with major number %d\n", dm510_major);
    return 0;
}

static void __exit dm510_cleanup(void) {
    int i;
    for (i = 0; i < DEVICE_COUNT; i++) {
        cdev_del(&devices[i].cdev);
    }
    unregister_chrdev_region(MKDEV(dm510_major, 0), DEVICE_COUNT);
    printk(KERN_INFO "DM510 device unregistered\n");
}

MODULE_AUTHOR("Your Name");
MODULE_LICENSE("GPL");
module_init(dm510_init);
module_exit(dm510_cleanup);
