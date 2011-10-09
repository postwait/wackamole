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
#include "wackamole.h"
#include "configuration.h"
#include "control.h"
#include "apue.h"
#include "alarm.h"
#include "wackatrl.h"

extern mailbox Mbox;
extern int spread_lock;

typedef struct {
  int size;
  int written;
  char *buffer;
} write_struct;

typedef struct {
  char operation;
  char state;
  union {
    write_struct writing;
    int retint;
  } data;
} control_state;

void create_control_socket(const char *filename) {
  int fd;
  if((fd = serv_listen(filename)) < 0) {
    wack_alarm(PRINT, "disabling wackatrl support.");
    return;
  }
  E_attach_fd(fd, READ_FD, handle_control_connect, 0, NULL, HIGH_PRIORITY);
}
void handle_control_connect(int fd, int code, void *data) {
  int nfd, ret, ioctl_cmd;
  control_state *state;
  nfd = serv_accept(fd);
  if(nfd < 0) {
    wack_alarm(PRINT, "error receiving wackatrl session");
    return;
  }
  ioctl_cmd = 1;
#ifdef WIN32
  ret = ioctlsocket(nfd, FIONBIO, &ioctl_cmd);
#else
  ret = ioctl(nfd, FIONBIO, &ioctl_cmd);
#endif
  wack_alarm(WACK_DEBUG, "starting wackatrl session");
  state = calloc(1, sizeof(control_state));
  state->operation = READ_COMMAND;
  state->state = READ_FD;
  E_attach_fd(nfd, READ_FD, handle_control_session,
		0, (void *)state, HIGH_PRIORITY);
  E_attach_fd(nfd, WRITE_FD, handle_control_session,
		0, (void *)state, HIGH_PRIORITY);
  E_deactivate_fd(nfd, WRITE_FD);
}

static int set_event_mask(int fd, control_state *sess) {
  if(sess->state == READ_FD) {
    E_activate_fd(fd, READ_FD);
    E_deactivate_fd(fd, WRITE_FD);
  } else if(sess->state == WRITE_FD) {
    E_activate_fd(fd, WRITE_FD);
    E_deactivate_fd(fd, READ_FD);
  } else {
    wack_alarm(PRINT, "Unknown session state: %d", sess->state);
    return -1;
  }
  return 0;
}

#define sprintif(a,b,c) do {\
  memcpy(a+(*(c)), (b).ifname, IFNAMSIZ); (*c)+=IFNAMSIZ; \
  memcpy(a+(*(c)), &((b).ipaddr.s_addr), sizeof(int)); (*c)+=sizeof(int); \
  memcpy(a+(*(c)), &((b).bcast.s_addr), sizeof(int)); (*c)+=sizeof(int); \
  memcpy(a+(*(c)), &((b).netmask.s_addr), sizeof(int)); (*c)+=sizeof(int); \
} while (0);
	
