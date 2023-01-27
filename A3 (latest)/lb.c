#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define send_chunk_size 3
#define recv_chunk_size 4
#define MAX_STRING_LENGTH 100
#define N_SERVERS 2

const char *s1_ip = "127.0.0.1"; /*kept as macros for the sake of */
const char *s2_ip = "127.0.0.1";
const char *lb_ip = "127.0.0.1";

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

/*function to poll on client for a time of 5000 ms and return the value returned by poll*/

int poll_on_client(int sockfd,int time)
{
    //printf("Polling\n");
    struct pollfd fdset[1];
    fdset[0].fd = sockfd;
    fdset[0].events = POLLIN;
    int res = poll(fdset, 1, time);
    return res;
}

/*function to connect to server and get it's load*/
int get_load_from_server(int s_port, int sno)
{
    // create new socket
    //printf("Load request start\n");
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(s_port);
    if (sno == 1)
        inet_aton(s1_ip, &s_addr.sin_addr);
    else
        inet_aton(s2_ip, &s_addr.sin_addr);
    connect(sockfd, (const struct sockaddr *)&s_addr, sizeof(s_addr));
    // sends load request to server
    strcpy(buf, "Send Load");
    send_buffer(sockfd);

    // receives load from server
    recv_buffer(sockfd);

    //printf("Load request end\n");
    close(sockfd);

    int load = atoi(buf);
    return load;
}
void service_client(int sockfd, int s_port, int sno)
{
    // now sends "Send Time" to server
    //printf("Servicing client\n");
    strcpy(buf, "Send Time");
    send_buffer(sockfd);
    if (sno == 1)
    {
        printf("Sending client request to %s and server no %d\n", s1_ip, 1);
    }
    else
    {
        printf("Sending client request to %s and server no %d\n", s2_ip, 2);
    }

    // receives time from server
    recv_buffer(sockfd);
}
int main(int argc, char *argv[])
{
    // 3 args: loab balancer port, server1 port, server2 port
    // store them in 3 separate variables

    /*buffers to receive and send chunks*/

    int lb_port = atoi(argv[1]);
    int s1_port = atoi(argv[2]);
    int s2_port = atoi(argv[3]);
    //printf("%d,%d,%d\n", lb_port, s1_port, s2_port);

    struct pollfd fdset[1]; // pollfd structure to poll the socket

    // define 2 variables to store the loads of S1 and S2
    int s1_load = 0;
    int s2_load = 0;

    struct sockaddr_in serv_addr, cli_addr;
    int clilen;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // will be used for communication with client
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(lb_port);

    bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(sockfd, 5); // upto 5 concurrent requests allowed

    // create a socket to poll on client request for a timeout of 5 second

    /** in a while loop,first check if there is a client request for a timeout of 5 second using fork
     *  if there is a client request, service it by connecting to server having lesser load
     *  and then after 5 seconds get the loads from
     * the servers and store them.For the next iteration,accept a client request again and service it the
     * same way **/

    while (1)
    {
        /*store the time to the granulity of milisecond before polling*/
        struct timeval start, temp;
        int time = 5000;
        gettimeofday(&start, NULL);
        do
        {
            //printf("Ready for client request\n");
            int ret = poll_on_client(sockfd,time);

            if (ret < 0)
            {
                perror("Error in polling\n");
                exit(0);
            }

            if (ret == 0) // timeout so re-initialise start time
            {
                //printf("Time out\n");
                break;
            }
            else
            {
                /*store current time in a temporary variable*/
                gettimeofday(&temp, NULL);

                /*accepting client request for connection*/
                clilen = sizeof(cli_addr);
                int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

                if (newsockfd < 0)
                {
                    printf("Error in accepting client request\n");
                    exit(0);
                }
                if (fork() == 0)
                {
                    close(sockfd);

                    // child process
                    int sno = (s1_load < s2_load) ? 1 : 2; // sno stores server number having less load

                    // printf("Servicing client request by server : %d\n", sno);

                    int s_port = (sno == 1) ? s1_port : s2_port; // s_port stores port number of server having less load

                    struct sockaddr_in s_addr;
                    s_addr.sin_family = AF_INET;
                    s_addr.sin_port = htons(s_port);

                    if (sno == 1)
                        inet_aton(s1_ip, &s_addr.sin_addr);
                    else
                        inet_aton(s2_ip, &s_addr.sin_addr);
                    // create new sockfd to communicate with server
                    int s_sockfd = socket(AF_INET, SOCK_STREAM, 0);

                    connect(s_sockfd, (const struct sockaddr *)&s_addr, sizeof(s_addr));
                    service_client(s_sockfd, s_port, sno);

                    // now send back date and time to client
                    send_buffer(newsockfd);
                    close(newsockfd);
                    close(s_sockfd);
                    exit(0);
                }
            }
        time = 5000 - (int)((temp.tv_sec - start.tv_sec) * 1000 + (temp.tv_usec - start.tv_usec) / 1000); 
        //printf("Time remaining is %d\n",time);
        } while ((temp.tv_sec - start.tv_sec) * 1000 + (temp.tv_usec - start.tv_usec) / 1000 <= 5000);
        /* if difference between start and temp less than 5000 ms then poll again on remaining time*/

        /*open connection to S1 and get it's load then open connection to S2 and get it's load*/
        s1_load = get_load_from_server(s1_port, 1);
        printf("Load received from %s and server 1 is %d\n", s1_ip, s1_load);
        s2_load = get_load_from_server(s2_port, 2);
        printf("Load received from %s and server 2 is %d\n", s2_ip, s2_load);
    }
    close(sockfd);
    return 0;
}