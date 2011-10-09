/*
 * The Wackamole Program.
 *     
 * The contents of this file are subject to the CNDS Open Source
 * License, Version 1.0 (the ``License''); you may not use
 * this file except in compliance with the License.  You may obtain a
 * copy of the License at:
 *
 * http://www.backhand.org/wackamole/license/
 *
 * or in the file ``license.txt'' found in this distribution.
 *
 * Software distributed under the License is distributed on an AS IS basis, 
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License 
 * for the specific language governing rights and limitations under the 
 * License.
 *
 * The Creators of Wackamole are:
 *  Yair Amir, Ryan Caudy, Aashima Munjal and Theo Schlossnagle.
 *
 *  Copyright (C) 2000-2001 The Johns Hopkins University
 *  All Rights Reserved.
 *
 *  This product uses the Spread toolkit, developed by Spread Concepts LLC.
 *  For more information about Spread see http://www.spread.org
 *
 *  Wackamole development was partially funded by a grant from the Defense
 *  Advanced Research Projects Agency (DARPA) to Johns Hopkins University. The 
 *  U.S. Government retains certain rights in this software.
 *
 */
#include "config.h"
#include "alarm.h"
#include "arpcache.h"
#include "abt.h"

static int arpcache_psize = 0;
static arp_entry *arpcache_private = NULL;
static abt *arpcache_shared = NULL;

const unsigned char ff_ff_ff_ff_ff_ff[ETH_ALEN] =
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

#define ROUNDUP(a) \
        ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

