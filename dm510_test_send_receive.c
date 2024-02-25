#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include "dm510_msgbox.h" // Adjust the path as necessary

#define MSG_SIZE 128

int main(void) {
    char send_msg[] = "Hello from DM510!";
    char recv_msg[MSG_SIZE];

    printf("Test 1: Sending and Receiving a Message\n");
    printf("Sending message: %s\n", send_msg);

    // Replace these with your actual syscall numbers
    long send_status = syscall(__NR_dm510_msgbox_put, send_msg, strlen(send_msg) + 1);
    if (send_status == 0) {
        printf("Message sent successfully.\n");
    } else {
        perror("Failed to send message");
        exit(EXIT_FAILURE);
    }

    long recv_status = syscall(__NR_dm510_msgbox_get, recv_msg, MSG_SIZE);
    if (recv_status > 0) {
        printf("Received message: %s\n", recv_msg);
    } else {
        perror("Failed to receive message");
        exit(EXIT_FAILURE);
    }

    return 0;
}
