/* $Id: equate.c,v 1.7 2001/05/09 10:36:50 timc Exp $
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
 * GURGLE REVERSED EQUATE, EQUATE AND TEXT BODY HANDLING
 */

#include <grg.h>

/*
 * GURGLE REVERSED EQUATE HANDLING
 */

/*
 * Whether to trace the equate processing
 */

unsigned short int gppeqtrace = 0;

/*
 * The equate Stack
 */

eqsdef eqstack[MAXEQSTACK];

/*
 * Equate stack pointer
 */

signed short int eqstack_ndx = 0;

/*
 * Local variable pointer
 */

signed short int eqlvar_ndx = MAXEQSTACK - 1;

/*
 * The equate variable table
 */

struct {
  char name[MAXMACRONAME];
  unsigned short int type;
  datumdef data;
} eqvartable[MAXEQVARS];

/*
 * Equate variable table index
 */

unsigned short int eqvartable_ndx = 0;

/*
 * Local variable table index
 */

unsigned short int eqlvartable_ndx = MAXEQVARS - 1;

/*
 * Local variable frame base
 */

signed short int eqlvartable_base = MAXEQVARS - 1;

/*
 * Macro setting a string according to data type
 */

#define eq_type(T)      (T==EQ_STR)?"EQ_STR":\
((T==EQ_NUM)?"EQ_NUM":\
((T==EQ_DEC)?"EQ_DEC":\
((T==EQ_DATE)?"EQ_DATE":\
((T==EQ_BOOL)?"EQ_BOOL":\
((T==EQ_FLD)?"EQ_FLD":"VOID")))))
  
/*
 * For holding discard or temporary values
 */

datumdef discdef = { NULL, 0, STRMAX, sizeof(char), "null" }, *disc = &discdef;
datumdef tempdef = { NULL, 0, STRMAX, sizeof(char), "temp" }, *temp = &tempdef;
  
/*
 * Initialise equate stack data
 */

void eqstack_init() {
unsigned long int ndx;
  for (ndx = 0; ndx < MAXEQSTACK; ndx++) {
    eqstack[ndx].data.block = STRMAX;
    eqstack[ndx].data.bsize = sizeof(char);
    sprintf(eqstack[ndx].data.name,"stack%d",ndx);
  }
}
  
/*
 * Create a new empty equate macro (if doesn't already exist), returns index
 */
  
unsigned int grg_define_equate(name) char *name; {
unsigned int ndx;
  grg_alloc(&equatetable,equatetable_ndx);
  for (ndx = 0; ndx < equatetable_ndx; ndx++)
    if (strcmp(GRGEQUATE[ndx].name,name) == 0) break;
  if (ndx == equatetable_ndx) {
    strcpy(GRGEQUATE[equatetable_ndx].name,name);
    sprintf(GRGEQUATE[equatetable_ndx].body.name,"EQUATE.BLOCK.%s",name);
    GRGEQUATE[equatetable_ndx].body.block = STRMAX*2;
    GRGEQUATE[equatetable_ndx].body.bsize = sizeof(char);
    grg_alloc_init(&GRGEQUATE[equatetable_ndx].body);
    sprintf(GRGEQUATE[equatetable_ndx].definition.name,"EQUATE.DEF.%s",name);
    GRGEQUATE[equatetable_ndx].definition.block = MAXEQUATEDEF;
    GRGEQUATE[equatetable_ndx].definition.bsize = sizeof(char);
    grg_alloc_init(&GRGEQUATE[equatetable_ndx].definition);
    equatetable_ndx++;
  }
  return(ndx);
}

/*
 * Errors from equate go here, prints error message plus a stack dump
 */

void gppeqerror(str) char *str; {
unsigned short int ctr;
  fprintf(stderr,"grg: %s.\n",str);
  fprintf(stderr,"Stack Trace:\n");
  for (ctr = 0; ctr < eqstack_ndx; ctr++)
    fprintf(stderr,"[%04d] %s \"%s\"\n",
      ctr,eq_type(eqstack[ctr].type),eqstack[ctr].data.value);
  exit(1);
}

/*
 * Push a value onto the local variable stack
 */

void grg_lvpush(i,d) datumdef *i; unsigned short int d; {
  if (gppeqtrace)
    printf("*LVCREAT{i=%d,%s} <- {%s}\n",eqlvar_ndx,eq_type(d),i->value);
  if (eqlvar_ndx < eqstack_ndx) {
    gpperror("local variable stack overflow",NULL);
  }
  else {
    eqstack[eqlvar_ndx].type = d;
    grg_strcpy(&eqstack[eqlvar_ndx].data,i->value);
    eqlvar_ndx--;
  }
}

/*
 * Retreive the value of an equate variable
 */

void grg_getvar(name,o,d) char *name; datumdef *o; unsigned short int *d; {
unsigned long int ndx;
  if (strcmp(name,"_") == 0) {
    char str[16];
    sprintf(str,"%d",eqstack_ndx);
    grg_strcpy(o,str);
    *d = EQ_NUM;
  }
  else if (name[0] == '_') {
    for (ndx = 0; ndx < eqvartable_ndx; ndx++) {
      if (strcmp(eqvartable[ndx].name,name) == 0) {
        grg_strcpy(o,eqvartable[ndx].data.value);
        *d = eqvartable[ndx].type;
        break;
      }
    }
    if (ndx == eqvartable_ndx) gpperror("undefined equate variable",name);
    if (gppeqtrace)
      fprintf(stdout,"*GETSYSVAR{%s,%s} -> {%s}\n",name,eq_type(*d),o->value);
  }
  else {
    for (ndx = eqlvartable_base; ndx > eqlvartable_ndx; ndx--) {
      if (strcmp(eqvartable[ndx].name,name) == 0) {
        grg_strcpy(o,eqstack[eqvartable[ndx].type].data.value);
        *d = eqstack[eqvartable[ndx].type].type;
        break;
      }
    }
    if (ndx == eqlvartable_ndx) gpperror("undefined local variable",name);
    if (gppeqtrace)
      fprintf(stdout,"*GETLOCVAR{%s,%s} -> {%s}\n",name,eq_type(*d),o->value);
  }
}

/*
 * Write a value to an equate/local variable, create if neccessary
 */

void grg_putvar(name,i,d) char *name; datumdef *i; unsigned short int d; {
unsigned long int ndx;
  if (strcmp(name,"_") == 0) {
    if (d != EQ_NUM) gppeqerror("numeric operand expected");
    else if (atoi(i->value) < 0 || atoi(i->value) > eqlvar_ndx)
      gppeqerror("stack value out of range");
    eqstack_ndx = atoi(i->value);
  }
  else if (name[0] == '_') {
    if (gppeqtrace)
      fprintf(stdout,"*PUTSYSVAR{%s,%s} <- {%s}\n",name,eq_type(d),i->value);
    for (ndx = 0; ndx < eqvartable_ndx; ndx++) {
      if (strcmp(eqvartable[ndx].name,name) == 0) {
        if (eqvartable[ndx].type != d)
          gpperror("equate var defined differently",name);
        break;
      }
    }
    if (ndx == eqvartable_ndx) {
      if (eqvartable_ndx > eqlvartable_ndx)
        gpperror("too many equate vars",NULL);
      else {
        ndx = eqvartable_ndx++;
        strcpy(eqvartable[ndx].name,name);
        eqvartable[ndx].type = d;
        eqvartable[ndx].data.block = STRMAX;
        eqvartable[ndx].data.bsize = sizeof(char);
        sprintf(eqvartable[ndx].data.name,"global%d",ndx);
      }
    }
    grg_strcpy(&eqvartable[ndx].data,i->value);
    if (strcmp(eqvartable[ndx].name,_EQ_TRACE) == 0)
      gppeqtrace = atoi(i->value);
  }
  else {
    if (gppeqtrace)
      fprintf(stdout,"*PUTLOCVAR{%s,%s} <- {%s}\n",name,eq_type(d),i->value);
    for (ndx = eqlvartable_base; ndx > eqlvartable_ndx; ndx--)
      if (strcmp(eqvartable[ndx].name,name) == 0) break;
    if (ndx == eqlvartable_ndx) {
      if (eqlvartable_ndx < eqvartable_ndx)
        gpperror("too many local vars",NULL);
      else {
        ndx = eqlvartable_ndx--;
        strcpy(eqvartable[ndx].name,name);
        eqvartable[ndx].type = eqlvar_ndx;
        grg_lvpush(i,d);
      }
    }
    else grg_strcpy(&eqstack[eqvartable[ndx].type].data,i->value);
  }
}

/*
 * Retrieve the value of a numeric system variable
 */

int getsysvar(name) char *name; {
int type;
  grg_getvar(name,temp,&type);
  return(atoi(temp->value));
}

/*
 * Write the value of a numeric system variable (create if neccessary)
 */

void putsysvar(name,type,value) char *name; long int type, value; {
char str[STRMAX];
  switch (type) {
    case EQ_NUM:
    case EQ_BOOL:
      sprintf(str,"%d",value);
      break;
    case EQ_DEC:
      sprintf(str,"%f",value);
      break;
    case EQ_STR:
    case EQ_FLD:
    case EQ_DATE:
      strcpy(str,(char *)value);
      break;
  }
  grg_strcpy(temp,str);
  grg_putvar(name,temp,type);
}

/*
 * Retrieve the value of a string system variable
 */

char *getstrsysvar(name) char *name; {
int type = EQ_STR;
  grg_getvar(name,temp,&type);
  return(temp->value);
}

/*
 * Push a value onto the equate stack
 */

void grg_push(i,d) datumdef *i; unsigned short int d; {
  if (gppeqtrace) fprintf(stdout,"*PUSH{%s} <- {%s}\n",eq_type(d),i->value);
  if (eqstack_ndx > eqlvar_ndx) {
    gpperror("equate stack overflow",NULL);
  }
  else {
    eqstack[eqstack_ndx].type = d;
    grg_strcpy(&eqstack[eqstack_ndx].data,i->value);
    eqstack_ndx++;
  }
}

void gpppush(str,type) char *str; unsigned short int type; {
  grg_strcpy(temp,str);
  grg_push(temp,type);
}

/*
 * Pop a value off the equate stack
 */

void grg_pop(o,d) datumdef *o; unsigned short int *d; {
  if (--eqstack_ndx < 0) {
    gpperror("equate stack underflow",NULL);
  }
  else {
    *d = eqstack[eqstack_ndx].type;
    grg_strcpy(o,eqstack[eqstack_ndx].data.value);
  }
  if (gppeqtrace) fprintf(stdout,"*POP{%s} -> {%s}\n",eq_type(*d),o->value);
}

void gpppop(str,type) char *str; unsigned short int *type; {
  grg_pop(temp,type);
  (void) strcpy(str,temp->value);
}

/*
 * Returns true if character could be part of an equate variable name
 */

