#include "mysocket.h"
//^Message format : < length of message in bytes [ 4 bytes ] > < space[ 1 byte ] > < message[ len bytes ] >

char *send_buf;
char *recv_buf;
int accept_cnt = 0;

//* A '0' suggests no accept is in progress
//*logic used:create threads for every new client and close them when client closes connection
//*the last my_close(sockfd) will do nothing since threads already closed
//*the threads are created on my_close and my_accept calls and the socket parameters are passed then and there
//*if a programmer forgets to do my_close(newsockfd),then the threads remain alive and will interfere in new threads operation

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
	pthread_cond_t table_full_cond;
	//&condition to wait on if table is full
	pthread_cond_t table_empty_cond;
	//&condition to wait on if table is empty
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
int globl_send_flags = 0;
int globl_recv_flags = 0;
pthread_mutex_t send_flags_mutex;
pthread_mutex_t recv_flags_mutex;

//*This is a producer consumer problem,you may need to use two semaphores for each table
//*even if entire message is not requested it will still be obtained and stored in table
//*go for deferred cancellation at my_close instead of pthread_kill()

void queue_push(Table *Tble, table_entry *entry)
{
	printf("Queue push function start\n");
	if (Tble->table_sz == 0)
	{
		Tble->head = entry;
		Tble->head->next = NULL;
		Tble->tail = Tble->head;
	}
	else
	{
		Tble->tail->next = entry;
		Tble->tail = entry;
		Tble->tail->next = NULL;
	}
	printf("Queue push function end\n");
	(Tble->table_sz)++;
}
table_entry *queue_pop(Table *Tble)
{
	table_entry *entry;
	if (Tble->table_sz == 0)
	{
		return NULL;
	}
	entry = Tble->head;
	if (Tble->table_sz == 1)
	{
		Tble->head = Tble->tail = NULL;
	}
	else
	{
		Tble->head = Tble->head->next;
	}
	Tble->table_sz--;
	return entry;
}
void *recv_thread(void *param)
{
	//*this thread receives and puts to RECEIVED_MESSAGE_TABLE
	int sockfd = (int)param;
	printf("Sockfd in recv_thread = %d\n", sockfd);
	while (1)
	{
		printf("Recv_hello1\n");
		// pthread_mutex_lock(&recv_flags_mutex);
		table_entry *entry = (table_entry *)malloc(sizeof(table_entry));
		int res, i, j, got_len = 0, len;
		//~got_len flag to check if we have got the length of message
		while (1)
		{
			int res = recv(sockfd, recv_buf, RECV_BUF_SZ, globl_recv_flags);
			printf("res in recv = %d\n", res);
			// print the error from errno
			if (res == -1)
			{
				printf("Error in recv = %s\n", strerror(errno));
				continue;
				// pthread_mutex_unlock(&recv_flags_mutex);
				// pthread_exit(NULL);
			}
			if (res == 0)
			{
				printf("Nothing to recv\n");
				continue; //& if no connection or closed
			}
			if (got_len == 0)
			{
				//~find first space from start in message if found
				for (i = 0; i < res; i++)
				{
					printf("Idhar hu\n");
					if (recv_buf[i] == ' ')
					{
						got_len = 1;
						break;
					}
					entry->msg_len[i] = recv_buf[i];
				}
				printf("Recv_hello2\n");
				entry->msg_len[i++] = '\0'; //*not only skipping ' ' but also adding '\0' to end of msg_len
				len = atoi(entry->msg_len);
				j = 0;
				while (j < len && i < res)
				{
					entry->msg[j++] = recv_buf[i++];
				}
			}
			else
			{
				i = 0;
				while (j < len && i < res)
				{
					entry->msg[j++] = recv_buf[i++];
				}
			}
			printf("Recv_hello3\n");
			if (j == len)
			{
				time(&entry->timestamp);
				//^Message read succesfully break
				break;
			}
		}
		printf("Recv_hello4\n");
		// pthread_mutex_unlock(&recv_flags_mutex);

		//&Add the message to RECEIVED MESSAGE TABLE
		//pthread_mutex_lock(&Received_Message_Table->table_lck);
		while (Received_Message_Table->table_sz == TABLE_SZ);
		{
			//pthread_cond_wait(&Received_Message_Table->table_empty_cond, &Received_Message_Table->table_lck);
		}
		queue_push(Received_Message_Table, entry);
		//pthread_cond_signal(&Received_Message_Table->table_full_cond);
		//pthread_mutex_unlock(&Received_Message_Table->table_lck);

		//?Need to sleep
		//^sleep(5);
	}
}

