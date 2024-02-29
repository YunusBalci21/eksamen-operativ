// Prevents the header file from being included multiple times in the same file
#ifndef __DM510_MSGBOX_H__
// Define a macro to indicate the header file has been included.
#define __DM510_MSGBOX_H__

// Declare a system call for sending a message to message box.
asmlinkage long sys_dm510_msgbox_put(const char __user *buffer, int length);
// Declare a system call for retrieving a message from a message box
asmlinkage long sys_dm510_msgbox_get(char __user *buffer, int length);

// Then we end the conditional inclusion to ensure the content is only included once.
#endif // __DM510_MSGBOX_H__

