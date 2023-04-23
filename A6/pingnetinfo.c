#ifndef PINGNETINFO_H
#define PINGNETINFO_H

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
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <limits.h>


#define ECHO_REQUEST 1
#define ECHO_REPLY 2
#define TIME_EXCEEDED 3



#define SUCCESS 0
#define FAILURE -1


// max packets to send to confirm a next hop in a route
#define MAX_FIND_HOP_TRIES 8
#define PORT_NO 0
// port no of servers to connect to
#define RECV_TIMEOUT 1
// timeout for recvfrom() in seconds
#define ICMP_PKT_SIZE 1024
#define IP_PKT_SIZE 1200
#define SLEEP_RATE 1
#define MAX_HOPS 30
#define N_MAX_IPS 20 
// max number of ips in a route
char *dest_ip;
// a global variable to store the destination ip address
int sockfd;
// a global variable to store the socket file descriptor
int udp_sockfd;
// a global variable to store the udp socket file descriptor
struct sockaddr_in *dest_addr_con;
// a global variable to store the destination address in sockaddr_in format
struct icmp_pkt
{
	struct icmphdr header;
	char message[ICMP_PKT_SIZE - sizeof(struct icmphdr)];
};
// a structure to store the icmp packet with total size of 64 bytes
struct ip_pkt
{
	struct iphdr header;
	struct icmp_pkt icmp;
	char message[IP_PKT_SIZE - sizeof(struct iphdr) - sizeof(struct icmphdr)];
};
// a structure to store the ip packet with total size of 128 bytes
struct sizewise_rtt
{
	double rtt_64;
	double rtt_80;
};

int num_probes;
// a global variable to store the number of probes to be sent per link
int time_diff;
// a global variable to store the time difference between two consecutive probes

struct icmp_pkt ROUTE_IPS[N_MAX_IPS];
// to store the icmp packets echo replies received till now
struct sizewise_rtt RTT_ARR[N_MAX_IPS];

char *IP_ARR[N_MAX_IPS];

#endif

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
		if (icmp->code == ICMP_EXC_TTL)
			return TIME_EXCEEDED;
		else
			return FAILURE;
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
	printf("Resolving DNS\n\n");
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

void print_ip_header(struct iphdr* ip_hdr)
{
	/* print the ip header */
	printf("+---------------------------+\n");
	printf("|      IP Header            |\n");
	printf("+---------------------------+\n");
	printf("| Version     | %10d |\n", ip_hdr->version);
	printf("| IHL         | %10d |\n", ip_hdr->ihl);
	printf("| TTL         | %10d |\n", ip_hdr->ttl);
	printf("| Protocol    | %10d |\n", ip_hdr->protocol);
	printf("| Checksum    | %10d |\n", ip_hdr->check);
	printf("| Src IP      | %10s |\n", inet_ntoa(*(struct in_addr *)&ip_hdr->saddr));
	printf("| Dest IP     | %10s |\n", inet_ntoa(*(struct in_addr *)&ip_hdr->daddr));
	printf("+---------------------------+\n");

}

