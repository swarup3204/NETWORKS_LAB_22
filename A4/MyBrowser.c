#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <poll.h>

#define DATE_BUF_MAXLEN (1 << 12)
#define MAX_FILE_SIZE (1 << 15)
#define MAX_CONTENT_LEN_SIZE (1 << 8)
#define MAX_TYPE_LEN (1 << 6)
#define MAX_RESPONSE_LEN (1 << 15)
#define MAX_LINE_SIZE (1 << 12)
#define WAITTIME 3000
#define BUF_SIZE 12

char file_buf[MAX_FILE_SIZE] = {'\0'};

char* PROMPT = "MyOwnBrowser> ";

char* concat_all(char** str_arr, int len)
{
    int i;
    int total_size = 3; // terminal '\n' and '\0' also to be counted
    for (i = 0; i < len; ++i){
        total_size += strlen(str_arr[i]);
    }
    
    char* concat_str = (char*)malloc(sizeof(char) * (total_size+2));
    concat_str[0] = '\0';

    for(i = 0 ; i < len; ++i){
        concat_str = strcat(concat_str,str_arr[i]);
    }

    concat_str[total_size-3] = '\r';
    concat_str[total_size-2] = '\n';
    concat_str[total_size-1] = '\0';

    return concat_str;
}

int parse_cmd(char* cmd, char** __REQ_TYPE, char** __SERV_URL, char** filename)
{
    char* str = strdup(cmd);
    *filename = NULL;
    while (*str == ' ' || *str =='\t')
    {
        str++;
    }
    
    char* tok = strtok(str," ");
    if (tok == NULL)
    {
        return -1;
    }
    
    *__REQ_TYPE = strdup(tok);

    tok = strtok(NULL, " \t\n");
    if (tok == NULL)
    {
        return -1;
    }
    
    *__SERV_URL = strdup(tok);

    if (strcmp(*__REQ_TYPE,"PUT") == 0)
    {

        tok = strtok(NULL, " \t\n");
        if (tok == NULL)
        {
            return -1;
        }
        
        *filename = strdup(tok);
    }

    free(str);
}

int parse_serv_url(char* SERV_URL, char** __SERV_IP, char** __SERV_FILEPATH, char** __SERV_PORT)
{

    char* str = strdup(SERV_URL);

    while (*str == ' ' || *str =='\t')
    {
        str++;
    }

    char* tok = str;

    tok = strtok(str,"/");
    if (tok == NULL)
    {
        return -1;
    }
    

    tok = strtok(NULL,"/");
    if (tok == NULL)
    {
        return -1;
    }
    
    *__SERV_IP = strdup(tok);

    tok = strtok(NULL,":");
    if (tok == NULL)
    {
        printf("returning -1\n");
        return -1;
    }
    
    *__SERV_FILEPATH = (char*)malloc(sizeof(char) * (strlen(tok)+2));

    (*__SERV_FILEPATH)[0] = '/';

    (*__SERV_FILEPATH)[1] = '\0';

    *__SERV_FILEPATH = strcat(*__SERV_FILEPATH,tok);

    tok = strtok(NULL," \t\n");
    if (tok == NULL)
    {
        *__SERV_PORT = "80";
    }
    else
    {
        
        *__SERV_PORT = tok;
    }

    free(str);

    return 0;
}

char* generate_status_line(char* REQ_TYPE, char* SERV_FILEPATH)
{
    char* http_version = "HTTP/1.1";
    char* space = " ";
    char* str_arr[] = {REQ_TYPE, space, SERV_FILEPATH, space, http_version}; 

    char* status_line = concat_all(str_arr,sizeof(str_arr)/sizeof(char*));


    return status_line;
}

char* generate_Host_header(char* SERV_IP, char* SERV_PORT)
{
    char* field = "Host: ";
    char* colon = ":";
    char* str_arr[] = {field, SERV_IP, colon, SERV_PORT}; 
    char* header = concat_all(str_arr,sizeof(str_arr)/sizeof(char*));

    return header;
}

