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
#include "abt.h"
#include "arpcache.h"

abt *new_abt() {
  abt *nh;
  nh = malloc(sizeof(abt));
  nh->size=0;
  nh->space=128;
  nh->data=malloc(sizeof(arp_entry)*nh->space);
  return nh;
}

void add_abt(abt *h, arp_entry toadd) {
  int f, l, c;
  f = 0; l = h->size;
  while(f <= l) {
    c = f + (l-f)/2;
    if(c == h->size) break;
    if(h->data[c].ip == toadd.ip) {
      memcpy(&h->data[c], &toadd, sizeof(arp_entry));
      return;
    }
    if(h->data[c].ip > toadd.ip) { l = c-1; }
    else { f = c+1; }
  }
  h->size++;
  if(h->size == h->space) {
    h->space *= 2;
    h->data = realloc(h->data, h->space*sizeof(arp_entry));
  }
  if(l == h->size - 1) {
    memcpy(&h->data[l], &toadd, sizeof(arp_entry));
  } else {
    if(toadd.ip > h->data[l].ip) l++;
    memmove(h->data + l + 1, h->data + l, sizeof(arp_entry)*(h->size - l));
    memcpy(&h->data[l], &toadd, sizeof(arp_entry));
  }
}

int chk_abt(abt *h, arp_entry tofind) {
  int f,l,c;
  f = 0; l = h->size;
  while(f <= l) {
    c = f + (l-f)/2;
    if(f == h->size) break;
    if(h->data[c].ip == tofind.ip) return 1;
    if(h->data[c].ip > tofind.ip) { l = c-1; }
    else { f = c+1; }
  }
  return 0;
}
