#include "mysocket.h"

//*This is a producer consumer problem,you may need to use two semaphores for each table
//*even if entire message is not requested it will still be obtained and stored in table
void queue_push(Table *Tble, table_entry *entry)
{
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

	Tble->table_sz++;
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
	int sockfd = (*(int *)param);
	while (1)
	{
		pthread_mutex_lock(&recv_flags_mutex);
		table_entry *entry = (table_entry *)malloc(sizeof(table_entry));
		int res, i, j, got_len = 0, len;
		while (1)
		{
			int res = recv(sockfd, recv_buf, RECV_BUF_SZ, globl_recv_flags);
			if (got_len == 0)
			{
				//~find first space from start in message if found
				for (i = 0; i < res; i++)
				{
					if (recv_buf[i] == ' ')
					{
						got_len = 1;
						break;
					}
					entry->msg_len[i] = recv_buf[i];
				}
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
			if (j == len)
			{
				time(&entry->timestamp);
				//^Message read succesfully break
				break;
			}
		}
		pthread_mutex_unlock(&recv_flags_mutex);

		//&Add the message to RECEIVED MESSAGE TABLE
		pthread_mutex_lock(&Received_Message_Table->table_lck);
		while (!(Received_Message_Table->table_sz == TABLE_SZ))
		{
			sleep(2);
		}
		queue_push(Received_Message_Table, entry);
		pthread_cond_signal(&Received_Message_Table->table_cond);
		pthread_mutex_unlock(&Received_Message_Table->table_lck);

		//?Need to sleep
		//^sleep(5);
	}
}

void *send_thread(void *param)
{
	int sockfd = *((int *)param);
	//*sleeps,wakes up and sends a message in table
	int total_len, i, j;
	while (1)
	{
		pthread_mutex_lock(&Send_Message_Table->table_lck);
		while (!(Send_Message_Table->table_sz > 0))
		{
			sleep(2); //! will be replaced by conditional wait
		}
		table_entry *entry = queue_pop(Send_Message_Table);
		pthread_cond_signal(&Send_Message_Table->table_cond);
		pthread_mutex_unlock(&Send_Message_Table->table_lck);

		pthread_mutex_lock(&send_flags_mutex);
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
		while (total_len > 0)
		{
			int x = (total_len > SEND_BUF_SZ) ? SEND_BUF_SZ : total_len;
			send(sockfd, send_msg, x, globl_send_flags);
			send_msg += x;
			total_len -= x;
		}
		sleep(5); //*sleeps for 5 seconds
	}
}
void initialise_table(Table *Tble)
{
	Tble->head = NULL;
	Tble->tail = NULL;
	Tble->table_sz = 0;
	pthread_cond_init(&Tble->table_cond, NULL);
	//! Tble->table_cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_init(&Tble->table_lck, NULL);
	//! Tble->table_lck = PTHREAD_MUTEX_INITIALIZER;
}
int my_socket(int domain, int type, int protocol)
{
	if (type != SOCK_MyTCP)
	{
		// TODO set errno appropriately*/
		return -1;
	}
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		return sockfd;
	//&create threads and allocate space for two tables*/
	pthread_create(&tid_R, NULL, recv_thread, &sockfd);
	pthread_create(&tid_S, NULL, send_thread, &sockfd);
	initialise_table(Send_Message_Table);
	initialise_table(Received_Message_Table);
	pthread_mutex_init(&send_flags_mutex, NULL);
	pthread_mutex_init(&recv_flags_mutex, NULL);
	send_buf = (char *)malloc(sizeof(char) * SEND_BUF_SZ);
	recv_buf = (char *)malloc(sizeof(char) * RECV_BUF_SZ);
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
	int new_fd = accept(sockfd, addr, addrlen);
	return new_fd;
}
int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int status = connect(sockfd, addr, addrlen);
	return status;
}
int my_recv(int sockfd, void *buf, size_t len, int flags)
{
	//*retrieve
	table_entry *last_entry;
	pthread_mutex_lock(&Received_Message_Table->table_lck);
	while (Received_Message_Table->table_sz == 0)
	{
		pthread_cond_wait(&Received_Message_Table->table_cond, &Received_Message_Table->table_lck);
	}
	last_entry = queue_pop(Received_Message_Table);
	pthread_mutex_unlock(&Received_Message_Table->table_lck);
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
		fprintf(stderr, "Message length greater than %d\n", MAX_MSG_LEN);
		return -1;
	}
	int i;
	for (i = 0; i < len; i++)
		new_entry->msg[i] = *((char *)buf + i);
	sprintf(new_entry->msg_len, "%ld", len);
	time(&new_entry->timestamp);
	pthread_mutex_lock(&Send_Message_Table->table_lck);
	while (Send_Message_Table->table_sz > TABLE_SZ)
	{
		pthread_cond_wait(&Send_Message_Table->table_cond, &Send_Message_Table->table_lck);
	}
	//*push to queue
	queue_push(Send_Message_Table, new_entry);
	pthread_mutex_unlock(&Send_Message_Table->table_lck);
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
	pthread_cond_destroy(&Tble->table_cond);
	Tble->table_sz = 0;
}
int my_close(int sockd)
{
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
	pthread_kill(tid_R, SIGKILL); //?correct way to do this
	pthread_kill(tid_S, SIGKILL);
}