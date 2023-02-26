#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8888
#define MAX_CLIENTS 4
#define BUFFER_SIZE 1024

typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    int handle;
} client_t;

void handle_new_connection(client_t *client) {
    if (client->handle != -1) {
        // already connected
        fprintf(stderr, "Already connected\n");
        return;
    }
    client->handle = rand() % 1000;
    sendto(client->sockfd, &client->handle, sizeof(client->handle), 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
}

void handle_command_execution(client_t *client, char *cmd) {
    if (client->handle == -1) {
        // not connected
        fprintf(stderr, "Not connected\n");
        return;
    }

    char *args[] = {"sh", "-c", cmd, NULL};
    int status;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    } else if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        close(pipefd[1]);
        char output[BUFFER_SIZE];
        int bytes_read = 0;
        int n;
        while ((n = read(pipefd[0], output + bytes_read, BUFFER_SIZE - bytes_read)) > 0) {
            bytes_read += n;
            if (bytes_read == BUFFER_SIZE) {
                break;
            }
        }
        close(pipefd[0]);

        if (bytes_read == 0) {
            fprintf(stderr, "No output from command\n");
            return;
        } else if (bytes_read == BUFFER_SIZE) {
            fprintf(stderr, "Output too large to send\n");
            return;
        }

        sendto(client->sockfd, output, bytes_read, 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
        if (n <= 0) {
            perror("sendto");
        }
    }
}

void handle_disconnect(client_t *client) {
    if (client->handle == -1) {
        // not connected
        fprintf(stderr, "Not connected\n");
        return;
    }
    client->handle = -1;
    printf("Client disconnected\n");
}

void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        // receive command from client
        memset(buffer, 0, BUFFER_SIZE);
        n = recvfrom(client->sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (n <= 0) {
            perror("recvfrom");
            break;
        }

        // parse command
        char *token = strtok(buffer, " ");
        if (strcmp(token, "connect") == 0) {
            handle_new_connection(client);
        } else if (strcmp(token, "exec") == 0) {
            int cmd_len = strlen(buffer) - strlen(token) - 1;
            char cmd[cmd_len];
            memcpy(cmd, buffer + strlen(token) + 1, cmd_len);
            handle_command_execution(client, cmd);
        } else if (strcmp(token, "disconnect") == 0) {
            handle_disconnect(client);
            break;
        } else {
            fprintf(stderr, "Invalid command\n");
        }
    }

    close(client->sockfd);
    free(client);
    return NULL;
}

int main() {
    char buffer[BUFFER_SIZE];
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    int addrlen = sizeof(server_addr);
    client_t *clients[MAX_CLIENTS];
    int i;

    // create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // set server address and bind socket to address
    memset(&server_addr, 0, addrlen);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, addrlen) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);

    // initialize clients
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = NULL;
    }

    // listen for incoming connections
    while (1) {
        // accept incoming connection
        memset(&client_addr, 0, addrlen);
        memset(buffer, 0, BUFFER_SIZE);
        int n = recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addrlen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        // check if there is space for new client
        int client_index = -1;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == NULL) {
                client_index = i;
                break;
            }
        }

        if (client_index == -1) {
            fprintf(stderr, "Too many clients\n");
            continue;
        }

        // create new client and handle connection in separate thread
        client_t *new_client = (client_t *)malloc(sizeof(client_t));
        if (new_client == NULL) {
            perror("malloc");
            continue;
        }

        new_client->sockfd = server_fd;
        new_client->addr = client_addr;
        new_client->handle = -1;

        clients[client_index] = new_client;
        pthread_t thread_id;
        pthread_create(&thread_id, NULL,
        &handle_client, (void *)new_client);
        pthread_detach(thread_id);
        }   
    return 0;
}