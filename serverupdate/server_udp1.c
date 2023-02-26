#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>

#define PORT 8888
#define MAX_CLIENTS 4
#define BUFFER_SIZE 1024

typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    int handle;
} client_t;

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

void handle_help() {
    printf("Commands:\n");
    printf("help: show the commands supported in the client shell.\n");
    printf("connect [hostname]: connect to the remote server. For example: connect spirit.eecs.csuohio.edu\n");
    printf("disconnect: disconnect from the remote server.\n");
    printf("[normal Linux shell command]: any Linux shell command supported by the standard Linux.\n");
    printf("quit: quit the client shell.\n");
}

void handle_connect(client_t *client, char *hostname) {
    if (client->handle != -1) {
        fprintf(stderr, "Already connected\n");
        return;
    }
    struct hostent *he = gethostbyname(hostname);
    if (he == NULL) {
        fprintf(stderr, "Unable to resolve host: %s\n", hostname);
        return;
    }
    memset(&client->addr, 0, sizeof(client->addr));
    client->addr.sin_family = AF_INET;
    client->addr.sin_port = htons(PORT);
    memcpy(&client->addr.sin_addr, he->h_addr_list[0], he->h_length);
}
void handle_disconnect(client_t *client) {
    // handle the disconnection of a client
    printf("Client disconnected: %s\n", inet_ntoa(client->addr.sin_addr));
    client->handle = -1;
}

void *handle_client(void *arg) {
    client_t *client = (client_t *) arg;
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);

    if (getpeername(client->sockfd, (struct sockaddr *) &clientaddr, &clientaddrlen) == -1) {
        perror("getpeername");
        return NULL;
    }

    char clientname[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &clientaddr.sin_addr, clientname, INET_ADDRSTRLEN) == NULL) {
        perror("inet_ntop");
        return NULL;
    }

    printf("Accepted connection from %s:%d\n", clientname, ntohs(clientaddr.sin_port));
    client->handle = 1;

    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(client->sockfd, buffer, BUFFER_SIZE, 0);
        if (n < 0) {
            perror("recv");
            break;
        } else if (n == 0) {
            printf("Client %s:%d disconnected\n", clientname, ntohs(clientaddr.sin_port));
            break;
        }

        if (strncmp(buffer, "quit", 4) == 0) {
            printf("Client %s:%d quit\n", clientname, ntohs(clientaddr.sin_port));
            break;
        } else {
            handle_command_execution(client, buffer);
        }
    }

    close(client->sockfd);
    client->handle = 0;
    return NULL;
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        return 1;
    }

    printf("Server started. Listening on port %d...\n", PORT);

    pthread_t threads[MAX_CLIENTS];
    client_t clients[MAX_CLIENTS];
    int num_clients = 0;

    while (1) {
        int clientfd = accept(sockfd, NULL, NULL);
        if (clientfd < 0) {
            perror("accept");
            continue;
        }

        if (num_clients == MAX_CLIENTS) {
            printf("Max number of clients reached\n");
            close(clientfd);
            continue;
        }

        client_t *client = &clients[num_clients];
        client->sockfd = clientfd;
        client->handle = -1;

        pthread_create(&threads[num_clients], NULL, handle_client, (void *) client);

        num_clients++;
    }

    for (int i = 0; i < num_clients; i++) {
        pthread_join(threads[i], NULL);
    }

    close(sockfd);
    return 0;
}
