#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h> 

#include "config.h"
#include "dhcp.h"
#include "socket.h"

MODE mode;

static void usage()
{
	printf("Usage : thclient [options] <config_interface>\n");
	printf("        options:\n");
	printf("            -f                                       running in foreground (default in background)\n");
	printf("            -p                                       Use port set\n");
	printf("            --network-interface <network_interface>  default the same as config_interface\n");
	printf("            --encap-mode <mode>                      available modes: ipv4(default), ipv6, dhcpv6\n");
	printf("            --server-addr <server_ipv6_addr>         IPv6 address of DHCPv4-over-IPv6 server\n");
	printf("            --local-addr <local_ipv6_addr>           local IPv6 address in ipv6/dhcpv6 mode\n");
}

static void init_daemon()
{
	int pid, sid;
	if ((pid = fork()) > 0) {//father process
		exit(0);
	} else if (pid < 0) {//fork error
		fprintf(err, "Failed to fork...");
		exit(1);
	}
	sid = setsid();
	if (sid < 0) {
		fprintf(err, "Failed to setsid\n");
		exit(1);
	}
	if ((pid = fork()) > 0) {//father process
		exit(0);
	} else if (pid < 0) {//fork error
		exit(1);
	}
	close(0);
	close(1);
	close(2);
	chdir("/tmp");
	umask(0);
	
	err = fopen(DAEMON_LOG, "a");
	if (err <= 0) {
		exit(1);
	}
	fprintf(err, "logging!\n");
	
}

int main(int argc, char **argv)
{
	mode = IPv4;
	daemon = 0; // running in foreground
	err = stderr;
	portset = 0;
	srand(time(NULL));
	int i;
	for (i = 1; i < argc; ++i) {
		if (i + 1 < argc && strcmp(argv[i], "--network-interface") == 0) {
			++i;
			strcpy(network_interface_name, argv[i]);
			strcpy(config_interface_name, argv[i]);
			
		} else {//config-interface
			strcpy(config_interface_name, argv[i]);
		}
	}

	if (config_interface_name[0] == '\0') {
		usage();
		exit(0);
	}
	if (network_interface_name[0] == '\0')
		strcpy(network_interface_name, config_interface_name);
	
	init_interfaces();
	init_dhcp();
	if (daemon)
		init_daemon();
		
	while (1) {
		handle_dhcp();
	}
	return 0;
}
