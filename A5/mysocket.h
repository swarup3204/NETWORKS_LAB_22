#ifndef _MYSOCKET_H_
#define _MYSOCKET_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>	
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>		
#define MAX_MSG_LEN 5000
#define MAX_MSG_DIGITS 4
#define SOCK_MyTCP 12 /* gave a new socket type number */
#define SEND_BUF_SZ 1000
#define RECV_BUF_SZ 50
#define TABLE_SZ 10


int my_socket(int domain, int protocol, int type);
int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int my_listen(int sockfd, int backlog);
int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int my_send(int sockfd, const void *buf, size_t len, int flags);
int my_recv(int sockfd, void *buf, size_t len, int flags);
int my_close(int fd);

#endif