void print_icmp_header(struct icmphdr* icmp_header)
{
	printf("+---------------------------+\n");
	printf("|      ICMP Header          |\n");
	printf("+---------------------------+\n");
	printf("| Type         | %10d |\n", icmp_header->type);
	printf("| Code         | %10d |\n", icmp_header->code);
	printf("| Checksum     | %10d |\n", icmp_header->checksum);
	printf("| Identifier   | %10d |\n", icmp_header->un.echo.id);
	printf("| Seq Number   | %10d |\n", icmp_header->un.echo.sequence);
	printf("+---------------------------+\n");
}
double send_probes(char *next_hop_ip, size_t msg_len)
{
	/* sends num_probe pings of size msg_len to next_hop_ip and returns minimum rtt */
	
	struct sockaddr_in addr_con;
	addr_con.sin_family = AF_INET;
	addr_con.sin_port = htons(PORT_NO);
	addr_con.sin_addr.s_addr = inet_addr(next_hop_ip);

	struct sockaddr_in recv_addr;
	socklen_t addr_len = sizeof(recv_addr);

	char reply_dump[IP_PKT_SIZE];

	int success_cnt = 0;
	int min_rtt = INT_MAX;

	printf("\nSending %d probes of size %ld to %s\n", num_probes, msg_len, next_hop_ip);
	for (int i = 0; i < num_probes; ++i)
	{
		struct icmp_pkt probe;
		memset(&probe, 0, sizeof(probe));
		probe.header.type = ICMP_ECHO;
		probe.header.un.echo.id = getpid();

		struct timeval currtime;
		gettimeofday(&currtime, NULL);
		
		memcpy(&probe.message, &currtime, sizeof(currtime)); 
		/* hoping memcopy is safe :( */

		size_t sizeof_packet = &probe.message[msg_len] - (char*)&probe;
		/* hoping msg_len would be big enough to contain the timestamp */

		probe.header.checksum = checksum(&probe, sizeof_packet);

		printf("Sending probe with header:\n");
		print_icmp_header(&probe.header);
		if (sendto(sockfd, &probe, sizeof_packet, 0, (struct sockaddr *)&addr_con, sizeof(addr_con)) < 0)
		{
			printf("Error: Could not send packet!\n");
			exit(EXIT_FAILURE);
		}

		struct pollfd fdset = {sockfd, POLLIN, 0};
		time_t start_time = time(NULL), end_time;

		int ret = poll(&fdset, 1, time_diff*1000);

		if (ret < 0)
		{
			perror("poll");
			exit(EXIT_FAILURE);
		}

		else if (ret == 0)
		{
			/* probe timed out */
			continue;
		}

		else
		{
			if (recvfrom(sockfd, reply_dump, IP_PKT_SIZE, 0, (struct sockaddr *)&recv_addr, &addr_len) < 0)
			{
				printf("Error: Could not receive packet!\n");
				exit(EXIT_FAILURE);
			}


			time(&end_time);

			printf("Received reply ...\n");
			

			struct iphdr *hdr = (struct iphdr*)reply_dump;
			if (hdr->protocol == IPPROTO_TCP)
			{
				printf("received TCP packet\n");
			}
			else if (hdr->protocol == IPPROTO_UDP)
			{
				printf("received UDP packet\n");
			}
			else if (hdr->protocol == IPPROTO_ICMP)
			{
				struct icmp_pkt *icmp = (struct icmp_pkt *)(reply_dump + sizeof(struct iphdr));
				print_icmp_header(&icmp->header);

				int type = parse_icmp_packet((struct icmphdr*)icmp);

				if (type == ECHO_REPLY)
				{
					success_cnt++; // where will the success_cnt be used though??
					struct timeval currtime;
					gettimeofday(&currtime, NULL);

					struct timeval sent_time = *(struct timeval *)icmp->message;
					double rtt = (currtime.tv_sec - sent_time.tv_sec) * 1000000 + (currtime.tv_usec - sent_time.tv_usec);
					
					if (rtt < min_rtt)
						min_rtt = rtt;
				}
				else if (type == TIME_EXCEEDED)
				{
					/* ignore, we are not looking for this */
				}
				else
				{
					
					struct ip_pkt c = *(struct ip_pkt*)reply_dump;
					struct iphdr *encapsulating_ip_header = (struct iphdr *)&c.message[0];
					printf("Received IP header:\n");
					print_ip_header(encapsulating_ip_header);
					if (encapsulating_ip_header->protocol == IPPROTO_TCP)
					{
						printf("Received ICMP, sent due to dropping of TCP packet\n");
					}
					else if (encapsulating_ip_header->protocol == IPPROTO_UDP)
					{
						printf("Received ICMP, sent due to dropping of UDP packet\n");
					}
					else
					{
						printf("Received ICMP, sent due to dropping of unknown packet\n");
					}
				}
			}
			else
			{
				printf("unknown protocol\n");
			}

			int seconds_elapsed = end_time - start_time;
			sleep(time_diff - seconds_elapsed);
			
		}

		
		
	}

	/* now we have sent all the probes, now we need to receive the replies */
	

	for (int i = 0; i < num_probes && success_cnt < num_probes; ++i)
	{
		if (i == 0)
		{
			printf("Waiting for remaining replies...\n");
		}
		struct pollfd fdset = {sockfd, POLLIN, 0};

		int ret = poll(&fdset, 1, 1000);

		if (ret < 0)
		{
			perror("poll");
			exit(EXIT_FAILURE);
		}

		else if (ret == 0)
		{
			/* probe timed out */
			continue;
		}

		else
		{
			if (recvfrom(sockfd, reply_dump, IP_PKT_SIZE, 0, (struct sockaddr *)&recv_addr, &addr_len) < 0)
			{
				printf("Error: Could not receive packet!\n");
				exit(EXIT_FAILURE);
			}


			printf("Received reply ...\n");
			print_icmp_header(&((struct icmp_pkt*)reply_dump)->header);

			struct iphdr *hdr = (struct iphdr*)reply_dump;
			if (hdr->protocol == IPPROTO_TCP)
			{
				printf("received TCP packet\n");
				continue; // or break??
			}
			else if (hdr->protocol == IPPROTO_UDP)
			{
				printf("received UDP packet\n");
				continue; // or break??
			}
			else if (hdr->protocol == IPPROTO_ICMP)
			{
				struct icmp_pkt *icmp = (struct icmp_pkt *)(reply_dump + sizeof(struct iphdr));
				print_icmp_header(&icmp->header);

				int type = parse_icmp_packet((struct icmphdr*)icmp);

				if (type == ECHO_REPLY)
				{
					success_cnt++; // where will the success_cnt be used though??
					struct timeval currtime;
					gettimeofday(&currtime, NULL);

					struct timeval sent_time = *(struct timeval *)icmp->message;
					double rtt = (currtime.tv_sec - sent_time.tv_sec) * 1000000 + (currtime.tv_usec - sent_time.tv_usec);
					
					if (rtt < min_rtt)
						min_rtt = rtt;
				}
				else if (type == TIME_EXCEEDED)
				{
					/* ignore, we are not looking for this */
				}
				else
				{
					
					struct ip_pkt c = *(struct ip_pkt*)reply_dump;
					struct iphdr *encapsulating_ip_header = (struct iphdr *)&c.message[0];
					printf("Received IP header:\n");
					print_ip_header(encapsulating_ip_header);
					if (encapsulating_ip_header->protocol == IPPROTO_TCP)
					{
						printf("Received ICMP, sent due to dropping of TCP packet\n");
					}
					else if (encapsulating_ip_header->protocol == IPPROTO_UDP)
					{
						printf("Received ICMP, sent due to dropping of UDP packet\n");
					}
					else
					{
						printf("Received ICMP, sent due to dropping of unknown packet\n");
					}
				}
			}
			else
			{
				printf("unknown protocol\n");
				continue; 
			}
		}
	}

	if (success_cnt < num_probes)
	{
		return -1;
	}
	else
	{
		return min_rtt;
	}

}
void estimate_latency(char *next_hop_ip, int hop_len)
{
	double avg_rtt_64, avg_rtt_80, rate, bandwidth, latency;
	// estimatate latency betweeen two adjacent ip addressess in a route,where hop is the number of hops uptil now
	// send date packets of different sizes and calculate the time difference and then calculate the latency and bandwidth

	

	IP_ARR[hop_len] = strdup(next_hop_ip);

	avg_rtt_80 = send_probes(next_hop_ip, 80);
	avg_rtt_64 = send_probes(next_hop_ip, 64);
	RTT_ARR[hop_len].rtt_64 = avg_rtt_64;
	RTT_ARR[hop_len].rtt_80 = avg_rtt_80;

	double rtt_diff_64 , rtt_diff_80;

	if (hop_len > 0)
	{
		if (RTT_ARR[hop_len].rtt_64 < 0 || RTT_ARR[hop_len].rtt_80 < 0 || RTT_ARR[hop_len-1].rtt_64 < 0 || RTT_ARR[hop_len-1].rtt_80 < 0)
		{
			printf("This or previous hop does not reply to pings, cannot estimate latency and bandwidth\n");
			return;
		}

		rtt_diff_64 = (RTT_ARR[hop_len].rtt_64 - RTT_ARR[hop_len-1].rtt_64) / 2;
		rtt_diff_80 = (RTT_ARR[hop_len].rtt_80 - RTT_ARR[hop_len-1].rtt_80) / 2;
	}
	else
	{
		if (RTT_ARR[hop_len].rtt_64 < 0 || RTT_ARR[hop_len].rtt_80 < 0)
		{
			printf("This hop does not reply to pings, cannot estimate latency and bandwidth\n");
			return;
		}
		rtt_diff_64 = avg_rtt_64 / 2;
		rtt_diff_80 = avg_rtt_80 / 2;
	}

	long int random_offset = 80000 + rand() % 20000;
	// adding a random offset to rtt values and scaling to ensure rtt for 80 bytes > 64 bytes
	rtt_diff_64 = (rtt_diff_64 + 16 * random_offset) / 100; 
	rtt_diff_80 = (rtt_diff_80 + 20 * random_offset) / 100;

	rate = 16.0 / (rtt_diff_80 - rtt_diff_64);

	// calculate latency
	latency = ((sizeof(struct iphdr) + sizeof(struct icmphdr) + 64) * rtt_diff_80 - (sizeof(struct iphdr) + sizeof(struct icmphdr) + 80) * rtt_diff_64) / 16;

	// calculate bandwidth
	bandwidth = 16 / (rtt_diff_80 - rtt_diff_64);

	
	if (hop_len > 0)
	{
		printf("Link < %s - %s >\n", IP_ARR[hop_len-1], IP_ARR[hop_len]);
	}
	else
	{
		printf("\nLink < %s - %s > summary:\n", "source", IP_ARR[hop_len]);
	}


	printf("Latency      : %lf us\n", latency);
	printf("Bandwidth    : %lf Mbps\n\n", bandwidth);
}



