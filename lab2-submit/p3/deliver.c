#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MAX_ATTEMPTS 3

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    FILE *file;

    //if the command line argument does not equal to 3 (./deliver ip port)
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
    file = fopen(filename, "rb");
    if (!file) {
        perror("File opening failed");
        exit(1);
    }
    
    //set file position indication to eof and get size, then reset to beginning
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //calculate total number of fragments needed
    unsigned int total_fragments = (file_size + 999) / 1000;
    struct packet pkt; 
    pkt.total_frag = total_fragments;
    pkt.frag_no = 1;

    ssize_t bytes_read;
    while((bytes_read = fread(pkt.filedata, 1, sizeof(pkt.filedata), file)) > 0){

        pkt.size = bytes_read;

        pkt.filename = malloc(strlen(filename) + 1);
        if (!pkt.filename) {
            perror("Memory allocation failed for filename");
            fclose(file);
            exit(1);
        }
        strcpy(pkt.filename, filename);

        int header_size = sprintf(buffer, "%u:%u:%u:%s:", pkt.total_frag, pkt.frag_no, pkt.size, pkt.filename);
        memcpy(buffer + header_size, pkt.filedata, pkt.size);

        int attempts = 0;
        int ack_received = 0;

        while(attempts < MAX_ATTEMPTS && !ack_received){
            if (sendto(sockfd, buffer, header_size + pkt.size, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
                perror("sendto failed");
                fclose(file);
                free(pkt.filename);
                exit(1);
            }

            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            setsockopt(sockfd,SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            ssize_t ack_size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
            if(ack_size > 0 && strncmp(buffer, "ACK", 3) == 0){
                printf("ACK received for fragment %u\n", pkt.frag_no);
                ack_received = 1;
            }else{
                fprintf(stderr,"Failed to receive ACK for %u, attempt %d\n", pkt.frag_no, attempts+1);
                attempts++;
            }
        }

        if(!ack_received){
            fprintf(stderr, "Max attempts reached.");
            fclose(file);
            free(pkt.filename);
            exit(1);
        }

        free(pkt.filename);
        pkt.frag_no++;
    }
    
    printf("Transfer completed.\n");
    fclose(file);
    close(sockfd);
    return 0;
}
