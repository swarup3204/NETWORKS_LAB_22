#include <netinet/in.h>
#include <arpa/inet.h>
#include <mysocket.h>
#include <string.h>

int main()
{
	int sockfd, newsockfd; /* Socket descriptors */
	int clilen;
	struct sockaddr_in cli_addr, serv_addr;

	int i;
	char buf[100]; /* We will use this buffer for communication */

	printf("Server started\n");
	// sockfd = my_socket(AF_INET, SOCK_MyTCP, 0);
	if ((sockfd = my_socket(AF_INET, SOCK_MyTCP, 0)) < 0)
	{
		perror("Cannot create socket\n");
		exit(-1);
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(20001);
	printf("Socket created in server with sockfd = %d\n", sockfd);
	if (my_bind(sockfd, (struct sockaddr *)&serv_addr,
				sizeof(serv_addr)) < 0)
	{
		perror("Unable to bind local address\n");
		exit(0);
	}
	printf("Bind successful\n");
	my_listen(sockfd, 5);
	printf("Server listening\n");
	while (1)
	{
		clilen = sizeof(cli_addr);
		newsockfd = my_accept(sockfd, (struct sockaddr *)&cli_addr,
							  &clilen);
		printf("Connection accepted from %s:%d\n",
			   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

		if (newsockfd < 0)
		{
			perror("Accept error\n");
			exit(0);
		}

		// scan uptil linefeed
		scanf(" %[^\n]s", buf);

		printf("Sending %s to client of length %ld \n", buf, strlen(buf));
		my_send(newsockfd, buf, strlen(buf) + 1, 0);
		printf("Sent %s to client\n", buf);
		my_recv(newsockfd, buf, 100, 0);
		printf("Received %s from client\n", buf);

		my_close(newsockfd);
	}
	my_close(sockfd);
	return 0;
}
