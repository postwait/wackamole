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
#ifndef _SPOOFMANAGER_H_
#define _SPOOFMANAGER_H_

#include "config.h"

void cancel_spoofer(entry *VE);
void invoke_spoofer(entry *VE);
void *arp_spoof_notifier(void *arg);
int calc_new_cidr(struct interface *, struct interface *,struct interface *out);
int send_arp_spoof_arp_cache(struct interface *, struct notification *,
		int *arpcache, int *count);
int send_arp_spoof_netblock(struct interface *, struct notification *,
		int *count);

#endif
