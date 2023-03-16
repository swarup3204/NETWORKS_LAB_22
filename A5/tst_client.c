#include <mysocket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

int main()
{
	int sockfd;
	struct sockaddr_in serv_addr;

	int i;
	char buf[5000];
	printf("Client started\n");

	/* Opening a socket is exactly similar to the server process */
	if ((sockfd = my_socket(AF_INET, SOCK_MyTCP, 0)) < 0)
	{
		perror("Unable to create socket\n");
		exit(-1);
	}
	printf("Socket created in client with sockfd = %d\n", sockfd);
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(20000);

	if ((my_connect(sockfd, (struct sockaddr *)&serv_addr,
					sizeof(serv_addr))) < 0)
	{
		perror("Unable to connect to server\n");
		exit(0);
	}
	printf("Connected to server\n");

	for (i = 0; i < 5000; i++)
		buf[i] = '\0';

	printf("Trying to receive from server\n");
	my_recv(sockfd, buf, 5000, 0);
	printf("%s\n", buf);

	for (i = 0; i < 20; i++)
	{
		strcpy(buf, "Hello I am a rogue !");
		my_send(sockfd, buf, strlen(buf) + 1, 0);
		sleep(1);
	}

	my_close(sockfd);
	return 0;
}