bool gppisvarchar(chr,pos) char chr; unsigned short int pos; {
  if (pos == EQF)
    if ((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z') ||
      chr == '_') return(1);
    else return(0);
  else if (pos == EQN || pos == EQL)
    if ((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z') ||
      (chr >= '0' && chr <= '9') || chr == '_') return(1);
    else return(0);
  else return(0);
}

/*
 * Returns true if character is whitespace (including control characters)
 */

bool gppiswschar(chr) char chr; {
  if (chr <= 32 || chr > 126) return(1);
  else return(0);
}

/*
 * Returns true if character could be part of an equate numeric literal
 */

bool gppisnumchar(chr,pos) char chr; unsigned short int pos; {
  if (pos == EQF)
    if (chr == '-' || chr == '+' || (chr >= '0' && chr <= '9')) return(1);
    else return(0);
  else if (pos == EQN || pos == EQL)
    if (chr >= '0' && chr <= '9') return(1);
    else return(0);
  else return(0);
}

/*
 * Returns true if character could be part of an equate string literal
 */

bool gppisstrchar(chr,pos) char chr; unsigned short int pos; {
  if (pos == EQF || pos == EQL)
    if (chr == '\"') return(1);
    else return(0);
  else if (pos == EQN)
    if (chr != '\"' && chr != '\0') return(1);
    else return(0);
  else return(0);
}

/*
 * Build up an equate variable name (or field/equate name) from expression
 */

void gppvarcollect(str,ostr,chr) char *str, *ostr; unsigned short int *chr; {
unsigned short int oldchr;
  for (oldchr = *chr; gppisvarchar(str[*chr],EQN); *chr = *chr + 1);
  (void) strncpy(ostr,str+oldchr,*chr-oldchr);
  ostr[*chr-oldchr] = '\0';
  *chr = *chr - 1;
}

int grg_collect_name(istr,ostr,ndx) datumdef *istr, *ostr; int ndx; {
int oldndx;
  for (oldndx = ndx; gppisvarchar(istr->value[ndx],EQN); ndx++);
  (void) grg_strncpy(ostr,istr->value+oldndx,ndx-oldndx);
  ostr->value[ndx-oldndx] = '\0';
  return(--ndx);
}

/*
 * Remove extraneous white space characters
 */

void gppwscollect(str,ostr,chr) char *str, *ostr; unsigned short int *chr; {
unsigned short int oldchr;
  for (oldchr = *chr; gppiswschar(str[*chr]); *chr = *chr + 1);
  (void) strncpy(ostr,str+oldchr,*chr-oldchr);
  ostr[*chr-oldchr] = '\0';
  *chr = *chr - 1;
}

int grg_collect_space(istr,ostr,ndx) datumdef *istr, *ostr; int ndx; {
int oldndx;
  for (oldndx = ndx; gppiswschar(istr->value[ndx]); ndx++);
  (void) grg_strncpy(ostr,istr->value+oldndx,ndx-oldndx);
  ostr->value[ndx-oldndx] = '\0';
  return(--ndx);
}

/*
 * Build up an equate numeric literal from expression
 */

int gppnumcollect(str,ostr,chr) char *str, *ostr; unsigned short int *chr; {
unsigned short int oldchr = *chr;
int type = EQ_NUM;
  *chr = *chr + 1;
  gppisnumchar(str[*chr],EQF);
  for (; gppisnumchar(str[*chr],EQN); *chr = *chr + 1);
  if (str[*chr] == '.' && gppisnumchar(str[*chr+1],EQN)) {
    type = EQ_DEC;
    for (*chr = *chr + 1; gppisnumchar(str[*chr],EQN); *chr = *chr + 1);
  }
  (void) strncpy(ostr,str+oldchr,*chr-oldchr);
  ostr[*chr-oldchr] = '\0';
  *chr = *chr - 1;
  return(type);
}

int grg_collect_number(i,o,ndx,d) datumdef *i, *o; int ndx; unsigned short int *d; {
int oldndx = ndx;
  *d = EQ_NUM;
  gppisnumchar(i->value[++ndx],EQF);
  for (; gppisnumchar(i->value[ndx],EQN); ++ndx);
  if (i->value[ndx] == '.' && gppisnumchar(i->value[ndx+1],EQN)) {
    *d = EQ_DEC;
    for (++ndx; gppisnumchar(i->value[ndx],EQN); ++ndx);
  }
  (void) grg_strncpy(o,i->value+oldndx,ndx-oldndx);
  o->value[ndx-oldndx] = '\0';
  return(--ndx);
}

/*
 * Build up an equate string literal from expression
 */

void gppstrcollect(str,ostr,chr) char *str, *ostr; unsigned short int *chr; {
unsigned short int oldchr, nchr, schr;
  *chr = *chr + 1;
  gppisstrchar(str[*chr],EQF);
  for (oldchr = *chr; gppisstrchar(str[*chr],EQN) ||
    (str[*chr] == '\"' && str[*chr-1] == '\\'); *chr = *chr + 1);
  for (nchr = oldchr, schr = 0; nchr < *chr; nchr++, schr++)
    if (str[nchr] == '\\' && str[nchr+1] == '\"') schr--;
    else ostr[schr] = str[nchr];
  ostr[schr] = '\0';
  if (!gppisstrchar(str[*chr],EQL)) gpperror("unterminated string",ostr);
}

/*
 * Build up a string literal (expand escapes)
 */

int grg_collect_xstring(i,o,n) datumdef *i, *o; int n; {
int on, nn, sn;
  gppisstrchar(i->value[++n],EQF);
  for (on = n; gppisstrchar(i->value[n],EQN) ||
    (i->value[n] == '\"' && i->value[n-1] == '\\'); n++);
  for (nn = on, sn = 0; nn < n; nn++, sn++) {
    if (sn == o->size) grg_alloc(o,sn);
    if (i->value[nn] == '\\' && i->value[nn+1] == '\"') sn--;
    else o->value[sn] = i->value[nn];
  }
  if (sn == o->size) grg_alloc(o,sn);
  o->value[sn] = '\0';
  if (!gppisstrchar(i->value[n],EQL)) gpperror("unterminated string",o->value);
  return(n);
}

/*
 * Build up a string literal (preserve escapes)
 */

int grg_collect_string(istr,ostr,ndx) datumdef *istr, *ostr; int ndx; {
int oldndx, nndx, sndx;
  gppisstrchar(istr->value[++ndx],EQF);
  for (oldndx = ndx; gppisstrchar(istr->value[ndx],EQN)
       || (istr->value[ndx] == '\"' && istr->value[ndx-1] == '\\'); ndx++);
  grg_alloc_init(ostr);
  /* STU FIX */
  grg_alloc (ostr, ndx - oldndx);
  /* END FIX */
  for (nndx = oldndx, sndx = 0; nndx < ndx; nndx++, sndx++)
    ostr->value[sndx] = istr->value[nndx];
  ostr->value[sndx] = '\0';
  if (!gppisstrchar(istr->value[ndx],EQL))
    gpperror("unterminated string",ostr->value);
  return(ndx);
}

/*
 * Build up an equate call or record looping construct from expression
 */

void gpprloopcollect(str,ostr,chr) char *str, *ostr; unsigned short int *chr; {
unsigned short int oldchr, ibc = 1, obc = 0;
  for (*chr = *chr + 1, oldchr = *chr; str[*chr] != '\0'; *chr = *chr + 1) {
    if (str[*chr] == '\"') gppstrcollect(str,ostr,chr);
    else if (str[*chr] == '(') ibc++;
    else if (str[*chr] == ')') obc++;
    if (ibc == obc) break;
  }
  if (str[*chr] == '\0') gppeqerror("mismatch in loop/call brackets");
  (void) strncpy(ostr,str+oldchr,*chr-oldchr);
  ostr[*chr-oldchr] = '\0';
}

#define grg_collect_old_roll_loop(i,o,n)  grg_collect_bracketed(i,o,n)

int grg_collect_bracketed(i,o,n) datumdef *i, *o; int n; {
int on, ibc = 1, obc = 0;
  for (on = ++n; i->value[n] != '\0'; n++) {
    if (i->value[n] == '\"') n = grg_collect_string(i,disc,n);
    else if (i->value[n] == '(') ibc++;
    else if (i->value[n] == ')') obc++;
    if (ibc == obc) break;
  }
  if (i->value[n] == '\0') gppeqerror("mismatch in loop/call brackets");
  (void) grg_strncpy(o,i->value+on,n-on);
  o->value[n-on] = '\0';
  return(n);
}

/*
 * Handles collection of new loop construct based on curly bracket pairs
 */

void gppnrloopcollect(str,ostr,chr) char *str, *ostr; unsigned short int *chr; {
unsigned short int oldchr, ibc = 1, obc = 0;
  for (*chr = *chr + 1, oldchr = *chr; str[*chr] != '\0'; *chr = *chr + 1) {
    if (str[*chr] == '\"') gppstrcollect(str,ostr,chr);
    else if (str[*chr] == '{') ibc++;
    else if (str[*chr] == '}') obc++;
    if (ibc == obc) break;
  }
  if (str[*chr] == '\0') gpperror("mismatch in loop/call brackets",NULL);
  (void) strncpy(ostr,str+oldchr,*chr-oldchr);
  ostr[*chr-oldchr] = '\0';
}

int grg_collect_roll_loop(i,o,n) datumdef *i, *o; int n; {
int on, ibc = 1, obc = 0;
  for (on = ++n; i->value[n] != '\0'; ++n) {
    if (i->value[n] == '\"') n = grg_collect_string(i,disc,n);
    else if (i->value[n] == '{') ibc++;
    else if (i->value[n] == '}') obc++;
    if (ibc == obc) break;
  }
  if (i->value[n] == '\0') gpperror("mismatch in roll-loop brackets",NULL);
  (void) grg_strncpy(o,i->value+on,n-on);
  o->value[n-on] = '\0';
  return(n);
}

/*
 * Build up an indexing construct from expression
 */

void gppndxcollect(str,ostr,chr) char *str, *ostr; unsigned short int *chr; {
unsigned short int oldchr, ibc = 1, obc = 0;
  for (*chr = *chr + 1, oldchr = *chr; str[*chr] != '\0'; *chr = *chr + 1) {
    if (str[*chr] == '\"') gppstrcollect(str,ostr,chr);
    else if (str[*chr] == '[') ibc++;
    else if (str[*chr] == ']') obc++;
    if (ibc == obc) break;
  }
  if (str[*chr] == '\0') gppeqerror("mismatch in index brackets");
  (void) strncpy(ostr,str+oldchr,*chr-oldchr);
  ostr[*chr-oldchr] = '\0';
}

int grg_collect_index(istr,ostr,ndx) datumdef *istr, *ostr; int ndx; {
unsigned int oldndx, ibc = 1, obc = 0;
  for (oldndx = ++ndx; istr->value[ndx] != '\0'; ndx++) {
    if (istr->value[ndx] == '\"') ndx = grg_collect_string(istr,ostr,ndx);
    else if (istr->value[ndx] == '[') ibc++;
    else if (istr->value[ndx] == ']') obc++;
    if (ibc == obc) break;
  }
  if (istr->value[ndx] == '\0') gppeqerror("mismatch in index brackets");
  (void) grg_strncpy(ostr,istr->value+oldndx,ndx-oldndx);
  ostr->value[ndx-oldndx] = '\0';
  return(ndx);
}

/*
 * Build up an equate while looping construct from expression
 */

void gppwloopcollect(str,lstr,rstr,chr) char *str, *lstr, *rstr;
unsigned short int *chr; {
unsigned short int oldchr, ibc = 1, obc = 0, nchr;
static char tmp[STRMAX];
  for (*chr = *chr + 1, oldchr = *chr; str[*chr] != '\0'; *chr = *chr + 1) {
    if (str[*chr] == '\"') gppstrcollect(str,lstr,chr);
    else if (str[*chr] == '[') ibc++;
    else if (str[*chr] == ']') obc++;
    if (ibc == obc) break;
  }
  if (str[*chr] == '\0') gppeqerror("mismatch in loop brackets");
  (void) strncpy(tmp,str+oldchr,*chr-oldchr);
  tmp[*chr-oldchr] = '\0';
  for (nchr = 0; tmp[nchr] != '\0'; nchr++) {
    if (tmp[nchr] == '\"') gppstrcollect(tmp,lstr,&nchr);
    else if (tmp[nchr] == ';') {
      (void) strncpy(lstr,tmp,nchr);
      lstr[nchr] = '\0';
      (void) strcpy(rstr,tmp+nchr+1);
      break;
    }
  }
  if (str[*chr] == '\0') gppeqerror("missing loop separator");
}

int grg_collect_loop(i,ol,or,n) datumdef *i, *ol, *or; int n; {
int on, ibc = 1, obc = 0, nn;
  for (on = ++n; i->value[n] != '\0'; ++n) {
    if (i->value[n] == '\"') n = grg_collect_string(i,disc,n);
    else if (i->value[n] == '[') ibc++;
    else if (i->value[n] == ']') obc++;
    if (ibc == obc) break;
  }
  if (i->value[n] == '\0') gppeqerror("mismatch in loop brackets");
  (void) grg_strncpy(temp,i->value+on,n-on);
  temp->value[n-on] = '\0'; 
  for (nn = 0; temp->value[nn] != '\0'; nn++) {
    if (temp->value[nn] == '\"') nn = grg_collect_string(temp,disc,nn);
    else if (temp->value[nn] == ';') {
      (void) grg_strncpy(ol,temp->value,nn);
      ol->value[nn] = '\0';
      (void) grg_strcpy(or,temp->value+nn+1);
      break;
    }
  }
  if (i->value[n] == '\0') gppeqerror("missing loop separator");
  return(n);
}

/*
 * Build up an equate conditional construct from expression
 */

short int gppcondcollect(str,lstr,rstr,chr,ctr,rev) char *str, *lstr, *rstr;
unsigned short int *chr; short int ctr; short int rev; {
unsigned short int oldchr;
char tmp[STRMAX];
  for (oldchr = *chr; str[*chr] != '\0'; *chr = *chr + 1) {
    if (str[*chr] == '\"') gppstrcollect(str,tmp,chr);
    else if (str[*chr] == '[') gppwloopcollect(str,tmp,tmp,chr);
    else if (str[*chr] == '{' && rev) {
      gppnrloopcollect(str,tmp,chr);
      if (str[*chr+1] == ':') *chr = *chr + 1;
    }
    else if (str[*chr] == '?') {
      *chr = *chr + 1;
      (void) gppcondcollect(str,lstr,rstr,chr,-1,rev);
    }
    else if (str[*chr] == ':') {
      (void) strncpy(lstr,str+oldchr,*chr-oldchr);
      lstr[*chr-oldchr] = '\0';
      break;
    }
  }
  if (str[*chr] != ':') gpperror("unseparated condition",NULL);
  *chr = *chr + 1;
  for (oldchr = *chr; str[*chr] != '\0'; *chr = *chr + 1) {
    if (str[*chr] == '\"') gppstrcollect(str,tmp,chr);
    else if (str[*chr] == '[') gppwloopcollect(str,tmp,tmp,chr);
    else if (str[*chr] == '{' && rev) {
      gppnrloopcollect(str,tmp,chr);
      if (str[*chr+1] == ':') *chr = *chr + 1;
    }
    else if (str[*chr] == '?') {
      *chr = *chr + 1;
      (void) gppcondcollect(str,tmp,rstr,chr,-1,rev);
    }
    else if (str[*chr] == ';') {
      (void) strncpy(rstr,str+oldchr,*chr-oldchr);
      rstr[*chr-oldchr] = '\0';
      ctr = 0;
      break;
    }
    else if (str[*chr] == ':' && str[*chr+1] == ';') {
      (void) strncpy(rstr,str+oldchr,*chr-oldchr);
      rstr[*chr-oldchr] = '\0';
      *chr = *chr + 1;
      ctr = 0;
      break;
    }
    else if (str[*chr] == ':') {
      (void) strncpy(rstr,str+oldchr,*chr-oldchr);
      rstr[*chr-oldchr] = '\0';
      break;
    }
  }
  if (str[*chr] != ';' && !ctr) gpperror("unterminated condition",NULL);
  return(ctr);
}

int grg_collect_condition(i,ol,or,n,ctr,rev) datumdef *i, *ol, *or; int n;
short int *ctr; short int rev; {
int on;
short int ret;
  for (on = n; i->value[n] != '\0'; n++) {
    if (i->value[n] == '\"') n = grg_collect_string(i,disc,n);
    else if (i->value[n] == '[') n = grg_collect_loop(i,disc,disc,n);
    else if (i->value[n] == '{' && rev) {
      n = grg_collect_roll_loop(i,disc,n);
      if (i->value[n+1] == ':') n++;
    }
    else if (i->value[n] == '?') {
      ret = -1;
      n = grg_collect_condition(i,disc,or,++n,&ret,rev);
    }
    else if (i->value[n] == ':') {
      (void) grg_strncpy(ol,i->value+on,n-on);
      ol->value[n-on] = '\0';
      break;
    }
  }
  if (i->value[n] != ':') gpperror("unseparated condition",NULL);
  n++;
  for (on = n; i->value[n] != '\0'; n++) {
    if (i->value[n] == '\"') n = grg_collect_string(i,disc,n);
    else if (i->value[n] == '[') n = grg_collect_loop(i,disc,disc,n);
    else if (i->value[n] == '{' && rev) {
      n = grg_collect_roll_loop(i,disc,n);
      if (i->value[n+1] == ':') n++;
    }
    else if (i->value[n] == '?') {
      ret = -1;
      n = grg_collect_condition(i,disc,or,++n,&ret,rev);
    }
    else if (i->value[n] == ';') {
      (void) grg_strncpy(or,i->value+on,n-on);
      or->value[n-on] = '\0';
      *ctr = 0;
      break;
    }
    else if (i->value[n] == ':' && i->value[n+1] == ';') {
      (void) grg_strncpy(or,i->value+on,n-on);
      or->value[n-on] = '\0';
      n++;
      *ctr = 0;
      break;
    }
    else if (i->value[n] == ':') {
      (void) grg_strncpy(or,i->value+on,n-on);
      or->value[n-on] = '\0';
      break;
    }
  }
  if (i->value[n] != ';' && !*ctr) gpperror("unterminated condition",NULL);
  return(n);
}

/*
 * Collect up an equate comment construct to be discarded from expression
 */

void gppcomcollect(str,ostr,chr) char *str, *ostr; unsigned short int *chr; {
unsigned short int oldchr, ibc = 1, obc = 0;
  for (*chr = *chr + 1, oldchr = *chr; str[*chr] != '\0'; *chr = *chr + 1) {
    if (str[*chr] == '{') ibc++;
    else if (str[*chr] == '}') obc++;
    if (ibc == obc) break;
  }
  if (str[*chr] == '\0') gppeqerror("mismatch in comment brackets");
  (void) strncpy(ostr,str+oldchr,*chr-oldchr);
  ostr[*chr-oldchr] = '\0';
}

int grg_collect_comment(istr,ostr,ndx) datumdef *istr, *ostr; int ndx; {
unsigned int oldndx, ibc = 1, obc = 0;
  for (oldndx = ++ndx; istr->value[ndx] != '\0'; ndx++) {
    if (istr->value[ndx] == '{') ibc++;
    else if (istr->value[ndx] == '}') obc++;
    if (ibc == obc) break;
  }
  if (istr->value[ndx] == '\0') gppeqerror("mismatch in comment brackets");
  (void) grg_strncpy(ostr,istr->value+oldndx,ndx-oldndx);
  ostr->value[ndx-oldndx] = '\0';
  return(ndx);
}

/*
 * The break and exit flags
 */

unsigned short int eqbreak, eqexit;

#ifdef GUILE

/*
 * The GUILE equate caller
 */

void gppeqguile(nam) char *nam; {
SCM s_out;
datumdef strdef = { NULL, 0, STRMAX, sizeof(char), "str" }, *str = &strdef;
char *str_out;
int num_out, len = STRMAX;
  s_out = gh_eval_str(nam);
  grg_alloc(str,STRMAX);
  if (gh_number_p(s_out)) {
    num_out = gh_scm2int(s_out);
    sprintf(str->value,"%d",num_out);
    grg_push(str,EQ_NUM);
  }
  else if (gh_boolean_p(s_out)) {
    num_out = gh_scm2bool(s_out);
    sprintf(str->value,"%d",num_out);
    grg_push(str,EQ_BOOL);
  }
  else if (gh_string_p(s_out)) {
    str_out = gh_scm2newstr(s_out,&len);
    grg_strcpy(str,str_out);
    grg_push(str,EQ_STR);
  }
  grg_dealloc(str);
}

#endif

/*
 * The main equate virtual machine, processes an equate expression
 */

datumdef eqpdef = { NULL, 0, STRMAX, sizeof(char), "eqp" }, *eqp = &eqpdef;

void gppequate(str,db,rc,rv) char *str; dbase *db; long rc; bool rv; {
  grg_strcpy(eqp,str);
  grg_equate(eqp,db,rc,rv);
}

datumdef namedef = { NULL, 0, STRMAX, sizeof(char), "name" }, *name = &namedef;
datumdef largdef = { NULL, 0, STRMAX, sizeof(char), "larg" }, *larg = &largdef;
datumdef rargdef = { NULL, 0, STRMAX, sizeof(char), "rarg" }, *rarg = &rargdef;
datumdef datadef = { NULL, 0, STRMAX, sizeof(char), "data" }, *data = &datadef;
unsigned short int larg_type, rarg_type, data_type;

void grg_equate(e,db,rc,rv) datumdef *e; dbase *db; long rc; bool rv; {
unsigned short int eqres, depth;
unsigned long int n, ndx;
char *value;
dbase *old_db;
int old_rc, old_ndx, old_base;
  grg_alloc(data,data->block);
  eqexit = eqbreak = 0;
  for (n = 0; e->value[n] != '\0'; n++) {
    if (eqexit) return;
    (void) grg_strcpy(name,"");
    switch (e->value[n]) {
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
      case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
      case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
      case 'v': case 'w': case 'x': case 'y': case 'z':
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
      case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
      case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
      case 'V': case 'W': case 'X': case 'Y': case 'Z':
      case '_': /* equate (function) call or variable push (unary) */
        n--;
      case '#': /* equate (function) call operator (unary) */
        if (gppisvarchar(e->value[n+1],EQF)) {
          n = grg_collect_name(e,name,++n);
          for (ndx = 0; ndx < equatetable_ndx; ndx++)
            if (strcmp(GRGEQUATE[ndx].name,name->value) == 0) break;
          if (ndx != equatetable_ndx) {
            if (gppeqtrace) printf("#\n");
            if (GRGEQUATE[ndx].guile) {
#ifdef GUILE
              if (gppeqtrace) printf("*EQGUILE{} <- %s(",name->value);
              old_ndx = eqstack_ndx;
              if (e->value[n+1] == '(') {
                datumdef eqdef =
                  { NULL, 0, STRMAX, sizeof(char), "eq" }, *eq = &eqdef;
                n = grg_collect_bracketed(e,eq,++n);
                if (gppeqtrace) printf("%s)\n",eq->value);
                if (!rv && GRGEQUATE[ndx].rev) {
                  grg_reverse(eq,data);
                  grg_strcpy(eq,data->value);
                  grg_equate(eq,db,rc,1);
                }
                else grg_equate(eq,db,rc,rv);
                grg_dealloc(eq);
              }
              else if (gppeqtrace) printf(")\n");
              grg_sprintf(larg,")");
              for (; old_ndx < eqstack_ndx; ) {
                grg_pop(rarg,&rarg_type);
                switch (rarg_type) {
                  case EQ_NUM: case EQ_BOOL: case EQ_DEC:
                    grg_sprintf(data,"%@ %@",rarg,larg);
                    break;
                  case EQ_STR: case EQ_FLD: case EQ_DATE:
                    grg_sprintf(data,"\"%@\" %@",rarg,larg);
                    break;
                }
                grg_strcpy(larg,data);
              }
              grg_sprintf(data,"(%s %@",GRGEQUATE[ndx].name,larg);
              gppeqguile(data->value);
#endif
            }
            else
            {
              if (gppeqtrace) printf("*EQUATE{} <- %s(",name->value);
              if (e->value[n+1] == '(') {
                datumdef eqdef =
                  { NULL, 0, STRMAX, sizeof(char), "eq" }, *eq = &eqdef;
                n = grg_collect_bracketed(e,eq,++n);
                if (gppeqtrace) printf("%s)\n",eq->value);
                if (!rv && GRGEQUATE[ndx].rev) {
                  grg_reverse(eq,data);
                  grg_strcpy(eq,data->value);
                  grg_equate(eq,db,rc,1);
                }
                else grg_equate(eq,db,rc,rv);
                grg_dealloc(eq);
              }
              else if (gppeqtrace) printf(")\n");
              old_ndx = eqlvar_ndx;
              old_base = eqlvartable_base;
              eqlvartable_base = eqlvartable_ndx;
              grg_equate(&GRGEQUATE[ndx].definition,db,rc,GRGEQUATE[ndx].rev);
              eqlvartable_ndx = eqlvartable_base;
              eqlvartable_base = old_base;
              eqlvar_ndx = old_ndx;
            }
          }
          else
          {
            if (gppeqtrace) printf("<<\n");
            grg_getvar(name->value,larg,&larg_type);
            grg_push(larg,larg_type);
          }
        }
        else gppeqerror("equate right operand expected");
        break;
      case '%': /* pushfield operator or field operator */
        switch (e->value[n+1]) {
          case '%': /* pushfield operator (unary) */
            if (gppeqtrace) printf("%%%%");
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              if (gppeqtrace) printf("%s",name->value);
              if (e->value[n+1] == '-' && e->value[n+2] == '>') {
                n += 2;
                if (gppisvarchar(e->value[n+1],EQF)) {
                  n = grg_collect_name(e,larg,++n);
                  grg_sprintf(name,"%@->%@",name,larg);
                  if (gppeqtrace) printf("->%s",larg->value);
                }
                else gppeqerror("field right operand expected");
              }
              if (e->value[n+1] == '[') {
                n = grg_collect_index(e,larg,++n);
                grg_sprintf(name,"%@[%@]",name,larg);
                if (gppeqtrace) printf("[%s]",larg->value);
              }
              if (gppeqtrace) printf("\n");
              larg_type = EQ_FLD;
              grg_sprintf(larg,"%%%@",name);
              grg_push(larg,larg_type);
            }
            else gppeqerror("field right operand expected");
            break;
          case '#': /* field length operator (unary) */
            if (gppeqtrace) printf("%%#");
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              old_db = db;
              n = grg_collect_name(e,name,++n);
              if (gppeqtrace) printf("%s",name->value);
              if (e->value[n+1] == '-' && e->value[n+2] == '>') {
                n += 2;
                if (gppisvarchar(e->value[n+1],EQF)) {
                  n = grg_collect_name(e,larg,++n);
                  for (ndx = 0; ndx < texdbffile_ndx; ndx++) {
                    if (strcasecmp(GRGDATA[ndx].name,name->value) == 0) {
                      db = GRGDATA[ndx].db;
                      break;
                    }
                  }
                  if (ndx == texdbffile_ndx) gppeqerror("no such database");
                  if (gppeqtrace) printf("->%s",larg->value);
                  grg_strcpy(name,larg->value);
                }
                else gppeqerror("field right operand expected");
              }
              old_rc = rc;
              if (e->value[n+1] == '[') {
                datumdef eq1def =
                  { NULL, 0, STRMAX, sizeof(char), "eq1" }, *eq1 = &eq1def;
                datumdef eq2def =
                  { NULL, 0, STRMAX, sizeof(char), "eq2" }, *eq2 = &eq2def;
                grg_strcpy(eq1,name->value);
                n = grg_collect_index(e,eq2,++n);
                if (gppeqtrace) printf("[%s]\n",eq2->value);
                grg_equate(eq2,db,rc,rv);
                grg_pop(larg,&larg_type);
                if (larg_type != EQ_NUM)
                  gppeqerror("numeric operand expected");
                rc = atoi(larg->value);
                grg_strcpy(name,eq1->value);
                grg_dealloc(eq1);
                grg_dealloc(eq2);
              }
              else if (gppeqtrace) printf("\n");
              if (gppeqtrace) printf("*GETFIELD{%s}\n",name->value);
              if (!db) gppeqerror("no database defined");
              if (rc < 1 || rc > db->dhead->db_nrecs)
                gppeqerror("record index out of range");
              if (dbGetf(db,name->value,rc,&value) != 0)
                gpperror("unknown field name",name->value);
              for (ndx = 0; strcmp((db->dfrec+ndx)->fl_fname,name->value) != 0;
                 ndx++);
              sprintf(data->value,"%d",(db->dfrec+ndx)->fl_fsize);
              grg_strcpy(larg,data->value);
              grg_push(larg,EQ_NUM);
              db = old_db;
              rc = old_rc;
            }
            else gppeqerror("field right operand expected");
            break;
          case '$': /* field type operator (unary) */
            if (gppeqtrace) printf("%%$");
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              old_db = db;
              n = grg_collect_name(e,name,++n);
              if (gppeqtrace) printf("%s",name->value);
              if (e->value[n+1] == '-' && e->value[n+2] == '>') {
                n += 2;
                if (gppisvarchar(e->value[n+1],EQF)) {
                  n = grg_collect_name(e,larg,++n);
                  for (ndx = 0; ndx < texdbffile_ndx; ndx++) {
                    if (strcasecmp(GRGDATA[ndx].name,name->value) == 0) {
                      db = GRGDATA[ndx].db;
                      break;
                    }
                  }
                  if (ndx == texdbffile_ndx) gppeqerror("no such database");
                  if (gppeqtrace) printf("->%s",larg->value);
                  grg_strcpy(name,larg->value);
                }
                else gppeqerror("field right operand expected");
              }
              old_rc = rc;
              if (e->value[n+1] == '[') {
                datumdef eq1def =
                  { NULL, 0, STRMAX, sizeof(char), "eq1" }, *eq1 = &eq1def;
                datumdef eq2def =
                  { NULL, 0, STRMAX, sizeof(char), "eq2" }, *eq2 = &eq2def;
                grg_strcpy(eq1,name->value);
                n = grg_collect_index(e,eq2,++n);
                if (gppeqtrace) printf("[%s]\n",eq2->value);
                grg_equate(eq2,db,rc,rv);
                grg_pop(larg,&larg_type);
                if (larg_type != EQ_NUM)
                  gppeqerror("numeric operand expected");
                rc = atoi(larg->value);
                grg_strcpy(name,eq1->value);
                grg_dealloc(eq2);
                grg_dealloc(eq1);
              }
              else if (gppeqtrace) printf("\n");
              if (gppeqtrace) printf("*GETFIELD{%s}\n",name->value);
              if (!db) gppeqerror("no database defined");
              if (rc < 1 || rc > db->dhead->db_nrecs)
                gppeqerror("record index out of range");
              if (dbGetf(db,name->value,rc,&value) != 0)
                gpperror("unknown field name",name->value);
              for (ndx = 0; strcmp((db->dfrec+ndx)->fl_fname,name->value) != 0;
                 ndx++);
              grg_sprintf(larg,"%c",(db->dfrec+ndx)->fl_ftype);
              grg_push(larg,EQ_STR);
              db = old_db;
              rc = old_rc;
            }
            else gppeqerror("field right operand expected");
            break;
          default: /* field operator (unary) */
            if (gppeqtrace) printf("%%");
            if (gppisvarchar(e->value[n+1],EQF)) {
              old_db = db;
              n = grg_collect_name(e,name,++n);
              if (gppeqtrace) printf("%s",name->value);
              if (e->value[n+1] == '-' && e->value[n+2] == '>') {
                n += 2;
                if (gppisvarchar(e->value[n+1],EQF)) {
                  n = grg_collect_name(e,larg,++n);
                  for (ndx = 0; ndx < texdbffile_ndx; ndx++) {
                    if (strcasecmp(GRGDATA[ndx].name,name->value) == 0) {
                      db = GRGDATA[ndx].db;
                      break;
                    }
                  }
                  if (ndx == texdbffile_ndx) gppeqerror("no such database");
                  if (gppeqtrace) printf("->%s",larg->value);
                  grg_strcpy(name,larg->value);
                }
                else gppeqerror("field right operand expected");
              }
              old_rc = rc;
              if (e->value[n+1] == '[') {
                datumdef eq1def =
                  { NULL, 0, STRMAX, sizeof(char), "eq1" }, *eq1 = &eq1def;
                datumdef eq2def =
                  { NULL, 0, STRMAX, sizeof(char), "eq2" }, *eq2 = &eq2def;
                grg_strcpy(eq1,name->value);
                n = grg_collect_index(e,eq2,++n);
                if (gppeqtrace) printf("[%s]\n",eq2->value);
                grg_equate(eq2,db,rc,rv);
                grg_pop(larg,&larg_type);
                if (larg_type != EQ_NUM)
                  gppeqerror("numeric operand expected");
                rc = atoi(larg->value);
                grg_strcpy(name,eq1->value);
                grg_dealloc(eq1);
                grg_dealloc(eq2);
              }
              else if (gppeqtrace) printf("\n");
              if (!db) gppeqerror("no database defined");
              if (rc < 1 || rc > db->dhead->db_nrecs)
                gppeqerror("record index out of range");
              if (dbGetf(db,name->value,rc,&value) != 0)
                gpperror("unknown field name",name->value);
              grg_strcpy(larg,value);
              strip(larg->value);
              if (gppeqtrace)
                printf("*GETFIELD{%s} -> {%s}\n",name->value,larg->value);
              for (ndx = 0; strcmp((db->dfrec+ndx)->fl_fname,name->value) != 0;
                 ndx++);
              switch ((db->dfrec+ndx)->fl_ftype) {
                case 'C': larg_type = EQ_STR; break;
                case 'N': larg_type = EQ_NUM; break;
                case 'D': larg_type = EQ_DATE; break;
                case 'L': larg_type = EQ_BOOL; break;
                default:
                  grg_sprintf(data,"%c",(db->dfrec+ndx)->fl_ftype);
                  gpperror("unknown field type",data->value);
                  break;
              }
              grg_push(larg,larg_type);
              db = old_db;
              rc = old_rc;
            }
            else gppeqerror("field right operand expected");
            break;
        }
        break;
      case '?': /* conditional operator (unary) */
        if (gppeqtrace) printf("?\n");
        if (gppisvarchar(e->value[n+1],EQF)) {
          n = grg_collect_name(e,name,++n);
          if (gppeqtrace) printf("GetF: %s=",name->value);
          if (!db) gppeqerror("no database defined");
          if (dbGetf(db,name->value,rc,&value) != 0)
            gpperror("unknown field name",name->value);
          grg_strcpy(larg,value);
          strip(larg->value);
          if (gppeqtrace) printf("[%s]\n",larg->value);
          for (ndx = 0; strcmp((db->dfrec+ndx)->fl_fname,name->value) != 0;
            ndx++);
          switch ((db->dfrec+ndx)->fl_ftype) {
            case 'C': larg_type = EQ_STR; break;
            case 'N': larg_type = EQ_NUM; break;
            case 'D': larg_type = EQ_DATE; break;
            case 'L': larg_type = EQ_BOOL; break;
            default:
              grg_sprintf(data,"%c",(db->dfrec+ndx)->fl_ftype);
              gpperror("unknown field type",data->value);
              break;
          }
        }
        else grg_pop(larg,&larg_type);
        eqres = 0;
        if ((larg_type == EQ_STR || larg_type == EQ_DATE)
          && larg->value[0] != '\0') eqres = 1;
        else if (larg_type == EQ_NUM && atoi(larg->value) != 0) eqres = 1;
        else if (larg_type == EQ_DEC && atof(larg->value) != 0.0) eqres = 1;
        else if (larg_type != EQ_STR && larg_type != EQ_DATE &&
          larg_type != EQ_NUM && larg_type != EQ_DEC)
            gppeqerror("numeric or string operand expected");
        if (name->value[0] != '\0') grg_strcpy(data,larg->value);
        {
          datumdef eq1def =
            { NULL, 0, STRMAX, sizeof(char), "eq1" }, *eq1 = &eq1def;
          datumdef eq2def =
            { NULL, 0, STRMAX, sizeof(char), "eq2" }, *eq2 = &eq2def;
          depth = 1;
          n = grg_collect_condition(e,eq1,eq2,++n,&depth,0);
          if (name->value[0] != '\0') {  
            if (larg->value[0] == '\0' && eqres == 1) grg_push(data,larg_type);
            else if (eqres == 1) grg_equate(eq1,db,rc,rv);
            else grg_equate(eq2,db,rc,rv);
          }
          else {
            if (eqres == 1) grg_equate(eq1,db,rc,rv);
            else grg_equate(eq2,db,rc,rv);
          }
          grg_dealloc(eq1);
          grg_dealloc(eq2);
        }
        break;
      case '$': /* expand or evaluate operator */
        switch (e->value[n+1]) {
          case '$': /* expand operator (unary) */
            if (gppeqtrace) printf("$$\n");
            n++;
            {
              datumdef bodydef =
                { NULL, 0, STRMAX, sizeof(char), "body" }, *body = &bodydef;
              datumdef donedef =
                { NULL, 0, STRMAX, sizeof(char), "done" }, *done = &donedef;
              if (gppisvarchar(e->value[n+1],EQF)) {
                n = grg_collect_name(e,name,++n);
                grg_getvar(name->value,body,&larg_type);
              }
              else grg_pop(body,&larg_type);
              grg_expand(body,db,rc,done);
              grg_push(done,EQ_STR);
              grg_dealloc(done);
              grg_dealloc(body);
            }
            break;
          default: /* evaluate operator (unary) */
            if (gppeqtrace) printf("$\n");
            {
              datumdef eqdef =
                { NULL, 0, STRMAX, sizeof(char), "eq" }, *eq = &eqdef;
              if (gppisvarchar(e->value[n+1],EQF)) {
                n = grg_collect_name(e,name,++n);
                grg_getvar(name->value,eq,&larg_type);
              }
              else grg_pop(eq,&larg_type);
              if (rv) {
                grg_reverse(eq,data);
                grg_strcpy(eq,data->value);
                grg_equate(eq,db,rc,rv);
              }
              else grg_equate(eq,db,rc,rv);
              grg_dealloc(eq);
            }
            break;
        }
        break;
      case '+': /* add operator, or increment operator */
        /* Need a case for EQ_DATE */
        switch (e->value[n+1]) {
          case '+': /* increment operator (unary) */
            if (gppeqtrace) printf("++\n");
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != EQ_NUM && larg_type != EQ_DEC)
              gppeqerror("numeric operand expected");
            if (larg_type == EQ_NUM)
              sprintf(data->value,"%d",(int)(atoi(larg->value)+1));
            else sprintf(data->value,"%f",(float)atof(larg->value)+1);
            grg_push(data,larg_type);
            break;
          default: /* add operator (binary) */
            if (gppeqtrace) printf("+\n");
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type == EQ_NUM && rarg_type == EQ_NUM)
              sprintf(data->value,"%d",
                      (int)(atoi(larg->value)+atoi(rarg->value)));
            else if (larg_type == EQ_NUM && rarg_type == EQ_DEC) {
              sprintf(data->value,"%f",
                      (float)atoi(larg->value)+atof(rarg->value));
              larg_type = EQ_DEC;
            }
            else if (larg_type == EQ_DEC && rarg_type == EQ_NUM) {
              sprintf(data->value,"%f",
                      (float)(atof(larg->value)+atoi(rarg->value)));
              larg_type = EQ_DEC;
            }
            else if (larg_type == EQ_DEC && rarg_type == EQ_DEC) {
              sprintf(data->value,"%f",
                      (float)(atof(larg->value)+atof(rarg->value)));
              larg_type = EQ_DEC;
            }
            else {
              grg_sprintf(data,"%@%@",larg,rarg);
              larg_type = EQ_STR;
            }
            grg_push(data,larg_type);
            break;
        }
        break;
      case '-': /* subtract operator, or decrement operator */
        /* Need a case for EQ_DATE */
        switch (e->value[n+1]) {
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9': /* unary minus */
            n = grg_collect_number(e,larg,++n,&larg_type);
            grg_push(larg,larg_type);
            break;
          case '-': /* decrement operator (unary) */
            if (gppeqtrace) printf("--\n");
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != EQ_NUM && larg_type != EQ_DEC)
              gppeqerror("numeric operand expected");
            if (larg_type == EQ_NUM)
              sprintf(data->value,"%d",(int)(atoi(larg->value)-1));
            else sprintf(data->value,"%f",(float)atof(larg->value)-1);
            grg_push(data,larg_type);
            break;
          default: /* subtract operator (binary) */
            if (gppeqtrace) printf("-\n");
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type == EQ_NUM && rarg_type == EQ_NUM)
              sprintf(data->value,"%d",
                      (int)(atoi(larg->value)-atoi(rarg->value)));
            else if (larg_type == EQ_NUM && rarg_type == EQ_DEC) {
              sprintf(data->value,"%f",
                      (float)atoi(larg->value)-atof(rarg->value));
              larg_type = EQ_DEC;
            }
            else if (larg_type == EQ_DEC && rarg_type == EQ_NUM) {
              sprintf(data->value,"%f",
                      (float)(atof(larg->value)-atoi(rarg->value)));
              larg_type = EQ_DEC;
            }
            else if (larg_type == EQ_DEC && rarg_type == EQ_DEC) {
              sprintf(data->value,"%f",
                      (float)(atof(larg->value)-atof(rarg->value)));
              larg_type = EQ_DEC;
            }
            else gppeqerror("numeric operands expected");
            grg_push(data,larg_type);
            break;
        }
        break;
      case '*': /* multiply operator (binary) */
        if (gppeqtrace) printf("*\n");
        grg_pop(rarg,&rarg_type);
        if (gppisvarchar(e->value[n+1],EQF)) {
          n = grg_collect_name(e,name,++n);
          grg_getvar(name->value,larg,&larg_type);
        }
        else grg_pop(larg,&larg_type);
        if (larg_type == EQ_NUM && rarg_type == EQ_NUM)
          sprintf(data->value,"%d",
                  (int)(atoi(larg->value)*atoi(rarg->value)));
        else if (larg_type == EQ_NUM && rarg_type == EQ_DEC) {
          sprintf(data->value,"%f",
                  (float)atoi(larg->value)*atof(rarg->value));
          larg_type = EQ_DEC;
        }
        else if (larg_type == EQ_DEC && rarg_type == EQ_NUM) {
          sprintf(data->value,"%f",
                  (float)(atof(larg->value)*atoi(rarg->value)));
          larg_type = EQ_DEC;
        }
        else if (larg_type == EQ_DEC && rarg_type == EQ_DEC) {
          sprintf(data->value,"%f",
                  (float)(atof(larg->value)*atof(rarg->value)));
          larg_type = EQ_DEC;
        }
        else gppeqerror("numeric operands expected");
        grg_push(data,larg_type);
        break;
      case '/': /* divide operator (binary) */
        if (gppeqtrace) printf("/\n");
        grg_pop(rarg,&rarg_type);
        if (gppisvarchar(e->value[n+1],EQF)) {
          n = grg_collect_name(e,name,++n);
          grg_getvar(name->value,larg,&larg_type);
        }
        else grg_pop(larg,&larg_type);
        if (atoi(rarg->value) == 0) gppeqerror("division by zero");
        if (larg_type == EQ_NUM && rarg_type == EQ_NUM)
          sprintf(data->value,"%d",
                  (int)(atoi(larg->value)/atoi(rarg->value)));
        else if (larg_type == EQ_NUM && rarg_type == EQ_DEC) {
          sprintf(data->value,"%f",
                  (float)atoi(larg->value)/atof(rarg->value));
          larg_type = EQ_DEC;
        }
        else if (larg_type == EQ_DEC && rarg_type == EQ_NUM) {
          sprintf(data->value,"%f",
                  (float)(atof(larg->value)/atoi(rarg->value)));
          larg_type = EQ_DEC;
        }
        else if (larg_type == EQ_DEC && rarg_type == EQ_DEC) {
          sprintf(data->value,"%f",
                  (float)(atof(larg->value)/atof(rarg->value)));
          larg_type = EQ_DEC;
        }
        else gppeqerror("numeric operands expected");
        grg_push(data,larg_type);
        break;
      case '>': /* greater than, greater than or equals, or pushvar operator */
        switch (e->value[n+1]) {
          case '=': /* greater than or equals operator (binary) */
            if (gppeqtrace) printf(">=\n");
            n++;
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            switch (larg_type) {
              case EQ_NUM:
                sprintf(data->value,"%d",
                        (atoi(larg->value)>=atoi(rarg->value))?1:0);
                break;
              case EQ_DEC:
                sprintf(data->value,"%d",
                        (atof(larg->value)>=atof(rarg->value))?1:0);
                break;
              case EQ_STR: case EQ_FLD:
                sprintf(data->value,"%d",
                        (strcmp(larg->value,rarg->value)>=0)?1:0);
                break;
              case EQ_DATE:
                sprintf(data->value,"%d",
                        (datcmp(larg->value,rarg->value)>=0)?1:0);
                break;
              default:
                gppeqerror("invalid operand type",NULL);
                break;
            }
            grg_push(data,EQ_NUM);
            break;
          case '>': /* popvar operator (unary) */
            if (gppeqtrace) printf(">>");
            n++;
            if (e->value[n+1] == '\'') { /* rhs put index operator */
              if (gppeqtrace) printf("'\n");
              n++;
              if (gppisvarchar(e->value[n+1],EQF)) {
                n = grg_collect_name(e,name,++n);
                grg_getvar(name->value,larg,&larg_type);
                grg_pop(data,&data_type);
                grg_pop(rarg,&rarg_type);
                if (rarg_type != EQ_NUM || data_type != EQ_NUM)
                  gppeqerror("numeric operand expected");
                if (atoi(rarg->value) < 0) gppeqerror("index underflow");
                if (atoi(rarg->value) >= larg->size)
                  grg_alloc(larg,rarg->value);
                larg->value[atoi(rarg->value)] = atoi(data->value);
                grg_putvar(name->value,larg,larg_type);
              }
              else gppeqerror("field right operand expected");
            }
            else if (gppisvarchar(e->value[n+1],EQF)) {
              if (gppeqtrace) printf("\n");
              n = grg_collect_name(e,name,++n);
              grg_pop(larg,&larg_type);
              grg_putvar(name->value,larg,larg_type);
            }
            else gppeqerror("field right operand expected");
            break;
          default: /* greater than operator (binary) */
            if (gppeqtrace) printf(">\n");
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            switch (larg_type) {
              case EQ_NUM:
                sprintf(data->value,"%d",
                        (atoi(larg->value)>atoi(rarg->value))?1:0);
                break;
              case EQ_DEC:
                sprintf(data->value,"%d",
                        (atof(larg->value)>atof(rarg->value))?1:0);
                break;
              case EQ_STR: case EQ_FLD:
                sprintf(data->value,"%d",
                        (strcmp(larg->value,rarg->value)>0)?1:0);
                break;
              case EQ_DATE:
                sprintf(data->value,"%d",
                        (datcmp(larg->value,rarg->value)>0)?1:0);
                break;
              default:
                gppeqerror("invalid operand type",NULL);
                break;
            }
            grg_push(data,EQ_NUM);
            break;
        }
        break;
      case '<': /* less than, less than equals, not equals or popvar operator */
        switch (e->value[n+1]) {
          case '=': /* greater than or equals operator (binary) */
            if (gppeqtrace) printf("<=\n");
            n++;
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            switch (larg_type) {
              case EQ_NUM:
                sprintf(data->value,"%d",
                        (atoi(larg->value)<=atoi(rarg->value))?1:0);
                break;
              case EQ_DEC:
                sprintf(data->value,"%d",
                        (atof(larg->value)<=atof(rarg->value))?1:0);
                break;
              case EQ_STR: case EQ_FLD:
                sprintf(data->value,"%d",
                        (strcmp(larg->value,rarg->value)<=0)?1:0);
                break;
              case EQ_DATE:
                sprintf(data->value,"%d",
                        (datcmp(larg->value,rarg->value)<=0)?1:0);
                break;
              default:
                gppeqerror("invalid operand type",NULL);
                break;
            }
            grg_push(data,EQ_NUM);
            break;
          case '>': /* not equals operator (binary) */
            if (gppeqtrace) printf("<>\n");
            n++;
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            switch (larg_type) {
              case EQ_NUM:
                sprintf(data->value,"%d",
                        (atoi(larg->value)==atoi(rarg->value))?0:1);
                break;
              case EQ_DEC:
                sprintf(data->value,"%d",
                        (atof(larg->value)==atof(rarg->value))?0:1);
                break;
              case EQ_STR: case EQ_FLD:
                sprintf(data->value,"%d",strcmp(larg->value,rarg->value)?1:0);
                break;
              case EQ_DATE:
                sprintf(data->value,"%d",datcmp(larg->value,rarg->value)?1:0);
                break;
              default:
                gppeqerror("invalid operand type",NULL);
                break;
            }
            grg_push(data,EQ_NUM);
            break;
          case '<': /* pushvar operator (unary) */
            if (gppeqtrace) printf("<<\n");
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
              grg_push(larg,larg_type);
            }
            else gppeqerror("field right operand expected");
            break;
          default: /* less than operator (binary) */
            if (gppeqtrace) printf("<\n");
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            switch (larg_type) {
              case EQ_NUM:
                sprintf(data->value,"%d",
                        (atoi(larg->value)<atoi(rarg->value))?1:0);
                break;
              case EQ_DEC:
                sprintf(data->value,"%d",
                        (atof(larg->value)<atof(rarg->value))?1:0);
                break;
              case EQ_STR: case EQ_FLD:
                sprintf(data->value,"%d",
                        (strcmp(larg->value,rarg->value)<0)?1:0);
                break;
              case EQ_DATE:
                sprintf(data->value,"%d",
                        (datcmp(larg->value,rarg->value)<0)?1:0);
                break;
              default:
                gppeqerror("invalid operand type",NULL);
                break;
            }
            grg_push(data,EQ_NUM);
            break;
        }
        break;
      case '=': /* equals operator (binary) */
        if (gppeqtrace) printf("=\n");
        grg_pop(rarg,&rarg_type);
        if (gppisvarchar(e->value[n+1],EQF)) {
          n = grg_collect_name(e,name,++n);
          grg_getvar(name->value,larg,&larg_type);
        }
        else grg_pop(larg,&larg_type);
        if (larg_type != rarg_type) gppeqerror("operands types differ");
        switch (larg_type) {
          case EQ_NUM:
            sprintf(data->value,"%d",
                    (atoi(larg->value)==atoi(rarg->value))?1:0);
            break;
          case EQ_DEC:
            sprintf(data->value,"%d",
                    (atof(larg->value)==atof(rarg->value))?1:0);
            break;
          case EQ_STR: case EQ_FLD:
            sprintf(data->value,"%d",strcmp(larg->value,rarg->value)?0:1);
            break;
          case EQ_DATE:
            sprintf(data->value,"%d",datcmp(larg->value,rarg->value)?0:1);
            break;
          default:
            gppeqerror("invalid operand type",NULL);
            break;
        }
        grg_push(data,EQ_NUM);
        break;
      case '~': /* logical negation, or one's complement  operator */
        switch (e->value[n+1]) {
          case '~': /* logical negation operator (unary) */
            if (gppeqtrace) printf("~~\n");
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != EQ_NUM) gppeqerror("numeric operand expected");
            sprintf(data->value,"%d",atoi(larg->value)?0:1);
            grg_push(data,larg_type);
            break;
          default: /* bitwise one's complement operator (unary) */
            if (gppeqtrace) printf("~\n");
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != EQ_NUM) gppeqerror("numeric operand expected");
            sprintf(data->value,"%d",~atoi(larg->value));
            grg_push(data,larg_type);
            break;
        }
        break;
      case '&': /* logical and, or bitwise and operator */
        switch (e->value[n+1]) {
          case '&': /* logical and operator (binary) */
            if (gppeqtrace) printf("&&\n");
            n++;
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            if (larg_type != EQ_NUM) gppeqerror("numeric operand expected");
              if (atoi(larg->value) && atoi(rarg->value))
                sprintf(data->value,"%d",1);
            else sprintf(data->value,"%d",0);
            grg_push(data,larg_type);
            break;
          default: /* bitwise and operator (binary) */
            if (gppeqtrace) printf("&\n");
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            if (larg_type != EQ_NUM) gppeqerror("numeric operand expected");
            sprintf(data->value,"%d",atoi(larg->value)&atoi(rarg->value));
            grg_push(data,larg_type);
            break;
        }
        break;
      case '|': /* logical or, or bitwise or operator */
        switch (e->value[n+1]) {
          case '|': /* logical or operator (binary) */
            if (gppeqtrace) printf("||\n");
            n++;
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            if (larg_type != EQ_NUM) gppeqerror("numeric operand expected");
            if (atoi(larg->value) || atoi(rarg->value))
              sprintf(data->value,"%d",1);
            else sprintf(data->value,"%d",0);
            grg_push(data,larg_type);
            break;
          default: /* bitwise or operator (binary) */
            if (gppeqtrace) printf("|\n");
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            if (larg_type != EQ_NUM) gppeqerror("numeric operand expected");
            sprintf(data->value,"%d",atoi(larg->value)|atoi(rarg->value));
            grg_push(data,larg_type);
            break;
        }
        break;
      case '^': /* logical exclusive or, or bitwise exclusive or operator */
        switch (e->value[n+1]) {
          case '^': /* logical exclusive or operator (binary) */
            if (gppeqtrace) printf("^^\n");
            n++;
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            if (larg_type != EQ_NUM) gppeqerror("numeric operand expected");
            if ((atoi(larg->value) && !atoi(rarg->value))
                || (!atoi(larg->value) && atoi(rarg->value)))
              sprintf(data->value,"%d",1);
            else sprintf(data->value,"%d",0);
            grg_push(data,larg_type);
            break;
          default: /* bitwise exclusive or operator (binary) */
            if (gppeqtrace) printf("^\n");
            grg_pop(rarg,&rarg_type);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            if (larg_type != rarg_type) gppeqerror("operands types differ");
            if (larg_type != EQ_NUM) gppeqerror("numeric operand expected");
            sprintf(data->value,"%d",atoi(larg->value)^atoi(rarg->value));
            grg_push(data,larg_type);
            break;
        }
        break;
      case '\'': /* get index operator (binary) */
        if (gppeqtrace) printf("'\n");
        grg_pop(rarg,&rarg_type);
        if (gppisvarchar(e->value[n+1],EQF)) {
          n = grg_collect_name(e,name,++n);
          grg_getvar(name->value,larg,&larg_type);
        }
        else grg_pop(larg,&larg_type);
        if (rarg_type != EQ_NUM) gppeqerror("numeric operand expected");
        if (atoi(rarg->value) < 0) gppeqerror("index underflow");
        if (atoi(rarg->value) >= larg->size) grg_alloc(larg,rarg->value);
        sprintf(data->value,"%d",larg->value[atoi(rarg->value)]);
        grg_push(data,EQ_NUM);
        break;
      case '`': /* put index operator (tertiary) */
        if (gppeqtrace) printf("`\n");
        grg_pop(rarg,&rarg_type);
        grg_pop(data,&data_type);
        if (gppisvarchar(e->value[n+1],EQF)) {
          n = grg_collect_name(e,name,++n);
          grg_getvar(name->value,larg,&larg_type);
        }
        else grg_pop(larg,&larg_type);
        if (rarg_type != EQ_NUM || data_type != EQ_NUM)
          gppeqerror("numeric operand expected");
        if (atoi(rarg->value) < 0) gppeqerror("index underflow");
        if (atoi(rarg->value) >= larg->size) grg_alloc(larg,rarg->value);
        larg->value[atoi(rarg->value)] = atoi(data->value);
        grg_push(larg,EQ_STR);
        break;
      case '.': /* raw read/write operators (unary) */
        switch (e->value[n+1]) {
          case '>': /* raw read operator */
            if (gppeqtrace) printf(".>\n");
            n++;
            fgets(data->value,STRMAX,stdin);
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_putvar(name->value,data,EQ_STR);
            }
            else grg_push(data,EQ_STR);
          case '.': /* raw file write operator */
            if (gppeqtrace) printf(".<\n");
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            fprintf(gpp_out,"%s",larg->value);
            fflush(gpp_out);
            break;
          case '<': /* raw write operator */
            if (gppeqtrace) printf(".<\n");
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            fprintf(stdout,"%s",larg->value);
            fflush(stdout);
            break;
          default: /* raw write operator (maintain compatibility) */
            if (gppeqtrace) printf(".\n");
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              grg_getvar(name->value,larg,&larg_type);
            }
            else grg_pop(larg,&larg_type);
            fprintf(stdout,"%s",larg->value);
            fflush(stdout);
            break;
        }
        break;
      case '!': /* pop top of stack operator (unary) */
        if (gppeqtrace) printf("!\n");
        grg_pop(larg,&larg_type);
        break;
      case '@': /* duplicate top of stack operator (unary) */
        if (gppeqtrace) printf("@\n");
        if (gppisvarchar(e->value[n+1],EQF)) {
          n = grg_collect_name(e,name,++n);
          grg_getvar(name->value,larg,&larg_type);
        }
        else grg_pop(larg,&larg_type);
        grg_push(larg,larg_type);
        grg_push(larg,larg_type);
        break;
      case '(': /* record loop separator */
        {
          datumdef eqdef =
            { NULL, 0, STRMAX, sizeof(char), "eq" }, *eq = &eqdef;
          n = grg_collect_old_roll_loop(e,eq,n);
          if (gppeqtrace) printf("( %s )",eq->value);
          old_db = db;
          if (e->value[n+1] == ':') {
            n++;
            if (gppisvarchar(e->value[n+1],EQF)) {
              n = grg_collect_name(e,name,++n);
              for (ndx = 0; ndx < texdbffile_ndx; ndx++) {
                if (strcasecmp(GRGDATA[ndx].name,name->value) == 0) {
                  db = GRGDATA[ndx].db;
                  break;
                }
              }
              if (ndx == texdbffile_ndx) gppeqerror("no such database");
              if (gppeqtrace) printf(":%s\n",name->value);
            }
            else gppeqerror("record loop right operand expected");
          }
          else if (gppeqtrace) printf("\n");
          if (!db) gppeqerror("no database defined");
          for (ndx = 1; ndx <= db->dhead->db_nrecs; ndx++)
            if (db != old_db || gppfilter(db,ndx)) {
              grg_equate(eq,db,ndx,rv);
              if (eqexit) return;
              else if (eqbreak) break;
            }
          db = old_db;
          grg_dealloc(eq);
        }
        break;
      case '[': /* while loop separator */
        {
          datumdef eq1def =
            { NULL, 0, STRMAX, sizeof(char), "eq1" }, *eq1 = &eq1def;
          datumdef eq2def =
            { NULL, 0, STRMAX, sizeof(char), "eq2" }, *eq2 = &eq2def;
          n = grg_collect_loop(e,eq1,eq2,n);
          if (gppeqtrace) printf("[ %s ; %s ]\n",eq1->value,eq2->value);
          grg_equate(eq1,db,rc,rv);
          grg_pop(data,&data_type);
          if (data_type != EQ_NUM) gppeqerror("numeric operand expected");
          while (atoi(data->value) != 0) {
            grg_equate(eq2,db,rc,rv);
            if (eqexit) return;
            else if (eqbreak) break;
            grg_equate(eq1,db,rc,rv);
            grg_pop(data,&data_type);
            if (data_type != EQ_NUM) gppeqerror("numeric operand expected");
          }
          grg_dealloc(eq1);
          grg_dealloc(eq2);
        }
        break;
      case '{': /* comment constant */
        n = grg_collect_comment(e,disc,n);
        break;
      case '\"': /* string constant */
        n = grg_collect_xstring(e,larg,n);
        grg_push(larg,EQ_STR);
        break;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': /* numeric constant */
        n = grg_collect_number(e,larg,n,&larg_type);
        grg_push(larg,larg_type);
        break;
      case ' ': case '\t': case ',': /* separation characters */
        break;
      case '\\': /* newline escape and separation character */
        if (e->value[n+1] == '\n') n++;
        /* break operator (no operands) */
        else if (e->value[n+1] == 'b') { n++; eqbreak = 1; return; }
        /* exit operator (no operands) */
        else if (e->value[n+1] == 'e') { n++; eqexit = 1; return; }
        break;
      default: /* illegal character or operator */
        sprintf(data->value,"%c",e->value[n]);
        gpperror("illegal character or operator in equate",data->value);
        break;
    }
  }
}

