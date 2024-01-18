#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 64
#define MAX_NAME 255
#define MAX_DATA 1024
#define MAX_SESSION 4

#define STD_MSG 8
#define LOGIN 1
#define LOGOUT 2
#define JOIN 3
#define LEAVE 4
#define CREATE 5
#define LIST 6
#define QUIT 7

struct client_info{
    char ID[MAX_NAME];
    char password[64];
    bool connection;
    int socket;
    int session;
};

struct client_info client_init(char id[], char password[]){
    struct client_info client_temp;

    memcpy(client_temp.ID, id, strlen(id));
    memcpy(client_temp.password, password, strlen(password));
    client_temp.connection = false;
    client_temp.socket = 0;
    client_temp.session = 0;
    
    return client_temp;
}

int sessions[MAX_SESSION];
int session_user[MAX_SESSION];

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

struct message create_ACK(char ack[], int type){
    struct message msg;
    strcpy((char *)msg.source, ack);
    msg.type = type;
    return msg;
}

struct client_info client[MAX_CLIENTS];

int client_sockets[MAX_CLIENTS];
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    int read_size;
    //char client_message[BUFFER_SIZE];
    struct message msg;

    while ((read_size = recv(sock, (char *)&msg, BUFFER_SIZE, 0)) > 0) {
        // Forward the message to all clients
        if(msg.type != STD_MSG){
            switch (msg.type){
                case LOGIN:
                    for(int i=0; i < MAX_CLIENTS; i++){
                        if(strcmp(client[i].ID, (char*)msg.source) == 0 && !client[i].connection){
                            if(strcmp(client[i].password, (char *)msg.data) == 0){
                                client[i].connection = true;
                                struct message msg;
                                msg = create_ACK("ACK", LOGIN);

                                printf("Client %s connected\n", client[i].ID);
                                send(client[i].socket, (char *)&msg, sizeof(msg), 0);
                                break;
                            }
                        }else if (strcmp(client[i].ID, (char*)msg.source) == 0 && client[i].connection){
                            struct message msg;
                            msg = create_ACK("NACK", LOGIN);

                            printf("Client already logged in\n");
                            send(client[i].socket, (char *)&msg, sizeof(msg), 0);
                            continue;
                        } else{
                            continue;
                        } 
                    }
                break;
                case LOGOUT:
                    for(int i=0; i < MAX_CLIENTS; i++){
                        if(strcmp(client[i].ID, (char*)msg.source) == 0 && client[i].connection){
                            client[i].connection = false;
                            printf("Client %s disconnected\n", client[i].ID);
                            break;
                        }
                    }
                break;
                case JOIN:
                break;
                case LEAVE:
                    for(int i=0; i < MAX_CLIENTS; i++){
                        if(strcmp(client[i].ID, (char*)msg.source) == 0){

                            if(client[i].session == 0){
                                break;
                            }
                            int curr_session = client[i].session;
                            client[i].session = 0;
                            session_user[0]++;
                            printf("Client %s left session %d\n", client[i].ID, curr_session);

                            session_user[curr_session]--;

                            if(session_user[curr_session] == 0){
                                sessions[curr_session] = 0;
                                printf("Session %d terminated\n", curr_session);
                            }

                            struct message msg;
                            msg = create_ACK("ACK", LEAVE);
                            send(client[i].socket, (char *)&msg, sizeof(msg), 0);

                            break;
                        }
                    }

                break;
                case CREATE:
                    if(sessions[atoi((char *)msg.data)] == 0){
                        sessions[atoi((char *)msg.data)] = 1;
                        session_user[atoi((char *)msg.data)]++;
                        printf("Created session %d\n", atoi((char *)msg.data));

                        for(int i=0; i < MAX_CLIENTS; i++){
                            if(strcmp(client[i].ID, (char*)msg.source) == 0){
                                client[i].session = atoi((char *)msg.data);
                                printf("Client %s joined session %d\n", client[i].ID, client[i].session);

                                struct message msg;
                                msg = create_ACK("ACK", CREATE);
                                send(client[i].socket, (char *)&msg, sizeof(msg), 0);
                                break;
                            }
                        }
                    }
                break;
                case LIST:
                    for(int j = 0; j < MAX_SESSION && sessions[j] == 1; j++){
                        printf("Session %d:\n", j);
                        for(int i=0; i < MAX_CLIENTS && strcmp(client[i].ID, "0") != 0 && client[i].connection && client[i].session == j; i++){
                            printf("    Client ID:%s\n", client[i].ID);
                        }
                    }
                    
                break;
            }
        }else{
            int i=0;
            for(i=0; i < MAX_CLIENTS; i++){
                if(strcmp(client[i].ID, (char*)msg.source) == 0){
                    break;
                }
            }

            printf("Received message: %s in session %d\n", msg.data, client[i].session);

            //std_msg, forwward to all other clients
            pthread_mutex_lock(&client_mutex);
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (client[j].socket != 0 && client[j].socket != sock && client[i].session == client[j].session) {
                    send(client[j].socket, (char *)&msg, sizeof(msg), 0);
                }
            }
            pthread_mutex_unlock(&client_mutex);
        }
       memset((char *)&msg, 0, BUFFER_SIZE);  // Clear the buffer
    }

    if (read_size == 0) {
        fflush(stdout);
    } else if (read_size == -1) {
        perror("recv failed");
    }

    // Remove the socket from the array
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == sock) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);

    close(sock);
    free(socket_desc);
    return 0;
}

int main(int argc, char *argv[]) {
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client_soc;
    for(int i=0; i < MAX_SESSION; i++){
        if(i == 0){
            session_user[i] = 1;
            sessions[i] = 1;
        }else{
            session_user[i] = 0;
            sessions[i] = 0;
        }
    }

    for(int i=0; i < MAX_CLIENTS; i++){
        client[i] = client_init("0","0");
    }
    
    client[0] = client_init("q", "1");
    client[1] = client_init("s", "1");

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("Could not create socket");
    }
    puts("Socket created");

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));

    // Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return 1;
    }
    puts("bind done");

    // Listen
    listen(socket_desc, 3);

    // Accept incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    while (1){
        client_sock = accept(socket_desc, (struct sockaddr *)&client_soc, (socklen_t*)&c);
        if(client_sock < 0){
            perror("accept failed");
            continue;
        }

        pthread_t thread_id;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        pthread_mutex_lock(&client_mutex);
        int slot_found = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client[i].socket == 0) {
                client[i].socket = client_sock;
                slot_found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&client_mutex);

        if (!slot_found) {
            fprintf(stderr, "Max clients reached. Rejecting connection.\n");
            close(client_sock);
            free(new_sock);
            continue;
        }
        if (pthread_create(&thread_id, NULL, client_handler, (void*) new_sock) < 0) {
            perror("could not create thread");
            close(client_sock);
            free(new_sock);
            continue;
        }

    }

    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }

    return 0;
}
