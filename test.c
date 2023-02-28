#define MAX_CLIENTS 4

// ...

int num_clients = 0;
pthread_t threads[MAX_CLIENTS];

// ...

while (num_clients < MAX_CLIENTS) {
    // accept connection from client
    client_t *client = (client_t *)malloc(sizeof(client_t));
    if (client == NULL) {
        perror("malloc");
        continue;
    }
    memset(client, 0, sizeof(client_t));
    int len = sizeof(client->addr);
    n = recvfrom(sockfd, NULL, 0, MSG_PEEK, (struct sockaddr *)&client->addr, &len);
    if (n < 0) {
        perror("recvfrom");
        free(client);
        continue;
    }
    client->sockfd = sockfd;
    num_clients++;

    // create thread to handle client
    if (pthread_create(&threads[num_clients - 1], NULL, handle_client, (void *)client) != 0) {
        perror("pthread_create");
        free(client);
        num_clients--;
        continue;
    }
}

// wait for client threads to complete
for (int i = 0; i < MAX_CLIENTS; i++) {
    pthread_join(threads[i], NULL);
}

// close server socket
close(sockfd);

return 0;




void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    // communicate with client using client->sockfd

    // close client connection and free resources
    close(client->sockfd);
    free(client);
    return NULL;
}




void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    int n;

    int handle = -1;

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
            if (handle != -1) {
                // already connected
                fprintf(stderr, "Already connected\n");
                continue;
            }
            handle = rand() % 1000;
            sendto(client->sockfd, &handle, sizeof(handle), 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
        } else if (strcmp(token, "exec") == 0) {
            // handle command execution request
            if (handle == -1) {
                // not connected
                fprintf(stderr, "Not connected\n");
                continue;
            }
            int cmd_len = strlen(buffer) - strlen(token) - strlen(" ") - sizeof(handle) - 1;
            char cmd[cmd_len + 1];
            sscanf(buffer, "%*s %d %[^\n]", &handle, cmd);
            char *args[] = {"sh", "-c", cmd, NULL};
            int status;

            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                continue;
            }

            int pid = fork();
            if (pid < 0) {
                perror("fork");
                close(pipefd[0]);
                close(pipefd[1]);
                continue;
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
                while ((n = read(pipefd[0], output + bytes_read, BUFFER_SIZE - bytes_read)) > 0) {
                    bytes_read += n;
                    if (bytes_read == BUFFER_SIZE) {
                        break;
                    }
                }
                close(pipefd[0]);

                if (bytes_read == 0) {
                    fprintf(stderr, "No output from command\n");
                    continue;
                } else if (bytes_read == BUFFER_SIZE) {
                    fprintf(stderr, "Output too large to send\n");
                    continue;
                }

                sendto(client->sockfd, output, bytes_read, 0, (struct sockaddr *)&client->
ddr, sizeof(client->addr));
if (n <= 0) {
perror("sendto");
}
}
} else if (strcmp(token, "disconnect") == 0) {
// handle disconnection request
if (handle == -1) {
// not connected
fprintf(stderr, "Not connected\n");
continue;
}
handle = -1;
printf("Client disconnected\n");
break;
} else {
fprintf(stderr, "Invalid command\n");
}
}

scss
Copy code
close(client->sockfd);
free(client);

return NULL;
}




