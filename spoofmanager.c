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
#include "ife.h"
#include "arpcache.h"

extern int Num_notifications;
extern struct notification Notification_table[MAX_NOTIF];

#ifndef DONT_USE_THREADS
void *arp_spoof_notifier_pthread_wrapper(void *);
#endif
void arp_spoof_notifier( int, void * );
int send_arp_spoof_arp_cache(struct interface *i, struct notification *n,
			     arp_entry *ac, int *count);
int send_arp_spoof_netblock(struct interface *i, struct notification *n,
			    int *count);

void cancel_spoofer(entry *VE) {
  /* You should have the lock when you enter this function */
#ifndef DONT_USE_THREADS
  if(VE->notifier) {
    wack_alarm(PRINT, "canceled unfinished notification thread");
    pthread_cancel(VE->notifier);
  }
#else
  E_dequeue( arp_spoof_notifier, 0, VE );
  wack_alarm( ARPING, "Dequeued arp spoof notifier." );
#endif
}
void invoke_spoofer(entry *VE) {
#ifndef DONT_USE_THREADS
  pthread_attr_t attr;
  /*	Spawn a pthread to do notifications for all VIPs in the VIF.
  	We iterate across each notification and spoofs each VIP
	for that notification */
  _ve_wait(*VE);
  cancel_spoofer(VE);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if(VE->arp_spoof_data.arpcache) free(VE->arp_spoof_data.arpcache);
  VE->arp_spoof_data.arpcache = (void *)fetch_shared_arp_cache();
  if(pthread_create(&(VE->notifier), &attr,
                    arp_spoof_notifier_pthread_wrapper, (void *)VE)) {
    wack_alarm(PRINT, "could not create wackamole notification thread.");
  } else {
    wack_alarm(PRINT, "created notification thread for virtual entry");
  }
  _ve_post(*VE);
  pthread_attr_destroy(&attr);
#else
  sp_time zero_time = { 0, 0 };

  cancel_spoofer(VE);
  if(VE->arp_spoof_data.arpcache) free(VE->arp_spoof_data.arpcache);
  VE->arp_spoof_data.arpcache = fetch_shared_arp_cache();
  E_queue( arp_spoof_notifier, 0, VE, zero_time ); 
  wack_alarm( ARPING, "Queued arp spoof notifier for virtual entry." );
#endif
}

#ifndef DONT_USE_THREADS
void *arp_spoof_notifier_pthread_wrapper ( void *data ) {
    arp_spoof_notifier( 0, data);
    /*  This is never reached as the above function results in a
        pthread_exit() call, but it will keep the compiler happy. */
    return (void *)0;
}
#endif
/* This version of arp_spoof_notifier is designed to work with events and
 * threads.
 * Treat code as a boolean representing whether or not this is the first time
 * the function has been called since the spoofer was invoked, and data as a
 * virtual entry.
 * If threaded, we do our working in a while loop with sleeps,
 * if event driven, we do our work and reschedule ourselved for subsequent
 * passes. */
void arp_spoof_notifier( int code, void *data ) {
  int i, j, old;
  entry *VE = (entry *)data;
#ifdef DONT_USE_THREADS
  sp_time sleep_time = { 1, 0 }; /* 1 second. */

#else

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);
#endif
  if( !code ) {
    /* how many VIPs we got? */
    if(_pif_ip_s(*VE)) {
      memcpy(&(VE->arp_spoof_data.lVIF[VE->arp_spoof_data.ifcount]),
             &(VE->pseudo_if), sizeof(struct interface) );
      VE->arp_spoof_data.ifcount++;
    }
    for(i=0;(i<MAX_PSEUDO) && _eif_ip_s(*VE, i);i++) {
      memcpy(&(VE->arp_spoof_data.lVIF[VE->arp_spoof_data.ifcount]), 
             &(VE->extra_ifs[i]), sizeof(struct interface) );
      VE->arp_spoof_data.ifcount++;
    }

    for(i=0;i<Num_notifications;i++) VE->arp_spoof_data.left_todo[i]=-1;
  }

#ifndef DONT_USE_THREADS
  do {
#endif
    old = 0;
    for( j = 0; j < VE->arp_spoof_data.ifcount; j++ ) {
      for( i = 0; i < Num_notifications; i++ ) {
        struct notification *ni = &Notification_table[i];
        if(_is_arp_cache(*ni)) {
          /* fetch arp cache and do that */
          old+=send_arp_spoof_arp_cache( &(VE->arp_spoof_data.lVIF[j]), ni,
                                         VE->arp_spoof_data.arpcache,
                                         &(VE->arp_spoof_data.left_todo[i]) );
        } else {
          old+=send_arp_spoof_netblock( &(VE->arp_spoof_data.lVIF[j]), ni,
                                        &(VE->arp_spoof_data.left_todo[i]) );
        }
      }
    }
#ifndef DONT_USE_THREADS
    usleep(1000000); /* 1 second */
  } while(old);
