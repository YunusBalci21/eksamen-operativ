#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "dm510"
#define DM510_IOC_MAGIC  'k'
#define DM510_IOCSQUANTUM _IOW(DM510_IOC_MAGIC, 1, int)
#define DM510_IOCTMAXREADERS _IO(DM510_IOC_MAGIC, 2)
#define BUFFER_SIZE 1024

static int dm510_major;
static int device_count = 2;
static DEFINE_MUTEX(device_mutex);
static atomic_t available = ATOMIC_INIT(1); // For write operation availability
static wait_queue_head_t read_queue, write_queue;

struct dm510_device {
    struct cdev cdev;
    char *buffer;
    unsigned int buffer_size;
    unsigned int current_size;
    unsigned int readers;
};

static struct dm510_device *devices;

static int dm510_open(struct inode *inode, struct file *filp) {
    struct dm510_device *dev;

    dev = container_of(inode->i_cdev, struct dm510_device, cdev);
    filp->private_data = dev; // For other methods to access

    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (!atomic_dec_and_test(&available)) {
            atomic_inc(&available);
            return -EBUSY; // Already open for writing
        }
    } else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
        mutex_lock(&device_mutex);
        dev->readers++;
        mutex_unlock(&device_mutex);
    }

    return 0;
}

static int dm510_release(struct inode *inode, struct file *filp) {
    struct dm510_device *dev = filp->private_data;

    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        atomic_inc(&available); // Release the device for writing
    } else if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
        mutex_lock(&device_mutex);
        dev->readers--;
        mutex_unlock(&device_mutex);
    }

    return 0;
}

static ssize_t dm510_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct dm510_device *dev = filp->private_data;
    ssize_t retval = 0;

    if (mutex_lock_interruptible(&device_mutex))
        return -ERESTARTSYS;

    while (dev->current_size == 0) { // Buffer is empty
        mutex_unlock(&device_mutex);

        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        if (wait_event_interruptible(read_queue, dev->current_size != 0))
            return -ERESTARTSYS; // Signal: can't sleep

        if (mutex_lock_interruptible(&device_mutex))
            return -ERESTARTSYS;
    }

    // Adjust count if it's larger than the data available
    if (count > dev->current_size)
        count = dev->current_size;

    if (copy_to_user(buf, dev->buffer, count)) {
        retval = -EFAULT;
        goto out;
    }

    // Update the buffer and current_size
    memmove(dev->buffer, dev->buffer + count, dev->current_size - count);
    dev->current_size -= count;
    retval = count;

out:
    mutex_unlock(&device_mutex);
    wake_up_interruptible(&write_queue); // Wake up any waiting writers

    return retval;
}

static ssize_t dm510_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    struct dm510_device *dev = filp->private_data;
    ssize_t retval = -ENOMEM;

    if (mutex_lock_interruptible(&device_mutex))
        return -ERESTARTSYS;

    while (dev->current_size == dev->buffer_size) { // Buffer is full
        mutex_unlock(&device_mutex);

        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        if (wait_event_interruptible(write_queue, dev->current_size != dev->buffer_size))
            return -ERESTARTSYS; // Signal: can't sleep

        if (mutex_lock_interruptible(&device_mutex))
            return -ERESTARTSYS;
    }

    // Adjust count if it's larger than the space available
    if (count > dev->buffer_size - dev->current_size)
        count = dev->buffer_size - dev->current_size;

    if (copy_from_user(dev->buffer + dev->current_size, buf, count)) {
        retval = -EFAULT;
        goto out;
    }

    dev->current_size += count;
    retval = count;

out:
    mutex_unlock(&device_mutex);
    wake_up_interruptible(&read_queue); // Wake up any waiting readers

    return retval;
}

long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    switch(cmd) {
        case DM510_IOCSQUANTUM:
            // Adjust buffer size logic here
            break;
        case DM510_IOCTMAXREADERS:
            // Adjust max readers logic here
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static struct file_operations dm510_fops = {
    .owner = THIS_MODULE,
    .read = dm510_read,
    .write = dm510_write,
    .open = dm510_open,
    .release = dm510_release,
    .unlocked_ioctl = dm510_ioctl,
};

static int __init dm510_init(void) {
    int result, i;
    dev_t devno = 0;

    result = alloc_chrdev_region(&devno, 0, device_count, DEVICE_NAME);
    dm510_major = MAJOR(devno);

    if (result < 0) {
        printk(KERN_WARNING "dm510: can't get major %d\n", dm510_major);
        return result;
    }

    devices = kmalloc(device_count * sizeof(struct dm510_device), GFP_KERNEL);
    if (!devices) {
        result = -ENOMEM;
        goto fail;
    }

    memset(devices, 0, device_count * sizeof(struct dm510_device));

    for (i = 0; i < device_count; i++) {
        devices[i].buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
        devices[i].buffer_size = BUFFER_SIZE;
        devices[i].current_size = 0;
        init_waitqueue_head(&read_queue);
        init_waitqueue_head(&write_queue);

        cdev_init(&devices[i].cdev, &dm510_fops);
        devices[i].cdev.owner = THIS_MODULE;
        result = cdev_add(&devices[i].cdev, MKDEV(dm510_major, i), 1);

        if (result) {
            printk(KERN_NOTICE "Error %d adding dm510%d", result, i);
            goto fail;
        }
    }

    printk(KERN_INFO "dm510 device initialized with major number %d\n", dm510_major);
    return 0; // Success

fail:
    dm510_cleanup();
    return result;
}

static void __exit dm510_cleanup(void) {
    int i;

    if (devices) {
        for (i = 0; i < device_count; i++) {
            if (devices[i].buffer)
                kfree(devices[i].buffer);

            cdev_del(&devices[i].cdev);
        }
        kfree(devices);
    }

    unregister_chrdev_region(MKDEV(dm510_major, 0), device_count);
    printk(KERN_INFO "dm510 device unregistered\n");
}

MODULE_AUTHOR("Your Name");
MODULE_LICENSE("GPL");

module_init(dm510_init);
module_exit(dm510_cleanup);