#if defined(HAVE_PROC_NET_ARP)
void sample_arp_cache()
{
  char line[200];
  char ip[100];
  int  hw_type, flags;
  char hw_address[100];    /* MAC address */
  char mask[100];    
  char device[100];
  FILE *fp;
  int num,count=0;
  int tmp_size;
	
  /* read the arp cache entries from the kernel via /proc */
  if ((fp = fopen("/proc/net/arp", "r")) == NULL) {
    /* perror("can not read arp cache"); */
    wack_alarm(WACK_DEBUG, "Can not read arp cache");
    return;
  }

  /* start with the assumption that the old size is the new size */
  tmp_size = arpcache_psize;
		
  if (fgets(line, sizeof(line), fp) != (char *) NULL){ /* skip first line */
    strcpy(mask, "-");
    strcpy(device, "-");
		
    /* read cache entries line by line */
    for ( count=0; fgets(line, sizeof(line), fp);){
      unsigned int *h, hint[ETH_ALEN];
      int i;
      num = sscanf(line, "%s 0x%x 0x%x %100s %100s %100s",
		 ip, &hw_type, &flags, hw_address, mask, device);
      if (num < 6)
        break;

      if( count+1 > tmp_size ){				
        if( tmp_size > 0 ){
	  tmp_size *= 2;
	  arpcache_private = realloc(arpcache_private,
				     sizeof(arp_entry)*(tmp_size+1));
        } else {
	  tmp_size = 2;
	  arpcache_private = malloc(sizeof(arp_entry)*(tmp_size+1));
        }
      }
      arpcache_private[count].ip = (address)inet_addr( ip );
      h = hint;
      sscanf(hw_address, "%02x:%02x:%02x:%02x:%02x:%02x",
             h, h+1, h+2, h+3, h+4, h+5);
      for(i=0; i<ETH_ALEN; i++) arpcache_private[count].mac[i] = hint[i];
      wack_alarm(ARPING, "Adding: (%s) %s [%s]",
            device, ip, hw_address);
      count++;
    }
  }
	
  if( count == 0 ){
    wack_alarm(WACK_DEBUG, "Local arp-cache seems to be empty");
    if(arpcache_private) free(arpcache_private);
    arpcache_psize = 0;
    arpcache_private = malloc(sizeof(arp_entry));
    arpcache_private[0].ip = 0;
  } else {
    /* adjust array size (FIXME: should we really do this?) */
    if( count != tmp_size ){
      arpcache_private = realloc(arpcache_private,sizeof(arp_entry)*(count+1));
    }
    arpcache_psize = count;
    arpcache_private[count].ip = 0;
  }
  fclose(fp);
}
#elif defined(CTL_NET)
void sample_arp_cache() {
  size_t len = 0;
  int count = 0;
  struct rt_msghdr *rtm;
  struct sockaddr_inarp *sa;
  struct sockaddr_dl *sdl;
  char *arpdata, *cp;
  int mib[6] = { CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS,  RTF_LLINFO };
  sysctl(mib, 6, NULL, &len, NULL, 0);
  arpdata = malloc(len);
  sysctl(mib, 6, arpdata, &len, NULL, 0);
  for(cp = arpdata; cp < (arpdata+len); cp+=rtm->rtm_msglen) {
    rtm = (struct rt_msghdr *)cp;
    sa = (struct sockaddr_inarp *)(rtm+1);
    count++;
  }
  if((!arpcache_private) || (arpcache_psize != count)) {
    if(arpcache_private) free(arpcache_private);
    arpcache_private = malloc(sizeof(arp_entry)*(count+1));
    arpcache_psize = count;
  }
  count = 0;
  for(cp = arpdata; cp < (arpdata+len); cp+=rtm->rtm_msglen) {
    unsigned char *h;
    rtm = (struct rt_msghdr *)cp;
    sa = (struct sockaddr_inarp *)(rtm+1);
    sdl = (struct sockaddr_dl *)(sa + 1);
    arpcache_private[count].ip = sa->sin_addr.s_addr;
    memcpy(arpcache_private[count].mac, ff_ff_ff_ff_ff_ff, ETH_ALEN);
    if(sdl->sdl_alen == ETH_ALEN) {
      memcpy(arpcache_private[count].mac, sdl->sdl_data+sdl->sdl_nlen, ETH_ALEN);
    }
    h = arpcache_private[count].mac;
    wack_alarm(ARPING, "Adding: (private) %s [%02x:%02x:%02x:%02x:%02x:%02x]",
               inet_ntoa(sa->sin_addr),
               h[0], h[1], h[2], h[3], h[4], h[5]);
    count++;
  }
  arpcache_private[count].ip = 0;
  free(arpdata);
}
#elif defined(DL_UDERROR_IND)
void sample_arp_cache() {
  static int s=-1;
  dl_data_ack_ind_t *req;
  dl_connect_res_t *ack;
  struct strbuf buf;
  int offset = 0;
  char buffer[512];
  char *dbuffer = NULL;
  int flagsp = 0;

  req = (dl_data_ack_ind_t *)buffer;
  ack = (dl_connect_res_t *)buffer;
  if(!arpcache_private) {
    arpcache_psize = 2;
    arpcache_private = malloc(arpcache_psize);
    arpcache_private[0].ip = 0;
  }
  if(s < 0) {
    if((s = open("/dev/ip", O_RDWR)) < 0) {
      wack_alarm(PRINT, "open(\"/dev/ip\", O_RDWR) for arp collection failed: %s", strerror(errno));
      return;
    }
    if(ioctl(s, I_PUSH, "arp") ||
       ioctl(s, I_PUSH, "tcp") ||
       ioctl(s, I_PUSH, "udp") ||
       ioctl(s, I_PUSH, "icmp") ) {
      wack_alarm(PRINT, "ioctl() failed: %s", strerror(errno));
      close(s);
      s = -1;
      return;
    }
  }
  req->dl_primitive = DL_UDERROR_IND;
  req->dl_dest_addr_length = 0x0c;
  req->dl_dest_addr_offset = 0x10;
  req->dl_src_addr_length = 0x80;
  req->dl_src_addr_offset = 0x104;
  buf.maxlen = 0;
  buf.len = sizeof(dl_data_ack_ind_t);
  buf.buf = (caddr_t)req;
  putmsg(s, &buf, NULL, 0);
  buf.maxlen = sizeof(buffer);
  buf.len = 0;
  buf.buf = (caddr_t)ack;
  while(1) {
    int data[7];

    buf.maxlen = sizeof(data);
    buf.len = 0;
    buf.buf = (caddr_t)data;
    if(getmsg(s, &buf, NULL, &flagsp) != MOREDATA)
      break;
    if(dbuffer)
      dbuffer = realloc(dbuffer, offset+data[6]);
    else
      dbuffer = malloc(offset+data[6]);
    buf.maxlen = data[6];
    buf.buf = (caddr_t)((char *)dbuffer + offset);
    if(getmsg(s, NULL, &buf, &flagsp))
      break;
    offset+=data[6];
  }
  if(dbuffer) {
    int r, count=0;
    for(r=0; r<offset/4; r++) { 
      unsigned int *b = (int *)dbuffer+r;
      unsigned char *h;
      struct in_addr a;
      if(b[0] == 0x8 && b[1] > 0x0 && b[1] < IFNAMSIZ) {
	char *ifname = (char *)(b+2);
	ifname[b[1]] = '\0';
        b += 2;
	b += IFNAMSIZ/sizeof(unsigned int);
	b += 4; /* something else here */
	if(*b != sizeof(ether_addr_t)) /* sizeof mac address */
	    continue;
	h = (unsigned char *)(b+1); 
	if(b[11] != sizeof(struct in_addr)) /* sizeof ipv4 address */
            continue;
        a.s_addr = b[9];  /* the address */
        if(count >= arpcache_psize) {
          arpcache_psize <<= 1;
          arpcache_private = realloc(arpcache_private, sizeof(arp_entry)*(arpcache_psize+1));
        }
        arpcache_private[count].ip = a.s_addr;
        memcpy(arpcache_private[count].mac, h, ETH_ALEN);
        wack_alarm(ARPING, "Adding: (%s) %s [%02x:%02x:%02x:%02x:%02x:%02x]",
              ifname, inet_ntoa(a),
              h[0], h[1], h[2], h[3], h[4], h[5]);
        count++;
      }
    }

    arpcache_private[count].ip = 0;
    free(dbuffer);
  }
}
#else
void sample_arp_cache() {
  if(!arpcache_private) {
    arpcache_psize = 0;
    arpcache_private = malloc(sizeof(arp_entry));
    arpcache_private[0].ip = 0;
  }
}
#endif

void insert_arp_cache_shared(arp_entry a) {
  struct in_addr ad;
  ad.s_addr = a.ip;
  if(!arpcache_shared) arpcache_shared = new_abt();
  wack_alarm(WACK_DEBUG, "Adding %s to the shared arp cache", inet_ntoa(ad));
  add_abt(arpcache_shared, a);
}
arp_entry *reference_private_arp_cache() {
  return arpcache_private;
}
arp_entry *fetch_shared_arp_cache() {
  int n=0;
  arp_entry *nl;
  if(!arpcache_shared) arpcache_shared = new_abt();
  n = arpcache_shared->size;
  nl = malloc(sizeof(arp_entry) * (n+1));
  memcpy(nl, arpcache_shared->data, sizeof(arp_entry)*n);
  nl[n].ip = 0;
  return nl;
}
