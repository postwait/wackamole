/*
 * The Spread Toolkit.
 *     
 * The contents of this file are subject to the Spread Open-Source
 * License, Version 1.0 (the ``License''); you may not use
 * this file except in compliance with the License.  You may obtain a
 * copy of the License at:
 *
 * http://www.spread.org/license/
 *
 * or in the file ``license.txt'' found in this distribution.
 *
 * Software distributed under the License is distributed on an AS IS basis, 
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License 
 * for the specific language governing rights and limitations under the 
 * License.
 *
 * The Creators of Spread are:
 *  Yair Amir, Michal Miskin-Amir, Jonathan Stanton.
 *
 *  Copyright (C) 1993-2001 Spread Concepts LLC <spread@spreadconcepts.com>
 *
 *  All Rights Reserved.
 *
 * Major Contributor(s):
 * ---------------
 *    Dan Schoenblum   dansch@cnds.jhu.edu - Java Interface Developer.
 *    John Schultz     jschultz@cnds.jhu.edu - contribution to process group membership.
 *    Theo Schlossnagle theos@cnds.jhu.edu - Perl library and Skiplists.
 *
 */

/* 2001-08-06 Aashima Munjal -- modified to use in wackamole, when compiling
   outside the Spread distribution */

#ifndef WACKAMOLE_INC_ALARM
#define WACKAMOLE_INC_ALARM

#include <stdio.h>
#define int32 int

#define		WACK_DEBUG		0x00000001
#define 	EXIT  		0x00000002
#define		PRINT		0x00000004
#define		ALL		0xffffffff
#define		NONE		0x00000000


void wack_alarm( int32 mask, char *message, ...);

void wack_alarm_enable_syslog(char *ident);

void wack_alarm_set_output(char *filename);

void wack_alarm_enable_timestamp(char *format);
void wack_alarm_disable_timestamp(void);

void wack_alarm_set(int32 mask);
void wack_alarm_clear(int32 mask);
int32 wack_alarm_get(void);

void wack_alarm_set_interactive(void);
int  wack_alarm_get_interactive(void);

#define IP1( address )  ( ( 0xFF000000 & (address) ) >> 24 )
#define IP2( address )  ( ( 0x00FF0000 & (address) ) >> 16 )
#define IP3( address )  ( ( 0x0000FF00 & (address) ) >> 8 )
#define IP4( address )  ( ( 0x000000FF & (address) ) )

#endif	/* WACKAMOLE_INC_ALARM */
