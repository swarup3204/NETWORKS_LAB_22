#include <mysocket.h>

char *send_buf;
char *recv_buf;
int accept_cnt = 0;
int connect_flag = 0;

typedef struct table_entry
{
	char msg[MAX_MSG_LEN];
	char msg_len[MAX_MSG_DIGITS + 1];
	time_t timestamp;
	struct table_entry *next;
} table_entry;

typedef struct Table
{
	table_entry *head;
	table_entry *tail;
	pthread_mutex_t table_lck;
	pthread_cond_t table_full_cond;
	pthread_cond_t table_empty_cond;
	size_t table_sz;

} Table;

Table *Received_Message_Table;
Table *Send_Message_Table;

pthread_t tid_R;
pthread_t tid_S;

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

	int sockfd = *((int *)param);
	free(param);

	while (1)
	{

		table_entry *entry = (table_entry *)malloc(sizeof(table_entry));
		int res, i, j, got_len = 0, len;

		while (1)
		{
			int res = recv(sockfd, recv_buf, RECV_BUF_SZ, 0);

			if (res == 0)
			{
				continue;
			}
			if (got_len == 0)
			{

				for (i = 0; i < res; i++)
				{
					if (recv_buf[i] == ' ')
					{
						entry->msg_len[i] = '\0';
						i++;
						got_len = 1;
						break;
					}
					entry->msg_len[i] = recv_buf[i];
				}

				if(got_len == 0){
					continue;
				}
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

				break;
			}
		}

		pthread_mutex_lock(&Received_Message_Table->table_lck);

		while (Received_Message_Table->table_sz == TABLE_SZ)
		{
			pthread_cond_wait(&Received_Message_Table->table_empty_cond, &Received_Message_Table->table_lck);
		}

		queue_push(Received_Message_Table, entry);

		pthread_cond_signal(&Received_Message_Table->table_full_cond);

		pthread_mutex_unlock(&Received_Message_Table->table_lck);
	}
}

