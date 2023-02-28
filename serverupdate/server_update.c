#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

void handle_help(int sockfd, struct sockaddr *clientaddr, socklen_t clientaddrlen) {
    // send the help information to the client
    char *help_info = "Commands:\nhelp: show the commands supported in the client shell.\nconnect [hostname]: connect to the remote server. For example: connect spirit.eecs.csuohio.edu\ndisconnect: disconnect from the remote server.\n[normal Linux shell command]: any Linux shell command supported by the standard Linux.\nquit: quit the client shell.\n";
    if (sendto(sockfd, help_info, strlen(help_info), 0, clientaddr, clientaddrlen) < 0) {
        perror("sendto");
        return;
    }
}

int main() {
    // create a socket
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
    servaddr.sin_port = htons(SERVER_PORT);

    // bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // enter the server loop
    char buffer[BUFFER_SIZE];
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        memset(&clientaddr, 0, clientaddrlen);

        // receive a message from a client
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        // determine the type of message
        if (strncmp(buffer, "help", 4) == 0) {
            handle_help(sockfd, (struct sockaddr *)&clientaddr, clientaddrlen);
        } else if (strncmp(buffer, "quit", 4) == 0) {
            break;
        } else {
            // execute the command using popen
            FILE *fp = popen(buffer, "r");
            if (fp == NULL) {
                perror("popen");
                continue;
            }

            // send the output of the command to the client
            while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
                if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&clientaddr, clientaddrlen) < 0) {
                    perror("sendto");
                    break;
                }
            }

            // close the command pipe
            pclose(fp);
        }
    }

    close(sockfd);
    return 0;
}
