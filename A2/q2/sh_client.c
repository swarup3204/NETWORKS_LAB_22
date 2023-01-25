#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define receive_chunk_size 10 // chunk size for receiving
#define send_chunk_size 10    // chunk size for sending
#define MAX_BUFFER_SIZE 200   // maximum buffer size

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    char buf[MAX_BUFFER_SIZE], send_chunk[send_chunk_size], recv_chunk[receive_chunk_size];
    // create a socket

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket cannot be opened\n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(20000);
    // connect to server

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Couldn't connect to server\n");
        exit(1);
    }

    char rec[receive_chunk_size]; // buffer to store the time received from server
    // receive LOGIN
    int j = 0; // index for buf

    while (1)
    {
        int res = recv(sockfd, rec, receive_chunk_size, 0); // receive in chunks
        if (res == -1)
        {
            perror("Error in receiving\n");
            exit(1);
        }
        for (int i = 0; i < res; i++)
        {
            buf[j] = rec[i];
            j++;
        }
        if (rec[res - 1] == '\0') // if last character is null then break
        {
            break;
        }
    }
    // LOGIN printed to user
    printf("%s\n", buf);
    printf("Enter a username to login of max size 25\n");
    scanf("%[^\n]", buf); // scan the username with spaces
    // send in chunks the username
    // char send_chunk[send_chunk_size];
    j = 0;
    while (1)
    {
        for (int i = 0; i < send_chunk_size; i++)
        {
            send_chunk[i] = buf[j];
            if (buf[j] == '\0')
                break;
            j++;
        }
        // printf("Sent chunk %s\n", send_chunk);
        if (buf[j] == '\0') // if last character is null then break
        {
            int res = send(sockfd, send_chunk, strlen(send_chunk) + 1, 0);
            if (res == -1)
            {
                perror("Error in sending\n");
                exit(1);
            }
            break;
        }
        else
        {
            int res = send(sockfd, send_chunk, send_chunk_size, 0);
            if (res == -1)
            {
                perror("Error in sending\n");
                exit(1);
            }
        }
    }
    // exit loop when whole username sent
    // now receive FOUND or NOT-FOUND
    // char recv_chunk[receive_chunk_size];
    j = 0;
    while (1)
    {
        int res = recv(sockfd, recv_chunk, receive_chunk_size, 0); // receive in chunks
        if (res == -1)
        {
            perror("Error in receiving\n");
            exit(1);
        }
        for (int i = 0; i < res; i++)
        {
            buf[j++] = recv_chunk[i];
        }
        if (recv_chunk[res - 1] == '\0')
        {
            break;
        }
    }
    // printf("recv_chunk%s\n",buf);
    // if FOUND continue else exit
    if (strcmp(buf, "NOT-FOUND") == 0) // if username not found
    {
        printf("Sorry,username does not exist\n");
        close(sockfd);
        exit(1);
    }
    while (1)
    {
        printf("Enter a shell command to be executed on the server side\n");
        getchar();                    // to clear the buffer
        scanf("%[^\n]", buf);         // scan the shell command
        if (strcmp(buf, "exit") == 0) // if exit command given then exit
        {
            printf("Exiting\n");
            close(sockfd);
            return 0;
        }

        // since size of shell command can be large in the server side,send in chunks
        j = 0;
        while (1)
        {
            for (int i = 0; i < send_chunk_size; i++)
            {
                send_chunk[i] = buf[j];
                if (buf[j] == '\0')
                    break;
                j++;
            }
            // printf("Sent chunk %s\n", send_chunk);
            if (buf[j] == '\0')
            {
                int res = send(sockfd, send_chunk, strlen(send_chunk) + 1, 0);
                if (res == -1)
                {
                    perror("Error in sending\n");
                    exit(1);
                }
                break;
            }
            else
            {
                int res = send(sockfd, send_chunk, send_chunk_size, 0);
                if (res == -1)
                {
                    perror("Error in sending\n");
                    exit(1);
                }
            }
        }
        //  receive result in chunks
        j = 0;
        // printf("starting to receive chunk\n");
        while (1)
        {
            int res = recv(sockfd, recv_chunk, receive_chunk_size, 0);
            if (res == -1)
            {
                perror("Error in receiving\n");
                exit(1);
            }
            for (int i = 0; i < res; i++)
            {
                buf[j++] = recv_chunk[i];
            }
            // printf("received %s\n", recv_chunk);
            if (recv_chunk[res - 1] == '\0')
            {
                break;
            }
        }
        // printf("Received %s\n", buf);
        if (strcmp(buf, "$$$$") == 0) // if invalid command
        {
            printf("Invalid command\n");
        }
        else if (strcmp(buf, "####") == 0) // if error in running command
        {
            printf("Error in running command\n");
        }
        else
        {
            printf("%s\n", buf);
        }
    }
    close(sockfd);
}