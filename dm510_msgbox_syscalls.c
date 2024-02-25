#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>

struct dm510_msg_t {
    struct dm510_msg_t *next;
    int length;
    char message[]; // Flexible array member for message content
};

static struct dm510_msg_t *msg_head = NULL;
static DEFINE_SPINLOCK(msgbox_lock);

SYSCALL_DEFINE2(dm510_msgbox_put, const char __user *, user_buffer, int, length) {
    struct dm510_msg_t *new_msg;
    unsigned long flags;

    if (length <= 0 || length > 4096) // Limit message size to 4KB
        return -EINVAL;

    if (!access_ok(user_buffer, length))
        return -EFAULT;

    new_msg = kmalloc(sizeof(struct dm510_msg_t) + length, GFP_KERNEL);
    if (!new_msg)
        return -ENOMEM;

    if (copy_from_user(new_msg->message, user_buffer, length)) {
        kfree(new_msg);
        return -EFAULT;
    }

    new_msg->length = length;

    spin_lock_irqsave(&msgbox_lock, flags);

    new_msg->next = msg_head;
    msg_head = new_msg;

    spin_unlock_irqrestore(&msgbox_lock, flags);

    return 0; // Success
}

SYSCALL_DEFINE2(dm510_msgbox_get, char __user *, buffer, int, bufsize) {
    struct dm510_msg_t *msg;
    unsigned long flags;

    if (bufsize <= 0)
        return -EINVAL;

    spin_lock_irqsave(&msgbox_lock, flags);

    if (!msg_head) {
        spin_unlock_irqrestore(&msgbox_lock, flags);
        return -ENOENT; // No message available
    }

    msg = msg_head;
    msg_head = msg->next;

    spin_unlock_irqrestore(&msgbox_lock, flags);

    if (bufsize < msg->length)
        return -EMSGSIZE; // Provided buffer is too small

    if (copy_to_user(buffer, msg->message, msg->length)) {
        return -EFAULT;
    }

    kfree(msg);

    return msg->length; // Return the length of copied message
}

