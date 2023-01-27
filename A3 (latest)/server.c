#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>


#define send_chunk_size 4
#define recv_chunk_size 3
#define MAX_STRING_LENGTH 100
#define N_SERVERS 2

char buf[MAX_STRING_LENGTH]; // to store the sent and received results

void send_buffer(int sockfd)
{
    char send_buf[send_chunk_size];
    int j = 0;
    while (1)
    {
        for (int i = 0; i < send_chunk_size; i++)
        {
            send_buf[i] = buf[j];
            if (buf[j] == '\0')
                break;
            j++;
        }
        if (buf[j] == '\0') // if end of string reached
        {
            send(sockfd, send_buf, strlen(send_buf) + 1, 0);
            break;
        }
        else
        {
            send(sockfd, send_buf, send_chunk_size, 0);
        }
    }
}
void recv_buffer(int sockfd)
{
    for (int i = 0; i < MAX_STRING_LENGTH; i++)
        buf[i] = '\0'; // clearing buffer
    char recv_buf[recv_chunk_size];
    int j = 0;
    while (1)
    {
        int res = recv(sockfd, recv_buf, recv_chunk_size, 0);
        for (int i = 0; i < res; i++)
        {
            buf[j] = recv_buf[i]; // concatentating the chunks received
            j++;
        }
        if (recv_buf[res - 1] == '\0')
        {
            break;
        }
    }
}

int main(int argc, char *argv[])
{
   
    /*get the port no from command line*/
    int portno = atoi(argv[1]);
    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;
    srand(time(NULL)*portno);//seeding to generate random number differently every time
    int i;

    if (!(sockfd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        perror("Socket can't be opened\n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if ((bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) == -1)
    {
        perror("Could not bind\n");
        exit(1);
    }
    listen(sockfd, 5);
    while (1)
    {
        // server blocked by accept until connection request arrives by accept()
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd == -1)
        {
            perror("Connection error\n");
            exit(1);
        }
        /*get the message from load balancer*/
        recv_buffer(newsockfd);
        /*if message "Send Load",generate a random integer from 1 to 100 with random send and send to load balancer*/
        if (strcmp(buf, "Send Load") == 0)
        {
            int load = rand() % 100 + 1;
            sprintf(buf, "%d", load);
            send_buffer(newsockfd); // send the load to lb
            printf("Load  sent: %d\n", load);
        }
        else if (strcmp(buf, "Send Time") == 0)
        {
            // send date and time
            time_t rawtime;             // time_t is a data type to store time
            struct tm *info;            // tm is a structure to store time in a readable format
            time(&rawtime);             // get the current time
            info = localtime(&rawtime); // convert it to a readable format
            strcpy(buf, asctime(info)); // convert it to a string
            send_buffer(newsockfd);     // send the time to client
        }
        close(newsockfd); // close the connection
    }
    close(sockfd); // close the socket
    return 0;
}