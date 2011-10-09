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
#include "configuration.h"
#include "apue.h"
#include "alarm.h"
#include "wackatrl.h"
#ifdef USE_EMBEDDED_PERL
#include "perl.h"
#endif
#include "getopt_long.h"

member Me;
struct interface *Table[MAX_PSEUDO];

static int mask2bits(int s) {
  int bits = 32;
  while(bits) {
    if((s | 1) == s) return bits;
    s >>= 1;
    bits--;
  }
  return 0;
}

int post_failure(int fd) {
  char operation;
  int size;
  operation = WACK_SERVICE_FAILURE;
  write(fd, &operation, 1);
  if(read(fd, &size, sizeof(int)) != sizeof(int))
    goto error;
  size = ntohl(size);
  if(size) fprintf(stderr, "Error posting failure.");
  return 0;
error:
  fprintf(stderr, "Error on read.");
  return 1;
}

int post_success(int fd) {
  char operation;
  int size;
  operation = WACK_SERVICE_SUCCESS;
  write(fd, &operation, 1);
  if(read(fd, &size, sizeof(int)) != sizeof(int))
    goto error;
  size = ntohl(size);
  if(size) fprintf(stderr, "Error posting failure.");
  return 0;
error:
  fprintf(stderr, "Error on read.");
  return 1;
}

int get_state(int fd) {
  int i,j,size=0, claim, ret, toread=0;
  char operation;
  char *buffer;

  operation = GET_WACK_STATE;
  write(fd, &operation, 1);
  if(read(fd, &size, sizeof(int)) != sizeof(int))
    goto error;
  size = ntohl(size);
  buffer = malloc(size);
  toread = size;
  while(toread>0) {
    ret = read(fd, buffer+(size-toread), toread);
    if(ret < 0) goto error;
    toread -= ret;
  }
  toread = 0;
  for(i=0;i<MAX_PSEUDO;i++) {
    /* claim */
    memcpy(&claim, buffer+toread, sizeof(int)); toread+=sizeof(int);

    Table[i] = malloc(sizeof(struct interface)*(MAX_DEP_IF+2));
    for(j=0;j<MAX_DEP_IF+2;j++) {
      memcpy(Table[i][j].ifname, buffer+toread, IFNAMSIZ);
	toread+=IFNAMSIZ;
      memcpy(&Table[i][j].ipaddr.s_addr,buffer+toread,sizeof(int));
      if ( j == 0 ) Table[i][j].ipaddr.s_addr = Table[i][j].ipaddr.s_addr;
	toread+=sizeof(int);
      memcpy(&Table[i][j].bcast.s_addr,buffer+toread,sizeof(int));
      Table[i][j].bcast.s_addr = Table[i][j].bcast.s_addr;
	toread+=sizeof(int);
      memcpy(&Table[i][j].netmask.s_addr,buffer+toread,sizeof(int));
      Table[i][j].netmask.s_addr = Table[i][j].netmask.s_addr;
	toread+=sizeof(int);

    }
	printf("Owner: %s\n", Table[i][0].ipaddr.s_addr?
		inet_ntoa(Table[i][0].ipaddr):"not acquired");
	printf("\t*   %5s:%s/%d\n", Table[i][1].ifname,
	  inet_ntoa(Table[i][1].ipaddr), mask2bits(Table[i][1].netmask.s_addr));
	for(j=2; j<MAX_DEP_IF+2 && Table[i][j].ipaddr.s_addr; j++)
	  printf("\t -> %5s:%s/%d\n", Table[i][j].ifname,
	  inet_ntoa(Table[i][j].ipaddr), mask2bits(Table[i][j].netmask.s_addr));

    if(toread>=size) break;
  }
  return 0;
error:
  fprintf(stderr, "Error on read.\n");
  return 1;
}

void usage() {
  wack_alarm(PRINT, "Usage:\n"
             "\t-c <filename>   : use filename instead of wackamole.conf\n"
             "\t-f              : tell the local instance to simulate failure\n"
             "\t-s              : tell the local instance to simulate success\n"
             "\t-l              : display the current VIF assignments\n");
}
int main(int argc, char **argv) {
  int fd, op=-1;
  int getoption;
  char *filename = NULL;
#ifdef WIN32
  WSADATA wsadata;

  WSAStartup(WINSOCK_VERSION, &wsadata);
#endif
 
  wack_alarm_set(PRINT | EXIT);

  while((getoption = getopt(argc, argv, "c:fsl")) != EOF) {
    switch(getoption) {
      case 'c':
	filename=optarg;
	break;
      case 'f':
	op = WACK_SERVICE_FAILURE;
	break;
      case 's':
	op = WACK_SERVICE_SUCCESS;
	break;
      case 'l':
	op = GET_WACK_STATE;
	break;
      default:
        usage();
        exit(0);
    }
  }
  if(op == -1) {
    usage();
    exit(0);
  }
#ifdef USE_EMBEDDED_PERL
  /* Start perl interpretter */
  perl_startup();
#endif

  Get_conf(filename, &Me);
  fd = cli_conn(control_socket);
  if(fd < 0) {
    wack_alarm(EXIT, "Perhaps wackamole is not running?\n");
  }
  switch(op) {
    case WACK_SERVICE_FAILURE:
	post_failure(fd);
	op = GET_WACK_STATE;
	break;
    case WACK_SERVICE_SUCCESS:
	post_success(fd);
	op = GET_WACK_STATE;
	break;
  }
 if(op == GET_WACK_STATE) {
    get_state(fd);
  }
  close(fd);
#ifdef USE_EMBEDDED_PERL
  perl_shutdown();
#endif

  return 0;
}
