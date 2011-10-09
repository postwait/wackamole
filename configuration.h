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
#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include "config.h"

/* configuration structures */
extern  char            Spread_name[80];
extern  char            Spread_group[MAX_GROUP_NAME];
extern  int              Spread_retry_interval;
extern  char            control_socket[MAXPATHLEN];
extern  char            default_library_path[MAXPATHLEN];
extern  sp_time         Maturity_timeout;
extern  sp_time         Balance_timer;
extern  sp_time         ArpRefresh_timer;
extern  int             Balance_rate;
extern  int             Complete_balance;
extern  int             Num_pseudo;
extern  int             Num_prefer;
extern  int             Num_notifications;
extern  struct notification     Notification_table[MAX_NOTIF];
extern  address         Prefer_address[MAX_PSEUDO];
extern  entry           Allocation_table[MAX_PSEUDO];
extern  entry           Old_table[MAX_PSEUDO];

/* The function */
void Get_conf(const char *, member *);

#endif
