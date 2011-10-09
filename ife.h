#ifndef _IFE_H_
#define _IFE_H_

#include "config.h"

int if_initialize(void);
char *if_error(void);
int if_send_spoof_request(char *dev, unsigned int new_ip,
			  unsigned int r_ip, unsigned char *rm, int count, int icmp);
int if_list_ips(struct interface *ifs, int size);
int if_down(struct interface *areq);
int if_up(struct interface *areq);
#endif
