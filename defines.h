#ifndef _DEFINES_H
#define _DEFINES_H

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <sys/types.h>
#ifndef WIN32
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_DLPI_H
#include <sys/dlpi.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_PCAP_H
#include <pcap.h>
#endif
#ifdef HAVE_NET_BPF_H
#include <net/bpf.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#endif
#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_BITYPES_H
# include <sys/bitypes.h>
#endif
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_CDEFS_H
# include <sys/cdefs.h> /* For __P() */
#endif
#ifdef HAVE_SYS_SYSMACROS_H
# include <sys/sysmacros.h> /* For MIN, MAX, etc */
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h> /* For S_* constants and macros */
#endif
#ifdef HAVE_NEXT
#  include <libc.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#ifdef HAVE_SP_H
#include <sp.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifndef MAXPATHLEN
# ifdef PATH_MAX
#  define MAXPATHLEN PATH_MAX
# else /* PATH_MAX */
#  define MAXPATHLEN 64 /* Should be safe */
# endif /* PATH_MAX */
#endif /* MAXPATHLEN */

#ifndef STDIN_FILENO
# define STDIN_FILENO    0
#endif
#ifndef STDOUT_FILENO
# define STDOUT_FILENO   1
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO   2
#endif

#ifndef O_NONBLOCK	/* Non Blocking Open */
# define O_NONBLOCK      00004
#endif

/* Types */

/* If sys/types.h does not supply intXX_t, supply them ourselves */
/* (or die trying) */

#ifndef HAVE_SOCKOPT_LEN_T
typedef long sockopt_len_t;
#endif

#ifndef HAVE_U_INT
typedef unsigned int u_int;
#endif

#ifndef HAVE_INTXX_T
# if (SIZEOF_CHAR == 1)
typedef char int8_t;
# else
#  error "8 bit int type not found."
# endif
# if (SIZEOF_SHORT_INT == 2)
typedef short int int16_t;
# else
#  ifdef _CRAY
typedef long  int16_t;
#  else
#   error "16 bit int type not found."
#  endif /* _CRAY */
# endif
# if (SIZEOF_INT == 4)
typedef int int32_t;
# else
#  ifdef _CRAY
typedef long  int32_t;
#  else
#   error "32 bit int type not found."
#  endif /* _CRAY */
# endif
#endif

/* If sys/types.h does not supply u_intXX_t, supply them ourselves */
#ifndef HAVE_U_INTXX_T
# ifdef HAVE_UINTXX_T
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
# define HAVE_U_INTXX_T 1
# else
#  if (SIZEOF_CHAR == 1)
typedef unsigned char u_int8_t;
#  else
#   error "8 bit int type not found."
#  endif
#  if (SIZEOF_SHORT_INT == 2)
typedef unsigned short int u_int16_t;
#  else
#   ifdef _CRAY
typedef unsigned long  u_int16_t;
#   else
#    error "16 bit int type not found."
#   endif
#  endif
#  if (SIZEOF_INT == 4)
typedef unsigned int u_int32_t;
#  else
#   ifdef _CRAY
typedef unsigned long  u_int32_t;
#   else
#    error "32 bit int type not found."
#   endif
#  endif
# endif
#endif

/* 64-bit types */
#ifndef HAVE_INT64_T
# if (SIZEOF_LONG_INT == 8)
typedef long int int64_t;
#   define HAVE_INT64_T 1
# else
#  if (SIZEOF_LONG_LONG_INT == 8)
typedef long long int int64_t;
#   define HAVE_INT64_T 1
#   define HAVE_LONG_LONG_INT
#  endif
# endif
#endif
#ifndef HAVE_U_INT64_T
# if (SIZEOF_LONG_INT == 8)
typedef unsigned long int u_int64_t;
#   define HAVE_U_INT64_T 1
# else
#  if (SIZEOF_LONG_LONG_INT == 8)
typedef unsigned long long int u_int64_t;
#   define HAVE_U_INT64_T 1
#  endif
# endif
#endif

#ifndef HAVE_SOCKLEN_T
typedef unsigned int socklen_t;
# define HAVE_SOCKLEN_T
#endif /* HAVE_SOCKLEN_T */

#ifndef HAVE_SIZE_T
typedef unsigned int size_t;
# define HAVE_SIZE_T
#endif /* HAVE_SIZE_T */

#ifndef HAVE_SSIZE_T
typedef int ssize_t;
# define HAVE_SSIZE_T
#endif /* HAVE_SSIZE_T */

#ifndef HAVE_CLOCK_T
typedef long clock_t;
# define HAVE_CLOCK_T
#endif /* HAVE_CLOCK_T */

#ifndef HAVE_SA_FAMILY_T
typedef int sa_family_t;
# define HAVE_SA_FAMILY_T
#endif /* HAVE_SA_FAMILY_T */

#ifndef HAVE_PID_T
typedef int pid_t;
# define HAVE_PID_T
#endif /* HAVE_PID_T */

#if !defined(HAVE_SS_FAMILY_IN_SS) && defined(HAVE___SS_FAMILY_IN_SS)
# define ss_family __ss_family
#endif /* !defined(HAVE_SS_FAMILY_IN_SS) && defined(HAVE_SA_FAMILY_IN_SS) */

