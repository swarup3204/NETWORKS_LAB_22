#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#define send_chunk_size 3
#define recv_chunk_size 4
#define MAX_HEADER_BUF_LEN 10000
#define MAX_BYTES_FILE 100000 // approx 100 KB
#define DATE_BUF_MAXLEN 100
#define CONTENT_TYPE_BUF 100
#define MAX_TOTAL_BUF_LEN (MAX_HEADER_BUF_LEN + MAX_BYTES_FILE)

#define GET 0
#define PUT 1

char buf[MAX_TOTAL_BUF_LEN]; // to store the sent and received results
char content_type[CONTENT_TYPE_BUF];
char header_buf[MAX_HEADER_BUF_LEN];
char content_buf[MAX_BYTES_FILE];
size_t header_len_read;
size_t content_len_read;
size_t buf_len_read;

void send_response(int sockfd, int len)
{

    char send_buf[send_chunk_size];
    int j = 0, i = 0;
    while (1)
    {
        for (i = 0; i < send_chunk_size && j < len; i++)
        {
            send_buf[i] = buf[j];
            j++;
        }
        send(sockfd, send_buf, i, 0);
    }
}
void recv_request(int sockfd)
{
    char recv_buf[recv_chunk_size];
    int msg_size;
    size_t total_content_len = 0;
    header_len_read = buf_len_read = content_len_read = 0;
    char *content_starts_here, *header_ends_just_before;
    char content_length[100];

    for (int i = 0; i < MAX_HEADER_BUF_LEN; ++i)
    {
        header_buf[i] = '\0';
        buf[i] = '\0';
    }
    // free content buf and remaining buf
    for (int i = 0; i < MAX_BYTES_FILE; ++i)
    {
        content_buf[i] = '\0';
        buf[i + MAX_HEADER_BUF_LEN] = '\0';
    }

    while ((msg_size = recv(sockfd, recv_buf, recv_chunk_size, 0)) > 0)
    {
        for (int i = 0; i < msg_size; ++i)
        {
            buf[buf_len_read++] = recv_buf[i];
        }

        if ((header_ends_just_before = strstr(buf, "\r\n\r\n")) != NULL)
        {


            content_starts_here = header_ends_just_before + 4;
            header_len_read = header_ends_just_before - buf;
            content_len_read = buf_len_read - (header_ends_just_before + 4 - buf);



            *header_ends_just_before = '\0';


            // store header part of content in header_buf
            for (int i = 0; i < header_len_read; i++)
            {
                header_buf[i] = buf[i];
            }
            header_buf[header_len_read] = '\0';
            // check the type of request in header
            // if GET then return
            // if PUT then continue
            if (strstr(header_buf, "GET") != NULL)
            {

                return;
            }



            // store content part of content in content_buf
            for (int i = header_len_read+4; i < buf_len_read; i++)
            {
                content_buf[i - (header_len_read+4)] = buf[i];
            }

            // find content length from header_buf

            char *p = strstr(header_buf, "Content-Length:");

            p = p + strlen("Content-Length:")+1;


            int i = 0;
            while(p[i]!=' ' && p[i]!='\r' && p[i]!='\0' && p[i]!='\n')
            {
                content_length[i]=p[i];
                i++;
            }
;
            content_length[i] = '\0';
            total_content_len = atoi(content_length);
            break;
        }
    }

    while (content_len_read < total_content_len)
    {
        msg_size = recv(sockfd, recv_buf, recv_chunk_size, 0);
        for (int j = 0; j < msg_size; ++j)
        {
            buf[buf_len_read++] = recv_buf[j];
            content_buf[content_len_read++] = recv_buf[j];
        }
        // content_len_read += msg_size;

    }
}

