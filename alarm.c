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

/* 2001-08-08 Theo Schlossnagle -- removed dependency on arch.h */

/* 2001-10-10 Aashima Munjal -- modified to add ability to write to syslog for wackamole */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "config.h"

#ifdef HAVE_GOOD_VARGS
#include <stdarg.h>
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

static          int             syslog_flag = 0;

/* We are on Linux.. for now.  And have goo varargs. */
#define HAVE_GOOD_VARGS

#include "alarm.h"

static int32	wack_alarm_mask = PRINT | EXIT ;
static char     *wack_alarm_timestamp_format = NULL;

static const char *DEFAULT_TIMESTAMP_FORMAT="[%a %d %b %Y %H:%M:%S]";

static int      wack_alarmInteractiveProgram = FALSE;

#ifdef HAVE_GOOD_VARGS

/* Probably should work on all platforms, but just in case, I leave it to the
   developers...
*/

void wack_alarm( int32 mask, char *message, ...)
{
  char *s;
  if ( wack_alarm_mask & mask )
    {
      va_list ap;
      
      if ( wack_alarm_timestamp_format )
	{
	  char timestamp[42];
	  struct tm *tm_now;
	  time_t time_now;
	  size_t length;
	  
	  time_now = time(NULL);
	  tm_now = localtime(&time_now);
	  length = strftime(timestamp, 40,
			    wack_alarm_timestamp_format, tm_now);
	  timestamp[length] = ' ';
#ifdef HAVE_SYSLOG_H
	  if(syslog_flag)
	    syslog(LOG_NOTICE, timestamp);
	  else
#endif
	    fwrite(timestamp, length+1, sizeof(char), stdout);
	}

      va_start(ap,message);
#ifdef HAVE_SYSLOG_H
      if(syslog_flag){
	int len = strlen(message)+100;
	s = malloc(len);/*estimation*/
	vsnprintf(s,len,message, ap);
	syslog(LOG_NOTICE, s);
	free(s);
      }
      else 
#endif
	  {
	vprintf(message, ap);
	putchar('\n');
      }
      va_end(ap);
    }
  
  if ( EXIT & mask )
    {
      exit( 0 );
    }
}

#else

void wack_alarm( int32 mask, char *message, 
                        void *ptr1, void *ptr2, void *ptr3, void *ptr4, 
                        void *ptr5, void *ptr6, void *ptr7, void *ptr8,
                        void *ptr9, void *ptr10, void*ptr11, void *ptr12,
                        void *ptr13, void *ptr14, void *ptr15, void *ptr16,
                        void *ptr17, void *ptr18, void *ptr19, void *ptr20){

  if ( wack_alarm_mask & mask )
    {
      if ( wack_alarm_timestamp_format )
	{
	  char timestamp[42];
	  struct tm *tm_now;
	  time_t time_now;
	  size_t length;
	  
	  time_now = time(NULL);
	  tm_now = localtime(&time_now);
	  length = strftime(timestamp, 40,
			    wack_alarm_timestamp_format, tm_now);
	  timestamp[length] = ' ';
	  if(syslog_flag)
	    syslog(LOG_NOTICE, timestamp);
	  else
	    fwrite(timestamp, length+1, sizeof(char), stdout);
	}
      if(syslog_flag){
	syslog(LOG_NOTICE, message, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6,
	       ptr7, ptr8,ptr9,ptr10, ptr11, ptr12, ptr13, ptr14, ptr15,
	       ptr16, ptr17, ptr18, ptr19, ptr20 );
      }
      else
	printf(message, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7, ptr8, ptr9,
	       ptr10, ptr11, ptr12, ptr13, ptr14, ptr15, ptr16, ptr17, ptr18,
	       ptr19, ptr20 );
        putchar('\n');
    }
  if ( EXIT & mask )
    {
      perror("errno say:");
      exit( 0 );
    }
}

#endif /* HAVE_GOOD_VARGS */


void wack_alarm_enable_syslog(char *ident){
#ifdef HAVE_SYSLOG_H
  openlog(ident,LOG_PID, LOG_DAEMON);
  syslog_flag = 1;
#endif
}


void wack_alarm_set_interactive(void) 
{
        wack_alarmInteractiveProgram = TRUE;
}

int  wack_alarm_get_interactive(void)
{
        return(wack_alarmInteractiveProgram);
}

void wack_alarm_set_output(char *filename) {
        FILE *newfile;
        newfile = freopen(filename, "a", stdout);
        if ( NULL == newfile ) {
                perror("failed to open file for stdout");
        }
        newfile = freopen(filename, "a", stderr);
        if ( NULL == newfile ) {
                perror("failed to open file for stderr");
        }
        setvbuf(stderr, (char *)0, _IONBF, 0);
        setvbuf(stdout, (char *)0, _IONBF, 0);
}

void wack_alarm_enable_timestamp(char *format)
{
        static char _local_timestamp[40];
	if(format)
	  strncpy(_local_timestamp, format, 40);
	else
	  strncpy(_local_timestamp, DEFAULT_TIMESTAMP_FORMAT, 40);
        wack_alarm_timestamp_format = _local_timestamp;
}

void wack_alarm_disable_timestamp(void)
{
        wack_alarm_timestamp_format = NULL;
}

void wack_alarm_set(int32 mask)
{
	wack_alarm_mask = wack_alarm_mask | mask;
}

void wack_alarm_clear(int32 mask)
{
	wack_alarm_mask = wack_alarm_mask & ~mask;
}

int32 wack_alarm_get(void)
{
        return(wack_alarm_mask);
}
