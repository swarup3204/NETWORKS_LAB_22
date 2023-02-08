#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define send_chunk_size 5
#define receive_chunk_size 5
#include <math.h>
/*
1. You may assume that parenthesis will not be nested (So no (...(...)...) etc.)

2.  Do not assume any maximum size of the string even for reading from keyboard on the client side. You can read character by character, form a bunch and send. The entire expression need not be sent together.

3. Do NOT send one character at a time from client to server, send at least a bunch together (you can choose the bunch size, however small (but > 1), but cannot assume that the bunch size chosen by client is known to the server or vice--versa). Sending 1 character at a time is extremely inefficient.

4. Cannot use unary minus operator
*/

// evaluation of expression correct upto 6 decimal points

// function to count number of digits in number (including decimal point)
int length_of_number(double num)
{
    int count = 0;

    // if number is negative, make it positive
    {
        num -= num;
    }
    int integer = (int)num;

    while (integer != 0)
    {
        integer = integer / 10;
        count++;
    }
    count += 7; // adding the fixed size to display 6 digits after decimal including decimal point
    return count;
}
// function to convert number to string
char *convert_number_to_string(double val)
{

    int num_digits = length_of_number(val); // number of digits in number

    char *exp = (char *)malloc(sizeof(char) * (num_digits + 1)); // +1 for null character

    if (val < 0) // if number is negative, make it positive and add '-' at the beginning
    {
        val = -val;
        sprintf(exp, "%lf", val);
        char *new_exp = (char *)malloc(sizeof(char) * (num_digits + 2)); // +2 for null character and '-'
        new_exp[0] = '-';
        int i;
        for (i = 0; i < num_digits + 1; i++) // copying the string to new string
        {
            new_exp[i + 1] = exp[i];
        }
        new_exp[num_digits + 1] = '\0'; // adding null character at the end

        return new_exp;
    }
    sprintf(exp, "%lf", val); // convert number to string
    exp[num_digits] = '\0';

    return exp;
}

// recursive function to evaluate expression
double evaluate_exp(char *expr)
{

    int i = 0;
    // i is the index of expression
    int s = 0;
    // s stores the number of digits after decimal point
    int integer, frac = 0;
    // integer stores the integer part of number and frac is a flag to check if decimal point is encountered
    double fraction;
    // fraction stores the fractional part of number

    double res, num; // res stores the result of expression and num stores the number
    char op = '\0';  // op stores the operator

    while (expr[i] != '\0')
    {
        if (expr[i] == '(') // if '(' is encountered, call the function recursively
        {
            i++;

            int j = 0;
            while (expr[j + i] != ')')
            {
                j++;
            }
            char *sub_exp = (char *)malloc(j * sizeof(char)); // dynamic string to store sub expression

            j = 0;
            while (expr[i] != ')')
            {
                // keep concatenating the characters in a dynamic string
                sub_exp[j] = expr[i];
                j++;
                i++;
            }
            sub_exp[j] = '\0'; // adding null character at the end

            num = evaluate_exp(sub_exp); // evaluate the sub expression

            if (op == '+')
                res = res + num;
            else if (op == '-')
                res = res - num;
            else if (op == '*')
                res = res * num;
            else if (op == '/')
                res = res / num;
            else
                res = num; // if no operator is encountered, assign the number to result

            i++;
        }
        else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '/' || expr[i] == '*') // if operator is encountered
        {
            op = expr[i];
            i++;
        }
        else if (expr[i] == ' ' || expr[i] == '\t') // if space is encountered, skip it
        {
            i++;
        }
        else // if number is encountered
        {
            s = 1;
            integer = 0;
            fraction = 0.0;
            frac = 0;
            while (expr[i] != '\0' && expr[i] != ' ' && expr[i] != '\t' && expr[i] != '+' && expr[i] != '-' && expr[i] != '/' && expr[i] != '*')
            {

                if (expr[i] == '.')
                {
                    frac = 1;
                    i++;
                    continue;
                }
                if (frac == 0)
                {
                    integer = integer * 10 + (expr[i] - '0');
                }
                else
                {
                    fraction = fraction + (expr[i] - '0') / pow(10, s);
                    s++;
                }
                i++;
            }

            num = (double)integer;
            num += fraction;

            if (op == '+')
                res = res + num;
            else if (op == '-')
                res = res - num;
            else if (op == '*')
                res = res * num;
            else if (op == '/')
                res = res / num;
            else // if no operator is encountered, assign the number to result
                res = num;
        }
    }

    return res;
}

int main()
{
    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;

    int i;
    char buf[receive_chunk_size];
    char *exp_new, *exp_old;

    if (!(sockfd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        perror("Socket can't be opened \n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(2000);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Cannot bind to socket \n");
        exit(1);
    }

    listen(sockfd, 5);

    while (1)
    {
        clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd == -1)
        {
            perror("Connection error\n");
            exit(1);
        }

        while (1)
        {
            exp_old = (char *)malloc(sizeof(char));
            exp_old[0] = '\0';
            int cnt = 0;
            // cnt keeps track of the total number of bytes received
            while (1)
            {

                // re-initialize chunk string to null
                for (int i = 0; i < receive_chunk_size; i++)
                    buf[i] = '\0';

                // accept chunks of some fixed bytes from client
                int res = recv(newsockfd, buf, receive_chunk_size, 0);

                if (res == -1)
                {
                    perror("Error in receiving data\n");
                    exit(1);
                }

                // if no more bytes are available to read from client then close socket
                if (res == 0)
                {
                    goto end;
                }

                cnt += res;

                // reallocate memory for exp
                // exp keeps on concatenating buf

                exp_new = (char *)malloc((cnt + 1) * sizeof(char)); // +1 for null character
                int i = 0;
                while (exp_old[i] != '\0') // copy the old expression to new expression
                {
                    exp_new[i] = exp_old[i];
                    i++;
                }

                while (i < cnt) // concatenate the new chunk to the new expression
                {
                    exp_new[i] = buf[i - cnt + res];
                    i++;
                }
                exp_new[cnt] = '\0'; // add null character at the end

                free(exp_old);     // free the old expression
                exp_old = exp_new; // assign the new expression to old expression

                if (buf[res - 1] == '\0') // if null character is encountered in chunk, break
                {
                    break;
                }
            }

            cnt -= 1; // to ignore the last '\0' sent by client
            exp_new[cnt] = '\0';
            printf("The expression sent by client is %s and it's length is %d\n", exp_new, cnt);

            char *result = convert_number_to_string(evaluate_exp(exp_new));
            // printf("The size of result is %s\n", result);

            // send result in chunks
            i = 0;
            while (1)
            {
                char *chunk = (char *)malloc(send_chunk_size * sizeof(char));
                // send the result back to client/ chunk is the string to be sent
                int j = 0; // j is the index of chunk
                while (j < send_chunk_size && result[i] != '\0')
                {
                    chunk[j] = result[i];
                    i++;
                    j++;
                }
                // if less than chunk size then end of result size reached so append a null character
                if (j < send_chunk_size)
                {
                    // set the last chunk byte to null terminated
                    chunk[j] = '\0';
                    j += 1;
                    send(newsockfd, chunk, j, 0);
                    break;
                }
                else
                {
                    send(newsockfd, chunk, j, 0);
                }
                // free the chunk
                free(chunk);
            }
            free(exp_old); // free the old expression (automatically frees exp_new)
        }
    end:
        close(newsockfd);
    }

    return 0;
}