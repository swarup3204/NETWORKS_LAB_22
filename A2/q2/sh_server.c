#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>

#define send_chunk_size 40
#define receive_chunk_size 40
#define MAX_USERNAME_LENGTH 25
#define MAX_BUFFER_SIZE 200

int main()
{
    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;
    char buf[MAX_BUFFER_SIZE], send_chunk[send_chunk_size], receive_chunk[receive_chunk_size];
    // global buffer to send and receive results

    if (!(sockfd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        perror("Socket can't be opened \n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(20000);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Cannot bind to socket \n");
        exit(1);
    }

    listen(sockfd, 5);
    // PROGRAM LOGIC
    // define another buffer to send the result in chunks one by one
    // the original 100 size buffer can store entire result or receive entire input
    // then extract the next consecutive size of the buffer

    while (1)
    {
        clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd == -1)
        {
            perror("Connection error\n");
            exit(1);
        }
        if (fork() == 0)
        {

            close(sockfd);
            // send LOGIN to user
            strcpy(send_chunk, "LOGIN:");
            send(newsockfd, send_chunk, strlen(send_chunk) + 1, 0);
            // receive USERNAME from client
            int j = 0;
            while (1)
            {
                int res = recv(newsockfd, receive_chunk, 25, 0);
                // printf("%d\n", res);
                for (int i = 0; i < res; i++)
                {
                    buf[j] = receive_chunk[i]; // concatentating the chunks received
                    j++;
                }
                // printf("received chunk %s\n",receive_chunk);
                if (receive_chunk[res - 1] == '\0')
                {
                    break;
                }
            }
            // printf("Username received %s\n",buf);
            //  now check the string in buf against usernames in users.txt
            FILE *fp = fopen("users.txt", "r");
            if (fp == NULL)
            {
                return 1;
            }
            char name[25]; // max length of username can be 25
            int flag = 0;
            // to store if username found or not
            while (fgets(name, MAX_USERNAME_LENGTH, fp))
            {
                // printf("username %s\n", name);
                name[strcspn(name, "\n")] = 0; // to remove newline character and replace with null
                if (strcmp(name, buf) == 0)
                {
                    strcpy(send_chunk, "FOUND");
                    flag = 1;
                    break;
                }
            }
            if (flag == 0)
            {
                strcpy(send_chunk, "NOT-FOUND");
            }
            send(newsockfd, send_chunk, strlen(send_chunk) + 1, 0);
            // sent at one go since size small

            // receive shell command in chunks....can be large
            while (1)
            {
                int j = 0;
                while (1)
                {
                    int res = recv(newsockfd, receive_chunk, 25, 0);
                    if (res == -1) // if client closes connection
                        goto end;
                    // printf("%d\n", res);
                    for (int i = 0; i < res; i++)
                    {
                        buf[j] = receive_chunk[i]; // concatentating the chunks received
                        j++;
                    }
                    // printf("received chunk %s\n",receive_chunk);
                    if (receive_chunk[res - 1] == '\0')
                    {
                        break;
                    }
                }
                // printf("Received command %s\n", buf);

                // execute command
                // remove spaces between cd and directory name or dir and directory name
                if (buf[0] == 'c' && buf[1] == 'd' && (buf[2] == ' ' || buf[2] == '\0'))
                {
                    int i;
                    for (i = 2; i < strlen(buf); i++) // to remove spaces between cd and directory name
                    {
                        if (buf[i] != ' ')
                        {
                            break;
                        }
                    }
                    if (i == strlen(buf)) // if cd is the only command
                    {
                        if (chdir(".") == -1) // change to current directory
                        {
                            strcpy(buf, "####");
                        }
                        else
                        {
                            strcpy(buf, "done");
                        }
                    }
                    else
                    {
                        char directory[MAX_BUFFER_SIZE]; // to store directory name
                        int k = 0;
                        for (; i < strlen(buf); i++)
                        {
                            if (buf[i] == ' ' || buf[i] == '\0')
                            {
                                break;
                            }
                            else
                            {
                                directory[k++] = buf[i];
                            }
                        }
                        directory[k] = '\0';
                        if (chdir(directory) == -1) // change to directory
                        {
                            strcpy(buf, "####");
                        }
                        else
                        {
                            strcpy(buf, "done");
                        }
                    }
                }
                else if (strcmp(buf, "pwd") == 0) // print working directory
                {
                    if (!getcwd(buf, MAX_BUFFER_SIZE)) // get current working directory
                    {
                        strcpy(buf, "####");
                    }
                }
                else if (buf[0] == 'd' && buf[1] == 'i' && buf[2] == 'r' && (buf[3] == ' ' || buf[3] == '\0')) // list files in directory
                {
                    struct dirent *de;
                    DIR *dr;
                    int i;
                    for (i = 3; i < strlen(buf); i++)
                    {
                        if (buf[i] != ' ')
                        {
                            break;
                        }
                    }
                    if (i == strlen(buf))
                    {
                        dr = opendir(".");
                    }
                    else
                    {
                        char directory[MAX_BUFFER_SIZE];
                        int k = 0;
                        for (; i < strlen(buf); i++)
                        {
                            if (buf[i] == ' ' || buf[i] == '\0')
                            {
                                break;
                            }
                            else
                            {
                                directory[k++] = buf[i];
                            }
                        }
                        directory[k] = '\0';
                        dr = opendir(directory);
                    }
                    if (dr == NULL) // opendir returns NULL if couldn't open directory
                    {
                        // printf("Could not open current directory");
                        strcpy(buf, "####");
                    }
                    else
                    {
                        memset(&buf[0], 0, sizeof(buf));   // clear buffer
                        while ((de = readdir(dr)) != NULL) // read all files in directory
                        {
                            strcat(buf, de->d_name);
                            strcat(buf, " ");
                        }
                    }
                    closedir(dr);
                }
                else // invalid command...send $$$$
                {
                    strcpy(buf, "$$$$");
                }
                //  send result in chunks
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
                    //  printf("Sent chunk %s\n", send_chunk);
                    if (buf[j] == '\0') // if end of string reached
                    {
                        send(newsockfd, send_chunk, strlen(send_chunk) + 1, 0);
                        break;
                    }
                    else
                    {
                        send(newsockfd, send_chunk, send_chunk_size, 0);
                    }
                }
            }
        end:                  //  label to jump to when connection closed
            close(newsockfd); // close connection
            exit(0);
        }
        close(newsockfd);
    }
}