/*
 * GURGLE EQUATE HANDLING
 */

/*
 * Operator precedence and symbol mapping table
 * NB: The order in the table MUST match the order in the enumerate
 */

enum {
  opOR,
  opXOR,
  opAND,
  opBIT_OR,
  opBIT_XOR,
  opBIT_AND,
  opEQUALS,
  opNOT_EQUALS,
  opLESS_THAN,
  opLESS_THAN_EQUALS,
  opGREATER_THAN,
  opGREATER_THAN_EQUALS,
  opADD,
  opSUBTRACT,
  opMULTIPLY,
  opDIVIDE,
  opINCREMENT,
  opDECREMENT,
  opNOT,
  opBIT_NOT,
  opMINUS,
  opPLUS,
  opREAD,
  opWRITE,
  opOUTPUT,
  opEVALUATE,
  opPOP,
  opDUP,
  opCONSTANT,
  opVARIABLE,
  opLEFT_BRACKET,
  opRIGHT_BRACKET,
  opGETCHAR,
  opSETCHAR,
  opBREAK,
  opEXIT,
  opVALUEOF,
  opASSIGN,
  opEMPTY,
  opQCONSTANT,
  opEXPAND,
  opNEXT,
  opCONDITION,
  opINPUTS
};

static struct {
  short int token;
  short int ipf;
  short int spf;
  char symbol[4];
} operators[] = {
  opOR, 11, 12, "||", 
  opXOR, 13, 14, "^^", 
  opAND, 15, 16, "&&", 
  opBIT_OR, 21, 22, "|", 
  opBIT_XOR, 23, 24, "^", 
  opBIT_AND, 25, 26, "&", 
  opEQUALS, 31, 32, "=",
  opNOT_EQUALS, 31, 32, "<>",
  opLESS_THAN, 51, 52, "<",
  opLESS_THAN_EQUALS, 51, 52, "<=",
  opGREATER_THAN, 51, 52, ">",
  opGREATER_THAN_EQUALS, 51, 52, ">=",
  opADD, 101, 102, "+",
  opSUBTRACT, 101, 102, "-",
  opMULTIPLY, 111, 112, "*",
  opDIVIDE, 111, 112, "/",
  opINCREMENT, 152, 151, "++",
  opDECREMENT, 152, 151, "--",
  opNOT, 152, 151, "~~", 
  opBIT_NOT, 152, 151, "~", 
  opMINUS, 152, 151, "+",
  opPLUS, 152, 151, "-",
  opREAD, 151, 152, ".>",
  opWRITE, 700, 800, ".<",
  opOUTPUT, 700, 800, "..",
  opEVALUATE, 152, 151, "$",
  opPOP, 152, 152, "!",
  opDUP, 152, 151, "@",
  opCONSTANT, 700, 800, "",
  opVARIABLE, 700, 800, "",
  opLEFT_BRACKET, 1000, 0, "(",
  opRIGHT_BRACKET, 0, 0, ")",
  opGETCHAR, 152, 151, "'",
  opSETCHAR, 152, 151, "`",
  opBREAK, 700, 800, "\\b",
  opEXIT, 700, 800, "\\e",
  opVALUEOF, 2, 1, "<<",
  opASSIGN, 700, 800, ">>",
  opEMPTY, 502, 501, "\\",
  opQCONSTANT, 700, 800, "",
  opEXPAND, 152, 151, "$$"
};

