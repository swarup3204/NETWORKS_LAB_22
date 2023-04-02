#include "pingnetinfo.h"

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

	dest_addr_con->sin_family = host_entity->h_addrtype;
	dest_addr_con->sin_port = htons(PORT_NO);
	dest_addr_con->sin_addr.s_addr = *(unsigned int *)host_entity->h_addr_list[0];

	return SUCCESS;
}
char *reverse_dns_lookup()
{
}
void estimate_latency(int hop)
{
	// estimatate latency betweeen two adjacent ip addressess in a route,where hop is the number of hops uptil now
	// send date packets of different sizes and calculate the time difference and then calculate the latency and bandwidth
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
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0)
	{
		printf("Error: Could not set socket options!\n");
		exit(EXIT_FAILURE);
	}

	// printf("Calculated checksum is %d\n", send_packet.header.checksum);

	flag = 0;
	for (int i = 0; i < 5; i++)
	{
		// try 5 times to get icmp time limit exceeded sending echo request packets with apt ttl
		bzero(&send_packet, sizeof(send_packet));
		// initialize the send_packet with '\0'

		memset(&send_packet, 0, sizeof(send_packet));

		// create icmp echo request,it may be dropped,but it will return another icmp send_packet
		send_packet.header.type = ICMP_ECHO;
		send_packet.header.un.echo.id = getpid();
		send_packet.header.un.echo.sequence = 0;
		

		for (int i = 0; i < sizeof(send_packet.message) - 1; i++)
		{
			send_packet.message[i] = i + '0'; // random data
		}

		send_packet.header.checksum = checksum(&send_packet, sizeof(send_packet));

		if (sendto(sockfd, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)dest_addr_con, sizeof(*dest_addr_con)) < 0)
		{
			printf("Error: Could not send ttl=%d packet!\n", ttl);
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
			printf("Received a packet on sending ttl = %d and packet type %d\n", ttl, recv_packet.icmp.header.type);
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
			struct sockaddr_in next_hop_addr;
			next_hop_addr.sin_family = AF_INET;
			next_hop_addr.sin_port = htons(PORT_NO);
			next_hop_addr.sin_addr.s_addr = inet_addr(next_hop_ip);

			int success_replies = 0;
			for (int i = 1; i <= MAX_FIND_HOP_TRIES && success_replies < 5; ++i)
			{
				// send echo request

				bzero(&send_packet, sizeof(send_packet));
				send_packet.header.type = ICMP_ECHO;
				send_packet.header.un.echo.id = getpid();
				send_packet.header.un.echo.sequence = i;
				send_packet.header.checksum = checksum(&send_packet, sizeof(send_packet));
				if (sendto(sockfd, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)&next_hop_addr, sizeof(next_hop_addr)) < 0)
				{
					printf("Error: Could not send echo request!\n");
					continue;
				}
				// wait for an icmp packet to arrive with echo request
				addr_len = sizeof(struct sockaddr_in);
				if (recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&recv_addr, &addr_len) < 0)
				{
					printf("Error: Could not receive echo reply!\n");
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
					printf("Packet type received for echo request: %d\n", parse_icmp_packet(&recv_packet.icmp.header));
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
				printf("Could not receive 5 consecutive echo replies, Cannot establish next hop\n");
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
	else
	{
		printf("Could not receive any icmp packet, Cannot establish next hop\n");
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

	num_probes = atoi(argv[2]);
	time_diff = atoi(argv[3]);

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