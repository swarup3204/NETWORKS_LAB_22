//*this file contains includes,macros and global variables for pingnetinfo.c

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
#define MAX_FIND_HOP_TRIES 8
#define PORT_NO 0
// port no of servers to connect to
#define RECV_TIMEOUT 1
// timeout for recvfrom() in seconds
#define ICMP_PKT_SIZE 64
#define IP_PKT_SIZE 128
#define SLEEP_RATE 100
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

int num_probes;
// a global variable to store the number of probes to be sent per link
int time_diff;
// a global variable to store the time difference between two consecutive probes

struct icmp_pkt ROUTE_IPS[N_MAX_IPS];
// to store the icmp packets echo replies received till now

#endif