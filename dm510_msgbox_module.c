#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("DM510 Message Box System Calls");

typedef struct msg_t {
    struct msg_t *previous;
    int length;
    char *message;
} msg_t;

static msg_t *bottom = NULL;
static msg_t *top = NULL;
static spinlock_t msgbox_lock;

// Initializes the spinlock
static int __init msgbox_init(void) {
    spin_lock_init(&msgbox_lock);
    printk(KERN_INFO "DM510 Message Box Module Initialized\n");
    return 0;
}

// System call to put a message into the box
asmlinkage long sys_dm510_msgbox_put(const char __user *buffer, int length) {
    msg_t *msg;
    unsigned long flags;

    if (length <= 0 || length > 4096) // Arbitrary max message size
        return -EINVAL;

    if (!access_ok(buffer, length))
        return -EFAULT;

    msg = kmalloc(sizeof(msg_t), GFP_KERNEL);
    if (!msg)
        return -ENOMEM;

    msg->message = kmalloc(length, GFP_KERNEL);
    if (!msg->message) {
        kfree(msg);
        return -ENOMEM;
    }

    if (copy_from_user(msg->message, buffer, length)) {
        kfree(msg->message);
        kfree(msg);
        return -EFAULT;
    }

    msg->length = length;
    msg->previous = NULL;

    spin_lock_irqsave(&msgbox_lock, flags);

    if (bottom == NULL) {
        bottom = msg;
        top = msg;
    } else {
        msg->previous = top;
        top = msg;
    }

    spin_unlock_irqrestore(&msgbox_lock, flags);

    return 0; // Success
}

// System call to get a message from the box
asmlinkage long sys_dm510_msgbox_get(char __user *buffer, int length) {
    msg_t *msg;
    unsigned long flags;
    int ret_length;

    spin_lock_irqsave(&msgbox_lock, flags);

    if (top == NULL) {
        spin_unlock_irqrestore(&msgbox_lock, flags);
        return -ENOENT; // Empty box
    }

    msg = top;
    top = msg->previous;
    if (top == NULL)
        bottom = NULL;

    spin_unlock_irqrestore(&msgbox_lock, flags);

    ret_length = msg->length > length ? length : msg->length;

    if (copy_to_user(buffer, msg->message, ret_length)) {
        kfree(msg->message);
        kfree(msg);
        return -EFAULT;
    }

    kfree(msg->message);
    kfree(msg);

    return ret_length; // Return the length of the message copied
}

static void __exit msgbox_exit(void) {
    msg_t *curr_msg = top;
    while (curr_msg != NULL) {
        msg_t *temp = curr_msg;
        curr_msg = curr_msg->previous;
        kfree(temp->message);
        kfree(temp);
    }
    printk(KERN_INFO "DM510 Message Box Module Exited\n");
}

module_init(msgbox_init);
module_exit(msgbox_exit);
