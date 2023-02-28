#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

// void handle_help(int sockfd) {
//     // send the help command to the server
//         if (send(sockfd, "help\n", strlen("help\n"), 0) < 0) {
//         perror("send");
//         return;
//         }

//     // receive the server's response
//     char buffer[BUFFER_SIZE];
//     memset(buffer, 0, BUFFER_SIZE);
//     int n = recv(sockfd, buffer, BUFFER_SIZE, 0);
//         if (n < 0) {
//         perror("recv");
//         return;
//         } else if (n == 0) {
//         printf("Server disconnected\n");
//         return;
//         }

//     // display the server's response in a user-friendly manner
//         printf("Commands:\n");
//         printf("%s", buffer);
// }

void handle_help(int sockfd) {
    // send the help command to the server
    if (send(sockfd, "help\n", strlen("help\n"), 0) < 0) {
        perror("send");
        return;
    }

    // receive the server's response
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int n = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (n < 0) {
        perror("recv");
        return;
    } else if (n == 0) {
        printf("Server disconnected\n");
        return;
    }

    // display the server's response in a user-friendly manner
    printf("Commands:\n");
    printf("%s", buffer);
    fflush(stdout); // flush stdout to ensure output is displayed immediately
}



int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [hostname]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *hostname = argv[1];

    // create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // resolve the server hostname
    struct hostent *he = gethostbyname(hostname);
    if (he == NULL) {
        fprintf(stderr, "Unable to resolve host: %s\n", hostname);
        exit(EXIT_FAILURE);
    }

    // set up the server address
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    memcpy(&servaddr.sin_addr, he->h_addr_list[0], he->h_length);

    // connect to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // enter the client shell loop
    char buffer[BUFFER_SIZE];
    while (1) {
        printf("client shell$ ");
        fflush(stdout);

        // read a line from stdin
    memset(buffer, 0, BUFFER_SIZE);
    if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
    break;
    }

    if (strncmp(buffer, "help", 4) == 0) {
    handle_help(sockfd);
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
                close(sockfd);
                sockfd = -1;
    } else if (strncmp(buffer, "quit", 4) == 0) {
    break;
    } else {
    if (sockfd == -1) {
        printf("Error: not connected to a server.\n");
        continue;
    }
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("send");
        continue;
    }
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = 0;
    int expected_bytes = strlen(buffer);
    while (bytes_received < expected_bytes) {
        int n = recv(sockfd, buffer + bytes_received, expected_bytes - bytes_received, MSG_WAITALL);
        if (n < 0) {
            perror("recv");
            break;
        } else if (n == 0) {
            printf("Server disconnected\n");
            break;
        }
            bytes_received += n;
        // }
            printf("%s", buffer);
            }
        }       
    }

    close(sockfd);
    return 0;
}


