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
 *  Copyright (C) 2000-2004 The Johns Hopkins University
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
#include "wackamole.h"
#include "version.h"
#include "configuration.h"
#include "alarm.h"
#include "ife.h"
#include "arpcache.h"
#include "spoofmanager.h"
#include "control.h"
#include "userloader.h"
#ifdef USE_EMBEDDED_PERL
#include "perl.h"
#endif

/* message types */
#define		STATE_MESS	1
#define		MATURING_MESS	2
#define		BALANCE_MESS	3
#define		OPERATOR_MESS	4
#define		ARPCACHE_MESS	5

/* automaton states */
#define		BOOT		1
#define		RUN		2
#define		GATHER		3
#define		BALANCE		4

/* claim priority */
#define		UNCLAIMED 	0
#define		CLAIMED_BOOT	1	/* pseudo ip is taken by machine but not by booting wackamole */
#define		CLAIMED		2	/* pseudo ip is managed by this wackamole */
#define		PREFER_BOOT	3	/* wackamole is the preferred one and it is taken by machine at boot */
#define		PREFER	 	4	/* wackamole is the preferred one to manage this pseudo ip */
#define		PREFER_CLAIMED  5	/* wackamole is the preferred one and is actively managing it */

/* ALARM Extension */
#define LOGIC 0x00000008

/* state structures */
static  int		global_exit = 0;
static	int		State;
static  int32           Old_maturity;
static  int32		Maturity;
	int		spread_lock;

/* static  struct interface iface, idown; */
struct  in_addr         vip_in_addr;
/* static  char            ifname[IFNAMSIZ]; */
/* static  char            my_host_name[55]; */
/* static  int             my_host_name_len; */
static	int		Num_members;
static	member		Members[MAX_PSEUDO];

static  member		My;
static	int		My_index; /* in Members */

static	group_id	Gid;

static  pid_t		pid;

/* Spread structures */
	mailbox		Mbox = -1;
static	char		User[80];
static	char		Private_group[MAX_GROUP_NAME];

static	char    	Mess[MAX_MESS_LEN];
static	int16		Mess_type;
static	int		Endian_mismatch;
static	int		Service_type, Num_groups;
static	int		Num_pending_states;
static	char		Target_groups[MAX_PSEUDO][MAX_GROUP_NAME];
static	char		Sender[MAX_GROUP_NAME];
static  char            File_name[100];
static  int		Debug = 0;

static	void	Usage( int argc, char *argv[] );
static	void	Handle_network(int,int,void*);

static	void	Handle_membership();
static	void	Handle_state();
static	void	Handle_mature();
static	void	Handle_balance();
static	void	Handle_operator();
static	void	Handle_arp_cache(arp_entry *, int);

static	void	Send_state_message();
static  void	Send_local_arp_cache_repeat();
static  void	Send_local_arp_cache(int,void*);
static	void	Turn_mature(int,void*);

static	void	Wackamole_init();
static	void	Bye();

static  void    Priority_claim();
static  void    Claim_unclaimed();
static  entry  *findVEFromAddress( address Addr );
/* static  void    AcquireAddress( address Addr ); */
static  void    Acquire( entry *VE );
static  void    Handle_Post_Acquire();
static  void    ReleaseAddress( address Addr );
static  void    Release( entry *VE );
static  void    Handle_Post_Release();
static  void    Balance(int, void *);
static  void    Print_alloc();
/* static  void    String_addr( address Addr, char IP[16] ); */
static  void    Sig_handler(int signum);
	void    Clean_up();
	void    Spread_reconnect(int ret);
#ifdef WIN32
static BOOL WINAPI ConsoleEventHandlerRoutine(DWORD dwCtrlType);
#endif

