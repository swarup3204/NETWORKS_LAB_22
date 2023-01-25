// A Simple Client Implementation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    // Server information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8181);
    inet_aton("127.0.0.1", &serv_addr.sin_addr);

    int n;
    socklen_t len;
    char *hello = "CLIENT:HELLO";

    sendto(sockfd, (const char *)hello, strlen(hello), 0,
           (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    printf("Hello message sent from client\n");

    close(sockfd);
    return 0;
}