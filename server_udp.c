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
            // handle connection request
            int handle = rand() % 1000;
            client->handle = handle;
            sendto(client->sockfd, &handle, sizeof(handle), 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
        } else if (strcmp(token, "exec") == 0) {
            // handle command execution request
            int handle;
            char cmd[BUFFER_SIZE];
            sscanf(buffer, "%*s %d %[^\n]", &handle, cmd);
            if (handle != client->handle) {
                fprintf(stderr, "Invalid handle\n");
                continue;
            }
            char *args[] = {"sh", "-c", cmd, NULL};
            int status;
            int pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            } else if (pid == 0) {
                dup2(client->sockfd, STDOUT_FILENO);
                dup2(client->sockfd, STDERR_FILENO);
                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                wait(&status);
                if (WIFEXITED(status)) {
                    printf("Command executed\n");
                } else {
                    printf("Command failed\n");
                }
            }
        } else if (strcmp(token, "disconnect") == 0) {
            // handle disconnection request
            int handle;
            sscanf(buffer, "%*s %d", &handle);
            if (handle != client->handle) {
                fprintf(stderr, "Invalid handle\n");
                continue;
            }
            printf("Client disconnected\n");
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
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[MAX_CLIENTS];
    int num_clients = 0;

    while (1) {
        if (num_clients >= MAX_CLIENTS) {
            printf("Maximum number of clients reached\n");
            sleep(1);
            continue;
           }

    // accept connection from client
    client_t *client = (client_t *)malloc(sizeof(client_t));
    if (client == NULL) {
        perror("malloc");
        continue;
    }
    memset(client, 0, sizeof(client_t));
    int len = sizeof(client->addr);
    int n = recvfrom(sockfd, NULL, 0, MSG_PEEK, (struct sockaddr *)&client->addr, &len);
    if (n < 0) {
        perror("recvfrom");
        free(client);
        continue;
    }
    client->sockfd = sockfd;
    num_clients++;

    // create thread to handle client
    pthread_create(&threads[num_clients - 1], NULL, handle_client, (void *)client);
}

close(sockfd);

return 0;
}