void *send_thread(void *param)
{
	int sockfd = (int)param;
	printf("Sockfd in send_thread = %d\n", sockfd);
	//*sleeps,wakes up and sends a message in table
	int total_len, i, j;
	while (1)
	{
		printf("Send_hello1\n");
		// pthread_mutex_lock(&Send_Message_Table->table_lck);
		// printf("x = %d\n", x);
		printf("Locked in send thread\n");
		while (!(Send_Message_Table->table_sz > 0))
			;
		{
			// pthread_cond_wait(&Send_Message_Table->table_full_cond, &Send_Message_Table->table_lck);
		}
		printf("Waiting over\n");
		table_entry *entry = queue_pop(Send_Message_Table);
		// pthread_cond_signal(&Send_Message_Table->table_empty_cond);
		// pthread_mutex_unlock(&Send_Message_Table->table_lck);
		printf("Send_hello2\n");
		// pthread_mutex_lock(&send_flags_mutex);
		//~construct message in appropriate format to send

		total_len = strlen(entry->msg_len) + atoi(entry->msg_len) + 1;
		char *send_msg = malloc(sizeof(char) * (MAX_MSG_LEN + MAX_MSG_DIGITS));
		//^my program semantics requires this re-definition notwithstanding
		for (i = 0; i < strlen(entry->msg_len); i++)
		{
			send_msg[i] = entry->msg_len[i];
		}
		send_msg[i++] = ' ';
		j = 0;
		while (i < total_len && /*redundant but still*/ j < atoi(entry->msg_len))
		{
			send_msg[i++] = entry->msg[j++];
		}
		send_msg[i] = '\0'; //^added this line,appropriately change constraints
		char *ptr = send_msg;
		while (total_len > 0)
		{
			int x = (total_len > SEND_BUF_SZ) ? SEND_BUF_SZ : total_len;
			send(sockfd, ptr, x, globl_send_flags);
			ptr += x;
			total_len -= x;
		}
		printf("Send_hello3\n");
		printf("Message sent: %s\n", send_msg);
		free(send_msg);
		sleep(5); //*sleeps for 5 seconds
	}
}
void initialise_table(Table *Tble)
{
	Tble = (Table *)malloc(sizeof(Table));
	Tble->head = NULL;
	Tble->tail = NULL;
	//! printf("ok\n");
	Tble->table_sz = 0;
	//! printf("ok1\n");
	int x = pthread_cond_init(&Tble->table_full_cond, NULL);
	printf("Cond initialisation %d\n", x);
	pthread_cond_init(&Tble->table_empty_cond, NULL);
	//! Tble->table_cond = PTHREAD_COND_INITIALIZER;
	x = pthread_mutex_init(&Tble->table_lck, NULL);
	printf("Mutex initialisation %d\n", x);
	//! Tble->table_lck = PTHREAD_MUTEX_INITIALIZER;
}
int my_socket(int domain, int type, int protocol)
{
	if (type != SOCK_MyTCP)
	{
		// TODO set errno appropriately
		printf("Hello\n");
		return -1;
	}
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	return sockfd;
}
int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int status = bind(sockfd, addr, addrlen);
	return status;
}
int my_listen(int sockfd, int backlog)
{
	int status = listen(sockfd, backlog);
	return status;
}
int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	printf("Hello\n");
	int new_fd = accept(sockfd, addr, addrlen);
	if (new_fd == -1)
		return new_fd;
	//&create threads and allocate space for two tables*/
	//! printf("Hello1\n");
	initialise_table(Send_Message_Table);
	initialise_table(Received_Message_Table);
	//! printf("Hello2\n");
	send_buf = (char *)malloc(sizeof(char) * SEND_BUF_SZ);
	recv_buf = (char *)malloc(sizeof(char) * RECV_BUF_SZ);
	//! printf("Hello4\n");
	pthread_mutex_init(&send_flags_mutex, NULL);
	pthread_mutex_init(&recv_flags_mutex, NULL);
	//! printf("Hello3\n");
	pthread_create(&tid_R, NULL, recv_thread, (void *)new_fd);
	pthread_create(&tid_S, NULL, send_thread, (void *)new_fd);
	accept_cnt++;
	return new_fd;
}
int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int status = connect(sockfd, addr, addrlen);
	if (status == -1)
		return status;
	//&create threads and allocate space for two tables*/
	//! printf("Hello1\n");
	initialise_table(Send_Message_Table);
	initialise_table(Received_Message_Table);
	//! printf("Hello2\n");
	send_buf = (char *)malloc(sizeof(char) * SEND_BUF_SZ);
	recv_buf = (char *)malloc(sizeof(char) * RECV_BUF_SZ);
	//! printf("Hello3\n");
	pthread_create(&tid_R, NULL, recv_thread, (void *)sockfd);
	pthread_create(&tid_S, NULL, send_thread, (void *)sockfd);
	//! printf("Hello4\n");
	pthread_mutex_init(&send_flags_mutex, NULL);
	pthread_mutex_init(&recv_flags_mutex, NULL);
	return status;
}
int my_recv(int sockfd, void *buf, size_t len, int flags)
{
	//*retrieve
	table_entry *last_entry;
	//pthread_mutex_lock(&Received_Message_Table->table_lck);
	while (Received_Message_Table->table_sz == 0);
	{
		//pthread_cond_wait(&Received_Message_Table->table_full_cond, &Received_Message_Table->table_lck);
	}
	last_entry = queue_pop(Received_Message_Table);
	//pthread_cond_signal(&Received_Message_Table->table_empty_cond);
	//pthread_mutex_unlock(&Received_Message_Table->table_lck);
	int i;
	int sz = atoi(last_entry->msg_len);
	char *tmp = (char *)malloc(sizeof(void) * len);
	////int mx = (len > sz) ? len : sz;
	int mn = (len < sz) ? sz : len;
	for (i = 0; i < mn; i++)
	{
		tmp[i] = last_entry->msg[i];
	}
	for (; i < len; i++)
	{
		tmp[i] = '\0';
	}
	buf = tmp;
	return (int)len;
}

