#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    while (1) {
        char input[BUFFER_SIZE];
        printf(">> ");
        fgets(input, BUFFER_SIZE, stdin);
        input[strcspn(input, "\n")] = 0;

        // send the input to the server
        int n = sendto(sockfd, input, strlen(input), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (n < 0) {
            perror("sendto");
            break;
        }

        // receive the response from the server
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (n < 0) {
            perror("recvfrom");
            break;
        } else if (n == 0) {
            printf("Server disconnected\n");
            break;
        } else {
            printf("%s\n", buffer);
        }
    }

    close(sockfd);
    return 0;
}
