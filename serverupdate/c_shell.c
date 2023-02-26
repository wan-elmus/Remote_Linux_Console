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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_hostname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_hostname = argv[1];

    // create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // resolve the server hostname
    struct hostent *he = gethostbyname(server_hostname);
    if (he == NULL) {
        fprintf(stderr, "Unable to resolve host: %s\n", server_hostname);
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
        printf("client> ");
        fflush(stdout);

        // read a line from stdin
        memset(buffer, 0, BUFFER_SIZE);
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        // send the command to the server
        int n = send(sockfd, buffer, strlen(buffer), 0);
        if (n < 0) {
            perror("send");
            break;
        }

        // receive the output from the server
        memset(buffer, 0, BUFFER_SIZE);
        n = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (n < 0) {
            perror("recv");
            break;
        } else if (n == 0) {
            printf("Server closed connection\n");
            break;
        }

        // print the output
        printf("%s", buffer);
    }

    // close the socket and exit
    close(sockfd);
    return 0;
}
