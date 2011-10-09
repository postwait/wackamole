%{
/* ======================================================================
 * Copyright (c) 2002 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#include "config.h"
#include "userloader.h"
#include "config_help.h"
#include "alarm.h"

#ifdef USE_EMBEDDED_PERL
#include "perl.h"
#endif

extern int semantic_errors;
int line_num;
extern int buffsize;
extern char *wacktext;
WACKSTYPE wacklval;

int wacklex();

	char            Spread_name[80];
        int              Spread_retry_interval;
        char            Spread_group[MAX_GROUP_NAME];
	char		control_socket[MAXPATHLEN] = "";
	char		default_library_path[MAXPATHLEN] = ".";
        sp_time         Maturity_timeout = { 5, 0 };
        sp_time         Balance_timer = { 4, 0 };
        sp_time         ArpRefresh_timer = { 60, 0 };
        int             Balance_rate = 1;
        int             Complete_balance = 0;
	int             Num_pseudo = 0;
	int             Num_prefer = 0;
	int		Num_notifications = 0;
	address         Prefer_address[MAX_PSEUDO];
	entry           Allocation_table[MAX_PSEUDO];
	entry           Old_table[MAX_PSEUDO];
struct notification	Notification_table[MAX_NOTIF];

entry *tmpe = Allocation_table;
int iplistn=0;

static void add_prefer(int);
static void lookup_name(char *, struct in_addr *);
static void wackstype_to_interface(YYSTYPE *, struct interface *);

#define SPIT while(0) printf
%}
%start Config
%token W_SPREAD W_SPREADRETRYINTERVAL W_LOG W_GROUP W_VIFS W_IPADDR W_PREFER
%token W_NOTIFY W_NONE W_ARPCACHE W_PING W_AQPR W_ALL W_THROTTLE W_OPENBRACE
%token W_CLOSEBRACE W_EQUALS W_STRING W_MATURE W_TIMEINTERVAL W_BALANCE
%token W_NUMBER W_INTERVAL W_CONTROL W_RUNDYNAMIC W_ACTION_TYPE W_MODULEDIR
%token W_PERLUSELIB W_PERLUSE
%%
Config		:	Settings
			{ int i, n;
			  SPIT("Main config:\n");
			  for(i=0;i<Num_pseudo;i++) {
			    entry *ve = &Allocation_table[i];
			    SPIT("\tVIF:\t[%s] %s ",
				ve->pseudo_if.ifname,
				inet_ntoa(ve->pseudo_if.ipaddr));
			    SPIT("netmask %s ",
				inet_ntoa(ve->pseudo_if.netmask));
			    SPIT("broadcast %s\n",
				inet_ntoa(ve->pseudo_if.bcast));
			    SPIT("living in a %s\n",
				inet_ntoa(ve->pseudo_if.network));
			    for(n=0;
				n<MAX_PSEUDO && ve->extra_ifs[n].ipaddr.s_addr;
				n++) {
			      SPIT("\t\tVE:\t[%s] %s ",
				ve->extra_ifs[n].ifname,
				inet_ntoa(ve->extra_ifs[n].ipaddr));
			      SPIT("netmask %s ",
				inet_ntoa(ve->extra_ifs[n].netmask));
			      SPIT("broadcast %s\n",
				inet_ntoa(ve->extra_ifs[n].bcast));
			      SPIT("living in a %s\n",
				inet_ntoa(ve->extra_ifs[n].network));
			    }
			  }
			}

Settings	:	Setting Settings
		|
		;

Setting		:	W_SPREAD W_EQUALS W_STRING
			{ snprintf(Spread_name, 80, "%s", $3.string);
			  SPIT("Setting Spread: %s\n", Spread_name); }
		|	W_SPREAD W_EQUALS W_NUMBER
			{ snprintf(Spread_name, 80, "%d", $3.number);
			  SPIT("Setting Spread: %s\n", Spread_name); }
		|	W_SPREADRETRYINTERVAL W_EQUALS W_TIMEINTERVAL
			{ Spread_retry_interval = $3.tv.tv_sec;
			  SPIT("Setting SpreadRetryInteral: %d seconds\n", Spread_retry_interval); }
		|	W_GROUP W_EQUALS W_STRING
			{ snprintf(Spread_group, MAX_GROUP_NAME, "%s", $3.string);
			  SPIT("Setting Group: %s\n", Spread_group); }
		|	W_LOG     W_EQUALS W_STRING
		|	W_CONTROL W_EQUALS W_STRING
			{ snprintf(control_socket, MAXPATHLEN, $3.string); }
		|	W_MATURE W_EQUALS W_TIMEINTERVAL
			{ Maturity_timeout.sec = $3.tv.tv_sec;
			  Maturity_timeout.usec = 0;
			  SPIT("Setting Maturity Interval: %ld sec\n",
				(long int)Maturity_timeout.sec);
			}
		|	W_ARPCACHE W_EQUALS W_TIMEINTERVAL
			{ ArpRefresh_timer.sec = $3.tv.tv_sec;
			  ArpRefresh_timer.usec = 0;
			  SPIT("Setting Maturity Interval: %ld sec\n",
				(long int)ArpRefresh_timer.sec);
			}
		|	W_BALANCE W_OPENBRACE BParams W_CLOSEBRACE
		|	W_NOTIFY W_ARPCACHE
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  strcpy(ni->destination.ifname, "arp-cache");
			  SPIT("Adding notification %d: arp-cache\n", Num_notifications);
			}
		|	W_NOTIFY ClosedNOTIFYList
		|	W_PREFER W_NONE
			{ SPIT("Prefer NONE\n");
			  add_prefer(0);
			}
		|	W_PREFER W_STRING
			{ struct in_addr ipaddr;
			  lookup_name($1.string, &ipaddr);
			  if(ipaddr.s_addr == 0) {
			    wack_alarm(PRINT, "Couldn't lookup %s.\n", $1.string);
			  } else {
			    add_prefer(ipaddr.s_addr);
			    SPIT("Prefer (%s) %s\n",
				$1.string, inet_ntoa(ipaddr));
			  }
			}
		|	W_PREFER W_IPADDR
			{ unsigned int mask;
			  address Addr;
			  mask = htonl((0xffffffff << (32-$2.ip.mask)));
			  $2.ip.addr.s_addr &= mask;
			  Addr = $2.ip.addr.s_addr;
			  while((Addr & mask) == ($2.ip.addr.s_addr & mask)) {
			    add_prefer($2.ip.addr.s_addr);
			    SPIT("Prefer %s\n", inet_ntoa($2.ip.addr));
			    $2.ip.addr.s_addr++;
			  }
			}
		|	W_MODULEDIR W_EQUALS W_STRING
			{ strncpy(default_library_path, $3.string, MAXPATHLEN); }
		|	W_PERLUSE W_STRING
			{
#ifdef USE_EMBEDDED_PERL
			  perl_use($2.string);
#else
			  wack_alarm(EXIT, "PerlUseLib used, without embedded perl interpreter");
#endif
			}
		|	W_PERLUSELIB W_STRING
			{
#ifdef USE_EMBEDDED_PERL
			  perl_inc($2.string);
#else
			  wack_alarm(EXIT, "PerlUseLib used, without embedded perl interpreter");
#endif
			}
		|	W_RUNDYNAMIC W_STRING W_ACTION_TYPE
			{ char *path = $2.string;
                          if(strstr(path, "::") || !strchr(path, ':')) {
#ifdef USE_EMBEDDED_PERL
			    register_perl(path, $3.number);
#else
			    wack_alarm(EXIT, "RunDynamic with perl-style parameter, without embedded perl interpreter");
#endif
			  } else {
			    char *func = strchr(path, ':');
			    if(func) { *(func++) = '\0'; }
			    register_shared(path, func, $3.number);
			  }
			}
		|	W_PREFER ClosedPIPList
		|	W_VIFS   ClosedVIFList

BParams		:	BParam BParams
		|	BParam

BParam		:	W_AQPR W_EQUALS W_NUMBER
			{ Balance_rate = $3.number;
			  SPIT("Setting Balance Rate: %d\n", Balance_rate); }
		|	W_AQPR W_EQUALS W_ALL
			{ Complete_balance = 1;
			  SPIT("Setting Immediate Balance\n"); }
		|	W_INTERVAL W_EQUALS W_TIMEINTERVAL
			{ Balance_timer.sec = $3.tv.tv_sec;
			  Balance_timer.usec = 0;
			  SPIT("Setting Balance Interval: %ld sec\n",
				(long int)Balance_timer.sec); }

ClosedNOTIFYList	:	W_NONE
			|	W_OPENBRACE NIPList W_CLOSEBRACE

NIPList		:	NIPList W_IPADDR W_THROTTLE W_NUMBER
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  wackstype_to_interface(&($2), &(ni->destination));
			  ni->throttle = $4.number;
			  ni->ping = 0;
			  SPIT("Adding notification %d: %s:%s:%s (%d/sec)\n", Num_notifications, ni->destination.ifname, inet_ntoa(ni->destination.ipaddr), inet_ntoa(ni->destination.netmask), ni->throttle);
			}
		|	NIPList W_IPADDR W_THROTTLE W_NUMBER W_PING
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  wackstype_to_interface(&($2), &(ni->destination));
			  ni->throttle = $4.number;
			  ni->ping = 1;
			  SPIT("Adding notification %d: %s:%s:%s (%d/sec) with ping\n", Num_notifications, ni->destination.ifname, inet_ntoa(ni->destination.ipaddr), inet_ntoa(ni->destination.netmask), ni->throttle);
			}
		|	NIPList W_IPADDR
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  wackstype_to_interface(&($2), &(ni->destination));
			  ni->throttle = 0;
			  ni->ping = 0;
			  SPIT("Adding notification %d: %s:%s:%s\n", Num_notifications, ni->destination.ifname, inet_ntoa(ni->destination.ipaddr), inet_ntoa(ni->destination.netmask));
			}
		|	NIPList W_IPADDR W_PING
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  wackstype_to_interface(&($2), &(ni->destination));
			  ni->throttle = 0;
			  ni->ping = 1;
			  SPIT("Adding notification %d: %s:%s:%s with ping\n", Num_notifications, ni->destination.ifname, inet_ntoa(ni->destination.ipaddr), inet_ntoa(ni->destination.netmask));
			}
		|	NIPList W_ARPCACHE
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  strcpy(ni->destination.ifname, "arp-cache");
			  ni->throttle = 0;
			  ni->ping = 0;
			  SPIT("Adding notification %d: arp-cache\n", Num_notifications);
			}
		|	NIPList W_ARPCACHE W_PING
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  strcpy(ni->destination.ifname, "arp-cache");
			  ni->throttle = 0;
			  ni->ping = 1;
			  SPIT("Adding notification %d: arp-cache with ping\n", Num_notifications);
			}
		|	W_IPADDR W_THROTTLE W_NUMBER
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  wackstype_to_interface(&($1), &(ni->destination));
			  ni->throttle = $3.number;
			  ni->ping = 0;
			  SPIT("Adding notification %d: %s:%s:%s (%d/sec)\n", Num_notifications, ni->destination.ifname, inet_ntoa(ni->destination.ipaddr), inet_ntoa(ni->destination.netmask), ni->throttle);
			}
		|	W_IPADDR W_THROTTLE W_NUMBER W_PING
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  wackstype_to_interface(&($1), &(ni->destination));
			  ni->throttle = $3.number;
			  ni->ping = 1;
			  SPIT("Adding notification %d: %s:%s:%s (%d/sec) with ping\n", Num_notifications, ni->destination.ifname, inet_ntoa(ni->destination.ipaddr), inet_ntoa(ni->destination.netmask), ni->throttle);
			}
		|	W_IPADDR
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  wackstype_to_interface(&($1), &(ni->destination));
			  ni->throttle = 0;
			  ni->ping = 0;
			  SPIT("Adding notification %d: %s:%s:%s\n", Num_notifications, ni->destination.ifname, inet_ntoa(ni->destination.ipaddr), inet_ntoa(ni->destination.netmask));
			}
		|	W_IPADDR W_PING
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  wackstype_to_interface(&($1), &(ni->destination));
			  ni->throttle = 0;
			  ni->ping = 1;
			  SPIT("Adding notification %d: %s:%s:%s with ping\n", Num_notifications, ni->destination.ifname, inet_ntoa(ni->destination.ipaddr), inet_ntoa(ni->destination.netmask));
			}
		|	W_ARPCACHE
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  strcpy(ni->destination.ifname, "arp-cache");
			  ni->throttle = 0;
			  ni->ping = 0;
			  SPIT("Adding notification %d: arp-cache\n", Num_notifications);
			}
		|	W_ARPCACHE W_PING
			{ struct notification *ni = &Notification_table[Num_notifications++];
			  strcpy(ni->destination.ifname, "arp-cache");
			  ni->throttle = 0;
			  ni->ping = 1;
			  SPIT("Adding notification %d: arp-cache with ping\n", Num_notifications);
			}

ClosedPIPList	:	W_OPENBRACE PIPList W_CLOSEBRACE

PIPList		:	PIPList W_IPADDR
			{ unsigned int mask;
			  address Addr;
			  mask = (0xffffffff << (32-$2.ip.mask));
			  $2.ip.addr.s_addr &= mask;
			  Addr = $2.ip.addr.s_addr;
			  while((Addr & mask) == ($2.ip.addr.s_addr & mask)) {
			    add_prefer($2.ip.addr.s_addr);
			    SPIT("Prefer %s\n", inet_ntoa($2.ip.addr));
			    $2.ip.addr.s_addr++;
			  }
			}
		|	W_IPADDR
			{ unsigned int mask;
			  address Addr;
			  mask = (0xffffffff << (32-$1.ip.mask));
			  $1.ip.addr.s_addr &= mask;
			  Addr = $1.ip.addr.s_addr;
			  while((Addr & mask) == ($1.ip.addr.s_addr & mask)) {
			    add_prefer($1.ip.addr.s_addr);
			    SPIT("Prefer %s\n", inet_ntoa($1.ip.addr));
			    $1.ip.addr.s_addr++;
			  }
			}

ClosedIPList	:	W_OPENBRACE IPList W_CLOSEBRACE
			{ if(iplistn<MAX_PSEUDO) {
			    tmpe[Num_pseudo].extra_ifs[iplistn].ipaddr.s_addr=0;
			  }
			  iplistn=0;
			}

IPList		:	IPList W_IPADDR
			{ unsigned int mask;
			  mask = htonl(0xffffffff << (32-$2.ip.mask));
			  memcpy(&tmpe[Num_pseudo].extra_ifs[iplistn].ifname,
				 $2.ip.iface, IFNAMSIZ);
			  tmpe[Num_pseudo].extra_ifs[iplistn].ipaddr = $2.ip.addr;
			  tmpe[Num_pseudo].extra_ifs[iplistn].netmask.s_addr = mask;
			  tmpe[Num_pseudo].extra_ifs[iplistn].bcast.s_addr =
				(($2.ip.addr.s_addr & mask) | (~mask));
			  tmpe[Num_pseudo].extra_ifs[iplistn].network.s_addr =
				$2.ip.network.s_addr;
			  iplistn++;
			}
		|	W_IPADDR
			{ unsigned int mask;
			  mask = htonl(0xffffffff << (32-$1.ip.mask));
			  memcpy(&tmpe[Num_pseudo].pseudo_if.ifname,
				 $1.ip.iface, IFNAMSIZ);
			  tmpe[Num_pseudo].pseudo_if.ipaddr = $1.ip.addr;
			  tmpe[Num_pseudo].pseudo_if.netmask.s_addr = mask;
			  tmpe[Num_pseudo].pseudo_if.bcast.s_addr =
				(($1.ip.addr.s_addr & mask) | (~mask));
			  tmpe[Num_pseudo].pseudo_if.network.s_addr =
				$1.ip.network.s_addr;
			}

ClosedVIFList	:	W_OPENBRACE VIFList W_CLOSEBRACE

VIFList		:	VIFList VIF
			{ Num_pseudo++; }
		|	VIF
			{ Num_pseudo++; }

VIF		:	W_IPADDR
			{ unsigned int mask;
			  mask = htonl(0xffffffff << (32-$1.ip.mask));
			  memcpy(&tmpe[Num_pseudo].pseudo_if.ifname,
				 $1.ip.iface, IFNAMSIZ);
			  tmpe[Num_pseudo].pseudo_if.ipaddr = $1.ip.addr;
			  tmpe[Num_pseudo].pseudo_if.netmask.s_addr = mask;
			  tmpe[Num_pseudo].pseudo_if.bcast.s_addr =
				(($1.ip.addr.s_addr & mask) | (~mask));
			  tmpe[Num_pseudo].pseudo_if.network.s_addr =
				$1.ip.network.s_addr;
			}
		|	ClosedIPList

%%
int wackerror(char *str) {
  fprintf(stderr, "Parser error on or before line %d\n", line_num);
  fprintf(stderr, "Offending token: %s\n", wacktext);
  return -1;
}

static void add_prefer(int addr) {
  if(Num_prefer >= MAX_PSEUDO) {
    wack_alarm(PRINT, "Only %d prefered allowed.\n", MAX_PSEUDO);
    return;
  }
  Prefer_address[Num_prefer++] = addr;
}
static void lookup_name(char *name, struct in_addr *ipaddr) {
}
static void wackstype_to_interface(YYSTYPE *in, struct interface *out) {
  unsigned int mask;
  mask = htonl((0xffffffff << (32-in->ip.mask)));
  memcpy(out->ifname,
	 in->ip.iface, IFNAMSIZ);
  out->ipaddr = in->ip.addr;
  out->netmask.s_addr = mask;
  out->bcast.s_addr =
	((in->ip.addr.s_addr & mask) | (~mask));
  out->network = in->ip.network;
}
