#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];
    FILE *file = NULL;

    //if number of command line argument was not passed, alert user the correct format
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <UDP listen port>\n", argv[0]);
        exit(1);
    }


    // Create UDP socket
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    // Configure settings in address struct
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1]));
    serverAddr.sin_addr.s_addr = inet_addr("0.0.0.0"); // bind to any local address
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    // Bind socket with address struct. Assign ip address, port and etc
    if (bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    addr_size = sizeof clientAddr;

    printf("Server started, waiting for connections...\n");

    unsigned int expected_frag_no = 1;

    while (1) {
        // Try to receive any incoming UDP datagram
        ssize_t msg_size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &clientAddr, &addr_size);
        if (msg_size < 0) {
            perror("recvfrom failed");
            continue;
        }

        //parse and format the received packet
        struct packet pkt;

        pkt.filename = malloc(256);
        if (!pkt.filename) {
            perror("Memory allocation failed for filename");
            continue;
        }
        
        sscanf(buffer, "%u:%u:%u:%255[^:]:",&pkt.total_frag,&pkt.frag_no,&pkt.size, pkt.filename);
        // memcpy(pkt.filedata, buffer+strlen(buffer)-pkt.size, pkt.size);
        char temp[256];  // A temporary buffer just to calculate the header size
        int header_size = snprintf(temp, sizeof(temp), "%u:%u:%u:%s:", pkt.total_frag, pkt.frag_no, pkt.size, pkt.filename);
        memcpy(pkt.filedata, buffer + header_size, pkt.size);
        
        //if packet is received, open a local file stream
        if(pkt.frag_no == expected_frag_no){
            if(pkt.frag_no == 1){
                if(file){
                    fclose(file);
                }
                file = fopen(pkt.filename, "wb");
                if(!file){
                    perror("File open failed");
                    free(pkt.filename);
                    continue;
                }
            }
            
        }

        //write data to local file system
        // fwrite(pkt.filedata, 1, pkt.size, file);
        size_t bytes_written = fwrite(pkt.filedata, 1, pkt.size, file);
        if (bytes_written != pkt.size) {
            perror("fwrite failed");
            continue;
        }

        printf("Received fragment %u of %u:\n", pkt.frag_no,pkt.total_frag);
        //fwrite(pkt.filedata,pkt.size,1,stdout);
        printf("\n");

        //if all packet is finished, close file stream to stop write
        if(pkt.frag_no == pkt.total_frag){
            fclose(file);
            file = NULL;
            expected_frag_no = 1;
        }

        // sleep(3);

        if(rand() % 3 == 0){
            printf("***%u of %u is dropped\n", pkt.frag_no,pkt.total_frag);
            printf("\n");
            continue;
        }

        char ack_msg[] = "ACK";
        // Send reply to client
        if (sendto(sockfd, ack_msg, strlen(ack_msg), 0, (struct sockaddr *) &clientAddr, addr_size) < 0) {
            perror("ACK send failed");
            continue;
        }
        free(pkt.filename);
    }   

    return 0;
}