char* generate_Connection_header(int option)
{
    char* field = "Connection: ";
    char* field_val = (option == 0) ? "close" : "keep-alive";
    char* str_arr[] = {field, field_val};
    char* header = concat_all(str_arr,sizeof(str_arr)/sizeof(char*));

    return header;
}

char* generate_Accept_header(char* SERV_FILEPATH)
{
    char* field = "Accept: ";
    char* field_val;
    char* end_ptr = SERV_FILEPATH + strlen(SERV_FILEPATH) - 1;
    while(*end_ptr != '.')
    {
        end_ptr--;
    }
    

    if (strcmp(end_ptr,".html") == 0)
    {
        field_val = "text/html";
    }

    else if (strcmp(end_ptr,".pdf") == 0)
    {
        field_val = "application/pdf";
    }

    else if (strcmp(end_ptr,".jpg") == 0 || strcmp(end_ptr,".jpeg") == 0)
    {
        field_val = "image/jpeg";
    }

    else{
        field_val = "text/*";
    }
    char* str_arr[] = {field, field_val};
    char* header = concat_all(str_arr,sizeof(str_arr)/sizeof(char*));

    return header;
}

char* generate_Accept_Language_header()
{
    char* header = "Accept-Language: en-us,en;q=0.5\r\n";
    return header;
}

char* generate_Date_header()
{
    char buf[DATE_BUF_MAXLEN];
    char* field = "Date: ";
    time_t now = time(0);
    struct tm t = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &t);

    char* str_arr[] = {field, buf};
    char* header = concat_all(str_arr,sizeof(str_arr)/sizeof(char*));

    return header;
}

char* generate_Last_Mod_Since_header()
{
    char buf[DATE_BUF_MAXLEN];
    char* field = "If-Modified-Since: ";
    time_t now = time(0) - 2*60*60*24;
    struct tm t = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &t);

    char* str_arr[] = {field, buf};
    char* header = concat_all(str_arr,sizeof(str_arr)/sizeof(char*));

    return header;
}

char* generate_Content_Language_header()
{
    char* header = "Content-Language: en-us\r\n";
    return header;
}

char* content_type_from_extension(char* extension)
{
    char* field_val;
    if (strcmp(extension,".html") == 0)
    {
        field_val = strdup("text/html");
    }

    else if (strcmp(extension,".pdf") == 0)
    {
        field_val = strdup("application/pdf");
    }

    else if (strcmp(extension,".jpg") == 0 || strcmp(extension,".jpeg") == 0)
    {
        field_val = strdup("image/jpeg");
    }

    else{
        field_val = strdup("text/*");
    }

    return field_val;
}

char* generate_Content_Type_header(char* SERV_FILEPATH)
{
    char* field = "Content-Type: ";
    char* field_val;
    char* end_ptr = SERV_FILEPATH + strlen(SERV_FILEPATH) - 1;
    while(*end_ptr != '.')
    {
        end_ptr--;
    }

    
    field_val = content_type_from_extension(end_ptr);

    char* str_arr[] = {field, field_val};
    char* header = concat_all(str_arr,sizeof(str_arr)/sizeof(char*));

    return header;
}

char* generate_GET_request(char* REQ_TYPE, char* SERV_IP, char* SERV_PORT, char* SERV_FILEPATH)
{
    char* str_arr[] = {
        generate_status_line(REQ_TYPE, SERV_FILEPATH),
        generate_Host_header(SERV_IP, SERV_PORT),
        generate_Date_header(),
        generate_Accept_header(SERV_FILEPATH),
        generate_Accept_Language_header(),
        // generate_Last_Mod_Since_header(),
        generate_Connection_header(0)  
    };

    char* request = concat_all(str_arr, sizeof(str_arr)/sizeof(char*));

    return request;
}


