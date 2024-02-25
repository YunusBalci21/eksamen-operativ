#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include "dm510_msgbox.h" // Adjust the path as necessary

#define MSG_SIZE 128
#define WAIT_TIME 5 // Time to wait before sending a message

int main(void) {
    char send_msg[] = "DM510 message exchange.";
    char recv_msg[MSG_SIZE];

    printf("Test 2: Waiting for a Message\n");
    printf("Waiting for any incoming message...\n");

    sleep(WAIT_TIME); // Simulate waiting for a message

    // Attempt to receive a message first
    long recv_status = syscall(__NR_dm510_msgbox_get, recv_msg, MSG_SIZE);
    if (recv_status > 0) {
        printf("Received message: %s\n", recv_msg);
    } else {
        printf("No message received. Continuing to send a new message.\n");
    }

    // Now send a new message
    printf("Sending new message: %s\n", send_msg);
    long send_status = syscall(__NR_dm510_msgbox_put, send_msg, strlen(send_msg) + 1);
    if (send_status == 0) {
        printf("Message sent successfully.\n");
    } else {
        perror("Failed to send message");
        exit(EXIT_FAILURE);
    }

    return 0;
}
