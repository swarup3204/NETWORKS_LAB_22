#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define ECHO_REQUEST 1
#define ECHO_REPLY 2
#define TIME_EXCEEDED 3
#define SUCCESS 0
#define FAILURE -1
// max packets to send to confirm a next hop in a route
#define MAX_FIND_HOP_TRIES 100
#define PORT_NO 0
// port no of servers to connect to
#define RECV_TIMEOUT 10
// timeout for recvfrom() in seconds
#define ICMP_PKT_SIZE 64
#define IP_PKT_SIZE 128
#define SLEEP_RATE 100
// a global variable to store the destination ip address
char *dest_ip;
// a global variable to send a non icmp udp packet,so that ttl = 0
int udp_sockfd;
// a global variable to store the socket file descriptor
int sockfd;
// a global variable to store the destination address in sockaddr_in format
struct sockaddr_in *dest_addr_con;

struct icmp_pkt
{
	struct icmphdr header;
	char message[ICMP_PKT_SIZE - sizeof(struct icmphdr)];
};

struct ip_pkt
{
	struct iphdr header;
	struct icmp_pkt icmp;
	char message[IP_PKT_SIZE - sizeof(struct iphdr) - sizeof(struct icmphdr)];
};
int parse_icmp_packet(struct icmphdr *icmp)
{
	// there are 3 types we care about:
	// ICMP_ECHOREPLY
	// ICMP_TIME_EXCEEDED
	// ICMP_ECHOREQUEST
	// check the type and return the appropriate value
	switch (icmp->type)
	{
	case ICMP_ECHOREPLY:
		return ECHO_REPLY;
	case ICMP_TIME_EXCEEDED:
		return TIME_EXCEEDED;
	case ICMP_ECHO:
		return ECHO_REQUEST;
	default:
		return FAILURE;
	}
}
int lookup_dns_and_assign(char *addr_host)
{
	printf("Resolving DNS\n");
	struct hostent *host_entity;
	if ((host_entity = gethostbyname(addr_host)) == NULL)
	{
		// No ip found for hostname
		return FAILURE;
	}

	dest_ip = (char *)malloc(sizeof(host_entity->h_addr_list[0]));
	strcpy(dest_ip, inet_ntoa(*(struct in_addr *)host_entity->h_addr_list[0]));
	// filling up dest_addr
	// memset(dest_addr_con, 0, sizeof(*dest_addr_con));
	dest_addr_con->sin_family = host_entity->h_addrtype;
	dest_addr_con->sin_port = htons(PORT_NO);
	dest_addr_con->sin_addr.s_addr = *(unsigned int *)host_entity->h_addr_list[0];

	// printf("%s resolved to %s\n", addr_host, dest_ip);
	return SUCCESS;
}
char *reverse_dns_lookup()
{
}
void compute_latency()
{
}
void print_tcp_header(struct tcphdr *header)
{
}
void print_udp_header()
{
}
void print_icmp_header()
{
}
void print_ip_header()
{
}
unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char *)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}
void establish_link(int ttl)
{
	struct timeval timeout;
	timeout.tv_sec = RECV_TIMEOUT;
	timeout.tv_usec = 0;

	struct icmp_pkt send_packet;
	struct ip_pkt recv_packet;

	struct sockaddr_in recv_addr;
	socklen_t addr_len;

	char *next_hop_ip;

	int flag = 0;

	// set only ttl of ip header
	if (setsockopt(sockfd, SOL_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
	{
		printf("Error: Could not set socket options!\n");
		exit(EXIT_FAILURE);
	}
	// set a timeout - this allows us to bypass the use of poll() or select()
	// if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
	// {
	// 	printf("Error: Could not set socket options!\n");
	// 	exit(EXIT_FAILURE);
	// }

	// printf("Calculated checksum is %d\n", send_packet.header.checksum);

	flag = 0;
	for (int i = 0; i < 5; i++)
	{ // try 5 times to get icmp time limit exceeded
		// first send the send_packet with nothing in icmp
		bzero(&send_packet, sizeof(send_packet));

		// fill the send_packet with '0'
		memset(&send_packet, 0, sizeof(send_packet));

		// create icmp echo request,it may be dropped,but it will return another icmp send_packet
		send_packet.header.type = ICMP_ECHO;
		send_packet.header.un.echo.id = getpid();
		send_packet.header.un.echo.sequence = 0;

		for (int i = 0; i < sizeof(send_packet.message) - 1; i++)
		{
			send_packet.message[i] = i + '0';
		}

		send_packet.header.checksum = checksum(&send_packet, sizeof(send_packet));

		if (sendto(sockfd, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)dest_addr_con, sizeof(*dest_addr_con)) < 0)
		{
			printf("Error: Could not send ttl=1 packet!\n");
			continue;
		}
		// wait for an icmp send_packet to arrive with time limit exceeded
		addr_len = sizeof(struct sockaddr_in);
		if (recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&recv_addr, &addr_len) < 0)
		{
			printf("Error: Could not receive tle icmp send_packet!\n");
			continue;
		}
		else
		{
			printf("Received a packet on sending ttl = %d \n", ttl);
			flag = 1; // received a packet
			break;
		}
		usleep(SLEEP_RATE);
	}

	if (flag == 1)
	{
		// check if packet received is icmp time limit exceeded
		if (parse_icmp_packet(&recv_packet.icmp.header) == TIME_EXCEEDED)
		{
			// check if the ip header of the packet received is the same as the ip header of the packet sent
			// store ip address of the packet received
			next_hop_ip = inet_ntoa(recv_addr.sin_addr);
			// confirm this by sending echo requests
			// if 5 success, confirm this as next hop
			// else print cannot establish next hop
			int echo_ttl = 100;
			if (setsockopt(sockfd, SOL_IP, IP_TTL, &echo_ttl, sizeof(echo_ttl)) < 0)
			{
				printf("Error: Could not set socket options!\n");
				exit(EXIT_FAILURE);
			}
			int success_replies = 0;
			for (int i = 1; i <= MAX_FIND_HOP_TRIES && success_replies < 5; ++i)
			{
				// send echo request

				bzero(&send_packet, sizeof(send_packet));
				send_packet.header.type = ICMP_ECHO;
				send_packet.header.un.echo.id = getpid();
				send_packet.header.un.echo.sequence = i;
				send_packet.header.checksum = checksum(&send_packet, sizeof(send_packet));
				if (sendto(sockfd, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)dest_addr_con, sizeof(*dest_addr_con)) < 0)
				{
					printf("Error: Could not send packet!\n");
					continue;
				}
				// wait for an icmp packet to arrive with echo request
				addr_len = sizeof(struct sockaddr_in);
				if (recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&recv_addr, &addr_len) < 0)
				{
					printf("Error: Could not receive packet!\n");
					continue;
				}
				//! store packets in an array
				//! packets may arrive out of order
				//! packets may be non-consecutive
				//! packets may be lost
				//! packets may be duplicated
				//! packets may be delayed
				//! packets may be corrupted
				//! at present naively assuming it comes as soon as u send
				else
				{
					printf("Packet type received for echo request %d\n", parse_icmp_packet(&recv_packet.icmp.header));
					// check if packet received is icmp echo reply
					if (parse_icmp_packet(&recv_packet.icmp.header) == ECHO_REPLY)
					{
						// check if the echo reply is for the same packet you sent as echo request
						success_replies++;
					}
				}

				usleep(SLEEP_RATE);
			}

			if (success_replies == 5)
			{
				printf("Next hop ip: %s\n", next_hop_ip);
				printf("Estimating latency of link\n");
			}
			else
			{
				printf("Could not receive 5 consecutive echo replies, Received icmp packet type, Cannot establish next hop\n");
			}
		}
		else
		{
			printf("Could not receive icmp time exceeded, Received icmp packet type: %d\n", recv_packet.icmp.header.type);
			// if echo reply received, next hop is the same as the destination
			// else print cannot establish next hop
			if (parse_icmp_packet(&recv_packet.icmp.header) == ECHO_REPLY)
			{
				printf("Next hop ip: %s\n", inet_ntoa(dest_addr_con->sin_addr));
				printf("Estimating latency of link\n");
			}
			else
			{
				printf("Cannot establish next hop\n");
			}
		}
	}
}
// void find_latency()
// {
// }
int main(int argc, char *argv[])
{
	// arguments 3:
	// 1.site to probe
	// 2.number of probes per link
	// 3.time difference between any two probes
	dest_addr_con = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	if (argc != 4)
	{
		printf("Error: Usage: %s <site> <number of probes per link> <time difference between any two probes in seconds> \n", argv[0]);
		exit(EXIT_FAILURE);
	}
	// check if site to probe is a name or not

	if (isalpha(argv[1][0]))
	{
		// get the ip address from lookup_dns()
		if (lookup_dns_and_assign(argv[1]) == FAILURE)
		{
			printf("DNS lookup failed!Could not resolve hostname!\n");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		// store the ip address in a global variable
		dest_ip = (char *)malloc(sizeof(argv[1]));
		// copy the ip address
		strcat(dest_ip, argv[1]);
	}

	// debug statement
	printf("The ip address of destination is %s\n", dest_ip);

	// create a socket
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	// check if socket creation was successful
	if (sockfd < 0)
	{
		printf("Error: Could not create socket!\n");
		exit(EXIT_FAILURE);
	}
	// set the socket options
	// int optval = 1;
	// if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval)) < 0)
	// {
	// 	printf("Error: Could not set socket options!\n");
	// 	exit(EXIT_FAILURE);
	// }
	int ttl = 1;
	while (1)
	{
		// establish links /next hops,also find latency of link
		// find_latency();
		establish_link(ttl);
		ttl++;
	}
}