/*************************************************/
static void notice() {
  wack_alarm( PRINT, "\n/==============================================================================\\");
  wack_alarm( PRINT, "| The Wackamole Program.                                                       |");
  wack_alarm( PRINT, "| Copyright (c) 2000-2004 The Johns Hopkins University                         |"); 
  wack_alarm( PRINT, "| All rights reserved.                                                         |");
  wack_alarm( PRINT, "|                                                                              |");
  wack_alarm( PRINT, "| Wackamole is developed at the Center for Networking and Distributed Systems, |");
  wack_alarm( PRINT, "| The Johns Hopkins University.                                                |");
  wack_alarm( PRINT, "|                                                                              |");
  wack_alarm( PRINT, "| The Wackamole package is licensed under the CNDS Open Source License         |");
  wack_alarm( PRINT, "| You may only use this software in compliance with the License.               |");
  wack_alarm( PRINT, "| A copy of the license can be found at                                        |");
  wack_alarm( PRINT, "| http://www.backhand.org/wackamole/license                                    |");
  wack_alarm( PRINT, "|                                                                              |");
  wack_alarm( PRINT, "| This product uses the Spread toolkit, developed by Spread Concepts LLC.      |");
  wack_alarm( PRINT, "| For more information about Spread see http://www.spread.org                  |");
  wack_alarm( PRINT, "|                                                                              |");
  wack_alarm( PRINT, "| This software is distributed on an \"AS IS\" basis, WITHOUT WARRANTY OF        |");
  wack_alarm( PRINT, "| ANY KIND, either express or implied.                                         |");
  wack_alarm( PRINT, "|                                                                              |");
  wack_alarm( PRINT, "| Creators:                                                                    |");
  wack_alarm( PRINT, "|    Yair Amir             yairamir@cnds.jhu.edu                               |");
  wack_alarm( PRINT, "|    Ryan Caudy            wyvern@cnds.jhu.edu                                 |");
  wack_alarm( PRINT, "|    Aashima Munjal        munjal@jhu.edu                                      |");
  wack_alarm( PRINT, "|    Theo Schlossnagle     jesus@cnds.jhu.edu                                  |");
  wack_alarm( PRINT, "|                                                                              |");
  wack_alarm( PRINT, "| For a full list of contributors, see Readme.txt in the distribution.         |");
  wack_alarm( PRINT, "|                                                                              |");
  wack_alarm( PRINT, "| WWW:     www.backhand.org     www.cnds.jhu.edu                               |");
  wack_alarm( PRINT, "| Contact: wackamole@backhand.org                                              |");
  wack_alarm( PRINT, "|                                                                              |");
  wack_alarm( PRINT, "| Version %-20s                                                 |", VERSION_STRING);
  wack_alarm( PRINT, "| Released %-20s                                                |", VERSION_RELEASE_DATE); 
  wack_alarm( PRINT, "|                                                                              |");
  wack_alarm( PRINT, "\\==============================================================================|");
}
int main( int argc, char *argv[] )
{
  int		fd;
#if HAVE_SIGNAL_H
  struct sigaction signalaction;
#endif
#ifdef WIN32
  WSADATA wsadata;

  WSAStartup(WINSOCK_VERSION, &wsadata);
#endif
 
  E_init();
  wack_alarm_set( PRINT | EXIT ); 
 
  Usage( argc, argv );
  Wackamole_init();

  if(Debug) {
    wack_alarm_enable_timestamp(NULL);
    wack_alarm_set( PRINT | ARPING | WACK_DEBUG | EXIT );
  }
  else {
    char pidstring[10];

    wack_alarm_set(PRINT | EXIT);
    wack_alarm_enable_syslog("wackamole");

#ifndef WIN32
    if(fork()) exit(0);
    setsid();
    if(fork()) exit(0);
#endif

    if( chdir("/") != 0 ){
      wack_alarm(PRINT,"chdir to root failed");
    }

    umask(0027);

    pid = getpid();
    fd = open(_PATH_WACKAMOLE_PIDDIR "/wackamole.pid",
	      O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if(fd < 0) {
      wack_alarm(EXIT,
	    "Cannot write PID file " _PATH_WACKAMOLE_PIDDIR "/wackamole.pid");
    }
    snprintf(pidstring, 10, "%d\n", pid);
#ifdef BROKEN_SNPRINTF
    if(pidstring[9] != '\0') {
      pidstring[8] = '\n';
      pidstring[9] = '\0';
    }
#endif

    write(fd, pidstring, strlen(pidstring));
    close(fd);

#ifndef WIN32
    fd = open("/dev/null", O_RDWR);
    if(fd <= 2) {
      wack_alarm(EXIT, "Cannot open /dev/null\n");
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    dup2(fd,STDIN_FILENO);
    dup2(fd,STDOUT_FILENO);
    dup2(fd,STDERR_FILENO);
    close(fd);
#endif
  }

#if HAVE_SIGNAL_H
  signalaction.sa_handler = Sig_handler;
  sigemptyset(&signalaction.sa_mask);
  signalaction.sa_flags = 0;

  if(sigaction(SIGINT, &signalaction, NULL)) {
    wack_alarm(EXIT, "An error occured while registering a SIGINT handler");
  }

  if(sigaction(SIGTERM, &signalaction, NULL)) {
    wack_alarm(EXIT, "An error occured while registering a SIGTERM handler");
  }
  if(sigaction(SIGBUS, &signalaction, NULL)) {
    wack_alarm(EXIT, "An error occured while registering a SIGBUS handler");
  }

  if(sigaction(SIGQUIT, &signalaction, NULL)) {
    wack_alarm(EXIT, "An error occured while registering a SIGQUIT handler");
  }

  if(sigaction(SIGSEGV, &signalaction, NULL)) {
    wack_alarm(EXIT, "An error occured while registering a SIGSEGV handler");
  }
#elif defined(WIN32)
  SetConsoleCtrlHandler(ConsoleEventHandlerRoutine, TRUE);
#endif

  if_initialize();
  
  /* connecting to the relevant Spread daemon, asking for group info */
  Spread_reconnect(-8);
  
  Send_local_arp_cache_repeat();
  E_handle_events();
  return -1;
}

/*************************************************/

void	Handle_network(int __unused1, int __unused2, void *__unused3)
{
  int	ret;
  
  ret = SP_receive( Mbox, &Service_type, Sender, MAX_PSEUDO, &Num_groups, 
		    Target_groups, &Mess_type, &Endian_mismatch, 
		    MAX_MESS_LEN, Mess );
  if( ret < 0 )
    {
      SP_error( ret );
      Spread_reconnect(ret);
    }
  
  if( Is_membership_mess( Service_type ) )
    {
      if( Is_reg_memb_mess( Service_type ) )
	{
	  Handle_membership();
	  Send_local_arp_cache(0, NULL);
	}else{
	  /* Ignore Transitional membership */
	}
    }else if( Is_regular_mess( Service_type ) ){
      
      /* Handle regular messages */
      if	( Mess_type == STATE_MESS    ) Handle_state();
      else if	( Mess_type == MATURING_MESS ) Handle_mature();
      else if	( Mess_type == BALANCE_MESS  ) Handle_balance();
      else if	( Mess_type == OPERATOR_MESS ) Handle_operator();
      else if	( Mess_type == ARPCACHE_MESS ) Handle_arp_cache((arp_entry *)Mess,ret);
      
      /* Ignore messages of other types */
      
    }else{
      wack_alarm(EXIT,"Error: received message of unknown message type %d with ret %d",
	     Service_type, ret );
     }
  
}

/*************************************************/

static	void	Turn_mature(int __code, void *__data)
{
  int	ret;
  
  ret = SP_multicast( Mbox, AGREED_MESS, Spread_group, MATURING_MESS, 0, 0 );
  if( ret < 0 )
    {
      SP_error( ret );
      Spread_reconnect(ret);
    }
  
  wack_alarm(LOGIC, "Turn_mature");
}

/*************************************************/

static	void	Handle_mature()
{
  if (Maturity == 1) return;
  
  Maturity = 1;
  E_dequeue( Turn_mature, 0, 0 );
  
  wack_alarm(LOGIC,"Handle_mature");
  if ( State != GATHER )
    {
      /* ### deterministically grab ip addresses */
      Priority_claim();
      Claim_unclaimed();
      wack_alarm(LOGIC, "Shifting to RUN in Handle_mature()" );
      State = RUN;
      E_queue( Balance, 0, 0, Balance_timer );
    }
}

/*************************************************/

static	void	Handle_operator()
{
  wack_alarm(LOGIC,"Handle_operator");
  
}

/*************************************************/

static	void	Handle_arp_cache(arp_entry *arps, int size)
{
  int i;
  size /= sizeof(arp_entry);
  wack_alarm(WACK_DEBUG, "Reading %d shared arp entries", size);
  for(i=0;i<size;i++)
    insert_arp_cache_shared(arps[i]);
}

/*************************************************/

static	void	Handle_balance()
{
  int        i, j, k;
  int        num_bytes;
  int        *num_balanced;
  int	     num_released;
  int	     num_acquired;
  address    *curr_real_ptr;
  address    *curr_pseudo_ptr;

  wack_alarm(LOGIC,"Handle_balance");
  if( State != RUN ) return;
  Print_alloc();

  num_bytes = 0;
  num_balanced = (int *)&Mess[num_bytes];
  num_bytes += sizeof( int );
  if( Endian_mismatch ) *num_balanced = Flip_int32( *num_balanced );

  num_released = 0;
  num_acquired = 0;
  for ( k = 0; k < *num_balanced; k++ )
    {
      curr_real_ptr = (address *)&Mess[num_bytes];
      num_bytes += sizeof( address );
      curr_pseudo_ptr = (address *)&Mess[num_bytes];
      num_bytes += sizeof( address );

      if( Endian_mismatch )
	{
	  *curr_real_ptr = Flip_int32( *curr_real_ptr );
	  *curr_pseudo_ptr = Flip_int32( *curr_pseudo_ptr );
	}
        
      for ( i = 0; i < Num_pseudo; i++ )
	{
	  if( *curr_pseudo_ptr == _pif_ip_s(Allocation_table[i]) )
	    {
	      if ( _rif_ip_s(My) == *curr_real_ptr ) {
                num_acquired++;
		Acquire( &Allocation_table[i] );
              }
	      if ( _rif_ip_s(My) == _rif_ip_s(Allocation_table[i]) ) {
                num_released++;
		Release( &Allocation_table[i] );
              }
	      for ( j = 0; j < Num_members; j++ )
		{
		  if ( _rif_ip_s(Members[j]) == _rif_ip_s(Allocation_table[i]) )
      		    Members[j].num_allocated--;
		  if ( _rif_ip_s(Members[j]) == *curr_real_ptr )
		    Members[j].num_allocated++;
		}
	      _rif_ip_s(Allocation_table[i]) = *curr_real_ptr;
	      i = Num_pseudo;
	    }  
	}
    }
  if(num_acquired) {
    Handle_Post_Acquire();
  }
  if(num_released) {
    Handle_Post_Release();
  }
  wack_alarm(LOGIC,  "Finished Handle_balance." );
  Print_alloc();
}

/*************************************************/

static	void	Handle_membership()
{
  int	i;
  
  wack_alarm(LOGIC, "Handle_membership");
  
  if( strcmp( Sender, Spread_group ) != 0 )
    {
      Bye();
      Clean_up();
      wack_alarm(EXIT, "Handle_membership:  Bug! got a membership for group %s",
	     Sender );
      
    }
  
  memcpy( &Gid, Mess, sizeof( group_id ) );
  Num_pending_states = Num_groups;
  
  Num_members = Num_groups;

  for( i = 0; i < Num_members; i++ )
    {
      memcpy( Members[i].private_group_name, Target_groups[i], MAX_GROUP_NAME );
      My_index = Mess_type;
      _rif_ip_s(Members[i]) = 0;
      Members[i].num_allocated = 0;
      Members[i].got_state_from = 0;
    }
  if(State != GATHER && State != BOOT)
    memcpy( Old_table, Allocation_table, MAX_PSEUDO * sizeof( entry ) );

  for ( i = 0; i < Num_pseudo; i++ )
    {
      _rif_ip_s(Allocation_table[i]) = 0;/* added 4/27/01 */
      Allocation_table[i].claim_priority = UNCLAIMED;
    }
  Send_state_message();		/* ### */
  wack_alarm(LOGIC,  "Shifting to GATHER" );
  State = GATHER;
  wack_alarm(LOGIC,  "Dequeuing Balance" ); /* Debug */
  E_dequeue( Balance, 0, 0 );

}

/*************************************************/

static	void	Handle_state()
{
  group_id	*curr_gid_ptr;
  address	*curr_real_address_ptr;
  int		num_bytes;
  int		num_released;
  int           *curr_index_ptr;
  int           *curr_maturity_ptr;
  int           *curr_num_allocated_ptr;  
  address       *curr_pseudo_addr_ptr;
  int           *curr_claim_priority_ptr;
  int           *curr_num_prefer_ptr;
  address       *curr_prefer_address;
  int           i, j, k;

  wack_alarm(LOGIC,  "Handle state" );

  num_bytes = 0;
  curr_gid_ptr = (group_id *)&Mess[num_bytes];
  num_bytes += sizeof( group_id );
  
  /* ### wackamole stuff */
  curr_real_address_ptr = (address *)&Mess[num_bytes];
  num_bytes += sizeof( address );
  curr_index_ptr = (int *)&Mess[num_bytes];
  num_bytes += sizeof( int );
  curr_maturity_ptr = (int *)&Mess[num_bytes];
  num_bytes += sizeof( int );
  curr_num_allocated_ptr = (int *)&Mess[num_bytes];
  num_bytes += sizeof( int );

  if( Endian_mismatch )
    {
      curr_gid_ptr->id[0] = Flip_int32( curr_gid_ptr->id[0] );
      curr_gid_ptr->id[1] = Flip_int32( curr_gid_ptr->id[1] );
      curr_gid_ptr->id[2] = Flip_int32( curr_gid_ptr->id[2] );
      
      /* ### wackamole stuff */
      *curr_real_address_ptr = Flip_int32( *curr_real_address_ptr );
      *curr_index_ptr = Flip_int32( *curr_index_ptr );
      *curr_maturity_ptr = Flip_int32( *curr_maturity_ptr );
      *curr_num_allocated_ptr = Flip_int32( *curr_num_allocated_ptr );
    }

  /* Check this here, don't process extra stuff if we don't need to. */
  if( Gid.id[0] != curr_gid_ptr->id[0] ||
      Gid.id[1] != curr_gid_ptr->id[1] ||
      Gid.id[2] != curr_gid_ptr->id[2] ) return;
  
  _rif_ip_s(Members[*curr_index_ptr]) = *curr_real_address_ptr;
  Members[*curr_index_ptr].num_allocated = *curr_num_allocated_ptr;
  Members[*curr_index_ptr].got_state_from = 1;

  num_released = 0;
  /* Rip addresses and compute stuff */
  for ( i = 0; i < *curr_num_allocated_ptr; i++ )
    {
      curr_pseudo_addr_ptr = (address *)&Mess[num_bytes];
      num_bytes += sizeof( address );
      curr_claim_priority_ptr = (int *)&Mess[num_bytes];
      num_bytes += sizeof( int );
      if( Endian_mismatch )
	{
	  *curr_pseudo_addr_ptr = Flip_int32( *curr_pseudo_addr_ptr );
	  *curr_claim_priority_ptr = Flip_int32( *curr_claim_priority_ptr );
	}
      for ( j = 0; j < Num_pseudo; j++)
	{
	  if ( *curr_pseudo_addr_ptr == _pif_ip_s(Allocation_table[j]) )
	    {
	      if( _rif_ip_s(Allocation_table[j]) == 0 )
		{
		  _rif_ip_s(Allocation_table[j]) = *curr_real_address_ptr;
		  Allocation_table[j].claim_priority = *curr_claim_priority_ptr;
		}
	      else
		{/*conflict in ip address case*/
		  if( Allocation_table[j].claim_priority == PREFER_CLAIMED ||
		      Allocation_table[j].claim_priority ==  *curr_claim_priority_ptr )
		    {
		      Members[*curr_index_ptr].num_allocated--;
		      if( _rif_ip_s(My) == *curr_real_address_ptr )
			{
                          num_released++;
			  ReleaseAddress(*curr_pseudo_addr_ptr);
			}
		      Old_table[j].claim_priority = Allocation_table[j].claim_priority;
		      _rif_ip_s(Old_table[j]) = _rif_ip_s(Allocation_table[j]);
		    }
		  else if( Allocation_table[j].claim_priority == CLAIMED &&
			   *curr_claim_priority_ptr > PREFER )
		    {
		      if( _rif_ip_s(My) == _rif_ip_s(Allocation_table[j]) )
                        {
                          num_released++;
			  ReleaseAddress( *curr_pseudo_addr_ptr );
                        }
		      for(k = 0; k < Num_members; ++k)
			{
			  if( _rif_ip_s(Members[k]) == _rif_ip_s(Allocation_table[j]) )
			    Members[k].num_allocated--;
			}
		      _rif_ip_s(Allocation_table[j]) = *curr_real_address_ptr;
		      Allocation_table[j].claim_priority = *curr_claim_priority_ptr;
		      Old_table[j].claim_priority = Allocation_table[j].claim_priority;
		      _rif_ip_s(Old_table[j]) = _rif_ip_s(Allocation_table[j]);
		    }
		}
	      j = Num_pseudo;
	    }
	}
    }
 
  if(num_released) {
    Handle_Post_Release();
  } 
  curr_num_prefer_ptr = (int *)&Mess[num_bytes];
  num_bytes += sizeof( int );
  if( Endian_mismatch ) *curr_num_prefer_ptr = Flip_int32( *curr_num_prefer_ptr );
  
  for ( i = 0; i < *curr_num_prefer_ptr; i++ )
    {
      curr_prefer_address = (address *)&Mess[num_bytes];
      num_bytes += sizeof( address );
      if( Endian_mismatch )
	*curr_prefer_address = Flip_int32( *curr_prefer_address );
      for ( j = 0; j < Num_pseudo; j++)
	if ( *curr_prefer_address == _pif_ip_s(Allocation_table[j]) )
	  {
	    if ( Allocation_table[j].claim_priority < PREFER )
	      {
		_rif_ip_s(Allocation_table[j]) = *curr_real_address_ptr;
		Allocation_table[j].claim_priority = PREFER;
	      }
	    j = Num_pseudo;
	  }
    }

  /* ### wackamole compute stuff */
  if ( Maturity == 0 && *curr_maturity_ptr == 1 )
    Handle_mature();
				   
  wack_alarm(LOGIC, "handle state:  got state from %d", *curr_real_address_ptr );
  Num_pending_states--;
  if( Num_pending_states == 0 )
    {
      wack_alarm(LOGIC,  "handle state: Finished getting states, maturity is %d.", Maturity );
      if ( Maturity ) 
	{
	  /* wackamole calculate, drop and aquire */
	  /* Do we do Priority_claim or Claim_unclaimed first? ... each has advantages. */
	  Priority_claim();
	  Claim_unclaimed();

	  State = RUN;
	  E_queue( Balance, 0, 0, Balance_timer );
	  wack_alarm(LOGIC,  "handle state: shifting to RUN");
	}
      else
	{
	  State = BOOT;
	  wack_alarm(LOGIC,  "handle state: shifting back to BOOT");
	}
      Print_alloc();
    }
}

/*****************************************************/

static	void	Send_state_message()
{
  int	num_bytes;
  int	ret;
  int   i;
  
  /* 
   * A State message looks like this:
   *	Gid
   *	real_address
   *    index
   *	maturity
   *	number of pseudo
   *		pseudo
   *		claim_priority
   *    number of  preferred pseudo
   *            pseudo
   */

  wack_alarm(LOGIC, "sending state");/*debug*/
  Print_alloc();
  num_bytes = 0;
  memcpy( &Mess[num_bytes], &Gid, sizeof( group_id ) );
  num_bytes += sizeof( group_id );
  memcpy( &Mess[num_bytes], &(_rif_ip_s(My)), sizeof( address ) );
  num_bytes += sizeof( address );
  memcpy( &Mess[num_bytes], &My_index, sizeof( int ) );
  num_bytes += sizeof( int );
  memcpy( &Mess[num_bytes], &Maturity, sizeof( int32 ) );
  num_bytes += sizeof( int32 );
  memcpy( &Mess[num_bytes], &My.num_allocated, sizeof( int32 ) );
  num_bytes += sizeof( int );

ret=0;
  for ( i = 0; i < Num_pseudo; i++ )
    {
      if ( _rif_ip_s(Old_table[i]) == _rif_ip_s(My) ) 
	{
	  memcpy( &Mess[num_bytes], &(_pif_ip_s(Old_table[i])), sizeof( address ) );
	  num_bytes += sizeof( address );
	  memcpy( &Mess[num_bytes], &(Old_table[i].claim_priority), sizeof( int ) );
	  num_bytes += sizeof( int );
ret++;
	}
    }
  assert(ret == My.num_allocated);
  memcpy( &Mess[num_bytes], &Num_prefer, sizeof( int ) );
  num_bytes += sizeof( int );

  for ( i = 0; i < Num_prefer; i++ )
    {
      memcpy( &Mess[num_bytes], &Prefer_address[i], sizeof( address ) );
      num_bytes += sizeof( address );
    }

  ret = SP_multicast( Mbox, AGREED_MESS, Spread_group, STATE_MESS, num_bytes, Mess );  
  if( ret < 0 )
    {
      SP_error( ret );
      Spread_reconnect(ret);
    }
}

/***************************************************/

static	void	Send_local_arp_cache_repeat()
{
  Send_local_arp_cache(0, NULL);
  E_queue( Send_local_arp_cache, 0, 0, ArpRefresh_timer );
}
static	void	Send_local_arp_cache(int __code, void *__data)
{
  int ret, num_bytes=0;
  arp_entry *addresses, *s;
  arp_entry *d = (arp_entry *)Mess;

  sample_arp_cache();
  s = addresses = reference_private_arp_cache();

  while(s->ip && (char *)d < &Mess[MAX_MESS_LEN-sizeof(arp_entry)]) {
    memcpy(d, s, sizeof(arp_entry));
    d++; s++;
    num_bytes += sizeof(arp_entry);
  }
  wack_alarm(WACK_DEBUG, "Sending %d local arp entries", num_bytes/sizeof(arp_entry));
  ret = SP_multicast( Mbox, RELIABLE_MESS, Spread_group, ARPCACHE_MESS, num_bytes, Mess );  
  if( ret < 0 )
    {
      SP_error( ret );
      Spread_reconnect(ret);
    }
}

/***************************************************/

static  void    Priority_claim()
{
  int        i, j;
  int        num_acquired;
  int        num_released;

  num_acquired = 0;
  num_released = 0;
  wack_alarm(LOGIC,  "Priority_claim called" );
  for ( i = 0; i < Num_pseudo; i++ )
    {
      if ( Allocation_table[i].claim_priority == PREFER )
	{
	  if ( _rif_ip_s(My) == _rif_ip_s(Allocation_table[i]) &&
	       _rif_ip_s(My) != _rif_ip_s(Old_table[i]) ) {
            num_acquired++;
	    Acquire( &Allocation_table[i] );
          }
	  if ( _rif_ip_s(My) == _rif_ip_s(Old_table[i]) &&
	       _rif_ip_s(My) != _rif_ip_s(Allocation_table[i]) ) {
            num_released++;
	    Release( &Allocation_table[i] );
          }
	  for ( j = 0; j < Num_members; j++ )
	    {
	      if ( _rif_ip_s(Members[j]) == _rif_ip_s(Old_table[i]) &&
		   _rif_ip_s(Members[j]) != _rif_ip_s(Allocation_table[i]) )
		{
		  Members[j].num_allocated--;
		}
	      if ( _rif_ip_s(Members[j]) == _rif_ip_s(Allocation_table[i]) &&
		   _rif_ip_s(Members[j]) != _rif_ip_s(Old_table[i]) )
		{
		  Members[j].num_allocated++;
		}
	    }
	  Allocation_table[i].claim_priority = PREFER_CLAIMED;
	}
    }

  if(num_released) {
    Handle_Post_Release();  
  }
  /** If we may need to revalidate num_allocated data. ***/
  if ( Maturity && !Old_maturity )
    {
      Old_maturity = 1;
      for ( i = 0; i < Num_members; i++ )
	{
	  Members[i].num_allocated = 0;
	  for ( j = 0; j < Num_pseudo; j++ )
	    if ( _rif_ip_s(Allocation_table[j]) == _rif_ip_s(Members[i]) )
	      Members[i].num_allocated++;
	}
    }
}

/*************************************************/

static  void    Claim_unclaimed()
{
  int         i, j;
  int         Member_to_claim;
  wack_alarm(LOGIC,  "Claim_unclaimed called" );
  for ( i = 0; i < Num_pseudo; i++ )
    if ( Allocation_table[i].claim_priority == UNCLAIMED )
      {
	Member_to_claim = 0;
        for ( j = 1; j < Num_members; j++ )
	  if ( Members[j].num_allocated < Members[Member_to_claim].num_allocated )
            Member_to_claim = j;
	if ( My_index == Member_to_claim )
	  Acquire( &Allocation_table[i] );
	Members[Member_to_claim].num_allocated++;
	_rif_ip_s(Allocation_table[i]) = _rif_ip_s(Members[Member_to_claim]);
	Allocation_table[i].claim_priority = CLAIMED;
      }
}

/*************************************************/
static  entry  *findVEFromAddress( address Addr )
{
  int n;
  for(n=0; n<Num_pseudo; n++) {
    if(_pif_ip_s(Allocation_table[n]) == Addr)
      return &Allocation_table[n];
  }
  return NULL;
}
/* static  void    AcquireAddress( address Addr )
{
  entry *VE = findVEFromAddress(Addr);
  struct in_addr ipaddr;
  ipaddr.s_addr = Addr;
  if(!VE) {
    wack_alarm(PRINT, "AcquireAddress: Can't find VirtualInterface for: %s", inet_ntoa(ipaddr));
  } else {
    Acquire(VE);
  }
} */
#ifdef NEEDS_FORCE_REUP
static  void    force_reup()
{
  int i;
  for(i=0; i < Num_pseudo; i++) {
    if ( _rif_ip_s(My) == _rif_ip_s(Allocation_table[i]) ) {
      /* This is ours */
      int ic, n=0;
      struct interface iface, *nif;
      entry *VE;

      VE = &Allocation_table[i];
      for(nif=&VE->pseudo_if; n<MAX_DEP_IF; nif = &(VE->extra_ifs[n++])) {
	if(nif->ipaddr.s_addr == 0) break;
        memcpy(&iface, nif, sizeof(struct interface));
        ic = if_up(&iface);
        if(ic)
          wack_alarm(PRINT, "%s", if_error());
        else {
          char buffer[16];
          snprintf(buffer, 16, inet_ntoa(iface.ipaddr));
          wack_alarm(PRINT, "  (re)UP: %s:%s/%s",
                     iface.ifname,buffer,inet_ntoa(iface.netmask));
        }
      }
      invoke_spoofer(VE);
    }
  }
}
#endif
static  void    Acquire( entry *VE )
{
  int ic, n=0;
  struct interface iface, *nif;

  for(nif=&VE->pseudo_if; n<MAX_DEP_IF; nif = &(VE->extra_ifs[n++])) {
	if(nif->ipaddr.s_addr == 0) break;
    memcpy(&iface, nif, sizeof(struct interface));
    ic = if_up(&iface);
    if(ic)
      wack_alarm(PRINT, "%d %s", __LINE__, if_error());
    else {
      char buffer[16];
      snprintf(buffer, 16, inet_ntoa(iface.ipaddr));
      wack_alarm(PRINT, "  UP: %s:%s/%s",
		iface.ifname,buffer,inet_ntoa(iface.netmask));
    }
  }
  invoke_spoofer(VE);
  execute_all_user(VE->pseudo_if, VE->extra_ifs, VE->real_if,
                   DLFUNCS_TYPE_ON_UP);
  My.num_allocated++;
}

static void Handle_Post_Acquire() {
  execute_all_user_simple(DLFUNCS_TYPE_POST_UP);
}
/***************************************************/

static  void    ReleaseAddress( address Addr )
{
  entry *VE = findVEFromAddress(Addr);
  struct in_addr ipaddr;
  ipaddr.s_addr = Addr;
  if(!VE) {
    wack_alarm(PRINT, "ReleaseAddress: Can't find VirtualInterface for: %s", inet_ntoa(ipaddr));
  } else {
    Release(VE);
  }
}
static  void    Release( entry *VE )
{
  int ic, n=0;
  struct interface idown, *nif;

  for(nif=&VE->pseudo_if; n<MAX_DEP_IF; nif = &(VE->extra_ifs[n++])) {
	if(nif->ipaddr.s_addr == 0) break;
    memcpy(&idown, nif, sizeof(struct interface));
    ic = if_down(&idown);
    if(ic) {
      const char *em = if_error();
      if(em && strlen(em)) {
      	wack_alarm(PRINT, "%d %s", __LINE__, if_error());
      }
    } else {
      char buffer[16];
      snprintf(buffer, 16, inet_ntoa(idown.ipaddr));
      wack_alarm(PRINT, "DOWN: %s:%s/%s",
	idown.ifname,buffer,inet_ntoa(idown.netmask));
    }
    execute_all_user(VE->pseudo_if, VE->extra_ifs, VE->real_if,
                     DLFUNCS_TYPE_ON_DOWN);
  }
  My.num_allocated--;
}

static void Handle_Post_Release() {
#ifdef NEEDS_FORCE_REUP
  force_reup();
#endif
  execute_all_user_simple(DLFUNCS_TYPE_POST_DOWN);
}

/***************************************************/

static  void    Balance(int __code, void *__data)
{
  int            ret;
  int            num_bytes;
  int            i, j, k;
  int            min_index;
  int            max_index;
  static member *tempMembers = NULL;
  static entry  *tempTable = NULL;
  wack_alarm(LOGIC, "Balance called." );

  if(!tempMembers) {
    tempMembers = (member *)malloc(sizeof(member) * MAX_PSEUDO);
  }
  if(!tempTable) {
    tempTable = (entry *)malloc(sizeof(entry) * MAX_PSEUDO);
  }
  if( My_index ) goto leave;

  if( State != RUN ) return;

  State = BALANCE;
  num_bytes = sizeof( int );

  min_index = 0;
  max_index = 0;

  memcpy( tempMembers, Members, MAX_PSEUDO * sizeof( member ) );
  memcpy( tempTable, Allocation_table, MAX_PSEUDO * sizeof( entry ) );

  for ( k = 0; k < Balance_rate; k++ )
    {
      for ( i = 0; i < Num_members; i++ )
	{
	  if( tempMembers[i].num_allocated < tempMembers[min_index].num_allocated )
	    min_index = i;
	  if( tempMembers[i].num_allocated > tempMembers[max_index].num_allocated )
	    max_index = i;
	}
      if( tempMembers[max_index].num_allocated - tempMembers[min_index].num_allocated > 1 )
	{
	  for( j = 0; j < Num_pseudo; j++ )
	    {
	      if( _rif_ip_s(tempTable[j]) == _rif_ip_s(tempMembers[max_index])
		  && tempTable[j].claim_priority < PREFER )
		{
		  memcpy( &Mess[num_bytes], &(_rif_ip_s(tempMembers[min_index])), sizeof( address ) );
		  num_bytes += sizeof( address );
		  memcpy( &Mess[num_bytes], &(_pif_ip_s(tempTable[j])), sizeof( address ) );
		  num_bytes += sizeof( address );
		  tempMembers[min_index].num_allocated++;
		  tempMembers[max_index].num_allocated--;
		  _rif_ip_s(tempTable[j]) = _rif_ip_s(tempMembers[min_index]);
		  j = Num_pseudo;
		}
	      else if ( j == Num_pseudo )
		{
		  if( k == 0 ) 
		    {
		      State = RUN;
		      goto leave;
		    }
		  memcpy( &Mess[0], &k, sizeof( int ) );
		  k = Balance_rate + 1;
		}
	    }
	}
      else 
	{
	  if( k == 0 ) 
	    {
	      State = RUN;
	      goto leave;
	    }
	  memcpy( &Mess[0], &k, sizeof( int ) );  
	  k = Balance_rate + 1;
	}
    }

  if ( k == Balance_rate )
      memcpy( &Mess[0], &k, sizeof( int ) );

  ret = SP_multicast( Mbox, AGREED_MESS, Spread_group, BALANCE_MESS, num_bytes, Mess );
  if( ret < 0 )
    {
      SP_error( ret );
      Spread_reconnect(ret);
    }
  State = RUN;
  wack_alarm(LOGIC, "Shifting to RUN in balance" );
 leave:
  if( !Complete_balance )
    E_queue( Balance, 0, 0, Balance_timer );
  return;
}

/*************************************************/

static	void    Wackamole_init()
{
  wack_alarm(LOGIC,"Wackamole_init");

  /* Set defaults */
  Maturity_timeout.sec = 5*60;
  Maturity_timeout.usec = 0;
  Balance_timer.sec = 5*60;
  Balance_timer.usec = 0;
  Balance_rate = 1;
  Complete_balance = 0;
  Spread_retry_interval = 5;

  spread_lock = 0;

#ifdef USE_EMBEDDED_PERL
  perl_startup();
#endif

  Get_conf(File_name, &My);

  /* start up control socket */
  create_control_socket(control_socket);

  if ( Balance_rate > Num_pseudo / 2)
    Balance_rate = Num_pseudo / 2;

  if( Complete_balance )
    {
      Balance_rate = Num_pseudo / 2;
      Balance_timer.sec = 0;
    }
}

/*************************************************/

/*************************************************/

static  void    Usage(int argc, char *argv[])
{

  /* Setting defaults */
  sprintf( User, "wack%d", (int)getpid() );
  sprintf( Spread_name, "4803" );

  while( --argc > 0 )
    {
      argv++;

      if( !strncmp( *argv, "-v", 2 ) ) {
          notice();
          exit(0);
        }else if( !strncmp( *argv, "-u", 2 ) ) {
	  strcpy( User, argv[1] );
	  argc--; argv++;
	}else if( !strncmp( *argv, "-s", 2 ) ){
	  strcpy( Spread_name, argv[1] );
	  argc--; argv++;
	}else if(!strncmp(*argv, "-c", 2) ){
	  if(argv[1][0] == '/')
	    strcpy(File_name, argv[1]);
	  else{
	    getcwd(File_name,100);
	    strcat(File_name,"/");	  
	    strcat(File_name, argv[1] );
            printf("%s", File_name);
	  }
	  argc--; argv++;
	}else if(!strncmp(*argv, "-d", 2) ){
	  Debug = 1;
	}else{
	  wack_alarm(PRINT, "Usage: wackamole\n"
		  "\t-c <filename>     : file used instread of wackamole.conf\n"
		  "\t-d                : debug mode and don't daemonize\n"
		  "\t-s <address>      : either port or port@machine\n"
                  "\t-u <username>     : connect to Spread as user username\n"
                  "\t-v                : version information\n");
          exit(0);
	}
    }
}

/*************************************************/

static  void    Print_alloc()
{
#if 0
  int        i;
  address    Addr;
  char       IP[16];

  wack_alarm(LOGIC, "Current allocation table:" );
  for ( i = 0; i < Num_pseudo; i++ )
    {
      Addr = Allocation_table[i].pseudo_address;
      String_addr( Addr, IP );
      wack_alarm(LOGIC, "Pseudo address: %s", IP );
      Addr = Allocation_table[i].real_address;
      String_addr( Addr, IP );
      wack_alarm(LOGIC, "\tis allocated to %s with priority %d.", 
	      IP, Allocation_table[i].claim_priority );
    }
#endif
}


/*************************************************/

/* static  void    String_addr( address Addr, char IP[16] )
{
  int one, two, three, four;

  one = (Addr & 0xff000000) >> 24;
  two = (Addr & 0x00ff0000) >> 16;
  three = (Addr & 0x0000ff00) >> 8;
  four = (Addr & 0x000000ff);
  sprintf(IP, "%d.%d.%d.%d", one, two, three, four );
} */

/*************************************************/

static void Sig_handler(int signum) {
  wack_alarm(WACK_DEBUG,"Sig_handler called");
#ifdef SIGINT
  if(signum == SIGINT)
    wack_alarm(WACK_DEBUG, "SIGINT Detected!");
#endif
#ifdef SIGTERM
  if(signum == SIGTERM)
    wack_alarm(WACK_DEBUG,"SIGTERM Detected!");  
#endif
#ifdef SIGBUS
  if(signum == SIGBUS)
    wack_alarm(WACK_DEBUG,"SIGBUS Detected!");
#endif
#ifdef SIGQUIT
  if(signum == SIGQUIT) 
    wack_alarm(WACK_DEBUG,"SIGQUIT Detected!");  
#endif
#ifdef SIGSEGV
  if(signum == SIGSEGV) 
    wack_alarm(WACK_DEBUG,"SIGSEGV Detected!");
#endif

  Clean_up();
  global_exit = 1;
#ifdef USE_EMBEDDED_PERL
  perl_shutdown();
#endif
  exit(0);
}

#ifdef WIN32
static BOOL WINAPI ConsoleEventHandlerRoutine(DWORD dwCtrlType)
{
  wack_alarm(WACK_DEBUG, "Console signal %d was delivered\n", dwCtrlType);
  Sig_handler(SIGINT);
  return TRUE;
}
#endif


/*************************************************/

void Clean_up(){
  
  int cl_index;
  wack_alarm(WACK_DEBUG, "Clean_up called");
 
  for(cl_index = 0; cl_index < Num_pseudo; cl_index++){
    cancel_spoofer(&Old_table[cl_index]);
    Release(&Old_table[cl_index]);
    Allocation_table[cl_index].real_if.ipaddr.s_addr = 0;
    Old_table[cl_index].real_if.ipaddr.s_addr = 0;
  }
}

/**************************************************/

void    handle_reconnect(int a, void *d) {
  sp_time delay;
  delay.sec = Spread_retry_interval;
  delay.usec = 0;
  E_queue((void(*)(int, void *))Spread_reconnect, -2, NULL, delay);
}
void    Spread_reconnect(int ret){

  if(ret != -8 && ret != -11 && ret != -2)
    wack_alarm(EXIT, "Spread_reconnnect: Unexpected Error (%d)", ret);
  
  wack_alarm(PRINT,"connecting to %s", Spread_name);
    
  Clean_up();

  if(Mbox >= 0) {
    SP_disconnect(Mbox);
    E_detach_fd( Mbox, READ_FD );
    Mbox = -1;
  }
  /* connecting to the relevant Spread daemon, asking for group info */
  if(spread_lock) {
    handle_reconnect(0, NULL);
    return;
  }
  ret = SP_connect( Spread_name, User, 0, 1, &Mbox, Private_group ) ;
  if(ret == ACCEPT_SESSION) {
    ret = SP_join( Mbox, Spread_group );  
    if( ret < 0 ) {
      SP_error( ret );
      SP_disconnect( Mbox );
      Mbox = -1;
      wack_alarm(PRINT, "Spread join on reconnect failed [%d].", ret);
      handle_reconnect(0, NULL);
      return;
    }
  } else {
    wack_alarm(PRINT, "Spread connect failed [%d].", ret);
    handle_reconnect(0, NULL);
    return;
  }

  /* State initializations */
  State = BOOT;
  Old_maturity = 0;
  Maturity = 0;
  E_queue( Turn_mature, 0, 0, Maturity_timeout );
  My.num_allocated = 0;
  
  strcpy(My.private_group_name, Private_group);

  E_attach_fd( Mbox, READ_FD, Handle_network, 0, NULL, HIGH_PRIORITY );
  E_set_active_threshold( HIGH_PRIORITY );
}


/********************************************************/

void Bye(){
  wack_alarm(WACK_DEBUG, "Disconnecting from Spread");
  SP_disconnect(Mbox);
}

/******************************************************/
/* vim:se sw=2 ts=2 et: */
