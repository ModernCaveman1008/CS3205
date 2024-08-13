#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PAGE "/index.html"
#define NOT_FOUND_PAGE "/404.html"

typedef struct {
    int client_socket;
    const char *root_directory;
} client_info;

void *handle_client(void *arg);
void send_file(int client_socket, const char *file_path);
void send_error(int client_socket);
const char *get_mime_type(const char *file_path);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <root_directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_socket, client_socket, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t tid;

    port = atoi(argv[1]);
    const char *root_directory = argv[2];

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, 10) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        // Accept incoming connections
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("Accept failed");
            continue;
        }

        // Create a client_info struct to pass necessary information to the thread
        client_info *info = (client_info *)malloc(sizeof(client_info));
        if (info == NULL) {
            perror("Memory allocation failed");
            close(client_socket);
            continue;
        }
        info->client_socket = client_socket;
        info->root_directory = root_directory;

        // Create a new thread to handle the client
        if (pthread_create(&tid, NULL, handle_client, (void *)info) != 0) {
            perror("Thread creation failed");
            close(client_socket);
            free(info);
            continue;
        }
    }

    close(server_socket);
    return 0;
}

int is_alphanumeric(char c)
{ 
    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return 1; 
    return 0; 
} 

int word_counter(char * string)
{ 
    int count = 0; int i = 0; int no_word_flag = 1; 
    while(string[i] != '\0'){ 
        if (i > 0 && is_alphanumeric(string[i]) == 1 && is_alphanumeric(string[i-1]) == 0) count++; 
        i++; 
        if (is_alphanumeric(string[i]) == 1) no_word_flag = 0; 
    } 
    if (is_alphanumeric(string[0]) == 0) count--; 
    count++; 
    if (no_word_flag == 1) count = 0; 
    return count;
}

int is_sentence_end(char c){
    if (c == '.' || c == '!' || c == '?' || c == '\n') return 1;
    return 0;
}

int sentence_counter(char * string){
    int count = 0;
    int i=0;
    int empty_sentence_flag = 1;
    while(string[i]!='\0'){
        if (i>0){
            if (is_sentence_end(string[i]) == 1 && is_sentence_end(string[i])==0) count++;
        }
        if (is_sentence_end(string[i]) == 0) empty_sentence_flag = 0;
        i++;
    }
    i--;
    if (is_sentence_end(string[i])==0) count++;
    if (empty_sentence_flag == 1) count = 0;
    return count;
}

void *handle_client(void *arg) {
    client_info *info = (client_info *)arg;
    int client_socket = info->client_socket;
    const char *root_directory = info->root_directory;
    char buffer[BUFFER_SIZE] = {0};
    char method[10], path[255], protocol[10];
    char file_path[255];
    ssize_t bytes_read;

    // Receive the HTTP request
    if ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0)) == -1) {
        perror("Receive failed");
        close(client_socket);
        free(info);
        pthread_exit(NULL);
    }

    // Parse HTTP request
    sscanf(buffer, "%s %s %s", method, path, protocol);

    // Construct full file path
    if (strcmp(path, "/") == 0) {
        sprintf(file_path, "%s%s", root_directory, DEFAULT_PAGE); // Skip the leading '/'
    } else {
        sprintf(file_path, "%s%s", root_directory, path); // Skip the leading '/'
    }

    // Print the file path for debugging purposes
    printf("File path: %s\n", file_path);

    printf("POST request received with data:\n%s\n", buffer);


    // Check if the request method is POST
    if (strcmp(method, "POST") == 0) {
        printf("POST request received\n");

        int len = strlen(buffer);
        int ind = 0;
        int st = 0, end = 0;
        for(int i=0;i<BUFFER_SIZE-4;i++){
            if(buffer[i] == '%' && buffer[i+1] == '*' && buffer[i+2] == '*' && buffer[i+3] == '%'){
                if(ind==0) st = i+4;
                ind++;
                end=i-1;
            }
        }
        char post_msg[BUFFER_SIZE]="";
        for(int i=st;i<=end;i++){
            post_msg[i-st] = buffer[i];
        }
        post_msg[end-st+1] = '\0';
        printf("%s\n",post_msg);
        char response[BUFFER_SIZE];
        sprintf(response, "\nChar Count: %d\nWord Count: %d\n Sentence Count: %d\n", strlen(post_msg), word_counter(post_msg), sentence_counter(post_msg));
        char http_header[100];
        sprintf(http_header, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", strlen(response));
        printf("%s\n",http_header);
        // Send HTTP header
        if (send(client_socket, http_header, strlen(http_header), 0) == -1) {
            perror("Error sending HTTP headers");
            close(client_socket);
        }
        printf("%s\n",response);
        // Send response data
        if (send(client_socket, response, strlen(response), 0) == -1) {
            perror("Error sending response data");
            close(client_socket);
        }
    } else {
        // Check if the requested file exists
        if (access(file_path, F_OK) != -1) {
            // Serve the requested file
            send_file(client_socket, file_path);
        } else {
            // Serve the 404 page
            send_file(client_socket, NOT_FOUND_PAGE);
        }
    }

    close(client_socket);
    free(info);
    pthread_exit(NULL);
}





void send_file(int client_socket, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("File open failed");
        send_error(client_socket);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Send HTTP headers
    dprintf(client_socket, "HTTP/1.1 200 OK\r\n");
    dprintf(client_socket, "Content-Length: %ld\r\n", file_size);
    dprintf(client_socket, "Content-Type: %s\r\n", get_mime_type(file_path));
    dprintf(client_socket, "\r\n");

    // Send file contents
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}

void send_error(int client_socket) {
    const char *error_message = "HTTP/1.1 404 Not Found\r\n\r\n";
    send(client_socket, error_message, strlen(error_message), 0);
}

const char *get_mime_type(const char *file_path) {
    const char *ext = strrchr(file_path, '.');
    if (ext != NULL) {
        if (strcmp(ext, ".html") == 0) return "text/html";
        if (strcmp(ext, ".css") == 0) return "text/css";
        if (strcmp(ext, ".js") == 0) return "text/javascript";
        if (strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".jpg") == 0) return "image/jpeg";
        if (strcmp(ext, ".png") == 0) return "image/png";
    }
    return "application/octet-stream"; // Default MIME type
}