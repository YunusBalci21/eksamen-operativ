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
    char *valid_msg = "Hello from user space!"; // Define a valid message to send
    char buffer[1024]; // Allocate a buffer for receiving messages
    int msglen;

    // Send a valid message
    printf("Sending valid message: %s\n", valid_msg);
    if (syscall(SYS_DM510_MSGBOX_PUT, valid_msg, strlen(valid_msg) + 1) == 0) {
        printf("Valid message sent successfully\n");
    } else {
        perror("Error sending valid message");
        exit(EXIT_FAILURE);
    }

    // Try to retrieve the valid message
    printf("Attempting to retrieve the valid message...\n");
    msglen = syscall(SYS_DM510_MSGBOX_GET, buffer, sizeof(buffer));
    if (msglen > 0) {
        printf("Received message: %s\n", buffer);
    } else {
        perror("Error receiving message");
        exit(EXIT_FAILURE);
    }

    // Test with invalid length (negative value)
    printf("Testing with invalid length (negative value)...\n");
    if (syscall(SYS_DM510_MSGBOX_PUT, valid_msg, -1) != 0) {
        printf("Correctly handled invalid length.\n");
    } else {
        perror("Failed to handle invalid length");
        exit(EXIT_FAILURE);
    }

    // Test with invalid memory address (NULL pointer)
    printf("Testing with invalid memory address (NULL pointer)...\n");
    if (syscall(SYS_DM510_MSGBOX_PUT, NULL, strlen(valid_msg)) != 0) {
        printf("Correctly handled invalid memory address.\n");
    } else {
        perror("Failed to handle invalid memory address");
        exit(EXIT_FAILURE);
    }

    // Attempt to retrieve from an empty message box
    printf("Attempting to retrieve from an empty message box...\n");
    msglen = syscall(SYS_DM510_MSGBOX_GET, buffer, sizeof(buffer));
    if (msglen < 0) {
        printf("Correctly handled attempt to retrieve from an empty message box.\n");
    } else {
        perror("Failed to handle empty message box correctly");
        exit(EXIT_FAILURE);
    }

    return 0;
}
