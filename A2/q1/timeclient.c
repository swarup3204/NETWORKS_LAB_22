// A Simple Client Implementation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

#define MAX_BUFFER_SIZE 1000 // maximum buffer size

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[MAX_BUFFER_SIZE]; // buffer to store the time received from server
    struct pollfd fdset[1];       // pollfd structure to poll the socket

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    fdset[0].fd = sockfd;     // set the socket to poll
    fdset[0].events = POLLIN; // poll for input

    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr)); // clear the server address

    // Server information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(20000);
    inet_aton("127.0.0.1", &serv_addr.sin_addr);

    int cnt = 5; // number of times to try to connect to server

    while (cnt--)
    {
        printf("Trying to connect to server\n"); // try to connect to server
        int n;
        socklen_t len;
        // len = sizeof(serv_addr);            // length of server address
        char *hello = "HELLO,it's me";

        sendto(sockfd, (const char *)hello, strlen(hello), 0,
               (const struct sockaddr *)&serv_addr, sizeof(serv_addr)); // send hello to server

        int ret = poll(fdset, 1, 3000); // poll the socket for 3 seconds
        if (ret == 0)                       // if timeout
        {
            printf("Timeout\n");
            continue;
        }
        if (ret < 0)                        // if error
        {
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&serv_addr, &len);  
        // receive the time from server
        printf("%s\n", buffer); // print the time received from server
        break;                  // break out of the loop
    }

    if (cnt < 0)        // if all tries failed
    {
        printf("Timeout exceeded\n");
    }

    close(sockfd);
    return 0;
}