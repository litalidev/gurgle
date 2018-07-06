/* $Id: filter.c,v 1.3 2001/04/27 10:38:10 timc Exp $
   Copyright (C) University of Edinburgh, 1992-1997, 2001, 2003-4, 2008-10.
   All rights reserved. */

/* 
 * GURGLE - GNU REPORT GENERATOR LANGUAGE
 *
 * Copyright (C) University of Edinburgh, 1992-1997, 2001, 2003-4, 2008-10.
 * All rights reserved.
 *
 * The Author, Tim Edward Colles, has exercised his right to be identified
 * as such under the Copyright, Designs and Patents Act 1988.
 *
 * This file is part of Gurgle.
 * 
 * Gurgle is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or any later version.
 * 
 * Gurgle is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Gurgle. If not, see <http://www.gnu.org/licences/>.
 *
 */

/*
 * GURGLE FILTER HANDLING
 */

#include <grg.h>

#ifdef REGEX

#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include <regexp.h>
#endif

char *regexp_ndx;

#ifdef HAVE_REGEX_H
regex_t re_pattern[TEXFILTERMAX];
#endif
char re_buffer[TEXFILTERMAX][REGEXPMAX];
char re_field[TEXFILTERMAX][DBFFIELDMAX];

/*
 * Filter records with any number of multiple filter conditions
 */

int gppfilter(db,rc) dbase *db; int rc; {
char pattern[STRMAX], regexp[STRMAX], *p = pattern, *s, *eqs = pattern;
unsigned short int ctr, chr, result = 0, tmp, eqt;
  for (ctr = 0; ctr < texfiltermax; ctr++) {
    if (texfilter[ctr].re[0] != '\0' && re_field[ctr][0] == '\0') {
      if (texfilter[ctr].re[0] != '%') {
        gpperror("reg. exp. syntax error \"%%<FLD>=<RE>\"",texfilter[ctr].re);
      }
      else {
        s = texfilter[ctr].re;
        for (chr=1; (s[chr]>='A'&&s[chr]<='Z')||s[chr]=='_'||(s[chr]>='0'&&s[chr]<='9'); chr++);
        (void) strncpy(re_field[ctr],s+1,chr-1);
        re_field[ctr][chr-1] = '\0';
        if (s[chr] != '=') {
          gpperror("reg. exp. syntax error \"%<FLD>=<RE>\"",texfilter[ctr].re);
        }
        else strcpy(regexp,s+chr+1);
      }
      if (strcmp(re_field[ctr],"_EQ") == 0) {
        strcpy(re_buffer[ctr],regexp);
      }
      else {
        regexp_ndx = regexp;
#ifdef HAVE_REGEX_H
        (void) regcomp(&re_pattern[ctr],regexp,REG_EXTENDED|REG_NOSUB);
#else
        (void) compile(regexp,re_buffer[ctr],&re_buffer[ctr][REGEXPMAX],'\0'); 
#endif
        strcpy(re_buffer[ctr],"X");
      }
    }
    if (re_buffer[ctr][0] == '\0') result++;
    else {
      if (strcmp(re_field[ctr],"_EQ") == 0) {
        gppequate(re_buffer[ctr],db,rc,0);
        if (eqstack_ndx > 1) gpperror("stack overflow",NULL);
        else if (eqstack_ndx == 1) {
          gpppop(eqs,&eqt);
          tmp = 0;
          if ((eqt == EQ_STR || eqt == EQ_DATE) && eqs[0] != '\0') tmp = 1;
          else if (eqt == EQ_NUM && atoi(eqs) != 0) tmp = 1;
          else if (eqt != EQ_STR && eqt != EQ_DATE && eqt != EQ_NUM)
            gpperror("filter equate - number/string/date expected",NULL);
          if (tmp && texfilter[ctr].st == 0) result++;
          else if (tmp) {
            tmp = texfilter[ctr].st;
            for (ctr++; ctr < texfiltermax && texfilter[ctr].st > tmp; ctr++);
            result += texfilter[--ctr].st;
          }
          else if (texfilter[ctr].st == 0) return(0);
        }
        else if (texfilter[ctr].st == 0) return(0);
      }
      else {
        if (dbGetf(db,re_field[ctr],rc,&p) != 0)
          gpperror("unknown field name",re_field[ctr]);
        else {
          strncpy(pattern,p,0xFF);
          strip(pattern);
        }
#ifdef HAVE_REGEX_H
        if (!regexec(&re_pattern[ctr],pattern,0,0,0)) {
#else
        if (step(pattern,re_buffer[ctr])) {
#endif
          if (texfilter[ctr].st == 0) result++;
          else {
            tmp = texfilter[ctr].st;
            for (ctr++; ctr < texfiltermax && texfilter[ctr].st > tmp; ctr++);
            result += texfilter[--ctr].st;
          }
        }
        else if (texfilter[ctr].st == 0) return(0);
      }
    }
  }
  if (result == texfiltermax) return(1);
  else return(0);
}

#else

/*
 * No regular expression support, stub always returns a match
 */

int gppfilter(db,rc) dbase *db; int rc; {
  return(1);
}

#endif

/****************************************************************************/
