#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#define INTERFACE_NAME_LEN 100
#include <unistd.h>
#include "lease.h"
#include "dhcp.h"

struct interface {
	char name[INTERFACE_NAME_LEN];
	char addr[6];/* macaddr */
};



void init_interfaces();
void configure_interface(struct lease* lease);


#endif /* __INTERFACE_H__ */