#define RSTACKMAX   4

typedef struct {
  unsigned char operator;
  datumdef value;
} stack;

#define RSTACK ((stack *)rstack.value)

/*
 * Converts infix equate to suffix (reverse polish) equate
 */

void grg_reverse(i,o) datumdef *i, *o; {
datumdef namedef = { NULL, 0, STRMAX, sizeof(char), "name" }, *name = &namedef;
datumdef largdef = { NULL, 0, STRMAX, sizeof(char), "larg" }, *larg = &largdef;
datumdef cargdef = { NULL, 0, STRMAX, sizeof(char), "carg" }, *carg = &cargdef;
datumdef rargdef = { NULL, 0, STRMAX, sizeof(char), "rarg" }, *rarg = &rargdef;
datumdef valudef = { NULL, 0, STRMAX, sizeof(char), "valu" }, *valu = &valudef;
datumdef tvaldef = { NULL, 0, STRMAX, sizeof(char), "tval" }, *tval = &tvaldef;
short int ret, ctr = 0, right_paren, top = 0;
unsigned short int d;
unsigned char next, tmp, token;
int n;
datumdef rstack = { NULL, 0, RSTACKMAX, sizeof(stack), "rstack" };

  grg_alloc(&rstack,top);
  RSTACK[top].operator = opLEFT_BRACKET;
  RSTACK[top].value.block = STRMAX;
  RSTACK[top].value.bsize = sizeof(char);
  sprintf(RSTACK[top].value.name,"rstack.value.%d",top);
  grg_alloc_init(&RSTACK[top].value);

  (void) grg_sprintf(i,"%@%s",i,operators[opRIGHT_BRACKET].symbol);
  (void) grg_strcpy(o,"");
  for (n = 0; i->value[n] != '\0'; n++) {
    (void) grg_strcpy(valu,"");
    token = 0;
    switch (i->value[n]) {
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
      case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
      case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
      case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B':
      case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
      case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
      case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W':
      case 'X': case 'Y': case 'Z': case '_':
        n = grg_collect_name(i,name,n);
        if (i->value[n+1] == '(') {
          n = grg_collect_bracketed(i,larg,++n);
          grg_reverse(larg,carg);
          (void) grg_sprintf(name,"%@(%@)",name,carg);
        }
        (void) grg_sprintf(o,"%@\\ %@",o,name);
        next = opEMPTY;
        break;
      case '#':
        (void) grg_strncat(o,i->value+n,1);
        if (gppiswschar(i->value[n+1])) n = grg_collect_space(i,disc,++n);
        if (gppisvarchar(i->value[n+1],EQF)) {
          n = grg_collect_name(i,name,++n);
          if (i->value[n+1] == '(') {
            n = grg_collect_bracketed(i,larg,++n);
            grg_reverse(larg,carg);
            (void) grg_sprintf(name,"%@(%@)",name,carg);
          }
          (void) grg_strcat(o,name->value);
        }
        else gpperror("equate right operand expected",NULL);
        next = opEMPTY;
        break;
      case '%':
        (void) grg_sprintf(o,"%@\\%c",o,i->value[n]);
        switch (i->value[n+1]) {
          case '%': case '#': case '$':
            (void) grg_sprintf(o,"%@%c",o,i->value[++n]);
          default:
            if (gppisvarchar(i->value[n+1],EQF)) {
              n = grg_collect_name(i,name,++n);
              if (i->value[n+1] == '-' && i->value[n+2] == '>') {
                n += 2;
                if (gppisvarchar(i->value[n+1],EQF)) {
                  n = grg_collect_name(i,larg,++n);
                  (void) grg_sprintf(name,"%@->%@",name,larg);
                }
                else gpperror("field right operand expected",NULL);
              }
              if (i->value[n+1] == '[') {
                n = grg_collect_index(i,larg,++n);
                grg_reverse(larg,carg);
                (void) grg_sprintf(name,"%@[%@]",name,carg);
              }
              (void) grg_strcat(o,name->value);
            }
            else gpperror("field right operand expected",NULL);
            break;
        }
        next = opEMPTY;
        break;
      case '?':
        (void) grg_sprintf(tval,"%c",i->value[n]);
        if (gppisvarchar(i->value[n+1],EQF)) n = grg_collect_name(i,disc,++n);
        ret = 1;
        n = grg_collect_condition(i,larg,rarg,++n,&ret,1);
        ctr += ret;
        grg_reverse(larg,carg);
        (void) grg_sprintf(tval,"%@%@:",tval,carg);
        if (i->value[n] == ';') {
          grg_reverse(rarg,carg);
          (void) grg_sprintf(tval,"%@%@;",tval,carg);
          if (ctr > 0) for (; ctr > 0; ctr--) (void) grg_strcat(tval,";");
        }
        next = opRIGHT_BRACKET;
        token = opCONDITION;
        break;
      case '$':
        switch(i->value[n+1]) {
          case '$':
            n++;
            next = opEXPAND;
            break;
          default:
            next = opEVALUATE;
            break;
        }
        break;
      case '+':
        switch(i->value[n+1]) {
          case '+':
            n++;
            next = opINCREMENT;
            break;
          default:
            next = opADD;
            break;
        }
        break;
      case '-':
        if (RSTACK[top].operator != opLEFT_BRACKET && i->value[n+1] != '-') {
          next = opSUBTRACT;
        }
        else {
          switch (i->value[n+1]) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
              n = grg_collect_number(i,valu,n,&d);
              next = opCONSTANT;
              break;
            case '-':
              n++;
              next = opDECREMENT;
              break;
            default:
              next = opSUBTRACT;
              break;
          }
        }
        break;
      case '*':
        next = opMULTIPLY;
        break;
      case '/':
        next = opDIVIDE;
        break;
      case '>':
        switch(i->value[n+1]) {
          case '=':
            n++;
            next = opGREATER_THAN_EQUALS;
            break;
          case '>':
            n++;
            if (gppiswschar(i->value[n+1])) n = grg_collect_space(i,disc,++n);
            if (gppisvarchar(i->value[n+1],EQF)) {
              n = grg_collect_name(i,tval,++n);
            }
            else gpperror("field right operand expected",NULL);
            next = opRIGHT_BRACKET;
            token = opASSIGN;
            break;
          default:
            next = opGREATER_THAN;
            break;
        }
        break;
      case '<':
        switch(i->value[n+1]) {
          case '=':
            n++;
            next = opLESS_THAN_EQUALS;
            break;
          case '>':
            n++;
            next = opNOT_EQUALS;
            break;
          case '<':
            n++;
            if (gppiswschar(i->value[n+1])) n = grg_collect_space(i,disc,++n);
            if (gppisvarchar(i->value[n+1],EQF)) {
              n = grg_collect_name(i,tval,++n);
            }
            else gpperror("field right operand expected",NULL);
            next = opEMPTY;
            token = opVALUEOF;
            break;
          default:
            next = opLESS_THAN;
            break;
        }
        break;
      case '=':
        next = opEQUALS;
        break;
      case '~':
        switch (i->value[n+1]) {
          case '~':
            n++;
            next = opNOT;
            break;
          default:
            next = opBIT_NOT;
            break;
        }
        break;
      case '&':
        switch (i->value[n+1]) {
          case '&':
            n++;
            next = opAND;
            break;
          default:
            next = opBIT_AND;
            break;
        }
        break;
      case '|':
        switch (i->value[n+1]) {
          case '|':
            n++;
            next = opOR;
            break;
          default:
            next = opBIT_OR;
            break;
        }
        break;
      case '^':
        switch (i->value[n+1]) {
          case '^':
            n++;
            next = opXOR;
            break;
          default:
            next = opBIT_XOR;
            break;
        }
        break;
      case '\'':
        next = opGETCHAR;
        break;
      case '`':
        next = opSETCHAR;
        break;
      case '.':
        switch (i->value[n+1]) {
          case '>':
            n++;
            next = opREAD;
            break;
          case '.':
            n++;
            (void) grg_strcpy(tval,"..");
            next = opRIGHT_BRACKET;
            token = opOUTPUT;
            break;
          case '<':
            n++;
          default:
            grg_strcpy(tval,".<");
            next = opRIGHT_BRACKET;
            token = opWRITE;
            break;
        }
        break;
      case '!':
        next = opPOP;
        break;
      case '@':
        if (gppiswschar(i->value[n+1])) n = grg_collect_space(i,disc,++n);
        if (i->value[n+1] != '(') gpperror("inputs left bracket expected",NULL);
        n++;
        (void) grg_strcpy(tval,"");
        (void) grg_strcpy(larg,"");
        while (i->value[n] != ')' && i->value[n] != '\0') {
          if (gppisvarchar(i->value[n+1],EQF)) {
            n = grg_collect_name(i,name,++n);
            (void) grg_strcat(name,larg->value);
            (void) grg_sprintf(tval,">>%@",name);
            (void) grg_strcpy(larg,tval->value);
          }
          else if (gppiswschar(i->value[n+1]) || i->value[n+1] == ',' ||
                   i->value[n+1] == ')' || i->value[n+1] == '\0') n++;
          else gpperror("inputs right bracket expected",NULL);
        }
        if (i->value[n] != ')') gpperror("inputs right bracket expected",NULL);
        next = opRIGHT_BRACKET;
        token = opINPUTS;
        break;
      case '{':
        n = grg_collect_roll_loop(i,larg,n);
        grg_reverse(larg,carg);
        (void) grg_sprintf(o,"%@(%@)",o,carg);
        if (i->value[n+1] == ':') {
          n++;
          if (gppisvarchar(i->value[n+1],EQF)) {
            n = grg_collect_name(i,name,++n);
            (void) grg_sprintf(o,"%@:%@",o,name);
          }
          else gpperror("record loop right operand expected",NULL);
        }
        next = opEMPTY;
        break;
      case '[':
        n = grg_collect_loop(i,larg,rarg,n);
        grg_reverse(larg,carg);
        (void) grg_sprintf(o,"%@[%@;",o,carg);
        grg_reverse(rarg,carg);
        (void) grg_sprintf(o,"%@%@]",o,carg);
        next = opEMPTY;
        break;
      case '\"':
        n = grg_collect_string(i,valu,n);
        next = opQCONSTANT;
        break;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        n = grg_collect_number(i,valu,n,&d);
        next = opCONSTANT;
        break;
      case ',':
        next = opRIGHT_BRACKET;
        token = opNEXT;
        break;
      case ' ': case '\n': case '\t':
        next = opEMPTY;
        break;
      case '\\':
        switch (i->value[n+1]) {
          case '\n':
            next = opEMPTY;
            n++;
            break;
          case 'b':
            next = opRIGHT_BRACKET;
            token = opBREAK;
            (void) grg_strcpy(tval,"\\b");
            n++;
            break;
          case 'e':
            next = opRIGHT_BRACKET;
            token = opEXIT;
            (void) grg_strcpy(tval,"\\e");
            n++;
            break;
          default:
            next = opEMPTY;
            break;
        }
        break;
      case '(':
        next = opLEFT_BRACKET;
        break;
      case ')':
        next = opRIGHT_BRACKET;
        break;
      default:
        (void) grg_sprintf(temp,"%c",i->value[n]);
        gpperror("illegal character or operator in equate",temp->value);
        break;
    }
    if (next != opEMPTY) {
      right_paren = 0;
      while (top >= 0
             && operators[next].ipf <= operators[RSTACK[top].operator].spf) {
        tmp = RSTACK[top].operator;
        (void) grg_strcpy(valu,RSTACK[top].value.value);
        top -= 1;
        if (operators[next].ipf < operators[tmp].spf) {
          if (tmp == opCONSTANT)
            (void) grg_sprintf(o,"%@\\%@",o,valu);
          else if (tmp == opQCONSTANT)
            (void) grg_sprintf(o,"%@\\\"%@\"",o,valu);
          else if (tmp == opVARIABLE)
            (void) grg_sprintf(o,"%@\\<<%@",o,valu);
          else
            (void) grg_sprintf(o,"%@\\%s",o,operators[tmp].symbol);
        }
        else {
          right_paren = 1;
          break;
        }
      }
      if (!right_paren) {
        top += 1;
        grg_alloc(&rstack,top);
        RSTACK[top].operator = next;
        RSTACK[top].value.block = STRMAX;
        RSTACK[top].value.bsize = sizeof(char);
        sprintf(RSTACK[top].value.name,"rstack.value.%d",top);
        grg_alloc_init(&RSTACK[top].value);
        (void) grg_strcpy(&RSTACK[top].value,valu->value);
      }
    }
    if (token == opASSIGN) {
      (void) grg_sprintf(o,"%@>>%@",o,tval);
      top = 0;
      RSTACK[top].operator = opLEFT_BRACKET;
    }
    else if (token == opCONDITION) {
      (void) grg_sprintf(o,"%@%@",o,tval);
      top = 0;
      RSTACK[top].operator = opLEFT_BRACKET;
    }
    else if (token == opINPUTS || token == opBREAK || token == opEXIT
             || token == opOUTPUT || token == opWRITE) {
      (void) grg_sprintf(o,"%@%@",o,tval);
      top = 0;
      RSTACK[top].operator = opLEFT_BRACKET;
    }
    else if (token == opVALUEOF) {
      (void) grg_sprintf(o,"%@<<%@",o,tval);
    }
    else if (token == opNEXT) {
      top = 0;
      RSTACK[top].operator = opLEFT_BRACKET;
    }
  }
  grg_dealloc(name);
  grg_dealloc(larg);
  grg_dealloc(carg);
  grg_dealloc(rarg);
  grg_dealloc(valu);
  grg_dealloc(tval);
  for(top=0;top<rstack.size;top++) grg_dealloc(&RSTACK[top].value);
  grg_dealloc(&rstack);
}

