#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#define BUFFER_SIZE 1024

void handle_help() {
    printf("Commands:\n");
    printf("help: show the commands supported in the client shell.\n");
    printf("connect [hostname]: connect to the remote server. For example: connect spirit.eecs.csuohio.edu\n");
    printf("disconnect: disconnect from the remote server.\n");
    printf("[normal Linux shell command]: any Linux shell command supported by the standard Linux.\n");
    printf("quit: quit the client shell.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [hostname]\n", argv[0]);
        return 1;
    }

    char *hostname = argv[1];
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8888);
    if (inet_pton(AF_INET, hostname, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        printf("client shell$ ");
        fflush(stdout);

        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);

        if (strncmp(buffer, "help", 4) == 0) {
            handle_help();
        } else if (strncmp(buffer, "connect ", 8) == 0) {
            char *hostname = strtok(buffer + 8, " \n");
            printf("connecting to %s... ", hostname);
            fflush(stdout);
            if (inet_pton(AF_INET, hostname, &servaddr.sin_addr) <= 0) {
                perror("inet_pton");
                continue;
            }
            if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                perror("connect");
                continue;
            }
            printf("successful\n");
        } else if (strncmp(buffer, "disconnect", 10) == 0) {
            printf("disconnecting... ");
            fflush(stdout);
            if (shutdown(sockfd, SHUT_RDWR) < 0) {
                perror("shutdown");
                continue;
            }
            printf("successful\n");
        } else if (strncmp(buffer, "quit", 4) == 0) {
            break;
        } else {
            if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
                perror("send");
                continue;
            }
            memset(buffer, 0, BUFFER_SIZE);
            int n = recv(sockfd, buffer, BUFFER_SIZE, 0);
            if (n < 0) {
                perror("recv");
                continue;
            } else if (n == 0) {
                printf("Server disconnected\n");
                break;
            } else {
                printf("%s", buffer);
            }
        }
    }

    close(sockfd);
    return 0;
}