int generate_content(char* filename, size_t* bytes_read)
{
    
    FILE* fptr;
    char ch;
    char* end_ptr = filename + strlen(filename) - 1;
    int is_bin = 0;
    while(*end_ptr != '.')
    {
        end_ptr--;
    }
    
    if (strcmp(end_ptr,".html") == 0)
    {
        fptr = fopen(filename,"r");
    }

    else if (strcmp(end_ptr,".pdf") == 0)
    {
        fptr = fopen(filename,"rb");
        is_bin = 1;
    }

    else if (strcmp(end_ptr,".jpg") == 0 || strcmp(end_ptr,".jpeg") == 0)
    {
        printf("opening file in binary mode\n");
        fptr = fopen(filename,"rb");
        is_bin = 1;
    }

    else
    {
        fptr = fopen(filename,"r");
    }

    *bytes_read = 0;

    if (is_bin == 0)
    {
        while(1) 
        {
            ch = fgetc(fptr);
            if (ch == EOF) break;

            file_buf[*bytes_read] = ch;

            (*bytes_read)++;
        }
        file_buf[*bytes_read] = '\0';
        // (*bytes_read)++;
    }

    else
    {
        printf("generating binary data\n");
        fseek(fptr,0,SEEK_END);
        long lSize = ftell(fptr);

        rewind(fptr);
        size_t ret;
        if ((ret = fread(file_buf,lSize,1,fptr)) == lSize)
        {
            if (feof(fptr) == 0)
            {
                return -1;
            }
        }
        *bytes_read = lSize;
    }

    return 0;

    
}

char* generate_Content_Length_header(int n)
{
    char* header = (char*)malloc(sizeof(char) * MAX_CONTENT_LEN_SIZE);
    sprintf(header,"Content-Length: %d\r\n",n);
    return header;
}

char* generate_PUT_request(char* REQ_TYPE, char* SERV_IP, char* SERV_PORT, char* SERV_FILEPATH, char* filename, size_t* request_size)
{
    size_t bytes_read;

    generate_content(filename,&bytes_read);

    char* str_arr[] = {
        generate_status_line(REQ_TYPE,SERV_FILEPATH),
        generate_Host_header(SERV_IP, SERV_PORT),
        generate_Date_header(),
        generate_Content_Language_header(),
        generate_Content_Length_header(bytes_read),
        generate_Content_Type_header(filename),
        // change here
    };
    char* request = concat_all(str_arr, sizeof(str_arr)/sizeof(char*));
    *request_size = strlen(request)+bytes_read;
    char* temp = (char*)malloc(sizeof(char)*(*request_size));
    temp[0] = '\0';
    strcat(temp,request);
    free(request);
    request = temp;
    int offst = strlen(request);
    for (int it = 0; it < bytes_read; ++it)
    {
        request[it+offst] = file_buf[it];
    }
    
    return request;
}

int parse_response_status_line(char* linebuf, char** __version, char** __status_code, char** __message)
{
    char* str = strdup(linebuf);

    while (*str == ' ' || *str =='\t')
    {
        str++;
    }

    char* tok = str;

    tok = strtok(str," ");
    if (tok == NULL)
    {
        return -1;
    }
    
    *__version = strdup(tok);

    tok = strtok(NULL," ");
    if (tok == NULL)
    {
        return -1;
    }
    
    *__status_code = strdup(tok);

    tok = strtok(NULL,"\0\t\n");
    if (tok == NULL)
    {
        return -1;
    }
    
    *__message = (char*)malloc(sizeof(char) * (strlen(tok)+2));
    

    free(str);

    return 0;
}

int handle_response_status_line(char* status_code, char* message)
{
    int status_val = atoi(status_code);

    
    if (status_val == 200)
    {
        return 0;
    }
    return -1;
}


char* get_extension(char* filename)
{
    char* end_ptr = filename + strlen(filename) - 1;
    while(*end_ptr != '.')
    {
        end_ptr--;
    }
    
    return end_ptr;
}