#ifndef HAVE_SYS_UN_H
struct	sockaddr_un {
	short	sun_family;		/* AF_UNIX */
	char	sun_path[108];		/* path name (gag) */
};
#endif /* HAVE_SYS_UN_H */

/* Macros */

#ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef roundup
# define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))
#endif

#ifndef timersub
#define timersub(a, b, result)					\
   do {								\
      (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;		\
      (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;		\
      if ((result)->tv_usec < 0) {				\
	 --(result)->tv_sec;					\
	 (result)->tv_usec += 1000000;				\
      }								\
   } while (0)
#endif

#ifndef __P
# define __P(x) x
#endif

#if !defined(__GNUC__) || (__GNUC__ < 2)
# define __attribute__(x)
#endif /* !defined(__GNUC__) || (__GNUC__ < 2) */

#ifndef SUN_LEN
#define SUN_LEN(su) \
	(sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif /* SUN_LEN */

#if !defined(HAVE_MEMMOVE) && defined(HAVE_BCOPY)
# define memmove(s1, s2, n) bcopy((s2), (s1), (n))
#endif /* !defined(HAVE_MEMMOVE) && defined(HAVE_BCOPY) */

/* which type of time to use? (api.c) */
#ifdef HAVE_SYS_TIME_H
#  define USE_TIMEVAL
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
#ifndef ETH_HLEN
#define ETH_HLEN 14
#endif
#ifndef ETH_P_ALL
#define ETH_P_ALL 0x0003
#endif
#ifndef ETH_P_IP
#define ETH_P_IP 0x0800
#endif
#ifndef ETH_P_ARP
#define ETH_P_ARP 0x0806
#endif

/** end of login recorder definitions */
#define MAX_PSEUDO      254
#define MAX_NOTIF	1024
#define MAX_DEP_IF      30
#define MAX_MESS_LEN    100000
#define address         unsigned int
#define int32           int

struct interface {
  char ifname[IFNAMSIZ];
  struct in_addr ipaddr;
  struct in_addr bcast;
  struct in_addr netmask;
  struct in_addr network;
  unsigned char mac[ETH_ALEN];
};

#define _if_ip(a) ((a).ipaddr)
#define _if_ip_s(a) ((a).ipaddr.s_addr)
#define _if_bc(a) ((a).bcast)
#define _if_bc_s(a) ((a).bcast.s_addr)
#define _if_nm(a) ((a).netmask)
#define _if_nm_s(a) ((a).netmask.s_addr)
#define _if_networkm(a) ((a).network)
#define _if_networkm_s(a) ((a).network.s_addr)

struct notification {
  struct interface destination;
  int throttle;
  int ping;
};

#define _is_arp_cache(a) (strncmp((a).destination.ifname, "arp-cache", 9) == 0)

typedef struct _arp_entry {
  address ip;
  unsigned char mac[ETH_ALEN];
} arp_entry;

typedef struct  dummy_entry{
#ifndef DONT_USE_THREADS
  pthread_t		notifier;
  pthread_mutex_t	lock;
#endif
  void			*thread_data;
  struct {
    int                 ifcount;
    struct interface    lVIF[MAX_PSEUDO+1];
    int                 left_todo[MAX_NOTIF];
    arp_entry           *arpcache;
  } arp_spoof_data;    

  struct interface      pseudo_if;
  struct interface      extra_ifs[MAX_DEP_IF];
  struct interface      real_if;
  int           claim_priority;
} entry;

#ifndef DONT_USE_THREADS
#define _ve_lock_init(a) pthread_mutex_init(&((a).lock), NULL)
#define _ve_post(a)      pthread_mutex_unlock(&((a).lock))
#define _ve_wait(a)      pthread_mutex_lock(&((a).lock))
#define _ve_lock_free(a) pthread_mutex_destroy(&((a).lock))

#define _lock_init(a) pthread_mutex_init(&(a), NULL)
#define _post(a)      pthread_mutex_unlock(&(a))
#define _wait(a)      pthread_mutex_lock(&(a))
#define _lock_free(a) pthread_mutex_destroy(&(a))
#else
#define _ve_lock_init(a) (a)
#define _ve_post(a)      (a)
#define _ve_wait(a)      (a)
#define _ve_lock_free(a) (a)

#define _lock_init(a) (a)
#define _post(a)      (a)
#define _wait(a)      (a)
#define _lock_free(a) (a)
#endif

#define _rif_ip(a) ((a).real_if.ipaddr)
#define _rif_ip_s(a) ((a).real_if.ipaddr.s_addr)
#define _pif_ip(a) ((a).pseudo_if.ipaddr)
#define _pif_ip_s(a) ((a).pseudo_if.ipaddr.s_addr)
#define _eif_ip(a, b) ((a).extra_ifs[b].ipaddr)
#define _eif_ip_s(a, b) ((a).extra_ifs[b].ipaddr.s_addr)
        
typedef struct  dummy_member{
  struct interface      real_if;
  char          private_group_name[MAX_GROUP_NAME];
  int           num_allocated;
  int           got_state_from; 
} member;

#define ARPING 0x00000010

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#endif /* _DEFINES_H */
