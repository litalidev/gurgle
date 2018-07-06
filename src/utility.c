/* $Id: utility.c,v 1.6 2001/04/27 10:38:10 timc Exp $
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
 * GURGLE GENERAL UTILITY FUNCTIONS
 */

#include <grg.h>

/*
 * Allocate dynamic memory when needed, added space initialised to zero
 */

void grg_alloc(datum,ndx) datumdef *datum; int ndx; {
unsigned int ptr;
  while ((ndx+1) >= datum->size) {
    ptr = datum->size * datum->bsize;
    datum->size += datum->block;
    if (gppdebug == 2)
      fprintf(stderr,"Allocating: %s(%d*%d)\n",datum->name,datum->size,
        datum->bsize);
    datum->value = realloc(datum->value,datum->size*datum->bsize);
    if (datum->value == NULL) gpperror("memory allocation failure",NULL);
    for (; ptr < datum->size * datum->bsize; ptr++) datum->value[ptr] = '\0';
  }
}

/*
 * Allocate 1 byte dynamic memory initialiser
 */

void grg_alloc_init(datum) datumdef *datum; {
  if (datum->value == NULL) {
    if (gppdebug == 2)
      fprintf(stderr,"Initial Allocating: %s(%d)\n",datum->name,1);
    datum->value = calloc(1,sizeof(char));
    if (datum->value == NULL) gpperror("memory allocation failure",NULL);
  }
}

/*
 * Deallocate dynamic memory
 */

void grg_dealloc(datum) datumdef *datum; {
  if (datum->value != NULL) {
    if (gppdebug == 2) fprintf(stderr,"Deallocating: %s\n",datum->name);
    free(datum->value);
    datum->value = NULL;
  }
}

/*
 * Dynamic resizing version of strcpy
 */

char *grg_strcpy(datum,str) datumdef *datum; char *str; {
  return(grg_strncpy(datum,str,strlen(str)+1));
}

/*
 * Dynamic resizing version of strncpy
 */

char *grg_strncpy(datum,str,num) datumdef *datum; char *str; int num; {
  if (num >= datum->size) grg_alloc(datum,num);
  return(strncpy(datum->value,str,num));
}

/*
 * Dynamic resizing version of strcat
 */

char *grg_strcat(datum,str) datumdef *datum; char *str; {
  return(grg_strncat(datum,str,strlen(str)));
}

/*
 * Dynamic resizing version of strncat
 */

char *grg_strncat(datum,str,num) datumdef *datum; char *str; int num; {
  if (datum->value == NULL) return(grg_strncpy(datum,str,num));
  else if (strlen(datum->value)+num >= datum->size)
    grg_alloc(datum,strlen(datum->value)+num);
  return(strncat(datum->value,str,num));
}

/*
 * Dynamic resizing version of sprintf (reduced), precalculates length
 */

#include <stdarg.h>

int grg_sprintf(datumdef *datum,const char *str,...) {
va_list ap;
datumdef *datumval;
char *sval;
int c, dc, self;
  va_start(ap,str);
  for (c = dc = 0; str[c] != '\0'; c++) {
    if (str[c] != '%') {
      if (dc == datum->size) grg_alloc(datum,dc);
      datum->value[dc++] = str[c];
      datum->value[dc] = '\0';
    }
    else {
      switch (str[++c]) {
        case '@': /* datum - dynamic string arg */
          datumval = va_arg(ap,datumdef *);
          sval = datumval->value;
          /* check if self referential, set flag if so since pointer
           * may change on reallocation */
          if (sval == (char *) datum->value) self = 1;
          else self = 0;
          grg_alloc(datum,dc+strlen(sval));
          if (self) sval = datum->value;
          for(; *sval; sval++) datum->value[dc++] = *sval;
          datum->value[dc] = '\0';
          break;
        case 's':
          sval = va_arg(ap,char *);
          /* check if self referential, set flag if so since pointer
           * may change on reallocation */
          if (sval == (char *) datum->value) self = 1;
          else self = 0;
          grg_alloc(datum,dc+strlen(sval));
          if (self) sval = datum->value;
          for(; *sval; sval++) datum->value[dc++] = *sval;
          datum->value[dc] = '\0';
          break;
        case 'c':
          if (dc == datum->size) grg_alloc(datum,dc);
          datum->value[dc++] = va_arg(ap,int);
          datum->value[dc] = '\0';
          break;
        default:
          if (dc == datum->size) grg_alloc(datum,dc);
          datum->value[dc++] = str[c];
          datum->value[dc] = '\0';
          break;
      }
    }
  }
  va_end(ap);
  return(dc);
}

/*
 * Remove leading and trailing blanks from a string
 */

void strip(st) char *st; {
register int ct, of;
  for (ct = strlen(st) - 1; (ct >= 0) && (st[ct] != '\0'); ct--)
    if (st[ct] != ' ') break;
    else st[ct] = '\0';
  for (ct = 0; st[ct] != '\0'; ct++)
    if (st[ct] != ' ') break;
  for (of = 0; (st[of] = st[ct+of]) != '\0'; of++);
}

/*
 * Compare two dBase3+ date strings
 */

int datcmp(s1,s2) char *s1, *s2; {
int d1, d2, y, m, d;
  y = ((s1[0]+'0')*1000)+((s1[1]+'0')*100)+((s1[2]+'0')*10)+(s1[3]+'0');
  m = ((s1[4]+'0')*10)+(s1[5]+'0');
  d = ((s1[6]+'0')*10)+(s1[7]+'0');
  d1 = d+(32*m)+(415*y);
  y = ((s2[0]+'0')*1000)+((s2[1]+'0')*100)+((s2[2]+'0')*10)+(s2[3]+'0');
  m = ((s2[4]+'0')*10)+(s2[5]+'0');
  d = ((s2[6]+'0')*10)+(s2[7]+'0');
  d2 = d+(32*m)+(415*y);
  if (d1 < d2) return(-1);
  else if (d1 > d2) return(1);
  else return(0);
}

/*
 * Replacement for what appears to be broken atof(3) function
 */

/*float atof(str) char *str; {
float a;
  (void) sscanf(str,"%f",&a);
  return(a);
}*/

/*
 * Makes a directory tree for the basename component of input string like doing
 * a `mkdir -p `dirname path``
 * Just does the system call for simplicity and compatibility (if not speed)!
 */

void makepathfor(path) char *path; {
datumdef datadef = { NULL, 0, STRMAX, sizeof(char), "data" }, *data = &datadef;
  grg_sprintf(data,"mkdir -p `dirname %s`",path);
  if (system(data->value)) {
    fprintf(stderr,"grg: couldn't make directory path for \"%s\": ",gppoutput);
    perror("");
    exit(1);
  }
}

/****************************************************************************/
