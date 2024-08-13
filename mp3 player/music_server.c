#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

// #define PORT 8800
#define BUFFER_SIZE 1024
// #define MAX_CLIENTS 10
const char *music_directory;

void stream_mp3(FILE *file, int client_socket) {
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) == -1) {
            perror("Error sending data to client");
            break;
        }
    }
    fclose(file);
}

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        perror("Error receiving data from client");
        close(client_socket);
        pthread_exit(NULL);
    }

    buffer[bytes_received] = '\0';

    int song_number = atoi(buffer);

    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/Song%d.mp3", music_directory, song_number);

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Error opening file");
        close(client_socket);
        pthread_exit(NULL);
    }
    stream_mp3(file, client_socket);
    close(client_socket);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <Port> <Music_directory> <Max_streams>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    music_directory = argv[2];
    int max_streams = atoi(argv[3]);

    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, max_streams) == -1) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server started listening on port %d\n", port);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        printf("Connection accepted from %s-%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Create a thread to handle the client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)&client_socket) != 0) {
            perror("Error creating thread");
            close(client_socket);
            continue;
        }

        // Limit the number of concurrent streams
        if (max_streams > 0) {
            pthread_detach(tid); // Detach the thread
            if (--max_streams == 0) {
                printf("Maximum streams reached. No more connections will be accepted.\n");
                break;
            }
        }
    }

    close(server_socket);

    return 0;
}


