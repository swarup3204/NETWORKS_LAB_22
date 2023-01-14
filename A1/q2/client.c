#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#define send_chunk_size 5
#define receive_chunk_size 5

// cannot use unary minus as a valid operator in expression

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    int i;
    char buf[send_chunk_size];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket cannot be opened\n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(2000);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Couldn't connect to server\n");
        exit(1);
    }

    while (1)
    {
        char inp;    // character to take input character by character
        int end = 0; // flag to store end of input
        printf("Enter an expression(character by character - '\n' denotes end of input) to evaluate arithmetic  expression  containing  real  no.s  (in  x.y  or  only  x  form),  the  binary  operators +, -, *, and /, and left and right brackets (( and )). or -1 to exit\n");

        // loop to receive input data character by character
        // input end to be taken as '\n'

        while (1)
        {
            i = 0;
            while (i < send_chunk_size)
            {
                scanf("%c", &inp);

                if (inp == '\n')
                {
                    end = 1;
                    break;
                }
                else
                {
                    buf[i] = inp;
                }
                i++;
            }

            // on reaching end of input
            if (end == 1)
            {
                buf[i] = '\0';
                i++;
                if (strcmp(buf, "-1") == 0)
                {
                    printf("Exiting\n");
                    return 0;
                }
            }

            // send data in chunks....assume size of chunk as something

            // sending the chunk received
            int sent = send(sockfd, buf, i, 0);

            // if input end attained
            if (end == 1)
            {
                printf("Waiting for server to send result\n");

                // receive in chunks and print the chunks together to form a double value
                //'\0' indicates end of result from server

                // rec_buf stores the buffer to receive result chunks
                char rec_buf[receive_chunk_size];

                while (1)
                {

                    int rec = recv(sockfd, rec_buf, receive_chunk_size, 0);

                    //  print character by character until null string
                    int i = 0;
                    while (i < rec)
                    {
                        printf("%c", rec_buf[i]);
                        i++;
                    }
                    // if null character character found break
                    if (rec_buf[i - 1] == '\0')
                    {
                        printf("\n");
                        break;
                    }
                }
                // the expression has been evaluated
                break;
            }
        }
    }
    close(sockfd);

    return 0;
}