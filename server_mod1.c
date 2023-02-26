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

void handle_new_connection(client_t *client) {
    if (client->handle != -1) {
        // already connected
        fprintf(stderr, "Already connected\n");
        return;
    }
    client->handle = rand() % 1000;
    send(client->sockfd, &client->handle, sizeof(client->handle), 0);
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
    // memcpy(&client->addr.sin_addr, he->h_addr_list[0], he->h_length);
}

void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    printf("New client connected: %s\n", inet_ntoa(client->addr.sin_addr));

    while (1) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        int n = recvfrom(client->sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (n < 0) {
            perror("recvfrom");
            break;
        } else if (n == 0) {
            // client disconnected
            handle_disconnect(client);
            break;
        } else {
            // process the command
            char *token = strtok(buffer, " \n");
            if (token == NULL) {
                continue;
            } else if (strcmp(token, "connect") == 0) {
                char *hostname = strtok(NULL, " \n");
                handle_connect(client, hostname);
            } else if (strcmp(token, "disconnect") == 0) {
                handle_disconnect(client);
            } else if (strcmp(token, "exec") == 0) {
                char *cmd = strtok(NULL, "\n");
                handle_command_execution(client, cmd);
            } else if (strcmp(token, "help") == 0) {
                handle_help();
            } else if (strcmp(token, "quit") == 0) {
                handle_disconnect(client);
                break;
            } else {
                fprintf(stderr, "Unrecognized command: %s\n", token);
            }
        }
    }

    close(client->sockfd);
    free(client);
    return NULL;
}



int main(int argc, char *argv[]) {
srand(time(NULL));
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
if (sockfd < 0) {
perror("socket");
exit(EXIT_FAILURE);
}

struct sockaddr_in servaddr, cliaddr;
memset(&servaddr, 0, sizeof(servaddr));
memset(&cliaddr, 0, sizeof(cliaddr));
servaddr.sin_family = AF_INET;
servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
servaddr.sin_port = htons(PORT);

if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
}

printf("Server listening on port %d\n", PORT);

while (1) {
    // accept a new client
    client_t *client = (client_t *)malloc(sizeof(client_t));
    memset(client, 0, sizeof(client_t));
    client->sockfd = sockfd;
    socklen_t clilen = sizeof(cliaddr);
    int n = recvfrom(sockfd, NULL, 0, MSG_PEEK, (struct sockaddr *)&cliaddr, &clilen);
    if (n < 0) {
        perror("recvfrom");
        continue;
    }

    // create a new thread to handle the client
    client->addr = cliaddr;
    pthread_t tid;
    if (pthread_create(&tid, NULL, &handle_client, (void *)client) != 0) {
{
        perror("pthread_create");
        free(client);
    }
}
    }
return 0;
}

