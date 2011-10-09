#ifndef IFE_ICMP_SUPPORT_H
#define IFE_ICMP_SUPPORT_H

void compose_ping(unsigned char *buf, unsigned char *mymac, unsigned char *rmmac, u_int32_t new_ip, u_int32_t r_ip);

#endif