static void set_state_wack_service_state(control_state *sess) {
  sess->state = WRITE_FD;
  sess->data.retint = htonl(-1);
  if(sess->operation == WACK_SERVICE_FAILURE) {
    sess->data.retint = htonl(0);
    spread_lock = 1;
    E_detach_fd(Mbox, READ_FD);
    SP_disconnect(Mbox);
    Mbox=-1;
    Clean_up();
  } else if(sess->operation == WACK_SERVICE_SUCCESS) {
    sess->data.retint = htonl(0);
    spread_lock = 0;
  }
  Spread_reconnect(-8);
}
static void set_state_wack_state(control_state *sess) {
  int netint, len=0, i, j;
  struct interface temp;
  sess->state = WRITE_FD;
  sess->data.writing.size = sizeof(int) +
	Num_pseudo*(sizeof(int)+(IFNAMSIZ+3*sizeof(int))*(MAX_DEP_IF+2));
  sess->data.writing.written = 0;
  sess->data.writing.buffer = malloc(sess->data.writing.size);
  /* How much we will write - minus the this int itself */
  netint = htonl(sess->data.writing.size - sizeof(int));
  memcpy(sess->data.writing.buffer+len, &netint, sizeof(int)); len+=sizeof(int);
  for(i=0;i<Num_pseudo;i++) {
    /* Claim priority */
    netint = htonl(Allocation_table[i].claim_priority);
    memcpy(sess->data.writing.buffer+len,&netint,sizeof(int)); len+=sizeof(int);
    /* Real info */
    memcpy(&temp, &Allocation_table[i].real_if, sizeof(struct interface));
    sprintif(sess->data.writing.buffer, temp, &len);
    /* Master VIF */
    memcpy(&temp, &Allocation_table[i].pseudo_if, sizeof(struct interface));
    sprintif(sess->data.writing.buffer, temp, &len);
    /* Other VIFs */
    for(j=0;j<MAX_DEP_IF;j++) {
      memcpy(&temp, &Allocation_table[i].extra_ifs[j],sizeof(struct interface));
      sprintif(sess->data.writing.buffer, temp, &len);
    }
  }
  wack_alarm(WACK_DEBUG, "Going to write state: %d bytes, filled %d", 
	sess->data.writing.size, len);
}

static int new_command(control_state *sess) {
  switch(sess->operation) {
    case GET_WACK_STATE:
      set_state_wack_state(sess);
      break;
    case WACK_SERVICE_FAILURE:
      set_state_wack_service_state(sess);
      break;
    case WACK_SERVICE_SUCCESS:
      set_state_wack_service_state(sess);
      break;
    default:
      return -1;
  }
  return 0;
}

void handle_control_session(int fd, int code, void *data) {
  int ret;
  control_state *sess = (control_state *)data;

  switch(sess->state) {
    case READ_FD:
      switch(sess->operation) {
        case READ_COMMAND:
	  ret = read(fd, &sess->operation, 1);
	  if(ret == 0) goto close_clean;
	  if(ret < 0) {
	    if(errno == EAGAIN || errno == EWOULDBLOCK)
	      return;
	    goto error;
	  }
	  if(new_command(sess)) goto error;
	  if(set_event_mask(fd, sess)) goto error;
	  break;
	default:
	  goto error;
      }
      break;
    case WRITE_FD:
      switch(sess->operation) {
	case GET_WACK_STATE:
	  ret = write(fd, sess->data.writing.buffer+sess->data.writing.written,
		    sess->data.writing.size-sess->data.writing.written);
	  if(ret == 0) goto error;
	  if(ret<0) {
	    if(errno == EAGAIN || errno == EWOULDBLOCK)
              return;
            goto error;
	  }
	  sess->data.writing.written += ret;
	  if(sess->data.writing.written == sess->data.writing.size) {
	    free(sess->data.writing.buffer);
	    sess->operation = READ_COMMAND;
	    sess->state = READ_FD;
	    if(set_event_mask(fd, sess)) goto error;
	  }
	  break;
	case WACK_SERVICE_FAILURE:
	case WACK_SERVICE_SUCCESS:
	  ret = write(fd, &sess->data.retint, sizeof(int));
	  if(ret == 0) goto error;
	  if(ret<0) {
	    if(errno == EAGAIN || errno == EWOULDBLOCK)
              return;
            goto error;
          }
          if(ret != sizeof(int)) goto error;
	  sess->operation = READ_COMMAND;
	  sess->state = READ_FD;
	  if(set_event_mask(fd, sess)) goto error;
	  break;
	default:
	  goto error;
      }
      break;
    default:
      goto error;
  }
  return;
error:
  wack_alarm(PRINT, "control session error: [fd=%d] %s\n\tState=%d, Operation=%d",
	fd, strerror(errno), sess->state, sess->operation);
close_clean:
  E_detach_fd(fd, READ_FD);
  E_detach_fd(fd, WRITE_FD);
  free(sess);
  close(fd);
}

