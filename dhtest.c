/*
 * DHCP client simulation tool. For testing pursose only.
 * This program needs to be run with root privileges.
 * Author - Saravanakumar.G E-mail: saravana815@gmail.com
 */

#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<net/if.h>
#include<linux/if_packet.h>
#include<getopt.h>
#include<time.h>
#include<stdlib.h>
#include "headers.h"

int sock_packet, iface = 2;	/* Socket descripter & transmit interface index */
struct sockaddr_ll ll = { 0 };	/* Socket address structure */
u_int16_t vlan = 0;
u_int8_t l3_tos = 0;
u_int16_t l2_hdr_size = 14;
u_int16_t l3_hdr_size = 20;
u_int16_t l4_hdr_size = 8;
u_int16_t dhcp_hdr_size = sizeof(struct dhcpv4_hdr);

/* All protocheader sizes */

/* DHCP packet, option buffer and size of option buffer */
u_char dhcp_packet_disc[1518] = { 0 };
u_char dhcp_packet_offer[1518] = { 0 };
u_char dhcp_packet_request[1518] = { 0 };
u_char dhcp_packet_ack[1518] = { 0 };
u_char dhcp_packet_release[1518] = { 0 };
u_char dhcp_packet_decline[1518] = { 0 };

u_char dhopt_buff[500] = { 0 };
u_int32_t dhopt_size = { 0 };
u_char dhmac[ETHER_ADDR_LEN] = { 0 };
u_char rtrmac[ETHER_ADDR_LEN] = { 0 };
u_char dmac[ETHER_ADDR_LEN];
u_char iface_mac[ETHER_ADDR_LEN] = { 0 };

/*
* For Custom DHCP options
* Static arrays for custom_dhcp_option_hdr
*/
#define MAX_CUSTOM_DHCP_OPTIONS 64
u_int8_t no_custom_dhcp_options = { 0 };
struct custom_dhcp_option_hdr custom_dhcp_options[MAX_CUSTOM_DHCP_OPTIONS];

char dhmac_fname[20];
char rtrmac_fname[20];
char iface_name[30] = { 0 };
char ip_str[128];
u_int8_t dhmac_flag = 0;
u_int8_t rtrmac_flag = 0;
u_int8_t strict_mac_flag = 0;
u_int32_t server_id = { 0 }, option50_ip = { 0 };
u_int32_t dhcp_xid = 0;
u_int16_t bcast_flag = 0; /* DHCP broadcast flag */
u_int8_t vci_buff[256] = { 0 }; /* VCI buffer*/
u_int16_t vci_flag = 0;
u_int8_t hostname_buff[256] = { 0 }; /* Hostname buffer*/
u_int16_t hostname_flag = 0;
u_int8_t fqdn_buff[256] = { 0 }; /* FQDN buffer*/
u_int16_t fqdn_flag = 0;
u_int16_t fqdn_n = 0;
u_int16_t fqdn_s = 0;
u_int32_t option51_lease_time = 0;
u_int8_t option55_req_flag = 0;
u_int8_t option55_req_list[256] = { 0 }; /* option55 request list buffer */
u_int32_t option55_req_len = 0; /* option55 request list buffer */
u_int32_t port = 67;
u_int8_t unicast_flag = 0;
u_int8_t nagios_flag = 0;
u_int8_t json_flag = 0;
u_int8_t dhcp_decline_flag = 0;
u_int8_t json_first = 1;
char *giaddr = "0.0.0.0";
char *server_addr = "255.255.255.255";

/* Pointers for all layer data structures */
struct ethernet_hdr *eth_hg = { 0 };
struct vlan_hdr *vlan_hg = { 0 };
struct iphdr *iph_g = { 0 };
struct udphdr *uh_g = { 0 };
struct dhcpv4_hdr *dhcph_g = { 0 };


u_int8_t *dhopt_pointer_g = { 0 };
u_int8_t verbose = 0;
u_int8_t dhcp_release_flag = 0;
u_int8_t padding_flag = 0;
u_int16_t timeout = 0;
time_t time_now, time_last;

