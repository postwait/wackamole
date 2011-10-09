%{
/* ======================================================================
 * Copyright (c) 2000 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle <jesus@omniti.com>
 * This code was written to facilitate clustered logging via Spread.
 * More information on Spread can be found at http://www.spread.org/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#include <string.h>

#include "config.h"
#include "config_help.h"
#include "y.tab.h"
#include "userloader.h"

extern int line_num;
extern int semantic_errors;

static void toCIDR(char *, YYSTYPE *);

#define yylval wacklval
#define yytext wacktext
#define yyparse wackparse
%}
ipaddr	[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\/[0-9]{1,2}
ipaddrn	[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\/[0-9]{1,2}n[0-9]{1,2}
timeint	[0-9]+[sSmMhH]
number	[0-9]+
qstring	\"[^\"]*\"|\'[^\']*\'
string	[^ \t\r\n#{}]+
snc	[^ \t\r\n#{}:]+
%option noyywrap nounput
%%
#.*                             {} /* Comments */
[ \t\r]                         {} /* White space */
\n                              { line_num++;}
"{"                             { return W_OPENBRACE; }
"}"                             { return W_CLOSEBRACE; }
"="                             { return W_EQUALS; }
[Nn][Oo][Nn][Ee]		{ return W_NONE; }
[Aa][Ll][Ll]			{ return W_ALL; }
[Aa][Rr][Pp]-[Cc][Aa][Cc][Hh][Ee]	{ return W_ARPCACHE; }
[Pp][Ii][Nn][Gg]	{ return W_PING; }
[Cc][Oo][Nn][Tt][Rr][Oo][Ll]	{ return W_CONTROL; }
[Tt][Hh][Rr][Oo][Tt][Tt][Ll][Ee]	{ return W_THROTTLE; }
[Ss][Pp][Rr][Ee][Aa][Dd]		{ return W_SPREAD; }
[Ss][Pp][Rr][Ee][Aa][Dd][Rr][Ee][Tt][Rr][Yy][Ii][Nn][Tt][Ee][Rr][Vv][Aa][Ll]		{ return W_SPREADRETRYINTERVAL; }
[Rr][Uu][Nn][Dd][Yy][Nn][Aa][Mm][Ii][Cc]        { return W_RUNDYNAMIC; }
[Mm][Oo][Dd][Uu][Ll][Ee][Dd][Ii][Rr]            { return W_MODULEDIR; }
[Pp][Ee][Rr][Ll][Uu][Ss][Ee][Ll][Ii][Bb]        { return W_PERLUSELIB; }
[Pp][Ee][Rr][Ll][Uu][Ss][Ee]                    { return W_PERLUSE; }
[Oo][Nn]" "[Uu][Pp]                     { yylval.number = DLFUNCS_TYPE_ON_UP; return W_ACTION_TYPE; }
[Oo][Nn]" "[Dd][Oo][Ww][Nn]                     { yylval.number = DLFUNCS_TYPE_ON_DOWN; return W_ACTION_TYPE; }
[Pp][Oo][Ss][Tt]" "[Uu][Pp]                     { yylval.number = DLFUNCS_TYPE_POST_UP; return W_ACTION_TYPE; }
[Pp][Oo][Ss][Tt]" "[Dd][Oo][Ww][Nn]                     { yylval.number = DLFUNCS_TYPE_POST_DOWN; return W_ACTION_TYPE; }
[Ll][Oo][Gg][Ff][Aa][Cc][Ii][Ll][Ii][Tt][Yy]	{ return W_LOG; }
[Mm][Aa][Tt][Uu][Rr][Ee]	{ return W_MATURE; }
[Bb][Aa][Ll][Aa][Nn][Cc][Ee]	{ return W_BALANCE; }
[Aa][Cc][Qq][Uu][Ii][Ss][Ii][Tt][Ii][Oo][Nn][Ss][Pp][Ee][Rr][Rr][Oo][Uu][Nn][Dd]		{ return W_AQPR; }
[Ii][Nn][Tt][Ee][Rr][Vv][Aa][Ll]	{ return W_INTERVAL; }
[Gg][Rr][Oo][Uu][Pp]		{ return W_GROUP; }
[Pp][Rr][Ee][Ff][Ee][Rr]	{ return W_PREFER; }
[Nn][Oo][Tt][Ii][Ff][Yy]	{ return W_NOTIFY; }
[Vv][Ii][Rr][Tt][Uu][Aa][Ll][Ii][Nn][Tt][Ee][Rr][Ff][Aa][Cc][Ee][Ss] { return W_VIFS; }
[Vv][Ii][FfPp][Ss]		{ return W_VIFS; }
{number}			{ yylval.number = atoi(yytext);
				  return W_NUMBER; }
{timeint}			{ int i, l;
				  i = atoi(yytext);
				  l = strlen(yytext)-1;
				  switch(yytext[l]) {
				    case 'm':
				    case 'M':
				      i*=60;
				    default:
				      break;
				  }
				  yylval.tv.tv_sec = i;
				  yylval.tv.tv_usec = 0L;
				  return W_TIMEINTERVAL; }
{snc}":"{ipaddr}		{ int i;
				  for(i=0;i<(IFNAMSIZ-1);i++) {
				    if(yytext[i]==':') break;
				    yylval.ip.iface[i] = yytext[i];
				  }
				  yylval.ip.iface[i++]='\0';
				  toCIDR(yytext+i, &yylval);
				  return W_IPADDR;
				}
{snc}":"{ipaddrn}		{ int i;
				  for(i=0;i<(IFNAMSIZ-1);i++) {
				    if(yytext[i]==':') break;
				    yylval.ip.iface[i] = yytext[i];
				  }
				  yylval.ip.iface[i++]='\0';
				  toCIDR(yytext+i, &yylval);
				  return W_IPADDR;
				}
{ipaddr}			{ yylval.ip.iface[0]='\0';
				  toCIDR(yytext, &yylval);
				  return W_IPADDR;
				}
{qstring}                       { int l = strlen(yytext);
                                  yytext[l-1] = 0;
                                  yylval.string = strdup(yytext+1);
                                  return W_STRING; }
{string}                        { yylval.string = strdup(yytext);
                                  return W_STRING; }
%%
static void toCIDR(char *yytext, YYSTYPE *lval) {
  struct in_addr inaddr;
  int a1,a2,a3,a4,a5=32,a6=24;
  char *c1,*c2,*c3,*c4,*c5=NULL,*c6=NULL;
  c1=strdup(yytext);
  c2=strchr(c1, '.'); *(c2++)=0;
  c3=strchr(c2, '.'); *(c3++)=0;
  c4=strchr(c3, '.'); *(c4++)=0;
  if(c4 && (c5=strchr(c4, '/')) != NULL)
    *(c5++)=0;
  if(c5 && (c6=strchr(c5, 'n')) != NULL)
    *(c6++)=0;
  a1=atoi(c1); a2=atoi(c2); a3=atoi(c3);
  a4=atoi(c4);
  if(c5) a5=atoi(c5);
  if(c6) a6=atoi(c6);
  free(c1);
  if ((a1 < 0) || (a1 > 255) ||
      (a2 < 0) || (a2 > 255) ||
      (a3 < 0) || (a3 > 255) ||
      (a4 < 0) || (a4 > 255) ||
      (a5 < 0) || (a5 > 32) ||
      (a6 < 0) || (a6 > 32))
  wackerror("Invalid IP");
  inaddr.s_addr = htonl(((a1<<24)|(a2<<16)|(a3<<8)|a4));
  (*lval).ip.addr = inaddr;
  (*lval).ip.mask = a5;
  (*lval).ip.network.s_addr = htonl(0xffffffff << (32-a6));
}