/****************************************************************************/

/*
 * GURGLE TEXT BODY HANDLING
 */

/*
 * Line counter and page counters (for auto new page)
 */

unsigned short int gppline = 0;
unsigned short int gpppage = 0;
unsigned short int gpppage1 = 0;

/*
 * Process a text body and write to the output stream
 */

void grg_write(s,db,rc,file) datumdef *s; dbase *db; long rc; FILE *file; {
unsigned long int n, on;
unsigned short int ctr, hit;
char fieldname[16], d[12], *value;
  if (strcmp(getstrsysvar(_EQ_OUTFILE),gppoutput)) {
    fclose(gpp_out);
    strcpy(gppoutput,getstrsysvar(_EQ_OUTFILE));
    if (getsysvar(_EQ_VERBOSE))
      fprintf(stdout,"--> %s\n",gppoutput);
    if (strcmp(gppoutput,"-") == 0) file = gpp_out = stdout;
    else {
      if (texmkdir) makepathfor(gppoutput);
      if ((file = gpp_out = fopen(gppoutput,"w")) == NULL) {
        fprintf(stderr,"grg: couldn't open \"%s\": ",gppoutput);
        perror("");
        exit(1);
      }
    }
  }
  for (n = 0; s->value[n] != '\0'; n++) {
    if (s->value[n] == '%' && (n == 0 || s->value[n-1] != '\\')) {
      on = ++n;
      for (; (s->value[n] >= 'A' && s->value[n] <= 'Z')
           || (s->value[n] >= '0' && s->value[n] <= '9')
           || s->value[n] == '_'; n++);
      (void) strncpy(fieldname,s->value+on,n-on);
      fieldname[n-on] = '\0';
      if (dbGetf(db,fieldname,rc,&value) != 0) {
        s->value[n] = '\0';
        gpperror("unknown field name",fieldname);
      }
      else {
        strip(value);
        for (ctr = 0; strcmp((db->dfrec+ctr)->fl_fname,fieldname) != 0; ctr++);
        if ((db->dfrec+ctr)->fl_ftype == 'D' && strlen(value) != 0) {
          strcpy(d,value);
          sprintf(value,"%c%c/%c%c/%c%c%c%c",
            d[6],d[7],d[4],d[5],d[0],d[1],d[2],d[3]);
        }
        fputs(value,file);
        n--;
      }
    }
    else if (s->value[n] == EQUATEESC && (n == 0 || s->value[n-1] != '\\')) {
      if (gppisvarchar(s->value[n+1],EQF)) {
        n = grg_collect_name(s,name,++n);
        for (ctr = hit = 0; ctr < texblock_ndx && !hit; ctr++) {
          if (strcmp(GRGBLOCK[ctr].name,name->value) == 0) {
            putsysvar(_EQ_BLOCK,EQ_STR,GRGBLOCK[ctr].name);
            gppequate(EQ_PRETEXBLOCK,db,rc,0);    /* deprecated */
            gppequate(EQ_PRE_BLOCK,db,rc,0);
            if (GRGBLOCK[ctr].body.value[0] != '\0') {
              if (eqstack_ndx < 0) gpperror("stack underflow",NULL);
              else if (eqstack_ndx > 1) gpperror("stack overflow",NULL);
              if (eqstack_ndx == 0
                  || eqstack[--eqstack_ndx].data.value[0] == '1')
                grg_write(&GRGBLOCK[ctr].body,db,rc,file);
            }
            gppequate(EQ_PSTTEXBLOCK,db,rc,0);    /* deprecated */
            gppequate(EQ_POST_BLOCK,db,rc,0);
            hit = 1;
          }
        }
        if (!hit) {
          if (s->value[n+1] == '(') {
            n = grg_collect_bracketed(s,data,++n);
            grg_sprintf(temp,"%@(%@)",name,data);
          }
          else grg_strcpy(temp,name->value);
          gppequate(temp->value,db,rc,0);
          if (eqstack_ndx < 0) gpperror("stack underflow",NULL);
          else if (eqstack_ndx > 1) gpperror("stack overflow",NULL);
          else if (eqstack_ndx == 1) {
            datumdef wrdef =
              { NULL, 0, STRMAX, sizeof(char), "wr" }, *wr = &wrdef;
            grg_strcpy(wr,eqstack[--eqstack_ndx].data.value);
            grg_write(wr,db,rc,file);
            grg_dealloc(wr);
          }
        }
      }
      else gpperror("bad equate name (probably unescaped equate esc)",NULL);
    }
    else if (s->value[n]=='\\'
             && (s->value[n+1]=='\0' || s->value[n+1]==EQUATEESC)) {
      fputc(s->value[++n],file);
    }
    else if (texfescsub && s->value[n]=='\\' && (s->value[n+1]=='%')) {
      fputc(s->value[++n],file);
    }
    else if (s->value[n] == '\\' && s->value[n+1] == '\n') n++;
    else if (s->value[n] == '\n') {
      fputc(s->value[n],file);
      if (++gppline > texpage_max && texpage_max != 0) {
        if (gpppage1) gpppage1 = 0;
        else gpppage = 0;
        gppline = 0;
        fprintf(file,texnewpage);
        gppequate(EQ_PRETEXPAGENN,db,rc,0);   /* deprecated */
        gppequate(EQ_PRE_PAGENN,db,rc,0);
        if (texpagenn.value[0] != '\0') grg_write(&texpagenn,db,rc,file);
        gppequate(EQ_PSTTEXPAGENN,db,rc,0);   /* deprecated */
        gppequate(EQ_POST_PAGENN,db,rc,0);
      }
    }
    else {
      fputc(s->value[n],file);
    }
  }
}

