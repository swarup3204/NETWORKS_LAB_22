#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int main()
{
    int sockfd,newsockfd;
    int clilen;
    struct sockaddr_in cli_addr,serv_addr;

    int i;
    char buf[100];
    if(!(sockfd = socket(AF_INET, SOCK_STREAM,0))){
        perror("Socket can't be opened\n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(2000);

    if((bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))) == -1)
    {
        perror("Oh no! Could not bind\n");
        exit(1);
    }
    listen(sockfd,5);
    while(1)
    {
        //server blocked by accept until connection request arrives by accept()
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);

        if(newsockfd == -1)
        {
            perror("Connection error\n");
            exit(1);
        }
        time_t rawtime;     //time_t is a data type to store time
        struct tm *info;        //tm is a structure to store time in a readable format
        time( &rawtime );           // get the current time
        info = localtime( &rawtime );       // convert it to a readable format
        strcpy(buf,asctime(info));          // convert it to a string
        send(newsockfd,buf,strlen(buf)+1,0);        // send the time to client
        
        close(newsockfd);   // close the connection

    }
    return 0;
}