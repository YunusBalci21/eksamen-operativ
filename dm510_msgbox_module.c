
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>

// Declare the license of the module
MODULE_LICENSE("GPL");
// Declares the author of the module
MODULE_AUTHOR("Your Name");
// Describe what the module does
MODULE_DESCRIPTION("DM510 Message Box System Calls");

//Define a structure for the messages in the message box.
typedef struct msg_t {
    struct msg_t *previous; // Pointer to the previous message
    int length;
    char *message; // Pointer to the message content
} msg_t;

// Pointer to the message in the stack
static msg_t *bottom = NULL;
static msg_t *top = NULL;
// Spinlock to protect access to the message box.
static spinlock_t msgbox_lock;


// Initializes the spinlock
static int __init msgbox_init(void) {
    spin_lock_init(&msgbox_lock);
    printk(KERN_INFO "DM510 Message Box Module Initialized\n");
    return 0;
}

// Add a message into the message box
asmlinkage long sys_dm510_msgbox_put(const char __user *buffer, int length) {
    msg_t *msg;
    unsigned long flags;
  	
    if (length <= 0 || length > 4096) // Check for valid message length
        return -EINVAL;

    if (!access_ok(buffer, length)) // Check if the user-space pointer is valid
        return -EFAULT;

    msg = kmalloc(sizeof(msg_t), GFP_KERNEL); // Allocate memory for the message structure
    if (!msg)
        return -ENOMEM;

    msg->message = kmalloc(length, GFP_KERNEL); // Allocate memory for the message content
    if (!msg->message) {
        kfree(msg);
        return -ENOMEM;
    }

    if (copy_from_user(msg->message, buffer, length)) {  // Copy the message from user space to kernel space
        kfree(msg->message);
        kfree(msg);
        return -EFAULT;
    }
// Set message length and reset previous link
    msg->length = length;
    msg->previous = NULL;

// Lock the message box and save the interrupt flags
    spin_lock_irqsave(&msgbox_lock, flags);

// Adds the message to the message box
    if (bottom == NULL) {
        bottom = msg;
        top = msg;
    } else {
        msg->previous = top;
        top = msg;
    }

// Unlock the message box and restores the interrupt flags
    spin_unlock_irqrestore(&msgbox_lock, flags);

    return 0; // Success
}

// Retrieves a message from the box
asmlinkage long sys_dm510_msgbox_get(char __user *buffer, int length) {
    msg_t *msg;
    unsigned long flags;
    int ret_length;

// Lock the message box and saves the interrupt flags
    spin_lock_irqsave(&msgbox_lock, flags);

// Check if the message box is empty and return an error indicating the box is empty
    if (top == NULL) {
        spin_unlock_irqrestore(&msgbox_lock, flags);
        return -ENOENT; // Empty box
    }

    msg = top;  // Retrieve the top message
    top = msg->previous;  // Update the top pointer
    if (top == NULL) // If the box is empty clear the bottom pointer
        bottom = NULL;

// Unlock the message box and restores the interrupt flags
    spin_unlock_irqrestore(&msgbox_lock, flags);

    ret_length = msg->length > length ? length : msg->length;

// Copy the message to user space
    if (copy_to_user(buffer, msg->message, ret_length)) {
        kfree(msg->message);
        kfree(msg);
        return -EFAULT;  // Return an error if copy_to_user fails
    }

// Free the message content and structure
    kfree(msg->message);
    kfree(msg);

    return ret_length; // Return the length of the message copied
}

//Cleans up the message box on module exit
static void __exit msgbox_exit(void) {
    msg_t *curr_msg = top;
// Free all remaining messages
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