/*
 * Process a text body and write output to a string
 */

void grg_expand(s,db,rc,os) datumdef *s; dbase *db; long rc; datumdef *os; {
unsigned long int n, on;
unsigned short int ctr, hit;
char fieldname[16], d[12], *value;
  for (n = 0; s->value[n] != '\0'; n++) {
    if (s->value[n] == '%' && (n == 0 || s->value[n-1] != '\\')
        && s->value[n+1] >= 'A' && s->value[n+1] <= 'Z' ) {
      on = ++n;
      for (; (s->value[n] >= 'A' && s->value[n] <= 'Z')
           || (s->value[n] >= '0' && s->value[n] <= '9')
           || s->value[n] == '_'; n++);
      (void) strncpy(fieldname,s->value+on,n-on);
      fieldname[n-on] = '\0';
      if (dbGetf(db,fieldname,rc,&value) != 0) {
        s->value[n] = '\0';
        gpperror("unknown field name",fieldname);
      }
      else {
        strip(value);
        for (ctr = 0; strcmp((db->dfrec+ctr)->fl_fname,fieldname) != 0; ctr++);
        if ((db->dfrec+ctr)->fl_ftype == 'D' && strlen(value) != 0) {
          strcpy(d,value);
          sprintf(value,"%c%c/%c%c/%c%c%c%c",
            d[6],d[7],d[4],d[5],d[0],d[1],d[2],d[3]);
        }
        grg_strcat(os,value);
        n--;
      }
    }
    else if (s->value[n] == EQUATEESC && (n == 0 || s->value[n-1] != '\\')
             && ( (s->value[n+1] >= 'A' && s->value[n+1] <= 'Z')
                  || (s->value[n+1] >= 'a' && s->value[n+1] <= 'z')
                  || s->value[n+1] == '_' ) ) {
      if (gppisvarchar(s->value[n+1],EQF)) {
        n = grg_collect_name(s,name,++n);
        for (ctr = hit = 0; ctr < texblock_ndx && !hit; ctr++) {
          if (strcmp(GRGBLOCK[ctr].name,name->value) == 0) {
            putsysvar(_EQ_BLOCK,EQ_STR,GRGBLOCK[ctr].name);
            gppequate(EQ_PRETEXBLOCK,db,rc,0);    /* deprecated */
            gppequate(EQ_PRE_BLOCK,db,rc,0);
            if (GRGBLOCK[ctr].body.value[0] != '\0') {
              if (eqstack_ndx < 0) gpperror("stack underflow",NULL);
              else if (eqstack_ndx > 1) gpperror("stack overflow",NULL);
              if (eqstack_ndx == 0
                  || eqstack[--eqstack_ndx].data.value[0] == '1')
                grg_expand(&GRGBLOCK[ctr].body,db,rc,os);
            }
            gppequate(EQ_PSTTEXBLOCK,db,rc,0);    /* deprecated */
            gppequate(EQ_POST_BLOCK,db,rc,0);
            hit = 1;
          }
        }
        if (!hit) {
          if (s->value[n+1] == '(') {
            n = grg_collect_bracketed(s,data,++n);
            grg_sprintf(temp,"%@(%@)",name,data);
          }
          else grg_strcpy(temp,name->value);
          gppequate(temp->value,db,rc,0);
          if (eqstack_ndx < 0) gpperror("stack underflow",NULL);
          else if (eqstack_ndx > 1) gpperror("stack overflow",NULL);
          else if (eqstack_ndx == 1) {
            datumdef wrdef =
              { NULL, 0, STRMAX, sizeof(char), "wr" }, *wr = &wrdef;
            grg_strcpy(wr,eqstack[--eqstack_ndx].data.value);
            grg_expand(wr,db,rc,os);
            grg_dealloc(wr);
          }
        }
      }
      else gpperror("bad equate name (probably unescaped equate esc)",NULL);
    }
    else if (s->value[n]=='\\'
             && (s->value[n+1]=='\0' || s->value[n+1]==EQUATEESC)) {
      grg_strncat(os,s->value+(++n),1);
    }
    else if (texfescsub && s->value[n]=='\\' && (s->value[n+1]=='%')) {
      grg_strncat(os,s->value+(++n),1);
    }
    else if (s->value[n] == '\\' && s->value[n+1] == '\n') n++;
    else {
      grg_strncat(os,s->value+n,1);
    }
  }
}

datumdef wrpdef = { NULL, 0, STRMAX, sizeof(char), "wrp" }, *wrp = &wrpdef;

void gppwrite(str,db,rc,file) char *str; dbase *db; long rc; FILE *file; {
  grg_strcpy(wrp,str);
  grg_write(wrp,db,rc,file);
}

/****************************************************************************/
