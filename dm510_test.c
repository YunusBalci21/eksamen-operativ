#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// System call number for sending a message
#define SYS_DM510_MSGBOX_PUT 455
// System call number for receiving a message
#define SYS_DM510_MSGBOX_GET 456

int main() {
    char *msg = "Hello from user space!"; // Define the message to send
    char buffer[1024]; // Then allocate a buffer for receiving messages
    int msglen;  

    // Send a message
    printf("Sending message: %s\n", msg);
    // Use syscall to invoke the system call for sending a message
    if (syscall(SYS_DM510_MSGBOX_PUT, msg, strlen(msg) + 1) == 0) {
        printf("Message sent successfully\n");
    } else {
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }

    // Try to read the message back
    printf("Attempting to retrieve the message...\n");
    // Use syscall to invoke the system call for receiving a message
    msglen = syscall(SYS_DM510_MSGBOX_GET, buffer, sizeof(buffer));
    if (msglen > 0) {
        printf("Received message: %s\n", buffer);
    } else {
        perror("Error receiving message");
        exit(EXIT_FAILURE);
    }

    return 0;
}
