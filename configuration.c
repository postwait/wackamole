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
#include "configuration.h"
#include "control.h"

extern FILE *wackin;
int wackparse();

void    Get_conf(const char *File_name, member *My)
{
  FILE*   fp = NULL;
  char    my_local_host_name[255];
  static const size_t  my_local_host_name_len=255;
  struct  hostent         *hent;
  int	  i, full;
  Num_prefer = 0;

  if (File_name && File_name[0] && (NULL != (fp = fopen(File_name,"r"))) )
    wack_alarm(WACK_DEBUG,"Get_conf: using file: %s", File_name);
  if (fp == NULL)
    if (NULL != (fp = fopen(ETCDIR "/wackamole.conf", "r")) )
      wack_alarm(WACK_DEBUG, "Get_conf: using file: " ETCDIR "/wackamole.conf");
  if (fp == NULL) 
    if (NULL != (fp = fopen("./wackamole.conf", "r")) )
      wack_alarm(WACK_DEBUG,"Get_conf: using file: ./wackamole.conf");
  if (fp == NULL){
    wack_alarm(WACK_DEBUG,"Get_conf: error opening wackamole config file.");
    exit(1);
  }

  wackin = fp;
  full = wackparse();
  if(full) {
	fprintf(stderr, "Error reading config file.\n");
	exit(-1);
  }

  if( gethostname(my_local_host_name , my_local_host_name_len) < 0 ){
    wack_alarm(EXIT, "gethostname Error number %d", errno);
  }
  if((hent = gethostbyname(my_local_host_name)) == NULL) {
    wack_alarm(EXIT, "Can't self resolve %s number %d",
               my_local_host_name, errno);
  }
  memcpy(&My->real_if.ipaddr.s_addr, hent->h_addr_list[0], 4);
  wack_alarm(WACK_DEBUG,"My real address is %d" , _rif_ip_s(*My)); 
#ifdef TEST
  _rif_ip_s(*My) = getpid();
#endif

  fclose(fp);

#ifndef DONT_USE_THREADS
  /* initialize entries */
  for(i=0;i<Num_pseudo;i++) {
    memset(&(Allocation_table[i].notifier), 0, sizeof(pthread_t));
    _ve_lock_init(Allocation_table[i]);
  }
#endif

  /* dup back to Old_table */
  memcpy(Old_table, Allocation_table, sizeof(Allocation_table));
}

