/* ======================================================================
 * Copyright (c) 1998-1999 The Johns Hopkins University.
 * All rights reserved.
 * The following code was written by Theo Schlossnagle for use in the
 * Backhand project at The Center for Networking and Distributed Systems
 * at The Johns Hopkins University.
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#include "config.h"
#include "alarm.h"

#define FIFO_MODE  (S_IRUSR|S_IWUSR)
#ifdef USE_FATTACH
int serv_listen(const char *name) {
  int tempfd, fd[2], len;
  
  unlink(name);
  if((tempfd = creat(name, FIFO_MODE)) < 0)
    return(-1);
  if (close(tempfd) < 0)
    return(-2);
  
  if(pipe(fd) < 0)
    return(-3);
  if(ioctl(fd[1], I_PUSH, "connld") < 0)
    return(-4);
  if(fattach(fd[1], name) < 0)
    return(-5);

  return(fd[0]);
}
int cli_conn(const char *name) {
  int fd;

  if((fd = open(name, O_RDWR)) < 0)
    return(-1);
  if(isastream(fd) == 0)
    return(-2);
  return(fd);
}

#elif defined(WIN32)
/* TCP sockets.  Should use named pipes, but I doubt spread can schedule/select on those */

int serv_accept(int listenfd) {
  int clifd;
  unsigned int len;
  struct sockaddr_in addr;
  
  len = sizeof(addr);
  if((clifd = accept(listenfd, (struct sockaddr *) &addr, &len)) < 0)
    return -1;
  
  return clifd;
}

int serv_listen(const char *name) {
  int fd, len, oldmask;
  struct sockaddr_in addr;
  
  /* create a Unix domain stream socket */
  if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    wack_alarm(PRINT, "could not open socket");
    return -1;
  }

  len = sizeof(addr);
  if (WSAStringToAddress((char*)name, AF_INET, NULL, (struct sockaddr*)&addr,&len) != 0) {
	  wack_alarm(PRINT, "couldn't resolve address string %s\n", name);
	  return -1;
  }

  if(bind(fd, (struct sockaddr *) &addr, len) < 0) {
    wack_alarm(PRINT, "bind %s failed.", name);
	close(fd);
    return -1;
  }
  if(listen(fd, 5) < 0) {
    wack_alarm(PRINT, "listen on %s failed.", name);
    close(fd);
    return -1;
  }
  return(fd);
}

int cli_conn(const char *name) {
  int fd, len;
  struct sockaddr_in addr;
  
  /* create a Unix domain stream socket */
  if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return(-1);

  len = sizeof(addr);
  if (WSAStringToAddress((char*)name, AF_INET, NULL, (struct sockaddr*)&addr,&len) != 0) {
	  wack_alarm(PRINT, "couldn't resolve address string %s\n", name);
	  return -1;
  }
  
  if(connect(fd, (struct sockaddr *) &addr, len) < 0) {
    close(fd);
    fd=-1;
    wack_alarm(PRINT, "connect to: %s failed.", name);
  }
  return(fd);
}


#else
int serv_accept(int listenfd) {
  int clifd;
  unsigned int len;
  struct sockaddr_un unix_addr;
  
  len = sizeof(unix_addr);
  if((clifd = accept(listenfd, (struct sockaddr *) &unix_addr, &len)) < 0)
    return -1;
  
  return clifd;
}

int serv_listen(const char *name) {
  int fd, len, oldmask;
  struct sockaddr_un unix_addr;
  
  /* create a Unix domain stream socket */
  if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    wack_alarm(PRINT, "could not open socket");
    return -1;
  }
  unlink(name);
  memset(&unix_addr, 0, sizeof(unix_addr));
  unix_addr.sun_family = AF_UNIX;
  strcpy(unix_addr.sun_path, name);
  len = SUN_LEN(&unix_addr);
  /* bind the name to the descriptor */
  oldmask = umask((~FIFO_MODE)&0777);
  if(bind(fd, (struct sockaddr *) &unix_addr, len) < 0) {
    wack_alarm(PRINT, "bind to %s failed.", name);
    umask(oldmask);
    close(fd);
    return -1;
  }
  umask(oldmask);
  if(listen(fd, 5) < 0) {
    wack_alarm(PRINT, "listen on %s failed.", name);
    close(fd);
    return -1;
  }
  return(fd);
}

int cli_conn(const char *name) {
  int fd, len;
  struct sockaddr_un unix_addr;
  
  /* create a Unix domain stream socket */
  if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    return(-1);
  memset(&unix_addr, 0, sizeof(unix_addr));
  unix_addr.sun_family = AF_UNIX;
  strcpy(unix_addr.sun_path, name);
  unix_addr.sun_path[strlen(name)]='\0';
  len = SUN_LEN(&unix_addr);
  
  if(connect(fd, (struct sockaddr *) &unix_addr, len) < 0) {
    close(fd);
    fd=-1;
    wack_alarm(PRINT, "connect to: %s failed.", name);
  }
  return(fd);
}
#endif
/* vim:se ts=2 sw=2 et: */