#endif

#ifdef DONT_USE_THREADS
  if( old ) {
    E_queue( arp_spoof_notifier, 1, VE, sleep_time );
    wack_alarm( PRINT, "Re-queued arp spoof notifier for virtual entry." );
  }
#else
  wack_alarm( PRINT, "Finished arp spoof notification for virtual entry." );
  _ve_wait(*VE);
  VE->notifier = 0;
  _ve_post(*VE);
  wack_alarm( PRINT, "Thread exiting." );
  pthread_exit(0);
#endif
}

#define cidrsize(a) (0xffffffff - ((a).netmask.s_addr) + 1)

unsigned int calc_new_cidr(struct interface *i1, struct interface *i2,
		   struct interface *out) {
  unsigned int s1, s2, e1, e2;
  struct in_addr a;
  s1 = ntohl(_if_ip_s(*i1) & _if_networkm_s(*i1));
  e1 = ntohl(_if_ip_s(*i1) | (~(_if_networkm_s(*i1))));
  s2 = ntohl(_if_ip_s(*i2) & _if_nm_s(*i2));
  e2 = ntohl(_if_ip_s(*i2) | (~(_if_nm_s(*i2))));
/* only debug */
  a.s_addr = htonl(s1);
  wack_alarm(ARPING, "i1: s1 -> %s", inet_ntoa(a));
  a.s_addr = htonl(e1);
  wack_alarm(ARPING, "i1: e1 -> %s", inet_ntoa(a));
  a.s_addr = htonl(s2);
  wack_alarm(ARPING, "i2: s2 -> %s", inet_ntoa(a));
  a.s_addr = htonl(e2);
  wack_alarm(ARPING, "i2: e2 -> %s", inet_ntoa(a));
/* only debug */
  s1 = MAX(s1,s2);
  e1 = MIN(e1,e2);
  if(e1<s1) {
    return 0;
  }
  _if_ip_s(*out) = htonl(s1);
  strcpy(out->ifname, i1->ifname);
  return e1-s1+1;
}
int send_arp_spoof_arp_cache(struct interface *i, struct notification *n,
			     arp_entry *ac, int *count) {
  /* Send notification about i to n */
  int s, c;
  struct interface arpi, dest;
  arp_entry *acp = ac;
wack_alarm(ARPING, "Count: %d", *count);
  if(*count == -1) {
    *count = 0;
    while(acp->ip) { acp++; (*count)++; }
  }
wack_alarm(ARPING, "Count: %d", *count);
  if(*count == 0) return 0;
  arpi.ifname[0]='\0';
  arpi.netmask.s_addr = ~0;
  for(s=0,c=0;s<*count && (!n->throttle || (c<n->throttle));s++) {
    if(ac[s].ip == 0) continue;
    arpi.ipaddr.s_addr = ac[s].ip;
    if(calc_new_cidr(i, &arpi, &dest)) {
	/* This arp entry can be delivered for this vip */
wack_alarm(ARPING, "Spoofing (arp-cache): from %s:%s", i->ifname, inet_ntoa(_if_ip(*i)));
wack_alarm(ARPING, "Spoofing (arp-cache): to %s:%s (ping %d)", i->ifname, inet_ntoa(dest.ipaddr), n->ping);
      if_send_spoof_request(i->ifname,
			    _if_ip_s(*i), _if_ip_s(dest), ac[s].mac,
			    2, n->ping);
	/* Zero out this entry so we ignore it if there is another round */
	/* this is a _copy_ of the shared arp cache.. so, we can mutilate it */
      ac[s].ip = 0;
      c++;
    }
  }
  return c;
}
int send_arp_spoof_netblock(struct interface *i, struct notification *n,
			    int *count) {
  /* Send notification about i to n */
  struct interface newcidr;
  int csize, c, s;

  if(*count == 0) return 0;
  csize = calc_new_cidr(i, &(n->destination), &newcidr);
  if(strcmp(i->ifname, n->destination.ifname)) {
    return 0;
  }
  if(newcidr.ifname[0] == '\0') return 0;
  if(*count == -1) *count = csize;
  _if_ip_s(newcidr) = htonl(ntohl(_if_ip_s(newcidr))+(csize-*count));
  for(s=0,c=(csize-*count);(c<csize) && (!n->throttle || (s<n->throttle));c++) {
wack_alarm(ARPING, "Spoofing (static): %s:%s", i->ifname, inet_ntoa(newcidr.ipaddr));
    if_send_spoof_request(i->ifname,
			  _if_ip_s(*i), _if_ip_s(newcidr), NULL,
			  2, n->ping);
    _if_ip_s(newcidr) = htonl(ntohl(_if_ip_s(newcidr))+1);
    s++;
    (*count)--;
  }
  return s;
}
