#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>  
#include <netinet/udp.h> 
#include <string.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/time.h>

#include "socket.h"
#include "config.h"
#include "interface.h"
#include "dhcp.h"



char buf[BUF_LEN];
int ipv4_fd;
int send4_fd;
int listen_raw_fd;

extern FILE *err;
extern MODE mode;
extern struct interface *network_interface;


void init_socket()
{	
	listen_raw_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (listen_raw_fd < 0) {
		fprintf(err, "Failed to create listening raw socket.\n");
		exit(0);
	}
	struct timeval timeout;
	timeout.tv_sec = RECV_TIMEOUT_SEC;
	timeout.tv_usec = 0;
	setsockopt(listen_raw_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	if (mode == IPv4) {
//		if (next_state == DISCOVER || next_state == REQUEST) {
			/* create a UDP socket to prevent ICMP port unreachable */
			struct sockaddr_in servaddr;
			if ((ipv4_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
				fprintf(err, "Failed to create ipv4 sockfd!\n");
				exit(1);
			}
			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			servaddr.sin_port = htons(IPv4_CLIENT_PORT);
			if (bind(ipv4_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
				//fprintf(err, "socket_init(): bind error\n");
				//exit(1);
			}
			send4_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
			if (send4_fd < 0) {
				fprintf(err, "Failed to create send4 socket.\n");
				exit(1);
			}
//		}
	}
}

void free_socket()
{
	if (listen_raw_fd) {
		close(listen_raw_fd);
		listen_raw_fd = 0;
	}
	if (mode == IPv4) {
//		if (next_state == OFFER || next_state == ACK) {
			if (ipv4_fd) {
				close(ipv4_fd);
				ipv4_fd = 0;
			}
			if (send4_fd) {
				close(send4_fd);
				send4_fd = 0;
			}
//		}
	}
}

static struct timeval base;

static void timeout_init()
{
	gettimeofday(&base, 0);
}

static int timeout_check()
{
	struct timeval cur;
	gettimeofday(&cur, 0);
	if (cur.tv_sec - base.tv_sec - 1 > RECV_TIMEOUT_SEC)
		return 1;
	return 0;
}

static uint16_t udpchecksum(char *iphead, char *udphead, int udplen, int type)
{
	udphead[6] = udphead[7] = 0;
	uint32_t checksum = 0;
	if (type == 4)
	{
		struct udp4_psedoheader header;
		memcpy((char*)&header.srcaddr, iphead + 12, 4);
		memcpy((char*)&header.dstaddr, iphead + 16, 4);
		header.zero = 0;
		header.protocol = 0x11;
		header.length = ntohs(udplen);
		uint16_t *hptr = (uint16_t*)&header;
		int hlen = sizeof(header);
		while (hlen > 0) {
			checksum += *(hptr++);
			hlen -= 2;
		}
	}	
	uint16_t *uptr = (uint16_t*)udphead;
	while (udplen > 1) {	
		checksum += *(uptr++);
		udplen -= 2;
	}
	if (udplen) {
		checksum += (*((uint8_t*)uptr)) ;
	}
	do {
		checksum = (checksum >> 16) + (checksum & 0xFFFF);
	} while (checksum != (checksum & 0xFFFF));
	uint16_t ans = checksum;
	return (ans == 0xFF)? 0xFF :ntohs(~ans);
}

static uint16_t checksum(uint16_t *addr, int len)
{
	int nleft = len;
	int sum = 0;
	uint16_t *w = addr;
	uint16_t answer = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= sizeof (uint16_t);
	}

	if (nleft == 1) {
		*(uint8_t *) (&answer) = *(uint8_t *) w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}

void send_packet(char *packet, int len)
{
	init_socket();
	switch (mode) {
	case IPv4:
		send_packet_ipv4(packet, len);
		return;
	default:
		fprintf(err, "send_packet : unknown mode!\n");
		exit(0);
	}
}

void send_packet_ipv4(char *packet, int len)
{
	memset(buf, 0, sizeof(buf));
	memcpy(buf + 14 + 20 + 8, packet, len);
	struct udphdr *udp = (struct udphdr*)(buf + 14 + 20);
	udp->source = htons(IPv4_CLIENT_PORT);
	udp->dest = htons(IPv4_SERVER_PORT);
	udp->len = htons(len + 8);
	udp->check = 0;
	
	struct iphdr* ip = (struct iphdr*)(buf + 14);
	ip->version = 4;
	ip->ihl = 5;
	ip->tos = 0x10;
	ip->tot_len = htons(len + 20 + 8);
	ip->id = 0;
	ip->frag_off = 0;
	ip->ttl = 128;
	ip->protocol = UDP;
	ip->check = 0;
	ip->saddr = 0;
	ip->daddr = 0xffffffff;
	// inet_aton("0.0.0.0", &(ip->saddr));
	// inet_aton("255.255.255.255", &(ip->daddr));
	
	udp->check = htons(udpchecksum((char*)ip, (char*)udp, len + 8, 4));
	ip->check = checksum((uint16_t*)ip, 20);
	
	memset(buf, 0xff, 6);
	memcpy(buf + 6, network_interface->addr, 6);
	buf[12] = 0x08;
	buf[13] = 0x00;
	
	int total_len = len + 14 + 20 + 8;
	
	
	struct sockaddr_ll device;
	if ((device.sll_ifindex = if_nametoindex(network_interface->name)) == 0) {
		fprintf(err, "Failed to resolve the index of %s.\n", network_interface->name);
		exit(1);
	}
	
	if (sendto(send4_fd, buf, total_len, 0, (struct sockaddr *)&device, sizeof(device)) < 0) {
		fprintf(err, "Failed to send ipv4 packet.\n");
		exit(1);
	}
}


int recv_packet(char* packet, int max_len)
{
	switch (mode) {
	case IPv4:
		return recv_packet_ipv4(packet, max_len);
	default:
		fprintf(err, "recv_packet : unknown mode!\n");
		exit(0);
	}
}

int recv_packet_ipv4(char* packet, int max_len)
{
	timeout_init();
	int len = recv(listen_raw_fd, buf, max_len, 0);
	if (len < 0 || timeout_check()) {
		fprintf(err, "recv timeout!\n");
		return -1;
	}
	//TODO : packet checking
	len -= 14 + 20 + 8;
	memcpy(packet, buf + 14 + 20 + 8, len);
	return len;
}


