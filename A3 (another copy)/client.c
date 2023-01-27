#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define send_chunk_size 10
#define recv_chunk_size 10
#define MAX_STRING_LENGTH 100
const char* s_ip="127.0.0.1";

char buf[MAX_STRING_LENGTH]; // to store the sent and received results

void send_buffer(int newsockfd)
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
            send(newsockfd, send_buf, strlen(send_buf) + 1, 0);
            break;
        }
        else
        {
            send(newsockfd, send_buf, send_chunk_size, 0);
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
    // get l's port
    int lport = atoi(argv[1]);
    int sockfd;
    struct sockaddr_in serv_addr;
    int i;

    if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
    {
        perror("Socket cannot be opened\n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    inet_aton(s_ip,&serv_addr.sin_addr);
    serv_addr.sin_port = htons(lport);

    if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
    {
        perror("Couldn't connect to server\n");
        exit(1);
    }

    recv_buffer(sockfd); // receive the time from server

    printf("Data and Time : %s\n",buf);// print the date and time

    close(sockfd);  // close the socket
    return 0;

}