int parse_header(char* header, char** __header_key, char** __header_val)
{
    char* str = strdup(header);

    *__header_key = (char*)malloc(sizeof(char)*(strlen(str)+1));
    *__header_val = (char*)malloc(sizeof(char)*(strlen(str)+1));

    char* ptr = str;
    while (*ptr != ':')
    {
        (*__header_key)[ptr-str] = *ptr;
        ptr++;
    }
    (*__header_key)[ptr-str] = '\0';

    ptr++;
    
    while(*ptr == ' ')
    {
        ptr++;
    }

    str = ptr;

    while(*ptr != '\r' && *ptr != '\0' && *ptr != '\n')
    {
        (*__header_val)[ptr-str] = *ptr;
        ptr++;
    }
    (*__header_val)[ptr-str] = '\0';

    return 0;
}

int parse_header_str(char* header_str, char** __content_language, char** __content_length, char** __content_type)
{
    char *tok, *header_key, *header_val, *status_code, *status_message;
    // char version[100], status_code[100], status_message[100];
    int lineno = 0;
    char* str = strdup(header_str);

    tok = strtok(str,"\r\n");

    while(tok != NULL)
    {
        if(lineno != 0)
        {
            parse_header(tok,&header_key,&header_val);

            if (strcmp(header_key,"Content-Language") == 0)
            {
                *__content_language = header_val;
            }

            else if (strcmp(header_key,"Content-Length") == 0)
            {
                *__content_length = header_val;
            }

            else if (strcmp(header_key,"Content-Type") == 0)
            {
                *__content_type = header_val;
            }
        }

        else
        {
            // sscanf(tok,"%s %s %s",version,status_code,status_message);
            

            /************/

            char* sptr = strdup(tok);

            while(*sptr != ' ')
            {
                sptr++;
            }
            sptr++;

            status_code = sptr;

            while(*sptr != ' '){
                sptr++;
            }
            *sptr = '\0';
            sptr++;

            status_message = sptr;

            /************/
            int status_num = atoi(status_code);
            switch(status_num)
            {
                case 200:
                {
                    printf("200: SUCCESS\n");
                    break;
                }
                case 400:
                {
                    printf("400: BAD_REQUEST\n");
                    break;
                }
                case 403:
                {
                    printf("403: FORBIDDEN ACCESS\n");
                    break;
                }
                case 404:
                {
                    printf("404: FILE NOT FOUND\n");
                    break;
                }
                default:
                {
                    printf("%d: Unknown error\n",status_num);
                }
            }

            printf("Status message: %s\n",status_message);
        }

        tok = strtok(NULL,"\r\n");

        lineno++;
    }

    return 0;
    
}