void *send_thread(void *param)
{
	int sockfd = *((int *)param);
	free(param);

	int total_len, i, j;
	while (1)
	{

		pthread_mutex_lock(&Send_Message_Table->table_lck);

		while (Send_Message_Table->table_sz == 0)
		{
			pthread_cond_wait(&Send_Message_Table->table_full_cond, &Send_Message_Table->table_lck);
		}

		table_entry *entry = queue_pop(Send_Message_Table);

		pthread_cond_signal(&Send_Message_Table->table_empty_cond);

		pthread_mutex_unlock(&Send_Message_Table->table_lck);

		total_len = strlen(entry->msg_len) + atoi(entry->msg_len) + 1;

		char *send_msg = malloc(sizeof(char) * (MAX_MSG_LEN + MAX_MSG_DIGITS));

		for (i = 0; i < strlen(entry->msg_len); i++)
		{
			send_msg[i] = entry->msg_len[i];
		}

		send_msg[i++] = ' ';

		j = 0;

		while (i < total_len && j < atoi(entry->msg_len))
		{
			send_msg[i++] = entry->msg[j++];
		}

		char *ptr = send_msg;

		while (total_len > 0)
		{
			int x = (total_len > SEND_BUF_SZ) ? SEND_BUF_SZ : total_len;

			send(sockfd, ptr, x, 0);

			ptr += x;

			total_len -= x;
		}

		free(send_msg);
	}
}
void initialise_table(Table **Tbl)
{
	Table *Tble = (Table *)malloc(sizeof(Table));
	Tble->head = NULL;
	Tble->tail = NULL;

	Tble->table_sz = 0;

	int x = pthread_cond_init(&Tble->table_full_cond, NULL);

	pthread_cond_init(&Tble->table_empty_cond, NULL);

	x = pthread_mutex_init(&Tble->table_lck, NULL);

	*Tbl = Tble;
}
int my_socket(int domain, int type, int protocol)
{
	if (type != SOCK_MyTCP)
	{
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
void create_threads(int sockfd)
{

	initialise_table(&Send_Message_Table);

	initialise_table(&Received_Message_Table);

	send_buf = (char *)malloc(sizeof(char) * SEND_BUF_SZ);
	recv_buf = (char *)malloc(sizeof(char) * RECV_BUF_SZ);

	int *ptr1 = malloc(sizeof(int));
	int *ptr2 = malloc(sizeof(int));
	*ptr1 = sockfd;
	*ptr2 = sockfd;
	pthread_create(&tid_R, NULL, recv_thread, (void *)ptr1);
	pthread_create(&tid_S, NULL, send_thread, (void *)ptr2);
}
int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{

	int new_fd = accept(sockfd, addr, addrlen);

	if (new_fd == -1)
		return new_fd;

	create_threads(new_fd);

	accept_cnt++;
	return new_fd;
}
int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int status = connect(sockfd, addr, addrlen);

	if (status == -1)
		return status;

	create_threads(sockfd);

	connect_cnt++;
	return status;
}
int my_recv(int sockfd, void *buf, size_t len, int flags)
{
	if (flags != 0)
	{
		errno = EINVAL;
		fprintf(stderr, "%s\n", strerror(errno));
		return -1;
	}
	int i, sz, mn;

	table_entry *last_entry;

	pthread_mutex_lock(&Received_Message_Table->table_lck);

	while (Received_Message_Table->table_sz == 0)
	{
		pthread_cond_wait(&Received_Message_Table->table_full_cond, &Received_Message_Table->table_lck);
	}
	last_entry = queue_pop(Received_Message_Table);

	pthread_cond_signal(&Received_Message_Table->table_empty_cond);

	pthread_mutex_unlock(&Received_Message_Table->table_lck);

	sz = atoi(last_entry->msg_len);

	mn = (len < sz) ? sz : len;

	for (i = 0; i < mn; i++)
	{
		*((char *)buf + i) = last_entry->msg[i];
	}
	for (; i < len; i++)
	{
		*((char *)buf + i) = '\0';
	}

	return (int)len;
}

int my_send(int sockfd, const void *buf, size_t len, int flags)
{
	int i, length;

	table_entry *new_entry = (table_entry *)malloc(sizeof(table_entry));

	if (len > MAX_MSG_LEN)
	{
		errno = ENOMEM;
		fprintf(stderr, "Message length greater than %d\n", MAX_MSG_LEN);
		return -1;
	}

	if (flags != 0)
	{
		errno = EINVAL;
		fprintf(stderr, "%s\n", strerror(errno));
		return -1;
	}

	for (i = 0; i < len; i++)
		new_entry->msg[i] = *((char *)buf + i);

	length = (int)len;

	sprintf(new_entry->msg_len, "%d", length);

	time(&new_entry->timestamp);

	pthread_mutex_lock(&Send_Message_Table->table_lck);

	while (Send_Message_Table->table_sz == TABLE_SZ)
	{
		pthread_cond_wait(&Send_Message_Table->table_empty_cond, &Send_Message_Table->table_lck);
	}

	queue_push(Send_Message_Table, new_entry);

	pthread_cond_signal(&Send_Message_Table->table_full_cond);

	pthread_mutex_unlock(&Send_Message_Table->table_lck);

	return length;
}

void uninitialise(Table **Tbl)
{
	Table *Tble = *Tbl;
	table_entry *temp;

	while (Tble->head != Tble->tail)
	{
		temp = Tble->head;
		Tble->head = Tble->head->next;
		free(temp);
	}

	free(Tble->head);

	Tble->head = Tble->tail = NULL;

	pthread_mutex_destroy(&Tble->table_lck);

	pthread_cond_destroy(&Tble->table_empty_cond);

	pthread_cond_destroy(&Tble->table_full_cond);

	Tble->table_sz = 0;

	free(Tble);

	*Tbl = NULL;
}
int my_close(int sockfd)
{

	// sleep(5);

	if (accept_cnt == 0 && connect_flag == 0)
	{
		int status = close(sockfd);
		return status;
	}

	pthread_cancel(tid_R);
	pthread_cancel(tid_S);

	free(send_buf);
	free(recv_buf);

	uninitialise(&Send_Message_Table);
	uninitialise(&Received_Message_Table);

	int status = close(sockfd);
	accept_cnt--;
	return status;
}