#ifndef __LEASE_H__
#define __LEASE_H__

#include <stdint.h>

#define DNS_NAME_LEN 100
#define IP_STR_LEN 16

struct lease {
	uint32_t server_ip;
	uint32_t client_ip;
	uint32_t mask_ip;
	uint32_t router_ip;
	uint32_t dns_ip;
	char dns[DNS_NAME_LEN];
	uint32_t lease_time;
	uint32_t renew_time;
	uint16_t portset_index;
	uint16_t portset_mask;
	uint32_t controller_ip;
};



void save_lease(struct lease* lease);
int load_lease(struct lease* lease);


#define DEFAULT_LEASE_PATH "."

#endif /* __LEASE_H__ */
