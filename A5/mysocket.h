#ifndef _MYSOCKET_H_
#define _MYSOCKET_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#define MAX_MSG_LEN 5000
#define MAX_MSG_DIGITS 4
#define SOCK_MyTCP 12 /* gave a new socket type number */
#define SEND_BUF_SZ 1000
#define RECV_BUF_SZ 50
#define TABLE_SZ 10

char *send_buf;
char *recv_buf;

typedef struct table_entry
{
	char msg[MAX_MSG_LEN];
	char msg_len[MAX_MSG_DIGITS + 1]; /* to be stored in characters */
	time_t timestamp;
	struct table_entry *next;
} table_entry;

typedef struct Table
{
	table_entry *head;
	table_entry *tail;
	pthread_mutex_t table_lck;
	pthread_cond_t table_cond;
	size_t table_sz;
} Table;

Table *Received_Message_Table;
Table *Send_Message_Table;

//& global since it is needed by both my_socket and my_close
pthread_t tid_R;
pthread_t tid_S;

/*
~The parameter of thread functions are passed only once,in such a scenario the flags for the recv and
~send call has to be updated from time to time,since it is accessed in both main and recv/send thread,we
~also need a lock to ensure mutual exclusion
*/
int globl_send_flags;
int globl_recv_flags;
pthread_mutex_t send_flags_mutex;
pthread_mutex_t recv_flags_mutex;


//! pthread_mutex_t recv_table;
//! pthread_mutex_t send_table;
//! pthread_cond_t recv_table_cond;
//! pthread_cond_t send_table_cond;


int my_socket(int domain, int protocol, int type);
int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int my_listen(int sockfd, int backlog);
int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int my_send(int sockfd, const void *buf, size_t len, int flags);
int my_recv(int sockfd, void *buf, size_t len, int flags);
int my_close(int fd);

void *recv_thread(void *param);
void *send_thread(void *param);

#endif