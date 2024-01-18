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

#define MAX_NAME 256
#define MAX_DATA 1024

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

// Function prototypes
void *receive_thread(void *arg);
void login(char *username, char *password, char *server_ip, int server_port);
void create_session(char *session_name);
void list_users_and_sessions();
void join_session(char *session_name);
void send_message(char* sockfd, char *message);
void logout();
void leavesession();
void quit();

int main() {
    // Get user input
    char username[MAX_NAME];
    char password[MAX_NAME];
    char server_ip[MAX_NAME];
    int server_port;

    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);
    printf("Enter server IP: ");
    scanf("%s", server_ip);
    printf("Enter server port: ");
    scanf("%d", &server_port);

    login(username, password, server_ip, server_port);

    // Start receive thread
    pthread_t tid;
    pthread_create(&tid, NULL, receive_thread, NULL);

    // Main command loop
    char command[256];
    while (1) {
        printf("Enter command: ");
        scanf("%s", command);

        if (strcmp(command, "/createsession") == 0) {
            char session_name[MAX_NAME];
            scanf("%s", session_name);
            create_session(session_name);
        } else if (strcmp(command, "/list") == 0) {
            list_users_and_sessions();
        } else if (strcmp(command, "/joinsession") == 0) {
            char session_name[MAX_NAME];
            scanf("%s", session_name);
            join_session(session_name);
        } else if (strcmp(command, "/message") == 0) {
            char message[MAX_DATA];
            scanf("%s", message);
            send_message("",message);
        } else if (strcmp(command, "/logout") == 0) {
            logout();
        } else if (strcmp(command, "/leavesession") == 0) {
            leavesession();
        } else if (strcmp(command, "/quit") == 0) {
            quit();
        } else {
            printf("Unknown command\n");
        }
    }

    return 0;
}

void *receive_thread(void *arg) {
    Packet packet;
    bool insession = false;
      if (packet.type == JN_ACK) {
      fprintf(stdout, "Successfully joined session %s.\n", packet.data);
      insession = true;
    } else if (packet.type == JN_NAK) {
      fprintf(stdout, "Join failure. Detail: %s\n", packet.data);
      insession = false;
    } else if (packet.type == NS_ACK) {
      fprintf(stdout, "Successfully created and joined session %s.\n", packet.data);
      insession = true;
    } else if (packet.type == QU_ACK) {
      fprintf(stdout, "User id\t\tSession ids\n%s", packet.data);
    } else if (packet.type == MESSAGE){   
      	fprintf(stdout, "%s: %s\n", packet.source, packet.data);
    } else {
      fprintf(stdout, "Unexpected packet received: type %d, data %s\n",
          packet.type, packet.data);
    }
    return NULL;
}

void login(char *username, char *password, char *server_ip, int server_port) {
    int sockfd;
    char login_request[MAX_DATA];
    snprintf(login_request, sizeof(login_request), "/login %s %s", username, password);
    if(1){
        int sockfd;
        fprintf(stderr, "Error sending login request\n");
        close(sockfd);
        return;
    }
    char response[MAX_DATA];
    ssize_t bytes_received = recv(1, response, sizeof(response) - 1, 0);
    if (bytes_received == -1) {
        fprintf(stderr, "Error receiving response\n");
    } else {
        response[bytes_received] = '\0';
        printf("Server Response: %s\n", response);
    }
    close(sockfd);
}

void create_session(char *session_name) {
    char create_session_request[MAX_DATA];
    int sockfd;
    snprintf(create_session_request, sizeof(create_session_request), "/createsession %s", session_name);
    if (send(sockfd, create_session_request, strlen(create_session_request), 0) == -1) {
        fprintf(stderr, "Error sending create session request\n");
        return;
    }
    char response[MAX_DATA];
    ssize_t bytes_received = recv(sockfd, response, sizeof(response) - 1, 0);
    if (bytes_received == -1) {
        fprintf(stderr, "Error receiving response\n");
    } else {
        response[bytes_received] = '\0';
        printf("Server Response: %s\n", response);
    }
}

void list_users_and_sessions() {
     char list_request[] = "/list";
     int sockfd;
    if (send(sockfd, list_request, strlen(list_request), 0) == -1) {
        fprintf(stderr, "Error sending list request\n");
        return;
    }
    char response[MAX_DATA];
    ssize_t bytes_received = recv(sockfd, response, sizeof(response) - 1, 0);
    if (bytes_received == -1) {
        fprintf(stderr, "Error receiving response\n");
    } else {
        response[bytes_received] = '\0';
        printf("Server Response: %s\n", response);
    }
}

void join_session(char *session_name) {
    bool insession = false;
    if (*session_name == -1) {
		fprintf(stdout, "You have not logged in to any server.\n");
		return;
	} else if (insession) {
    fprintf(stdout, "You have already joined a session.\n");
    return;
}
}

void send_message(int sockfd, char *message) {
    char messages[MAX_DATA];
    snprintf(messages, sizeof(messages), "/message %s", message);
    if (send(sockfd, messages, strlen(messages), 0) == -1) {
        fprintf(stderr, "Error sending message\n");
    }
}

void logout(int sockfd) {
    char logout_request[] = "/logout";
    if (send(sockfd, logout_request, strlen(logout_request), 0) == -1) {
        fprintf(stderr, "Error sending logout request\n");
    }
}

void leavesession(int sockfd) {
    char leavesession_request[] = "/leavesession";
    if (send(sockfd, leavesession_request, strlen(leavesession_request), 0) == -1) {
        fprintf(stderr, "Error sending leavesession request\n");
    }
}

void quit(int sockfd) {
    char quit_request[] = "/quit";
    if (send(sockfd, quit_request, strlen(quit_request), 0) == -1) {
        fprintf(stderr, "Error sending quit request\n");
    }
    close(sockfd);
    exit(0);
}