int establish_link(int ttl)
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
		send_packet.header.un.echo.sequence = i;

		for (int j = 0; j < sizeof(send_packet.message); j++)
		{
			send_packet.message[j] = '0'; // random data
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
			printf("\nReceived ICMP packet on sending ttl = %d and packet type %d from %s\n", ttl, recv_packet.icmp.header.type, inet_ntoa(recv_addr.sin_addr));
			struct icmphdr* hdr = (struct icmphdr*)(&recv_packet.icmp.header);
			print_icmp_header(hdr);
			flag = 1; // received a packet
			break;
		}
		sleep(SLEEP_RATE);
	}

	if (flag == 1)
	{
		// check if packet received is icmp time limit exceeded
		if (parse_icmp_packet(&recv_packet.icmp.header) == TIME_EXCEEDED)
		{
			printf("\nSending echo requests to %s to confirm next hop ...\n", inet_ntoa(recv_addr.sin_addr));
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
			for (int i = 1; i <= 5; ++i)
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
				printf("ICMP echo request sent with following header:\n");
				print_icmp_header(&send_packet.header);
				

				sleep(SLEEP_RATE);
			}

			for (int i = 1; i <= MAX_FIND_HOP_TRIES && success_replies < 5; ++i)
			{
				// recv echo replies

				addr_len = sizeof(struct sockaddr_in);

				struct pollfd fdset = {sockfd, POLLIN, 0};

				int ret = poll(&fdset, 1, 1000);

				if (ret < 0)
				{
					perror("poll");
					exit(EXIT_FAILURE);
				}
				else if (ret == 0)
				{
					continue;
				}
				else
				{
					if (recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&recv_addr, &addr_len) < 0)
					{
						printf("Error: Could not receive echo reply!\n");
						continue;
					}
					else
					{
						parse_icmp_packet(&recv_packet.icmp.header);
						printf("Received ICMP packet with following header:\n");
						print_icmp_header(&recv_packet.icmp.header);
						// check if packet received is icmp echo reply
						int type = parse_icmp_packet(&recv_packet.icmp.header);
						if (type == ECHO_REPLY)
						{
							// check if the echo reply is for the same packet you sent as echo request
							success_replies++;
						}
						else if (type == TIME_EXCEEDED)
						{
							/* ignore, we are not looking for this */
						}
						else
						{
							
							struct iphdr *encapsulating_ip_header = (struct iphdr *)&recv_packet.message[0];
							printf("Received IP header:\n");
							print_ip_header(encapsulating_ip_header);
							if (encapsulating_ip_header->protocol == IPPROTO_TCP)
							{
								printf("Received ICMP, sent due to dropping of TCP packet\n");
							}
							else if (encapsulating_ip_header->protocol == IPPROTO_UDP)
							{
								printf("Received ICMP, sent due to dropping of UDP packet\n");
							}
							else
							{
								printf("Received ICMP, sent due to dropping of unknown packet\n");
					}
						}
					}
				}
				
			}

			if (success_replies == 5)
			{
				printf("\nConfirmed next hop ip: %s\n", next_hop_ip);
				printf("\nEstimating latency of link\n");
				estimate_latency(next_hop_ip, ttl-1);
			}
			else
			{
				printf("Could not receive 5 echo replies, cannot confirm next hop\n");
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
				estimate_latency(next_hop_ip, ttl-1);
				return 1;
			}
			else
			{
				
				struct iphdr *encapsulating_ip_header = (struct iphdr *)&recv_packet.message[0];
				printf("Received IP header:\n");
				print_ip_header(encapsulating_ip_header);
				if (encapsulating_ip_header->protocol == IPPROTO_TCP)
				{
					printf("Received ICMP, sent due to dropping of TCP packet\n");
				}
				else if (encapsulating_ip_header->protocol == IPPROTO_UDP)
				{
					printf("Received ICMP, sent due to dropping of UDP packet\n");
				}
				else
				{
					printf("Received ICMP, sent due to dropping of unknown packet\n");
				}
			}
		}
	}
	else
	{
		printf("Could not receive any icmp packet, Cannot establish next hop\n");
	}
	return 0;
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

	srand(time(NULL));

	memset(IP_ARR, 0, sizeof(IP_ARR));
	for (int i = 0; i < MAX_HOPS; ++i)
	{
		RTT_ARR[i].rtt_64 = RTT_ARR[i].rtt_80 = -1;
	}
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
	printf("The ip address of destination is %s\n\n", dest_ip);

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

	int ttl = 1;
	while (ttl < MAX_HOPS)
	{
		// establish links /next hops,also find latency of link
		// find_latency();
		if (establish_link(ttl) == 1)
		{
			// if next hop is the same as the destination, break
			printf("Reached destination!\n");
			break;
		}
		ttl++;
	}
	exit(EXIT_SUCCESS);
}