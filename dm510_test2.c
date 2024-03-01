#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

// System call number for sending a message
#define SYS_DM510_MSGBOX_PUT 455
// System call number for receiving a message
#define SYS_DM510_MSGBOX_GET 456

void perform_test(const char *test_message) {
    char buffer[1024]; // Allocate a buffer for receiving messages
    int msglen;

    // Send a valid message
    printf("Process %d sending message: %s\n", getpid(), test_message);
    if (syscall(SYS_DM510_MSGBOX_PUT, test_message, strlen(test_message) + 1) == 0) {
        printf("Process %d sent message successfully\n", getpid());
    } else {
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }

    // Try to retrieve a message
    printf("Process %d attempting to retrieve message...\n", getpid());
    msglen = syscall(SYS_DM510_MSGBOX_GET, buffer, sizeof(buffer));
    if (msglen > 0) {
        printf("Process %d received message: %s\n", getpid(), buffer);
    } else {
        perror("Error receiving message");
        exit(EXIT_FAILURE);
    }
}

int main() {
    pid_t pid1, pid2;
    const char *message1 = "Hello from Process 1";
    const char *message2 = "Hello from Process 2";

    pid1 = fork();
    if (pid1 == 0) {
        // Child process 1
        perform_test(message1);
        return 0; // End child process
    } else if (pid1 > 0) {
        // Parent process
        pid2 = fork();
        if (pid2 == 0) {
            // Child process 2
            perform_test(message2);
            return 0; // End child process
        } else if (pid2 > 0) {
            // Parent process waits for both children to complete
            wait(NULL);
            wait(NULL);
        } else {
            perror("Fork failed for Process 2");
            exit(EXIT_FAILURE);
        }
    } else {
        perror("Fork failed for Process 1");
        exit(EXIT_FAILURE);
    }

    return 0;
}