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

int client_handles[MAX_CLIENTS];

int handle_command_execution(client_t *client, char *cmd) {
    if (client->handle == -1) {
        // not connected
        fprintf(stderr, "Not connected\n");
        return -1;
    }

    char *args[] = {"sh", "-c", cmd, NULL};
    int status;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    } else if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
        return -1;
    } else {
        close(pipefd[1]);
        char output[BUFFER_SIZE];
        int bytes_read = 0;
        int n;
        while ((n = read(pipefd[0], output + bytes_read, BUFFER_SIZE - bytes_read - 1)) > 0) {
            bytes_read += n;
            if (bytes_read == BUFFER_SIZE) {
                break;
            }
            output[bytes_read] = '\0';
        }
        close(pipefd[0]);

        if (bytes_read == 0) {
            fprintf(stderr, "No output from command\n");
            return -1;
        } else if (bytes_read == BUFFER_SIZE) {
            fprintf(stderr, "Output too large to send\n");
            return;
        }

        int result = sendto(client->sockfd, output, bytes_read, 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
        if (result == -1) {
            perror("sendto");
            return -1;
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
    client->handle = -1;
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
if (client->handle == -1) {
fprintf(stderr, "Not connected\n");
return;
}
client->handle = -1;
}

void *handle_client(void *arg) {
    int sockfd = *(int *)arg;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    getpeername(sockfd, (struct sockaddr *)&cliaddr, &len);
    printf("New client connected: %s\n", inet_ntoa(cliaddr.sin_addr));

    // generate a unique client handle
    int handle = rand() % MAX_CLIENTS;

    // initialize the client
    client_t client;
    client.sockfd = sockfd;
    client.addr = cliaddr;
    client.handle = handle;

    // set up cleanup handler
    void cleanup_handler(void *arg); {
        client_t *client = (client_t *)arg;
        close(client->sockfd);
        printf("Client disconnected: %d\n", client->handle);
    }
    pthread_cleanup_push(cleanup_handler, &client);

    while (1) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        int n = recvfrom(client.sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (n < 0) {
            perror("recvfrom");
            break;
        } else if (n == 0) {
            // client disconnected
            handle_disconnect(&client);
            break;
        } else {
            // process the command
            char *token = strtok(buffer, " \n");
            if (token == NULL) {
                continue;
            } else if (strcmp(token, "connect") == 0) {
                char *hostname = strtok(NULL, " \n");
                handle_connect(&client, hostname);
            } else if (strcmp(token, "disconnect") == 0) {
                handle_disconnect(&client);
            } else if (strcmp(token, "exec") == 0) {
                char *cmd = strtok(NULL, "\n");
                int result = handle_command_execution(&client, cmd);
                if (result == -1) {
                    break;
                }
            } else if (strcmp(token, "help") == 0) {
                handle_help();
            } else if (strcmp(token, "quit") == 0) {
                handle_disconnect(&client);
                break;
            } else {
                fprintf(stderr, "Unrecognized command: %s\n", token);
            }
        }
    }

    // pop the cleanup handler
    pthread_cleanup_pop(1);
    return NULL;
}


int main() {
// create the socket
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
if (sockfd < 0) {
perror("socket");
exit(EXIT_FAILURE);
}
// set up the server address
struct sockaddr_in servaddr;
memset(&servaddr, 0, sizeof(servaddr));
servaddr.sin_family = AF_INET;
servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
servaddr.sin_port = htons(PORT);

// bind the socket to the server address
if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
}

// listen for incoming connections
while (1) {
    int connfd;
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
    if (connfd < 0) {
        perror("accept");
        continue;
    }
    // create a new thread to handle the client
    start_client_thread(connfd);
}

close(sockfd);
return 0;
}