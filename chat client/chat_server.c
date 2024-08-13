#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

// #define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Structure to hold client information
typedef struct {
    int socket;
    char username[50];
} ClientInfo;

// Global variables
ClientInfo *clients;
int client_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to broadcast message to all clients
void broadcast(char *message,char *user) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        if(strcmp(clients[i].username,user) != 0) send(clients[i].socket, message, strlen(message), 0);
    }
    pthread_mutex_unlock(&mutex);
}

// Function to handle client connections
void *handle_client(void *arg) {
    ClientInfo *client = (ClientInfo *)arg;
    char buffer[BUFFER_SIZE];

    // Request username from client
    int ok = 0;
    while(!ok){
        ok=1;
        send(client->socket, "Enter your username: ", strlen("Enter your username: "), 0);
        recv(client->socket, client->username, sizeof(client->username), 0);
        client->username[strcspn(client->username, "\n")] = 0; // Remove newline character
        for (int i = 0; i < client_count; i++) {
            if(strcmp(clients[i].username,client->username)==0){
                ok=0;
            }
        }
        if(!ok){
            send(client->socket, "Username already exists: \n", strlen("Username already exists: \n"), 0);
        }
    }
      

    // Add client to the list
    pthread_mutex_lock(&mutex);
    clients[client_count++] = *client;
    pthread_mutex_unlock(&mutex);

    // Send welcome message and list of connected users
    char welcome_message[BUFFER_SIZE];
    sprintf(welcome_message, "Welcome, %s!\n", client->username);
    send(client->socket, welcome_message, strlen(welcome_message), 0);
    printf("[Connected] %s joined the chatroom\n",client->username);

    char user_list[BUFFER_SIZE] = "Users currently in the chatroom:\n";
    for (int i = 0; i < client_count; i++) {
        strcat(user_list, "- ");
        strcat(user_list, clients[i].username);
        strcat(user_list, "\n");
    }
    send(client->socket, user_list, strlen(user_list), 0);
    char msg[BUFFER_SIZE] = "";
    strcat(strcat(strcat(msg,"Client ") , client->username) , " joined the chatroom");
    broadcast(msg,client->username);

    // Listen for messages from the client
    while (1) {
        int bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            printf("[DisConnected] %s left the chatroom due to Timeout\n",client->username);
            for (int i = 0; i < client_count; i++) {
                if (clients[i].socket == client->socket) {
                    char msg[BUFFER_SIZE] = "";
                    strcat(strcat(strcat(msg,"Client ") , client->username) , " joined the chatroom");
                    broadcast(msg,client->username);
                    close(client->socket);
                    for (int j = i; j < client_count - 1; j++) {
                        clients[j] = clients[j + 1];
                    }
                    client_count--;
                    break;
                }
            }
            break;
        }
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            if (strncmp(buffer, "\\list",5) == 0) {
                char list_message[BUFFER_SIZE] = "Users currently in the chatroom:\n";
                for (int i = 0; i < client_count; i++) {
                    strcat(list_message, "- ");
                    strcat(list_message, clients[i].username);
                    strcat(list_message, "\n");
                }
                send(client->socket, list_message, strlen(list_message), 0);
            } else if (strncmp(buffer, "\\bye",4) == 0) {
                // pthread_mutex_lock(&mutex);
                printf("[DisConnected] %s left the chatroom\n",client->username);
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].socket == client->socket) {
                        char msg[BUFFER_SIZE] = "";
                        strcat(strcat(strcat(msg,"Client ") , client->username) , " joined the chatroom");
                        broadcast(msg,client->username);
                        close(client->socket);
                        for (int j = i; j < client_count - 1; j++) {
                            clients[j] = clients[j + 1];
                        }
                        client_count--;
                        break;
                    }
                }
                // pthread_mutex_unlock(&mutex);
                break;
            } else {
                char message[BUFFER_SIZE];
                sprintf(message, "%s: %s", client->username, buffer);
                broadcast(message,client->username);
                printf("[%s] %s",client->username,buffer);
            }
        } else if (bytes_received == 0) {
            // Client disconnected
            printf("[DisConnected] %s got disconneted from the chatroom\n",client->username);
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < client_count; i++) {
                if (clients[i].socket == client->socket) {
                    char msg[BUFFER_SIZE] = "";
                    strcat(strcat(strcat(msg,"Client ") , client->username) , " joined the chatroom");
                    broadcast(msg,client->username);
                    close(client->socket);
                    for (int j = i; j < client_count - 1; j++) {
                        clients[j] = clients[j + 1];
                    }
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            break;
        }
    }
    free(client);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int max_clients = atoi(argv[2]);
    clients = (ClientInfo*)malloc(sizeof(ClientInfo)*max_clients);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, max_clients) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Waiting for connections...\n");

    // Accept connections and create threads
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        ClientInfo *client = (ClientInfo *)malloc(sizeof(ClientInfo));
        client->socket = client_socket;

        struct timeval tv;
        tv.tv_sec = atoi(argv[3]);
        tv.tv_usec = 0;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)client) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }
    

    close(server_socket);
    return 0;
}