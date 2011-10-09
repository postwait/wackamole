/*
 * Copyright (c) 1997 Theo Schlossnagle
 */
#include "ife.h"
#include "arpcache.h"

#ifdef WIN32
#undef DELETE
#endif

enum {
 LIST = 1,
 ADD = 2,
 DELETE = 4,
 SPOOF = 8,
 ARPLIST = 16
};

int main (int argc, char *argv[]) {
  int A,B,C,D, i, ic;
  struct interface req, myips[300];
  int state;
#ifdef WIN32
  WSADATA wsadata;

  WSAStartup(WINSOCK_VERSION, &wsadata);
#endif


  i=1;
  if(argc < 2) goto usage_error;
  if(argv[i][0] != '-')
    goto usage_error;
  
  switch(argv[i++][1]) {
  case 'r':
    state=ARPLIST;
    break;
  case 'l':
    state=LIST;
    break;
  case 'a':
    state=ADD;
    strcpy(req.ifname, argv[i++]);
    sscanf(argv[i++], "%d.%d.%d.%d", &A,&B,&C,&D);
    req.ipaddr.s_addr = htonl((A << 24) | (B << 16) | (C << 8) | D);
    sscanf(argv[i++], "%d.%d.%d.%d", &A,&B,&C,&D);
    req.bcast.s_addr = htonl((A << 24) | (B << 16) | (C << 8) | D);
    sscanf(argv[i++], "%d.%d.%d.%d", &A,&B,&C,&D);
    req.netmask.s_addr = htonl((A << 24) | (B << 16) | (C << 8) | D);
    break;
  case 'd':
    state=DELETE;
    sscanf(argv[i++], "%d.%d.%d.%d", &A,&B,&C,&D);
    req.ifname[0] = '\0';
    req.ipaddr.s_addr = htonl((A << 24) | (B << 16) | (C << 8) | D);
    break;
  case 's':
    state=SPOOF;
    strcpy(req.ifname, argv[i++]);
    sscanf(argv[i++], "%d.%d.%d.%d", &A,&B,&C,&D);
    req.ipaddr.s_addr = htonl((A << 24) | (B << 16) | (C << 8) | D);
    sscanf(argv[i++], "%d.%d.%d.%d", &A,&B,&C,&D);
    req.bcast.s_addr = htonl((A << 24) | (B << 16) | (C << 8) | D);
    break;
  usage_error:
  default:
    fprintf(stderr, "%s:\n\tUsage:\n\t\t-l\t\t\t\t\tlist interfaces\n\t\t-a <interface> <ip> <bcast> <netmask>\tAdd this VIP\n\t\t-d <IP>\t\t\t\t\tDelete this VIP\n\t\t-s interface VIP destinationIP\n", argv[0]);
    exit(-1);
  }
  

  if_initialize();
  if(state&ARPLIST) {
    arp_entry *ad, *ac;
    fprintf(stderr, "Sampling and printing local arp-cache\n");
    sample_arp_cache();
    sample_arp_cache();
    ac = ad = fetch_shared_arp_cache();
    while(ad && ad->ip) {
	struct in_addr iad;
	iad.s_addr = ad->ip;
	fprintf(stderr, "\t%s\n", inet_ntoa(iad));
	ad++;
    }
    free(ac);
  }
  if(state&ADD) {
    ic = if_up(&req);
    if(ic) {
      perror("ioctl");
      fprintf(stderr, "%s\n", if_error());
    } else {
      fprintf(stderr, "if_up: %s\t%s", req.ifname, inet_ntoa(req.ipaddr));
      fprintf(stderr, "\t%s", inet_ntoa(req.bcast));
      fprintf(stderr, "\t%s\n", inet_ntoa(req.netmask));
    }
  }
  if(state&DELETE) {
    ic = if_down(&req);
    if(ic)
      fprintf(stderr, "%s\n", if_error());
    else {
      fprintf(stderr, "if_down: %s\t%s", req.ifname, inet_ntoa(req.ipaddr));
      fprintf(stderr, "\t%s", inet_ntoa(req.bcast));
      fprintf(stderr, "\t%s\n", inet_ntoa(req.netmask));
    }
  }
  if(state&SPOOF) {
    ic = if_send_spoof_request(req.ifname,
			       req.ipaddr.s_addr, req.bcast.s_addr, NULL, 5, 0);
    if(ic)
      fprintf(stderr, "%s\n", if_error());
    else {
      fprintf(stderr, "if_spoofed: %s\t%s\n",req.ifname, inet_ntoa(req.ipaddr));
    }
  }
  if(state) {
    ic = if_list_ips(myips, 300);
    fprintf(stderr, "Found %d IPs:\n", ic);
    for(i=0; i<ic; i++) {
      fprintf(stderr, "%s\t[%02x:%02x:%02x:%02x:%02x:%02x]",
	myips[i].ifname,
	myips[i].mac[0], myips[i].mac[1], myips[i].mac[2],
	myips[i].mac[3], myips[i].mac[4], myips[i].mac[5]);
        fprintf(stderr, "\t%s", inet_ntoa(myips[i].ipaddr));
        fprintf(stderr, "\t%s", inet_ntoa(myips[i].bcast));
        fprintf(stderr, "\t%s\n", inet_ntoa(myips[i].netmask));
    }
  }
  return 0;
}
