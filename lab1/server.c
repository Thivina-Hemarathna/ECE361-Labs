#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <UDP listen port>\n", argv[0]);
        exit(1);
    }

    // Create UDP socket
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    // Configure settings in address struct
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1]));
    serverAddr.sin_addr.s_addr = inet_addr("192.168.122.1"); // bind to any local address
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    // Bind socket with address struct
    if (bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    addr_size = sizeof clientAddr;

    printf("Server started, waiting for connections...\n");

    while (1) {
        // Try to receive any incoming UDP datagram
        ssize_t msg_size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &clientAddr, &addr_size);
        if (msg_size < 0) {
            perror("recvfrom failed");
            continue;
        }
        buffer[msg_size] = '\0'; // Null terminate the string

        printf("Received message: %s\n", buffer);

        // Check if the received message is "ftp"
        if (strcmp(buffer, "ftp") == 0) {
            strcpy(buffer, "yes");
        } else {
            strcpy(buffer, "no");
        }

        // Send reply to client
        if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &clientAddr, addr_size) < 0) {
            perror("sendto failed");
            continue;
        }
    }

    //close(sockfd);

    return 0;
}