void GET_response_handler(int sockfd, char* SERV_FILEPATH)
{
    int msg_size, content_len_read = 0, total_bytes_read = 0, ret, total_content_len = 0;
    char *content_length = NULL, *content_type = NULL, *content_language = NULL, *expected_content_type, *extension;
    char* content_starts_here = NULL;
    char* header_ends_just_before = NULL;
    char buf[BUF_SIZE];
    char* recv_buf=(char*)malloc(sizeof(char));
    recv_buf[0]='\0';
    FILE* fptr = fopen("file.download","wb");

    for(int l=0;l<BUF_SIZE;++l){ buf[l]='\0'; }
    while((msg_size=recv(sockfd,buf,BUF_SIZE,0))>0){
        int k,start=total_bytes_read;
        total_bytes_read += msg_size;
        char* new_str=(char*)malloc(sizeof(char)*(total_bytes_read+1));
        // strcpy(new_str,recv_buf);
        for(k=0;k<start;++k){
            new_str[k] = recv_buf[k];
        }
        free(recv_buf);
        recv_buf=new_str;
        
        for(k=start;k<start+msg_size;++k){
            recv_buf[k]=buf[k-start];
        }
        recv_buf[k]='\0';
        
        if ((header_ends_just_before = strstr(recv_buf,"\r\n\r\n")) != NULL)
        {
            content_starts_here = header_ends_just_before+4;
            content_len_read = total_bytes_read - (header_ends_just_before+4-recv_buf);


            *header_ends_just_before = '\0';

            printf("Response: \n%s\n",recv_buf);

            ret = parse_header_str(recv_buf,&content_language,&content_length,&content_type);

            if (content_length == NULL)
            {
                return;
            }

            if (ret < 0)
            {
                printf("error in parsing headers\n");
                return;
            }

            extension = get_extension(SERV_FILEPATH);
            expected_content_type = content_type_from_extension(extension);

            if (content_language != NULL){
                if (strcmp(content_language,"en-us") != 0 && strcmp(content_language,"en") != 0)
                {
                    printf("incompatible language\n");
                    return;
                }
            }


            total_content_len = atoi(content_length);

            if (content_len_read > 0)
            {
                // write to file
                
                int write_size = (content_len_read < total_content_len) ? content_len_read:total_content_len;

                fwrite(header_ends_just_before+4,1,write_size,fptr);

            }

            total_content_len -= content_len_read;

            break;
        }

        for(int l=0;l<BUF_SIZE;++l){ buf[l]='\0'; }


    }

    while(total_content_len > 0)
    {
        msg_size = recv(sockfd,buf,BUF_SIZE,0);
        int write_size = (msg_size < total_content_len) ? msg_size:total_content_len;
        fwrite(buf,1,write_size,fptr);

        total_content_len -= msg_size;
    }

    free(recv_buf);
    fclose(fptr);

    pid_t pid = fork();

    if(pid == 0)
    {
        char* args[3];
        args[2] = NULL;

        if (strcmp(extension,".pdf") == 0)
        {
            printf("opening document viewer\n");
            args[0] = "xdg-open";
            args[1] = "file.download";
            if (execvp("xdg-open",args) == -1)
            {
                perror("execvp");
            }
            exit(EXIT_FAILURE);
        }

        else if (strcmp(extension,".jpeg") == 0 || strcmp(extension,".jpg") == 0)
        {
            printf("opening image viewer\n");
            args[0] = "eog";
            args[1] = "file.download";
            if (execvp("eog",args) == -1)
            {
                perror("execvp");
            }
            exit(EXIT_FAILURE);
        }

        else if (strcmp(extension,".html") == 0)
        {
            args[0] = "google-chrome";
            args[1] = "file.download";
            if (execvp("google-chrome",args) == -1)
            {
                perror("execvp");
            }
            exit(EXIT_FAILURE);
        }

        else
        {
            args[0] = "gedit";
            args[1] = "file.download";
            if (execvp("gedit",args) == -1)
            {
                perror("execvp");
            }
            exit(EXIT_FAILURE);
        }
    }

    int wstatus;

    do
    {
        if(waitpid(pid,&wstatus,WUNTRACED) == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));

    close(sockfd);


}

void PUT_response_handler(int sockfd)
{
    int msg_size, content_len_read = 0, total_bytes_read = 0, ret, total_content_len = 0;
    char *content_length = NULL, *content_type = NULL, *content_language = NULL, *expected_content_type, *extension;
    char* content_starts_here = NULL;
    char* header_ends_just_before = NULL;
    char buf[BUF_SIZE];
    char* recv_buf=(char*)malloc(sizeof(char));
    recv_buf[0]='\0';


    for(int l=0;l<BUF_SIZE;++l){ buf[l]='\0'; }
    while((msg_size=recv(sockfd,buf,BUF_SIZE,0))>0){
        int k,start=total_bytes_read;
        total_bytes_read += msg_size;
        char* new_str=(char*)malloc(sizeof(char)*(total_bytes_read+1));
        for(k=0;k<start;++k){
            new_str[k] = recv_buf[k];
        }
        free(recv_buf);
        recv_buf=new_str;
        
        for(k=start;k<start+msg_size;++k){
            recv_buf[k]=buf[k-start];
        }
        recv_buf[k]='\0';
        
        if ((header_ends_just_before = strstr(recv_buf,"\r\n\r\n")) != NULL)
        {
            *header_ends_just_before = '\0';
            printf("Response: \n%s\n",recv_buf);
            ret = parse_header_str(recv_buf,&content_language,&content_length,&content_type);
            
            break;
        }

        for(int l=0;l<BUF_SIZE;++l){ buf[l]='\0'; }

    }

    free(recv_buf);


    close(sockfd);
}

