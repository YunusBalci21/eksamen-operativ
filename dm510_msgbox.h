#ifndef __DM510_MSGBOX_H__
#define __DM510_MSGBOX_H__

asmlinkage long sys_dm510_msgbox_put(const char __user *buffer, int length);
asmlinkage long sys_dm510_msgbox_get(char __user *buffer, int length);

#endif // __DM510_MSGBOX_H__

