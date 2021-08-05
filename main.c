#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h> 
#include<time.h>
#include "./dhcp/config.h"
#include "./dhcp/dhcp.h"
#include "./dhcp/socket.h"

MODE mode;

FILE *err;


struct interface *config_interface;
struct interface *network_interface;

char config_interface_name[INTERFACE_NAME_LEN];
char network_interface_name[INTERFACE_NAME_LEN];


int main()
{
	char if_name[] = "ens38";

	mode = IPv4;
	err = stderr;
	srand(time(NULL));

	strcpy(network_interface_name, if_name);
	strcpy(config_interface_name, if_name);

	init_interfaces();
	init_dhcp();
		
	while (1) {
		handle_dhcp();
	}
	return 0;
}
