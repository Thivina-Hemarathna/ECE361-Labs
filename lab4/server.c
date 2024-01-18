#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Constants
#define MAX_CLIENTS 100
#define MAX_NAME 256
#define MAX_DATA 1024

// Struct to represent a connected client
enum msgType {
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_ACK,
    JN_NAK,
    LEAVE_SESS,
    NEW_SESS,
    NS_ACK,
    MESSAGE,
    QUERY,
    QU_ACK
};

typedef struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
} Packet;

enum MessageType {
    LOGIN_REQUEST,
    LOGOUT_REQUEST,
    JOIN_SESSION_REQUEST,
    SESSION_MESSAGE,
};

// Function declarations
void handle_client(int client_socket);
void send_message_to_session(int session_id, char *source, char *message);
void broadcast_message(char *message, int sender_sockfd);

// Declare a counter for assigning unique session IDs
int session_counter = 1;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <TCP port number to listen on>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address struct
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    // Bind the server socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding server socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) == -1) {
        perror("Error listening for connections");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %s...\n", argv[1]);

    // Accept incoming connections and handle each client in a new thread
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        handle_client(client_socket);
    }

    // Close the server socket (this part will not be reached in the current setup)
    close(server_socket);

    return 0;
}

void handle_client(int client_socket) {
   struct message received_message;
    while (recv(client_socket, &received_message, sizeof(struct message), 0) > 0) {
        switch (received_message.type) {
            case LOGIN_REQUEST:
                printf("Received LOGIN_REQUEST from %s\n", received_message.source);
                break;
            case LOGOUT_REQUEST:
                printf("Received LOGOUT_REQUEST from %s\n", received_message.source);
                break;
            case JOIN_SESSION_REQUEST:
                printf("Received JOIN_SESSION_REQUEST from %s\n", received_message.source);
                break;
            default:
                printf("Received unknown message type from %s\n", received_message.source);
                break;
        }
    }

    close(client_socket);
}

void send_message_to_session(int session_id, char *source, char *message) {
    struct message session_message;
    session_message.type = SESSION_MESSAGE;
    session_message.size = snprintf(session_message.data, MAX_DATA, "%s: %s", source, message);
    strncpy(session_message.source, source, MAX_NAME);
    printf("Sending SESSION_MESSAGE to Session %d: %s\n", session_id, session_message.data);
}

void broadcast_message(char *message, int sender_sockfd) {
    int connected_clients[MAX_CLIENTS];  
    int num_connected_clients = 0;      

    for (int i = 0; i < num_connected_clients; ++i) {
        if (connected_clients[i] == sender_sockfd) {
            continue;
        }
        ssize_t bytes_sent = send(connected_clients[i], message, strlen(message), 0);
        if (bytes_sent == -1) {
            perror("send");
        }
    }
}