char *get_date_time()
{
    char date[DATE_BUF_MAXLEN]; // date has to be allocated some size
    time_t t = time(NULL);
    struct tm tm = *gmtime(&t);
    strftime(date, 100, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    char *date_header = (char *)malloc(strlen(date) + strlen("Date: ") + 1);
    strcpy(date_header, "Date: ");
    strcat(date_header, date);
    return date_header;
}
char *make_response_message(char *fname, char *filecontent, size_t file_size, int req_type, int *response_length)
{


    char date[DATE_BUF_MAXLEN]; // date has to be allocated some size
    time_t t = time(NULL);
    struct tm tm = *gmtime(&t);
    strftime(date, 100, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    char *date_header = (char *)malloc(strlen(date) + strlen("Date: ") + 1);
    strcpy(date_header, "Date: ");
    strcat(date_header, date);


    char *cache_control_header = strdup("Cache-Control: no-store");
    char *connection_header = strdup("Connection: close");

    char *content_length = '\0';
    char *content_type_header = '\0';
    char *content_length_header = '\0';
    char *content_language_header = '\0';

    if (req_type == GET) // set content types
    {
        content_type_header = (char *)malloc(strlen(content_type) + strlen("Content-Type: ") + 1);
        strcpy(content_type_header, "Content-Type: ");
        strcat(content_type_header, content_type);

        content_length = (char *)malloc(10);
        sprintf(content_length, "%ld", file_size);

        content_length_header = (char *)malloc(strlen(content_length) + strlen("Content-Length: ") + 1);
        strcpy(content_length_header, "Content-Length: ");
        strcat(content_length_header, content_length);

        content_language_header = strdup("Content-Language: en-us");
    }

    char *last_modified_header;
    // find out last modified date of file fname
    struct stat attr;
    stat(fname, &attr);
    char last_modified_date[DATE_BUF_MAXLEN];
    strftime(last_modified_date, 100, "%a, %d %b %Y %H:%M:%S %Z", localtime(&attr.st_mtime));

    last_modified_header = (char *)malloc(strlen(last_modified_date) + strlen("Last-Modified: ") + 1);
    strcpy(last_modified_header, "Last-Modified: ");
    strcat(last_modified_header, last_modified_date);


    char *expires_header;
    char expires_date[DATE_BUF_MAXLEN];
    // current time+3 days
    time_t t1 = time(NULL);
    struct tm tm1 = *localtime(&t1);
    tm1.tm_mday += 3;
    strftime(expires_date, sizeof(expires_date), "%a, %d %b %Y %H:%M:%S %Z", &tm1);


    expires_header = (char *)malloc(strlen(expires_date) + strlen("Expires: ") + 1);
    strcpy(expires_header, "Expires: ");
    strcat(expires_header, expires_date);


    char *response_line = strdup("HTTP/1.0 200 OK");


    /*integrate all into response message*/
    // first find out length of all and malloc appropriately

    // 25 is for the newlines \r\n
    // keep adding response lengths
    char *response_message = (char *)malloc(MAX_TOTAL_BUF_LEN);
    strcpy(response_message, response_line);
    *response_length = strlen(response_line);

    strcat(response_message, "\r\n");
    *response_length += 2;
    strcat(response_message, date_header);
    *response_length += strlen(date_header);
    strcat(response_message, "\r\n");

    *response_length += 2;
    strcat(response_message, cache_control_header);
    *response_length += strlen(cache_control_header);
    strcat(response_message, "\r\n");
    *response_length += 2;
    strcat(response_message, connection_header);
    *response_length += strlen(connection_header);
    strcat(response_message, "\r\n");

    *response_length += 2;
    if (req_type == GET)
    {
        strcat(response_message, content_length_header);
        *response_length += strlen(content_length_header);
        strcat(response_message, "\r\n");
        *response_length += 2;
        strcat(response_message, content_type_header);
        *response_length += strlen(content_type_header);
        strcat(response_message, "\r\n");
        *response_length += 2;
        strcat(response_message, content_language_header);
        *response_length += strlen(content_language_header);
        strcat(response_message, "\r\n");
        *response_length += 2;
    }

    strcat(response_message, last_modified_header);
    *response_length += strlen(last_modified_header);
    strcat(response_message, "\r\n");
    *response_length += 2;
    strcat(response_message, expires_header);
    *response_length += strlen(expires_header);
    strcat(response_message, "\r\n\r\n");
    *response_length += 4;

    if (req_type == GET)
    {

        // use strncat
        // strncat(response_message, filecontent, file_size);
        for (int i = 0; i < file_size; i++)
        {
            response_message[*response_length + i] = filecontent[i];
        }
        *response_length += file_size;
    }
    // free all the malloced strings

    free(date_header);
    free(cache_control_header);
    free(connection_header);
    free(last_modified_header);
    free(expires_header);
    free(response_line);
    if (req_type == GET)
    {
        free(content_length);
        free(content_length_header);
        free(content_type_header);
        free(content_language_header);
    }
    return response_message;
}

int main()
{

    socklen_t clilen;
    int sockfd, newsockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr, cli_addr;
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }
    int def_portno = 80; // default port number
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(20000);

    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);

    while (1)
    {
        clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);


        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }

        if (fork() == 0)
        {
            close(sockfd);

            int response_length; // to store the length of response
            char *response;      // to store the response message

            recv_request(newsockfd);

            // we need to print request message received
            printf("HTTP Request\n%s\n", header_buf);

            // store the details of server in a log file AccessLog.txt

            FILE *fp = fopen("AccessLog.txt", "a");

            //<Date(ddmmyy)>:<Time(hhmmss)>:<Client IP>:<Client Port>:<GET/PUT>:<URL>

            char date[DATE_BUF_MAXLEN], time_[DATE_BUF_MAXLEN];
            time_t t = time(NULL);                           // get current time
            struct tm tm = *localtime(&t);                   // convert to local time
            strftime(date, sizeof(date), "%d%m%y", &tm);     // format date
            strftime(time_, sizeof(time_), "%H%M%S", &tm);   // format time
            fprintf(fp, "%s:%s:", date, time_);              // write date and time
            int client_ip = ntohs(cli_addr.sin_addr.s_addr); // get client ip
            int client_port = ntohs(cli_addr.sin_port);      // get client port

            char str_cpy[MAX_TOTAL_BUF_LEN];
            strcpy(str_cpy, buf);
            char *method = strtok(str_cpy, " ");                               // GET or PUT
            char *url = strtok(NULL, " ");                                     // url or location form which to get or put
            fprintf(fp, "%d:%d:%s:%s\n", client_ip, client_port, method, url); // write to file
            fclose(fp);

            if (strcmp(method, "GET") == 0)
            {
                // send file
                // url is absolute path of file -> https://www.tutorialspoint.com/http/http_requests.htm (says absolute path)
                // get the file from path and store in binary format
                char file_content[MAX_BYTES_FILE];
                FILE *fp = fopen(url, "rb");
                if (fp == NULL)
                {
                    // send error message
                    // set apt status code
                    char *response_line;

                    if (errno == EACCES)
                    {
                        // make response message with data and 403 status
                        response_line = strdup("HTTP/1.1 403 Forbidden");

                    }
                    else if (errno == ENOENT)
                    {
                        response_line = strdup("HTTP/1.1 404 Not Found");

                    }
                    char *date_header = get_date_time();
                    // add null character at end of response_line

                    response_length = strlen(date_header) + strlen(response_line) + strlen("\r\n\r\n") + 3;
                    response = (char *)malloc(response_length);

                    strcpy(response, response_line);
                    strcat(response, "\r\n");
                    strcat(response, date_header);
                    strcat(response, "\r\n\r\n");

                    // free all the malloced strings
                    free(date_header);
                    free(response_line);
                }
                else
                {
                    // fill content_type with apt content type from request message
                    // get starting index of "Accept:" in request message buf
                    // printf("Hi %s\n",buf);
                    char *p = strstr(buf, "Accept: ");
                    // printf("%c\n",*p);
                    // fill content_type
                    p = p + strlen("Accept: ");
                    // clear content_type
                    for (int i = 0; i < CONTENT_TYPE_BUF; i++)
                        content_type[i] = '\0';
                    int i = 0;
                    while (*p != '\r')
                    {
                        content_type[i++] = *p;
                        p++;
                        // add to content_type
                    }
                    // printf("Reached\n");
                    // read file in binary format using fread
                    fseek(fp, 0, SEEK_END);
                    long lSize = ftell(fp);

                    rewind(fp);
                    size_t ret;
                    ret = fread(file_content, 1, lSize, fp);
                    // if ((ret = fread(file_content, 1, lSize, fp)) == lSize)
                    // {
                    //     if (feof(fp) == 0)
                    //     {
                    //         return -1;
                    //     }
                    // }
                    size_t bytes_read = ret;
                    // bytes_read = fread(file_content, 1, MAX_BYTES_FILE, fp);
                    //  write filecontent using fwrite to test file
                    fclose(fp);
                    FILE *tmp = fopen("tsting.txt", "wb");
                    int wt = fwrite(file_content, 1, bytes_read, tmp);



                    // does fread return a null terminated
                    fclose(tmp);
                    response = make_response_message(url, file_content, bytes_read, GET, &response_length);
                    // printf("Done\n");

                    // send response message
                }
            }
            else if (strcmp(method, "PUT") == 0)
            {

                FILE *fp = fopen(url, "wb");
                if (fp == NULL)
                {
                    // send error message
                    // set apt status code
                    char *response_line;
                    if (errno == EACCES)
                    {
                        // make response message with data and 403 status
                        response_line = strdup("HTTP/1.1 403 Forbidden");
                    }
                    else if (errno == ENOENT)
                    {
                        response_line = strdup("HTTP/1.1 404 Not Found");
                    }
                    char *date_header = get_date_time();
                    response_length = strlen(date_header) + strlen(response_line) + strlen("\r\n\r\n") + 2;
                    response = (char *)malloc(response_length);
                    strcpy(response, response_line);
                    strcat(response, "\r\n");
                    strcat(response, date_header);
                    strcat(response, "\r\n\r\n");
                    // free all the malloced strings
                    free(date_header);
                    free(response_line);
                }
                else
                {

                    int bytes_written = fwrite(content_buf, 1, content_len_read, fp);

                    fclose(fp);
                    response = make_response_message(url, NULL, -1, PUT, &response_length);

                }
            }
            else
            {
                // send error message
                // set apt status code
                // bad request status code 400
                char *response_line = "HTTP/1.1 400 Bad Request";
                // append date field
                char *date_header = get_date_time();
                // get length to send
                int len = strlen(response_line) + strlen(date_header) + strlen("\r\n\r\n") + 2;
                response_length = len;
                response = (char *)malloc(response_length);
                strcpy(response, response_line);
                strcat(response, "\r\n");
                strcat(response, date_header);
                strcat(response, "\r\n\r\n");
            }

            // first empty buf
            for (int i = 0; i < MAX_TOTAL_BUF_LEN; i++)
                buf[i] = '\0';
            // then copy byte by byte of response
            for (int i = 0; i < response_length; i++)
                buf[i] = response[i];

            send_response(newsockfd, response_length);

            close(newsockfd);
            exit(0);
        }
    }
}