int my_send(int sockfd, const void *buf, size_t len, int flags)
{
	table_entry *new_entry = (table_entry *)malloc(sizeof(table_entry));
	if (len > MAX_MSG_LEN)
	{
		// TODO set errno appropriately
		errno = E2BIG;
		fprintf(stderr, "Message length greater than %d\n", MAX_MSG_LEN);
		return -1;
	}
	printf("Constructing new entry in my_send\n");
	int i;
	for (i = 0; i < len; i++)
		new_entry->msg[i] = *((char *)buf + i);
	int length = (int)len;
	printf("Now here\n");
	sprintf(new_entry->msg_len, "%d", length);
	time(&new_entry->timestamp);
	printf("Constructed new entry in my_send,msg = %s,msg_len = %s\n", new_entry->msg, new_entry->msg_len);
	// sleep(2);
	// pthread_mutex_lock(&Send_Message_Table->table_lck);
	printf("Locked in my_send\n");
	while (Send_Message_Table->table_sz == TABLE_SZ);
	{
		// pthread_cond_wait(&Send_Message_Table->table_empty_cond, &Send_Message_Table->table_lck);
	}
	//*push to queue
	printf("Pushing to queue in my_send\n");
	queue_push(Send_Message_Table, new_entry);
	printf("Pushed to queue in my_send\n");
	// pthread_cond_signal(&Send_Message_Table->table_full_cond);
	// pthread_mutex_unlock(&Send_Message_Table->table_lck);
	return (int)len;
}
// TODO clear table function
void uninitialise(Table *Tble)
{
	//! start from head to tail and free individual elements
	table_entry *temp;
	// temp = Tble->head;
	while (Tble->head != Tble->tail)
	{
		temp = Tble->head;
		Tble->head = Tble->head->next;
		free(temp->next);
		free(temp);
	}
	free(Tble->head);
	Tble->head = Tble->tail = NULL;
	pthread_mutex_destroy(&Tble->table_lck);
	pthread_cond_destroy(&Tble->table_empty_cond);
	pthread_cond_destroy(&Tble->table_full_cond);
	Tble->table_sz = 0;
}
int my_close(int sockfd)
{
	if (accept_cnt == 0)
	{
		//&no threads spawned,nothing allocated therefore
		int status = close(sockfd);
		return status;
	}
	//&free all pointers
	free(send_buf);
	free(recv_buf);
	//&destroy all mutexes
	pthread_mutex_destroy(&recv_flags_mutex);
	pthread_mutex_destroy(&send_flags_mutex);
	//&destroy tables
	uninitialise(Send_Message_Table);
	uninitialise(Received_Message_Table);
	//&kill all threads
	//! deferred cancellation
	//^recv is a cancellation point,so should be fine in this setup
	pthread_cancel(tid_R);
	pthread_cancel(tid_S);
	// pthread_kill(tid_R, SIGKILL); //?correct way to do this
	// pthread_kill(tid_S, SIGKILL);
	int status = close(sockfd);
	accept_cnt--;
	return status;
}