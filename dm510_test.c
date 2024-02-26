#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Replace these with the actual syscall numbers
#define SYS_DM510_MSGBOX_PUT 455
#define SYS_DM510_MSGBOX_GET 456

int main() {
    char *msg = "Hello from user space!";
    char buffer[1024];
    int msglen;

    // Send a message
    printf("Sending message: %s\n", msg);
    if (syscall(SYS_DM510_MSGBOX_PUT, msg, strlen(msg) + 1) == 0) {
        printf("Message sent successfully\n");
    } else {
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }

    // Try to read the message back
    printf("Attempting to retrieve the message...\n");
    msglen = syscall(SYS_DM510_MSGBOX_GET, buffer, sizeof(buffer));
    if (msglen > 0) {
        printf("Received message: %s\n", buffer);
    } else {
        perror("Error receiving message");
        exit(EXIT_FAILURE);
    }

    return 0;
}