/* Used for ip listening functionality */
struct arp_hdr *arp_hg = { 0 };
struct icmp_hdr *icmp_hg = { 0 };

u_int32_t unicast_ip_address = 0;
u_int32_t ip_address;
u_char ip_listen_flag = 0;
struct timeval tval_listen = { 3600, 0 };
u_int32_t listen_timeout = 3600;
u_char arp_icmp_packet[1514] = { 0 };
u_char arp_icmp_reply[1514] = { 0 };
u_int16_t icmp_len = 0;



int main(int argc, char *argv[])
{
	int get_tmp = 1, get_cmd;
	char PHY_NIC[] = "ens39";
	iface = if_nametoindex(PHY_NIC);
	if(iface == 0) {
		fprintf(stdout, "Interface doesnot exist\n");
		exit(2);
	}
	strncpy(iface_name, PHY_NIC, 29);




	if(!dhmac_flag || strict_mac_flag) {
	  /* obtain the MAC address of the interface we're
	     transmitting on */
	  if(get_if_mac_address(iface_name, iface_mac) != 0)
	    exit(2);

	  if (!dhmac_flag)
	       memcpy (dhmac, iface_mac, ETHER_ADDR_LEN);

	  if (!strict_mac_flag)
	       memcpy (iface_mac, dhmac, ETHER_ADDR_LEN);

	  if (verbose)
	    {
	      fprintf (stderr, "Using Ethernet source addr: %s\n", mac2str (iface_mac));
	      fprintf (stderr, "Using DHCP chaddr: %s\n", mac2str (dhmac));
	    }

	  /** build the file name used fot saving lease informations */
	  strcpy(dhmac_fname, mac2str(dhmac));
	} else {
          /* dhmac_flag is set and strict_mac_flag is not set */
          memcpy (iface_mac, dhmac, ETHER_ADDR_LEN);
  }

	if (verbose && rtrmac_flag){
			fprintf (stderr, "Using Router MAC addr: %s\n", mac2str(rtrmac));
	}

	if(json_flag) {
		fprintf(stdout, "[");
	}

	/* Opens the PF_PACKET socket */
	if(open_socket() < 0) {
		if (nagios_flag) {
			fprintf(stdout, "CRITICAL: Socket error.");
		} else if(json_flag) {
			if(!json_first) {
				fprintf(stdout, ",");
			} else {
				json_first = 0;
			}

			fprintf(stdout, "{\"msg\":\"Socket error.\","
					"\"result\":\"error\","
					"\"error-type\":\"socket\","
					"\"error-msg\":\"Socket error.\"}"
					"]");
		} else {
			fprintf(stdout, "Socket error\n");
		}

		exit(2);
	}

	/* Sets the promiscuous mode */
	set_promisc();

	if (unicast_flag && !unicast_ip_address) {
		unicast_ip_address = get_interface_address();
	}

	/* Sets a random DHCP xid */
	set_rand_dhcp_xid();

	/*
	 * If DHCP release flag is set, send DHCP release packet
	 * and exit. get_dhinfo parses the DHCP info from log file
	 * and unlinks it from the system
	 */
	if(dhcp_release_flag) {
		if(get_dhinfo() == ERR_FILE_OPEN) {
			if (nagios_flag) {
				fprintf(stdout, "CRITICAL: Error on opening DHCP info file.");
			} else if(json_flag) {
				if(!json_first) {
					fprintf(stdout, ",");
				} else {
					json_first = 0;
				}

				fprintf(stdout, "{\"msg\":\"Error on opening DHCP info file.\","
						"\"result\":\"error\","
						"\"error-type\":\"info-file\","
						"\"error-msg\":\"Error on opening DHCP info file.\"}"
						"]");
			} else {
				fprintf(stdout, "Error on opening DHCP info file\n");
				fprintf(stdout, "Release the DHCP IP after acquiring\n");
			}
			exit(2);
		}
		build_option53(DHCP_MSGRELEASE); /* Option53 DHCP release */
		if(hostname_flag) {
			build_option12_hostname();
		}
		if(fqdn_flag) {
			build_option81_fqdn();
		}
		build_option54();		 /* Server id */
		build_optioneof();		 /* End of option */
		build_dhpacket(DHCP_MSGRELEASE); /* Build DHCP release packet */
		send_packet(DHCP_MSGRELEASE);	 /* Send DHCP release packet */
		clear_promisc();		 /* Clear the promiscuous mode */
		close_socket();
		return 0;
	}

	/*
	 * If DHCP DECLINE flag is set, send DHCP DECLINE packet
	 * and exit. get_dhinfo parses the DHCP info from log file
	 * and unlinks it from the system
	 */
	if(dhcp_decline_flag) {
		if(get_dhinfo() == ERR_FILE_OPEN) {
			if (nagios_flag) {
				fprintf(stdout, "CRITICAL: Error on opening DHCP info file.");
			} else if(json_flag) {
				if(!json_first) {
					fprintf(stdout, ",");
				} else {
					json_first = 0;
				}

				fprintf(stdout, "{\"msg\":\"Error on opening DHCP info file.\","
						"\"result\":\"error\","
						"\"error-type\":\"info-file\","
						"\"error-msg\":\"Error on opening DHCP info file.\"}"
						"]");
			} else {
				fprintf(stdout, "Error on opening DHCP info file\n");
				fprintf(stdout, "Release the DHCP IP after acquiring\n");
			}
			exit(2);
		}
		build_option53(DHCP_MSGDECLINE); /* Option53 DHCP decline */
		if(hostname_flag) {
			build_option12_hostname();
		}
		if(fqdn_flag) {
			build_option81_fqdn();
		}
		build_option54();		 /* Server id */
		build_option50();		 /* Requested IP Address */

		if(vci_flag) {
			build_option60_vci();	 /* Vendor Class Identifier */
		};
		build_optioneof();		 /* End of option */
		build_dhpacket(DHCP_MSGDECLINE); /* Build DHCP release packet */
		send_packet(DHCP_MSGDECLINE);	 /* Send DHCP release packet */
		clear_promisc();		 /* Clear the promiscuous mode */
		close_socket();
		return 0;
	}

	if(timeout) {
		time_last = time(NULL);
	}
	build_option53(DHCP_MSGDISCOVER);	/* Option53 for DHCP discover */
	if(hostname_flag) {
		build_option12_hostname();
	}
	if(fqdn_flag) {
		build_option81_fqdn();
	}
	if(option50_ip) {
		build_option50();		/* Option50 - req. IP  */
	}
        build_option55();                       /* Option55 - parameter request list */
	if(option51_lease_time) {
		build_option51();               /* Option51 - DHCP lease time requested */
	}

	if(vci_flag) {
		build_option60_vci(); 		/* Option60 - VCI  */
	}
        /* Build custom options */
        if(no_custom_dhcp_options) {
            build_custom_dhcp_options();
        }
	build_optioneof();			/* End of option */
	build_dhpacket(DHCP_MSGDISCOVER);	/* Build DHCP discover packet */

	int dhcp_offer_state = 0;
	while(dhcp_offer_state != DHCP_OFFR_RCVD) {

		/* Sends DHCP discover packet */
		send_packet(DHCP_MSGDISCOVER);
		/*
		 * recv_packet functions returns when the specified
		 * packet is received
		 */
		dhcp_offer_state = recv_packet(DHCP_MSGOFFER);

		if(timeout) {
			time_now = time(NULL);
			if((time_now - time_last) >= timeout) {
				if (nagios_flag) {
					fprintf(stdout, "CRITICAL: Timeout reached: DISCOVER.");
				} else if(json_flag) {
					if(!json_first) {
						fprintf(stdout, ",");
					} else {
						json_first = 0;
					}

					fprintf(stdout, "{\"msg\":\"Timeout reached: DISCOVER.\","
                                                "\"result\":\"error\","
                                                "\"error-type\":\"timeout\","
                                                "\"error-subtype\":\"DISCOVER\","
                                                "\"error-msg\":\"Timeout reached: DISCOVER.\"}"
						"]");
				}

				close_socket();
				exit(2);
			}
		}
		if(dhcp_offer_state == DHCP_OFFR_RCVD) {
			if (!nagios_flag && !json_flag) {
				fprintf(stdout, "DHCP offer received\t - ");
			} else if(!nagios_flag) {
				if(!json_first) {
					fprintf(stdout, ",");
				} else {
					json_first = 0;
				}

				fprintf(stdout, "{\"msg\":\"DHCP offer received - %s\","
						"\"result\":\"success\","
						"\"result-type\":\"OFFER\","
						"\"result-value\":\"%s\""
						"}",
					get_ip_str(dhcph_g->dhcp_yip), get_ip_str(dhcph_g->dhcp_yip));
			}

			set_serv_id_opt50();
			if (!nagios_flag && !json_flag)
  				fprintf(stdout, "Offered IP : %s\n", get_ip_str(dhcph_g->dhcp_yip));
			if(!nagios_flag && verbose) {
				print_dhinfo(DHCP_MSGOFFER);
			}
		}
	}
	/* Reset the dhopt buffer to build DHCP request options  */
	reset_dhopt_size();
	build_option53(DHCP_MSGREQUEST);
	build_option50();
	build_option54();
	if(hostname_flag) {
		build_option12_hostname();
	}
	if(fqdn_flag) {
		build_option81_fqdn();
	}
	if(vci_flag) {
		build_option60_vci();
	}
	if(option51_lease_time) {
		build_option51();                       /* Option51 - DHCP lease time requested */
	}
        build_option55();                               /* Option55 - parameter request list */
        /* Build custom options */
        if(no_custom_dhcp_options) {
                build_custom_dhcp_options();
        }
	build_optioneof();
	build_dhpacket(DHCP_MSGREQUEST); 		/* Builds specified packet */
	int dhcp_ack_state = 1;
	while(dhcp_ack_state != DHCP_ACK_RCVD) {

		send_packet(DHCP_MSGREQUEST);
		dhcp_ack_state = recv_packet(DHCP_MSGACK);

		if(timeout) {
			time_now = time(NULL);
			if((time_now - time_last) > timeout) {
				if (nagios_flag) {
					fprintf(stdout, "CRITICAL: Timeout reached: REQUEST.");
				} else if(json_flag) {
					if(!json_first) {
						fprintf(stdout, ",");
					} else {
						json_first = 0;
					}

					fprintf(stdout, "{\"msg\":\"Timeout reached: REQUEST.\","
                                                "\"result\":\"error\","
                                                "\"error-type\":\"timeout\","
                                                "\"error-subtype\":\"REQUEST\","
                                                "\"error-msg\":\"Timeout reached: REQUEST.\"}"
                                                "]");
				} else {
					fprintf(stdout, "Timeout reached. Exiting\n");
				}
				close_socket();
				exit(1);
			}
		}

		if(dhcp_ack_state == DHCP_ACK_RCVD) {
			if (nagios_flag) {
				fprintf(stdout, "OK: Acquired IP: %s", get_ip_str(dhcph_g->dhcp_yip));
			} else if(json_flag) {
				if(!json_first) {
					fprintf(stdout, ",");
				} else {
					json_first = 0;
				}

				fprintf(stdout, "{\"msg\":\"DHCP ack received - %s\","
                                                "\"result\":\"success\","
                                                "\"result-type\":\"ACK\","
                                                "\"result-value\":\"%s\""
						"}",
                                        get_ip_str(dhcph_g->dhcp_yip), get_ip_str(dhcph_g->dhcp_yip));
			} else {
				fprintf(stdout, "DHCP ack received\t - ");
				fprintf(stdout, "Acquired IP: %s\n", get_ip_str(dhcph_g->dhcp_yip));
			}

			/* Logs DHCP IP details to log file. This file is used for DHCP release */
			log_dhinfo();
			if(!nagios_flag && verbose) {
				print_dhinfo(DHCP_MSGACK);
			}
		} else if (dhcp_ack_state == DHCP_NAK_RCVD) {
			if (!nagios_flag && !json_flag) {
				fprintf(stdout, "DHCP nack received\t - ");
				fprintf(stdout, "Client MAC : %02x:%02x:%02x:%02x:%02x:%02x\n", \
					dhmac[0], dhmac[1], dhmac[2], dhmac[3], dhmac[4], dhmac[5]);
			} else if(json_flag) {
				if(!json_first) {
					fprintf(stdout, ",");
				} else {
					json_first = 0;
				}

				fprintf(stdout, "{\"msg\":\"DHCP nack received\","
                                                "\"result\":\"info\","
                                                "\"result-type\":\"NACK\","
                                                "\"result-value\":\"%02x:%02x:%02x:%02x:%02x:%02x\""
						"}",
						dhmac[0], dhmac[1], dhmac[2], dhmac[3], dhmac[4], dhmac[5]);
			}
		}
	}
	/* If IP listen flag is enabled, Listen on obtained for ARP, ICMP protocols  */
	if(!nagios_flag && ip_listen_flag) {
		if(!json_flag) {
			fprintf(stdout, "\nListening on %s for ARP and ICMP protocols\n", iface_name);
			fprintf(stdout, "IP address: %s, Listen timeout: %d seconds\n", get_ip_str(htonl(ip_address)), listen_timeout);
		} else {
			if(!json_first) {
				fprintf(stdout, ",");
			} else {
				json_first = 0;
			}

			fprintf(stdout, "{\"msg\":\"Listening on %s for ARP and ICMP protocols."
					"IP address: %s, Listen timeout: %d seconds\","
					"\"result\":\"info\","
					"\"result-type\":\"listen\","
					"\"result-ip\":\"%s\","
					"\"result-timeout\":\"%d\""
					"}",
					iface_name, get_ip_str(htonl(ip_address)), listen_timeout,
					get_ip_str(htonl(ip_address)), listen_timeout);
		}

		int arp_icmp_rcv_state = 0;
		while(arp_icmp_rcv_state != LISTEN_TIMOUET) {
			arp_icmp_rcv_state = recv_packet(ARP_ICMP_RCV);
			/* Send ARP reply if ARP request received */
			if(arp_icmp_rcv_state == ARP_RCVD) {
				/*if(verbose) {
				  fprintf(stdout, "ARP request received\n");
				  fprintf(stdout, "Sending ARP reply\n");
				  }*/
				build_packet(ARP_SEND);
				send_packet(ARP_SEND);
			} else if(arp_icmp_rcv_state == ICMP_RCVD) {
				/* Send ICMP reply if ICMP echo request received */
				/*if(verbose) {
				  fprintf(stdout, "ICMP request received\n");
				  fprintf(stdout, "Sending ICMP reply\n");
				  }*/
				build_packet(ICMP_SEND);
				send_packet(ICMP_SEND);
			}
		}

		if(!json_flag) {
			fprintf(stdout, "Listen timout reached\n");
		} else {
			if(!json_first) {
				fprintf(stdout, ",");
			} else {
				json_first = 0;
			}

			fprintf(stdout, "{\"msg\":\"Listen timout reached.\","
					"\"result\":\"error\","
					"\"error-type\":\"timeout\","
					"\"error-subtype\":\"listen\","
					"\"error-msg\":\"Listen timout reached.\"}");
		}
	}
	/* Clear the promiscuous mode */
	clear_promisc();
	/* Close the socket */
	close_socket();

	if(json_flag) {
		fprintf(stdout, "]");
	}

	return 0;
}
