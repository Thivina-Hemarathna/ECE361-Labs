#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    FILE *file;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server address> <server port number>\n", argv[0]);
        exit(1);
    }

    // Create UDP socket
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    // Configure settings in address struct for the server
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[2]));
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    printf("Enter a message (ftp <file name>): ");
    fgets(buffer, BUFFER_SIZE, stdin);
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') buffer[--len] = '\0'; // Remove newline

    // Check the message format and extract file name
    char cmd[BUFFER_SIZE], filename[BUFFER_SIZE];
    if (sscanf(buffer, "%s %s", cmd, filename) != 2 || strcmp(cmd, "ftp") != 0) {
        fprintf(stderr, "Invalid message format!\n");
        exit(1);
    }

    // Check the existence of the file
    file = fopen(filename, "r");
    if (!file) {
        perror("File opening failed");
        exit(1);
    }
    fclose(file);

    // Send "ftp" message to server
    strcpy(buffer, "ftp");
    if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("sendto failed");
        exit(1);
    }

    // Receive a message from the server
    ssize_t msg_size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (msg_size < 0) {
        perror("recvfrom failed");
        exit(1);
    }
    buffer[msg_size] = '\0';

    // Check server response
    if (strcmp(buffer, "yes") == 0) {
        printf("A file transfer can start.\n");
    } else {
        fprintf(stderr, "File transfer cannot start.\n");
        exit(1);
    }

    close(sockfd);
    return 0;
}
