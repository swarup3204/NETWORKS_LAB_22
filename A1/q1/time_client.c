#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    int i;
    char buf[100];

    if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
    {
        perror("Socket cannot be opened\n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1",&serv_addr.sin_addr);
    serv_addr.sin_port = htons(2000);

    if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
    {
        perror("Couldn't connect to server\n");
        exit(1);
    }
    for(i=0;i<100;i++)buf[i]='\0'; // clear the buffer

    recv(sockfd,buf,100,0); // receive the time from server

    printf("%s\n",buf);// print the date and time

    close(sockfd);  // close the socket
    return 0;

}