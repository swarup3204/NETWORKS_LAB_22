#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define MAX_BUFFER_SIZE 1000  // maximum buffer size

int main()
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    // Creating socket file descriptor

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // create a udp socket

    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(20000);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("\nServer Running....\n");  // print server running


    int n;                 // number of bytes received           
    socklen_t len;         // length of client address
    char buffer[MAX_BUFFER_SIZE]; // buffer to store the time received from server

    while (1)
    {

        len = sizeof(cliaddr);   // length of client address
        n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER_SIZE, 0,
                     (struct sockaddr *)&cliaddr, &len);  // receive hello from client
        buffer[n] = '\0';
        printf("%s\n", buffer); // print the hello received from udp client

        time_t rawtime;                                                                              // time_t is a data type to store time
        struct tm *info;                                                                             // tm is a structure to store time in a readable format
        time(&rawtime);                                                                              // get the current time
        info = localtime(&rawtime);                                                                  // convert it to a readable format
        strcpy(buffer, asctime(info));                                                               // convert it to a string
        sendto(sockfd, buffer, strlen(buffer) + 1, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)); // send the time to client
    }

    close(sockfd);
    return 0;
}