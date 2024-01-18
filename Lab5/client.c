#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define MAX_NAME 255
#define MAX_DATA 1024

#define STD_MSG 8
#define LOGIN 1
#define LOGOUT 2
#define JOIN 3
#define LEAVE 4
#define CREATE 5
#define LIST 6
#define QUIT 7

int sockfd;
pthread_t recv_thread;
bool connected;
char *id;

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};


void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    struct message msg;
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recv(sockfd, (char *)&msg, BUFFER_SIZE, 0);
        if (len <= 0) {
            printf("Disconnected from server\n");
            break;
        }
        switch(msg.type){
            case STD_MSG:
                if(strcmp((char *)msg.source, id) != 0){
                    printf("Received Message: %s\n", msg.data);
                }
            break;
            case LOGIN:
                if(strcmp(buffer, (char *)msg.data) == 0){
                    connected = true;
                    printf("Login successful\n");
                }else if (strcmp(buffer, (char *)msg.data) == 0){
                    connected = false;
                    printf("Already logged in\n");
                }else{
                    printf("No available account\n");
                }
            break;
            case LOGOUT:
            break;
            case JOIN:
            break;
            case LEAVE:
                if(strcmp(buffer, (char *)msg.data) == 0){
                    connected = true;
                    printf("Left successful\n");
                }
            break;
            case CREATE:
                if(strcmp(buffer, (char *)msg.data) == 0){
                    connected = true;
                    printf("Create successful\n");
                }
            break;
            case LIST:
            break;
        }
    
        memset((char *)&msg, 0, BUFFER_SIZE); 
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    connected  = false;
    char *password = (char*)malloc(BUFFER_SIZE);
    id = (char*)malloc(BUFFER_SIZE);

    while (1) {
        
        fgets(buffer, BUFFER_SIZE, stdin);
        
        //input is control instructions
        if(buffer[0] == '/'){
            char *instruction = (char*)malloc(BUFFER_SIZE);
            int x = 0;
            for(int i=0; i < strlen(buffer); i++){
                if(buffer[i] == ' ' || buffer[i] == '\n'){
                    break;
                }
                instruction[x] = buffer[i];
                x++;
            }
            instruction[x] = '\0';

            if(strcmp(instruction, "/login") == 0){
                if(connected){
                    printf("Already logged in");
                    continue;
                }

                int i;
                int x=0;
                //extract id
                for(i=7; buffer[i] != ' '; i++){
                    id[x] = buffer[i];
                    x++;
                }
                x=0;

                //extract password
                for(i+=1; buffer[i] != ' '; i++){
                    password[x] = buffer[i];
                    x++;
                }
                x=0;

                //extract ip address
                char *ip = (char*)malloc(BUFFER_SIZE);
                for(i+=1; buffer[i] != ' '; i++){
                    ip[x] = buffer[i];
                    x++;
                }
                x=0;

                //extract port number
                int port;
                char *port_temp = (char*)malloc(BUFFER_SIZE);
                for(i+=1; buffer[i] != ' '; i++){
                    port_temp[x] = buffer[i];
                    x++;
                }
                port = atoi(port_temp);

                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("Error creating socket");
                    return -1;
                }

                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(port);
                server_addr.sin_addr.s_addr = inet_addr(ip);

                if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    perror("Connection failed");
                    return -1;
                }

                struct message msg;

                msg.size = MAX_DATA;
                msg.type = LOGIN;
                strcpy((char *)msg.source, id);
                strcpy((char *)msg.data, password);

                if(send(sockfd, (char *)&msg, sizeof(msg), 0) < 0){
                    perror("Error sending login instruction");
                }

                if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
                    perror("Could not create receive thread");
                    return -1;
                }

                free(port_temp);
                free(ip);
            }else if(strcmp(instruction, "/logout") == 0){
                if(!connected){
                    printf("Not logged in\n");
                    continue;
                }

                struct message msg;
                msg.size = MAX_DATA;
                msg.type = LOGOUT;
                strcpy((char *)msg.source, id);
                strcpy((char *)msg.data, password);
                if(send(sockfd, (char *)&msg, sizeof(msg), 0) < 0){
                                    perror("Error sending logout instruction");
                                }
                connected = false;

                shutdown(sockfd, SHUT_WR);
                if(close(sockfd) < 0){
                    perror("Error closing socket");
                }

            }else if(strcmp(instruction, "/joinsession") == 0){
                
            }else if(strcmp(instruction, "/leavesession") == 0){
                struct message msg;
                msg.type = LEAVE;
                strcpy(msg.source, id);

                if(send(sockfd, (char *)&msg, sizeof(msg), 0) < 0){
                    perror("Error sending leave instruction");
                }

            }else if(strcmp(instruction, "/createsession") == 0){
                struct message msg;

                char *session_id = (char*)malloc(BUFFER_SIZE);
                session_id[0] = buffer[15];
                session_id[1] = '\0';

                msg.type = CREATE;
                strcpy(msg.source, id);
                strcpy(msg.data, session_id);

                if(send(sockfd, (char *)&msg, sizeof(msg), 0) < 0){
                    perror("Error sending create instruction");
                }

                free(session_id);

            }else if(strcmp(instruction, "/list") == 0){
                struct message msg;

                msg.size = MAX_DATA;
                msg.type = LIST;
                
                if(send(sockfd, (char *)&msg, sizeof(msg), 0) < 0){
                    perror("Error sending list instruction");
                }

            }else if(strcmp(instruction, "/quit") == 0){
                exit(0);
            }

            free(instruction);
        }else{
            struct message msg;
            strcpy((char *)msg.data, buffer);
            strcpy((char *)msg.source, id);
            msg.type = STD_MSG;
            send(sockfd, (char *)&msg, sizeof(msg), 0);
        }
        
    }

    free(password);
    pthread_join(recv_thread, NULL);
    close(sockfd);
    return 0;
}
