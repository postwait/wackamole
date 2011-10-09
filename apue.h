/* ======================================================================
 * Copyright (c) 1998-1999 The Johns Hopkins University.
 * All rights reserved.
 * The following code was written by Theo Schlossnagle for use in the
 * Backhand project at The Center for Networking and Distributed Systems
 * at The Johns Hopkins University.
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#ifndef _APUE_H_
#define _APUE_H_

#include <sys/types.h>

int serv_accept(int);
int serv_listen(const char *);
int cli_conn(const char *);

#endif