void run_client_loop()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    
    char* cmd = NULL;
    ssize_t len, read;
    int status;

    while(1)
    {
        printf("%s",PROMPT);

        if ((read = getline(&cmd,&len,stdin)) < 0)
        {
            perror("getline\n");
            exit(EXIT_FAILURE);
        }

        cmd[read-1] = '\0';

        char *REQ_TYPE, *SERV_URL, *filename, *SERV_IP, *SERV_FILEPATH, *SERV_PORT;

        if (strcmp(cmd,"QUIT") == 0)
        {
            break;
        }
        status = parse_cmd(cmd,&REQ_TYPE,&SERV_URL,&filename);
        if (status < 0)
        {
            /* erroneous command */
            continue;
        }


        status = parse_serv_url(SERV_URL,&SERV_IP,&SERV_FILEPATH,&SERV_PORT);
        if (status < 0)
        {
            /* erroneous command */
            continue;
        }


        if (strcmp(REQ_TYPE,"GET") == 0)
        {

            sockfd = socket(AF_INET,SOCK_STREAM,0);
            struct pollfd fdset[1];
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(atoi(SERV_PORT));
            inet_aton(SERV_IP,&serv_addr.sin_addr);

            char* request = generate_GET_request(REQ_TYPE, SERV_IP, SERV_PORT, SERV_FILEPATH);

            connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

            send(sockfd,request,strlen(request),0);
            /* request sent not wait for response */

            fdset[0].fd = sockfd;
            fdset[0].events = POLLIN;

            int poll_ret = poll(fdset,1,WAITTIME);

            if (poll_ret < 0)
            {
                perror("poll");
                exit(EXIT_FAILURE);
            }

            if (poll_ret > 0)
            {
                GET_response_handler(sockfd,SERV_FILEPATH);
            }


            if (poll_ret == 0)
            {
                printf("request timed out\n");
            }

            close(sockfd);
        }

        else if (strcmp(REQ_TYPE,"PUT") == 0)
        {
            size_t request_size = 0;
            

            sockfd = socket(AF_INET,SOCK_STREAM,0);
            struct pollfd fdset[1];
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(atoi(SERV_PORT));
            inet_aton(SERV_IP,&serv_addr.sin_addr);

            char* request = generate_PUT_request(REQ_TYPE, SERV_IP, SERV_PORT, SERV_FILEPATH,filename,&request_size);

            connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

            send(sockfd,request,request_size,0);

            fdset[0].fd = sockfd;
            fdset[0].events = POLLIN;

            int poll_ret = poll(fdset,1,WAITTIME);

            if (poll_ret < 0)
            {
                perror("poll");
                exit(EXIT_FAILURE);
            }

            if (poll_ret > 0)
            {
                PUT_response_handler(sockfd);
            }

            if (poll_ret == 0)
            {
                printf("request timed out\n");
            }

            close(sockfd);
        }

        if (REQ_TYPE != NULL) free(REQ_TYPE);
        if (SERV_URL != NULL) free(SERV_URL);
    }

    if (cmd != NULL)
    {
        free(cmd);
    }
}



int main()
{
    run_client_loop();

    return 0;
}

/*
GET:
o Host
o Connection
    close
o Date
o Accept:
    text/html if the file asked in the url has extension .html
    application/pdf if the file asked in the url has extension .pdf
    image/jpeg if the file asked for has extension .jpg
    text/* for anything else
o Accept-Language: en-us preferred, otherwise just English (find out how to specify this)
o If-Modified-Since: should always put current time – 2 days
*/