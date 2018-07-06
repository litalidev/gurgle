/* $Id: pattern.c,v 1.3 2001/04/27 10:38:10 timc Exp $
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
 * GURGLE PATTERN AND DYNAMIC DBF HANDLING
 */

#include <grg.h>

/*
 * Array of structures of pre-defined token to character mappings
 */

tokendef pattern[TEXMAXPATTOK] = {
  "<nul>", 0, '\x00', "",
  "<sot>", 0, '\x01', "",
  "<eot>", 0, '\x02', "",
  "<sol>", 0, '\x02', "",
  "<eol>", 0, '\x03', "",
  "<spc>", 0, 0, " ",
  "<tab>", 0, 0, "\t",
  "<wht>", 0, 0, " \t",
  "<new>", 0, 0, "\n",
  "<car>", 0, 0, "\r",
  "<del>", 0, 0, " \t",
  "<abc>", 0, 0, "abcdefghijklmnopqrstuvwxyz",
  "<ABC>", 0, 0, "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
  "<dec>", 0, 0, "0123456789",
  "<hex>", 0, 0, "0123456789abcdef",
  "<HEX>", 0, 0, "0123456789ABCDEF",
  "<sym>", 0, 0, "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~",
  "<any>", 0, 0, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t",
  "<all>", 0, 0, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r",
  "<bar>", 0, 0, "|",
  "<not>", 0, 0, "!",
  "<mul>", 0, 0, "*",
  "<lbr>", 0, 0, "(",
  "<rbr>", 0, 0, ")",
};

/*
 * Temporary array and index used to build unique mode identifiers
 */

struct {
  char mode_nm[TEXTEXTPATMAX];
  short int mode_id;
} modes[TEXMAXUNQMODES];

short int modes_ndx = 0;

/*
 * Temporary array and index used to build unique character identifiers
 */

struct {
  unsigned char char_nm[6];
  short int char_id;
  short int char_md[TEXMAXUNQMODES];
} chars[TEXMAXUNQCHARS][TEXMAXUNQPOSTS];

short int chars_ndx[TEXMAXUNQPOSTS];

/*
 * Structures and arrays representing the DFA
 */

typedef struct {
  int chr_n;
  int *chr_a;
} chr_el;

typedef struct {
  int *mid;
  int ndx;
  int max;
} mid_el;

typedef struct {
  int mode_id;
  int tk;
  char equ[STRMAX];
} end_el;

struct {
  int max;
  chr_el (*chr)[TEXMAXCHARSET];
  int *beg;
  mid_el mid[TEXMAXUNQPOSTS];
  end_el *end;
} dfa;

unsigned short int dfa_loaded = 0;

/****************************************************************************/

/*
 * GURGLE PATTERN HANDLING
 */

/*
 * Builds optimised pattern state array from pattern definitions, this is
 * then effectively a reduced Deterministic Finite Automata (DFA)
 */

void dfaLoad() {
short int i, j, k, l, m, n, o, p;
short int mode_id, char_id, post_id, stat_id, post_max;
unsigned char token[TEXMODEPATMAX];
char newtoken[TEXMODEPATMAX];
char tmptoken[TEXMODEPATMAX];
  mode_id = 1;
  char_id = 1;
  post_max = 0;
  for (i = 0; i < texpattern_ndx; i++) {
    /* BUILD UNIQUE MODES ARRAY FROM TEXPATTERNS */
    for (j = m = 0; j < strlen(texpattern[i].modep); j++) {
      for (k = 0; texpattern[i].modep[j+k] != '(' &&
        texpattern[i].modep[j+k] != ')' && texpattern[i].modep[j+k] != '|' &&
        texpattern[i].modep[j+k] != '\0'; k++)
        token[k] = texpattern[i].modep[j+k];
      token[k] = '\0';
      if (token[0] != '\0') {
        for (l = 0; l < modes_ndx; l++)
          if (strcmp(token,modes[l].mode_nm) == 0) break;
        if (l == modes_ndx) {
          if (modes_ndx == TEXMAXUNQMODES)
            gpperror("too many unique pattern modes",NULL);
          strcpy(modes[l].mode_nm,token);
          modes[l].mode_id = mode_id++;
          modes_ndx++;
        }
        texpattern[i].modes[m++] = modes[l].mode_id;
      }
      j += k;
    }
    for (j = 0; j < modes_ndx; j++)
      if (strcmp(texpattern[i].nmode,modes[j].mode_nm) == 0) break;
    if (j == modes_ndx) {
      if (modes_ndx == TEXMAXUNQMODES)
        gpperror("too many unique pattern modes",NULL);
      strcpy(modes[j].mode_nm,texpattern[i].nmode);
      modes[j].mode_id = mode_id++;
      modes_ndx++;
    }
    /* BUILD UNIQUE CHARACTERS ARRAY FROM TEXPATTERNS */
    post_id = 0;
    newtoken[0] = '\0';
    for (j = 0; j < strlen(texpattern[i].textp); j++) {
      switch (texpattern[i].textp[j]) {
        case '(':
        case ')':
        case '|':
          token[0] = '\0';
          break;
        case '<':
          if ((j+4) < TEXTEXTPATMAX && texpattern[i].textp[j+4] == '>') {
            strncpy(token,texpattern[i].textp+j,5);
            token[5] = '\0';
            j += 4;
          }
          else gpperror("left token without right in pattern",NULL);
          break;
        case '>':
          gpperror("right token without left in pattern",NULL);
          break;
        case '\\':
          j++;
          if (j >= strlen(texpattern[i].textp))
            gpperror("premature end of pattern after escape sequence",NULL);
        default:
          token[0] = texpattern[i].textp[j];
          token[1] = '\0';
          break;
      }
      if (token[0] != '\0') {
        for (k = 0; k < chars_ndx[post_id]; k++)
          if (strcmp(token,chars[k][post_id].char_nm) == 0) break;
        if (post_id == 0 && k != chars_ndx[post_id]) {
          for (l = p = 0; texpattern[i].modes[l] != 0 && p == 0; l++) {
            for (m = 0; chars[k][post_id].char_md[m] != 0; m++) {
              if (texpattern[i].modes[l] == chars[k][post_id].char_md[m]) {
                k = chars_ndx[post_id];
                for (n = o = 0; n < chars_ndx[post_id]; n++)
                  if (strcmp(token,chars[n][post_id].char_nm+1) == 0) o++;
                sprintf(tmptoken,"%c%s",0x80+o,token);
                strcpy(token,tmptoken);
                p = 1;
                break;
              }
            }
          }
        }
        if (k == chars_ndx[post_id]) {
          if (chars_ndx[post_id] == TEXMAXUNQCHARS)
            gpperror("too many unique pattern characters",NULL);
          strncpy(chars[k][post_id].char_nm,token,5);
          chars[k][post_id].char_id = ++chars_ndx[post_id];
          for (l = 0; texpattern[i].modes[l] != 0; l++) {
            for (m = 0; chars[k][post_id].char_md[m] != 0; m++)
              if (texpattern[i].modes[l] == chars[k][post_id].char_md[m]) break;
            if (chars[k][post_id].char_md[m] == 0)
              chars[k][post_id].char_md[m] = texpattern[i].modes[l];
          }
        }
        strcat(newtoken,token);
        post_id++;
      }
    }
    strcpy(texpattern[i].textp,newtoken);
    if (post_id > post_max) post_max = post_id;
  }
  dfa.max = modes_ndx;
  /* PRINT UNIQUE MODES ARRAY */
  if (dfadebug) {
    fprintf(stdout,"+DFA: Unique Modes Mappings\n");
    fprintf(stdout,"%-30s%-8s\n","Unique Mode","ID");
    for (i = 0; i < dfa.max; i++) {
      fprintf(stdout,"%-30s%-04d\n",modes[i].mode_nm,modes[i].mode_id);
    }
  }
  /* PRINT UNIQUE CHARACTERS ARRAY */
  if (dfadebug) {
    fprintf(stdout,"+DFA: Unique Characters Mappings\n");
    for (i = 0; i < post_max; i++) {
      fprintf(stdout,"%-10s%-20s%-8s%-30s\n",
        "Position","Unique Character","ID","Unique Modes List");
      for (j = 0; j < chars_ndx[i]; j++) {
        if (chars[j][i].char_nm[0] >= 0x80) {
          fprintf(stdout,"%-04d      (%02d)",i+1,chars[j][i].char_nm[0]-0x80);
          if (chars[j][i].char_nm[1] > 32 && chars[j][i].char_nm[1] < 127)
            fprintf(stdout,"%-16s%-04d    ",chars[j][i].char_nm+1,
              chars[j][i].char_id);
          else
            fprintf(stdout,"(%02x)%-12s%-04d    ",chars[j][i].char_nm[1],"",
              chars[j][i].char_id);
        }
        else {
          fprintf(stdout,"%-04d      ",i+1);
          if (chars[j][i].char_nm[0] > 32 && chars[j][i].char_nm[0] < 127)
            fprintf(stdout,"%-20s%-04d    ",chars[j][i].char_nm,
              chars[j][i].char_id);
          else
            fprintf(stdout,"(%02x)%-16s%-04d    ",chars[j][i].char_nm[0],"",
              chars[j][i].char_id);
        }
        for (k = 0; chars[j][i].char_md[k] != 0; k++)
          fprintf(stdout,"%-04d ",chars[j][i].char_md[k]);
        fprintf(stdout,"\n");
      }
    }
  }
  /* BUILD DFA CHARACTER MAPPING ARRAY FROM CHARACTERS AND PATTERNS ARRAYS */
  dfa.chr = (chr_el (*)[TEXMAXCHARSET]) calloc(post_max*TEXMAXCHARSET,sizeof(chr_el));
  if (!dfa.chr) gpperror("dfa.chr alloc failed",NULL);
  for (i = 0; i < post_max; i++) {
    for (j = 0; j < chars_ndx[i]; j++) {
      if (chars[j][i].char_nm[0] >= 0x80) strcpy(token,chars[j][i].char_nm+1);
      else strcpy(token,chars[j][i].char_nm);
      if (strlen(token) > 1 && token[0] == '<') {
        for (k = 0; k < sizeof(pattern)/sizeof(pattern[0]); k++)
          if (strcmp(pattern[k].token,token) == 0) break;
        if (k == sizeof(pattern)/sizeof(pattern[0]))
          gpperror("unknown character token",token);
        if (pattern[k].char_mcmap[0] != '\0') {
          for (l = 0; l < strlen(pattern[k].char_mcmap); l++) {
            if (dfa.chr[i][pattern[k].char_mcmap[l]].chr_n == 0) {
              dfa.chr[i][pattern[k].char_mcmap[l]].chr_a =
                (int *) malloc(sizeof(int));
            }
            else {
              dfa.chr[i][pattern[k].char_mcmap[l]].chr_a =
                (int *) realloc(dfa.chr[i][pattern[k].char_mcmap[l]].chr_a,
                  (dfa.chr[i][pattern[k].char_mcmap[l]].chr_n+1)*sizeof(int));
            }
            dfa.chr[i][pattern[k].char_mcmap[l]].chr_a[dfa.chr[i][pattern[k].char_mcmap[l]].chr_n++] = chars[j][i].char_id;
          }
        }
        else {
          if (dfa.chr[i][pattern[k].char_scmap].chr_n == 0) {
            dfa.chr[i][pattern[k].char_scmap].chr_a =
              (int *) malloc(sizeof(int));
          }
          else {
            dfa.chr[i][pattern[k].char_scmap].chr_a =
              (int *) realloc(dfa.chr[i][pattern[k].char_scmap].chr_a,
                (dfa.chr[i][pattern[k].char_scmap].chr_n+1)*sizeof(int));
          }
          dfa.chr[i][pattern[k].char_scmap].chr_a[dfa.chr[i][pattern[k].char_scmap].chr_n++] = chars[j][i].char_id;
        }
      }
      else {
        if (dfa.chr[i][token[0]].chr_n == 0) {
          dfa.chr[i][token[0]].chr_a =
            (int *) malloc(sizeof(int));
        }
        else {
          dfa.chr[i][token[0]].chr_a =
            (int *) realloc(dfa.chr[i][token[0]].chr_a,
              (dfa.chr[i][token[0]].chr_n+1)*sizeof(int));
        }
        dfa.chr[i][token[0]].chr_a[dfa.chr[i][token[0]].chr_n++] = chars[j][i].char_id;
      }
    }
  }
  /* PRINT DFA CHARACTER MAPPINGS ARRAY */
  if (dfadebug) {
    fprintf(stdout,"+DFA: Input Character Mappings\n");
    for (i = 0; i < post_max; i++) {
      fprintf(stdout,"%-10s%-20s%-40s\n",
        "Position","Character","Unique Character List");
      for (j = 0; j < TEXMAXCHARSET; j++) {
        if (dfa.chr[i][j].chr_n != 0) {
          if (j > 32 && j < 127) fprintf(stdout,"%-04d      %c%-19s",i+1,j,"");
          else fprintf(stdout,"%-04d      (%02x)%-16s",i+1,j,"");
          for (k = 0; k < dfa.chr[i][j].chr_n; k++)
            fprintf(stdout,"%-04d ",dfa.chr[i][j].chr_a[k]);
          fprintf(stdout,"\n");
        }
      }
    }
  }
  /* BUILD DFA STATE MAPPING ARRAYS FROM MODE AND CHAR MAPPINGS ARRAYS */
  if (dfadebug == 2) fprintf(stdout,"+DFA: State Maps Build Trace\n");
  stat_id = 1;
  dfa.beg = (int *) calloc((modes_ndx+1)*(chars_ndx[0]+1),sizeof(int));
  if (!dfa.beg) gpperror("dfa.beg alloc failed",NULL);
  for (i = 0; i < texpattern_ndx; i++) {
    if (dfadebug == 2) fprintf(stdout,"MODE = %s\n",texpattern[i].modep);
    for (j = 0; j < strlen(texpattern[i].modep); j++) {
      for (k = 0; texpattern[i].modep[j+k] != '(' &&
        texpattern[i].modep[j+k] != ')' && texpattern[i].modep[j+k] != '|' &&
        texpattern[i].modep[j+k] != '\0'; k++)
        token[k] = texpattern[i].modep[j+k];
      token[k] = '\0';
      if (dfadebug == 2) fprintf(stdout,"  TOKEN = %s\n",token);
      if (token[0] != '\0') {
        for (l = 0; strcmp(token,modes[l].mode_nm) != 0; l++);
        mode_id = modes[l].mode_id;
        post_id = 0;
        if (dfadebug == 2) fprintf(stdout,"  MODE_ID = %d, POST_ID = %d\n",mode_id,post_id);
        if (dfadebug == 2) fprintf(stdout,"    PATTERN = %s\n",texpattern[i].textp);
        for (m = 0; m < strlen(texpattern[i].textp); m++) {
          switch (texpattern[i].textp[m]) {
            case '(':
            case ')':
            case '|':
              token[0] = '\0';
              break;
            case '<':
              if ((m+4) < TEXTEXTPATMAX && texpattern[i].textp[m+4] == '>') {
                strncpy(token,texpattern[i].textp+m,5);
                token[5] = '\0';
                m += 4;
              }
              break;
            case '>':
              break;
            case '\\':
              m++;
            default:
              token[0] = texpattern[i].textp[m];
              token[1] = '\0';
              break;
          }
          if (token[0] >= 0x80 && texpattern[i].textp[m+1] == '<') {
            strncat(token,texpattern[i].textp+m+1,5);
            m += 5;
          }
          else if (token[0] >= 0x80) {
            token[1] = texpattern[i].textp[++m];
            token[2] = '\0';
          }
          if (token[0] >= 0x80)
            if (dfadebug == 2) fprintf(stdout,"      TOKEN = {%02d}%s, POST_ID = %d\n",
              token[0]-0x80,token+1,post_id);
          else if (token[0] != '\0')
            if (dfadebug == 2) fprintf(stdout,"      TOKEN = %s, POST_ID = %d\n",token,post_id);
          if (token[0] != '\0') {
            if (post_id == 0) {
              for (l = 0; strcmp(token,chars[l][post_id].char_nm) != 0; l++);
              char_id = chars[l][post_id].char_id;
              if (dfadebug == 2) fprintf(stdout,"      CHAR_ID = %d\n",char_id);
              if (*(dfa.beg+mode_id+(char_id*modes_ndx)) == 0)
                *(dfa.beg+mode_id+(char_id*modes_ndx)) = stat_id;
              if (dfadebug == 2) fprintf(stdout,"        BEG[%d][%d] = %d\n",mode_id,char_id,*(dfa.beg+mode_id+(char_id*modes_ndx)));
              /* last - won't neccessarily work */
              if ((m+1) == strlen(texpattern[i].textp)) {
                *(dfa.beg+mode_id+(char_id*modes_ndx)) =
                  *(dfa.beg+mode_id+(char_id*modes_ndx))
                  | (int)((post_id+1)<<12);
                  if (dfadebug == 2) fprintf(stdout,"        ADD[%d][%d] = %d\n",stat_id,char_id,*(dfa.beg+mode_id+(char_id*modes_ndx)));
              }
            }
            else {
              if (dfa.mid[post_id].mid == 0) {
                dfa.mid[post_id].mid = (int *)
                  calloc(TEXPATTERNMAX*(chars_ndx[post_id]+1),sizeof(int));
                if (!dfa.mid[post_id].mid)
                  gpperror("dfa.mid alloc failed",NULL);
              }
              dfa.mid[post_id].ndx = stat_id;
              for (l = 0; strcmp(token,chars[l][post_id].char_nm) != 0; l++);
              char_id = chars[l][post_id].char_id;
              if (dfadebug == 2) fprintf(stdout,"      CHAR_ID = %d\n",char_id);
              if (*(dfa.mid[post_id].mid+char_id+(stat_id*chars_ndx[post_id])) == 0)
                *(dfa.mid[post_id].mid+char_id+(stat_id*chars_ndx[post_id])) =
                  stat_id;
              if (dfadebug == 2) fprintf(stdout,"        MID[%d][%d] = %d\n",stat_id,char_id,*(dfa.mid[post_id].mid+char_id+(stat_id*chars_ndx[post_id])));
              /* last - won't neccessarily work */
              if ((m+1) == strlen(texpattern[i].textp)) {
                *(dfa.mid[post_id].mid+char_id+(stat_id*chars_ndx[post_id])) =
                *(dfa.mid[post_id].mid+char_id+(stat_id*chars_ndx[post_id]))
                | (int)((post_id+1)<<12);
                if (dfadebug == 2) fprintf(stdout,"        ADD[%d][%d] = %d\n",stat_id,char_id,*(dfa.mid[post_id].mid+char_id+(stat_id*chars_ndx[post_id])));
              }
            }
            if (post_id != 0) dfa.mid[post_id].max = chars_ndx[post_id];
            post_id++;
          }
        }
        if (dfa.end == 0) {
          dfa.end = (end_el *) calloc((stat_id+1),sizeof(end_el));
          if (!dfa.end) gpperror("dfa.end alloc failed",NULL);
        }
        else {
          dfa.end = (end_el *) realloc(dfa.end,(stat_id+1)*sizeof(end_el));
          if (!dfa.end) gpperror("dfa.end realloc failed",NULL);
          dfa.end[stat_id].mode_id = 0;
        }
        for (l = 0; strcmp(texpattern[i].nmode,modes[l].mode_nm) != 0; l++);
        mode_id = modes[l].mode_id;
        if (dfa.end[stat_id].mode_id == 0) {
          dfa.end[stat_id].mode_id = mode_id;
          dfa.end[stat_id].tk = texpattern[i].token;
          strcpy(dfa.end[stat_id].equ,texpattern[i].eqstr);
        }
        if (dfadebug == 2) fprintf(stdout,"        END[%d] = %d (%s), %s\n",
          stat_id, dfa.end[stat_id].mode_id, texpattern[i].nmode,
          dfa.end[stat_id].equ);
      }
      j += k;
    }
    stat_id++;
  }
  /* PRINT BEG STATE ARRAY */
  if (dfadebug) {
    fprintf(stdout,"+DFA: Begin State Machine Mapping\n");
    fprintf(stdout,"%s: %-04d\n%-18s","Position",1,"Unique Char/Mode");
    for (i = 0; i < modes_ndx; i++)
      fprintf(stdout,"%-10s  ",modes[i].mode_nm);
    fprintf(stdout,"\n");
    for (i = 0; i < chars_ndx[0]; i++) {
      if (chars[i][0].char_nm[0] >= 0x80) {
        fprintf(stdout,"(%02d)",chars[i][0].char_nm[0]-0x80);
        if (chars[i][0].char_nm[1] > 32 && chars[i][0].char_nm[1] < 127)
          fprintf(stdout,"%-14s",chars[i][0].char_nm+1);
        else fprintf(stdout,"(%02x)%-10s",chars[i][0].char_nm[1],"");
      }
      else {
        if (chars[i][0].char_nm[0] > 32 && chars[i][0].char_nm[0] < 127)
          fprintf(stdout,"%-18s",chars[i][0].char_nm);
        else fprintf(stdout,"(%02x)%-14s",chars[i][0].char_nm[0],"");
      }
      for (j = 0; j < modes_ndx; j++)
        fprintf(stdout,"%-04x        ",*(dfa.beg+(j+1)+((i+1)*modes_ndx)));
      fprintf(stdout,"\n");
    }
  }
  /* PRINT MID STATE ARRAYS */
  if (dfadebug) {
    fprintf(stdout,"+DFA: Middle State Machine Mapping\n");
    for (k = 1; k < post_max; k++) {
      fprintf(stdout,"%s: %-04d\n%-18s","Position",k,"Unique Char/Stat");
      for (i = 0; i < dfa.mid[k].ndx; i++) fprintf(stdout,"%-04x ",i+1);
      fprintf(stdout,"\n");
      for (i = 0; i < chars_ndx[k]; i++) {
        if (chars[i][k].char_nm[0] >= 0x80) {
          fprintf(stdout,"(%02d)",chars[i][k].char_nm[0]-0x80);
          if (chars[i][k].char_nm[1] > 32 && chars[i][k].char_nm[1] < 127)
            fprintf(stdout,"%-14s",chars[i][k].char_nm+1);
          else fprintf(stdout,"(%02x)%-10s",chars[i][k].char_nm[1],"");
        }
        else {
          if (chars[i][k].char_nm[0] > 32 && chars[i][k].char_nm[0] < 127)
            fprintf(stdout,"%-18s",chars[i][k].char_nm);
          else fprintf(stdout,"(%02x)%-14s",chars[i][k].char_nm[0],"");
        }
        for (j = 0; j < dfa.mid[k].ndx; j++)
          fprintf(stdout,"%-04x ",*(dfa.mid[k].mid+(i+1)+((j+1)*chars_ndx[k])));
        fprintf(stdout,"\n");
      }
    }
  }
  /* PRINT END STATE ARRAY */
  if (dfadebug) {
    fprintf(stdout,"+DFA: End State Machine Mapping\n");
    fprintf(stdout,"%-8s %-20s %-44s\n",
      "State","New Unique Mode","Equate");
    for (i = 1; i <= (stat_id-1); i++) {
      for (j = 0; modes[j].mode_id != dfa.end[i].mode_id; j++);
      fprintf(stdout,"%-04x     %-20s %-44s\n",
        i,modes[j].mode_nm,dfa.end[i].equ);
    }
  }
  dfa_loaded = 1;
}

/*
 * Sets the unique mode
 */

static int um;

void dfaMode(st) char *st; {
int i;
  for (i = 0; strcmp(st,modes[i].mode_nm) != 0 && i < modes_ndx; i++);
  if (i == modes_ndx) gpperror("unknown dfa mode",NULL);
  um = modes[i].mode_id;
}

/*
 * Each call returns a token from the input file using the prebuilt DFA
 */

void dfaunput(chr) char chr; {
  grg_alloc(&pbbuffer,pbbuffer_ndx);
  pbbuffer.value[pbbuffer_ndx++] = chr;
}

char dfainput(file) FILE *file; {
  if (pbbuffer_ndx > 0) return(pbbuffer.value[--pbbuffer_ndx]);
  else return(fgetc(file));
}

static int s[TEXDFAMAXSTATES];

char eq_pat[STRMAX*4];

int dfaNext(db) dbase *db; {
int c, n, m, t, i, p, j = 0, st, v, u, x, y;
  if ((eq_pat[j++] = c = dfainput(db->dfile)) == EOF) return(tkEOT);
  if ((m = n = dfa.chr[0][c].chr_n) == 0) {
    if ((m = n = dfa.chr[0][0].chr_n) == 0) return(tkNUL);
    else {
      c = 0;
      dfaunput(eq_pat[--j]);
    }
  }
  for (t = 0; --n >= 0;)
    if ((s[n] = *(dfa.beg+um+(dfa.chr[0][c].chr_a[n]*dfa.max))) == 0) t++;
  if (t == m) {
    if ((m = n = dfa.chr[0][0].chr_n) != 0) {
      c = 0;
      dfaunput(eq_pat[--j]);
      for (; --n >= 0;)
        s[n] = *(dfa.beg+um+(dfa.chr[0][c].chr_a[n]*dfa.max));
    }
  }
  for (n = 1, t = 0; t != m; n++) {
    x = y = 0;
    for (i = t = u = v = 0; i < m; i++) {
      if (s[i] != 0 && (s[i] & 0xF000) == 0) {
        if (u == 0) {
          eq_pat[j++] = c = dfainput(db->dfile);
          u = 1;
        }
        if (c == EOF) s[i] = 0;
        else if (dfa.chr[n][c].chr_n == 0) {
          if (dfa.chr[n][0].chr_n == 0) s[i] = 0;
          else {
            s[i] = *(dfa.mid[n].mid+dfa.chr[n][0].chr_a[0]+(s[i]*dfa.mid[n].max));
            x = 1;
          }
        }
        else {
          s[i] = *(dfa.mid[n].mid+dfa.chr[n][c].chr_a[0]+(s[i]*dfa.mid[n].max));
          if (s[i] == 0) y = 1;
        }
        v = i;
      }
      else t++;
    }
    if (x > y) dfaunput(eq_pat[--j]);
  }
  for (i = p = 0, t = -1; i < m; i++)
    if (s[i] != 0) {
      if (dfa.end[s[i]&0x0FFF].equ[0] != '\0') {
        st = eqstack_ndx;
        gppequate(dfa.end[s[i]&0x0FFF].equ,db,1,0);
        if (eqstack_ndx != st)
          if (eqstack_ndx < st) gpperror("stack underflow",NULL);
          else if (eqstack_ndx - st > 1) gpperror("stack overflow",NULL);
          else if (eqstack[--eqstack_ndx].data.value[0] != '1') s[i] = 0;
      }
      if ((s[i] & 0xF000) > p) {
        p = s[i] & 0xF000;
        t = i;
      }
    }
  if ((t == -1 || v != t) && j > 1)
    if (t != -1) for (j--; j >= (p>>12); j--) dfaunput(eq_pat[j]);
    else for (j--; j > (p>>12); j--) dfaunput(eq_pat[j]);
  if (t == -1) return(tkNUL);
  else {
    eq_pat[p>>12] = '\0';
    um = dfa.end[s[t]&0x0FFF].mode_id;
    return(dfa.end[s[t]&0x0FFF].tk);
  }
}

/****************************************************************************/

/*
 * GURGLE DYNAMIC DBF FILE HANDLING
 */

static char *DynamicDBF;
static long int ddbfp = 0;

/*
 * Open DBF file structures with initial default sizes which can grow
 */

int gppDynamicOpen(nm,da) char *nm; dbase **da; {
dbase *db;
  if (!(db = (dbase *) malloc(sizeof(dbase)))) return(db_MEM);
  if (strcmp(nm,"-") == 0) db->dfile = stdin;
  else if (!(db->dfile = fopen(nm,"rb"))) return(db_FOP);
  db->dname = nm;
  db->dnfld = 0;
  db->dnrec = 0;
  db->dtrec = 0;
  if (!(db->dhead = (db_head *) malloc(sizeof(db_head)))) return(db_MEM);
  db->dhead->db_nrecs = 0;
  if (!(db->dfrec = (db_field *) calloc(DYNFLDINI,sizeof(db_field))))
    return(db_MEM);
  *da = db;
  if (!(DynamicDBF = (char *) malloc(DYNRECINI*STRMAX*4))) return(db_MEM);
  ddbfp = 0;
  return(0);
}

/*
 * Fill, update, and grow DBF from input file accorrding to defined patterns
 */

int gppDynamicFill(db) dbase *db; {
int fldrac = DYNFLDINI, fld;
int recrac = DYNRECINI * STRMAX * 4;
int tok, i, j, ctr, chr;
char eq_fld[STRMAX*4], eq_pfn[11];
  if (!dfa_loaded) dfaLoad();
  dfaMode(texdfamode);
  putstrsysvar(_EQ_PFN,"");
  putnumsysvar(_EQ_PFT,0);
  putnumsysvar(_EQ_PFL,0);
  pbbuffer_ndx = 0;
  while ((tok = dfaNext(db)) != tkEOT) {
    switch(tok) {
      case tkSOR:
        fld = 0;
        break;
      case tkSOF:
        eq_fld[0] = '\0';
        break;
      case tkFLD:
        strcat(eq_fld,eq_pat);
        break;
      case tkEOF:
        if (fld == fldrac) {
          fldrac += DYNFLDINI;
          db->dfrec = (db_field *) realloc(db->dfrec,fldrac*sizeof(db_field));
          if (!db->dfrec) return(db_MEM);
          for (j = fldrac - DYNFLDINI; j < fldrac; j++) {
            db->dfrec[j].fl_fname[0] = '\0';
            db->dfrec[j].fl_ftype = '\0';
            db->dfrec[j].fl_foffs = 0;
            db->dfrec[j].fl_fsize = 0;
          }
        }
        if (db->dfrec[fld].fl_fsize == 0)
          if (getnumsysvar(_EQ_PFL) == 0)
            db->dfrec[fld].fl_fsize = strlen(eq_fld);
          else db->dfrec[fld].fl_fsize = getnumsysvar(_EQ_PFL);
        else
          if (getnumsysvar(_EQ_PFL) == 0)
            if (strlen(eq_fld) > db->dfrec[fld].fl_fsize)
              db->dfrec[fld].fl_fsize = strlen(eq_fld);
          else if (getnumsysvar(_EQ_PFL) > db->dfrec[fld].fl_fsize)
            db->dfrec[fld].fl_fsize = getnumsysvar(_EQ_PFL);
        if (db->dfrec[fld].fl_fname[0] == '\0') {
          if (strlen(getstrsysvar(_EQ_PFN)) > 10)
            gpperror("field name too long",NULL);
          strcpy(eq_pfn,getstrsysvar(_EQ_PFN));
          if (eq_pfn[0] != '\0') strcpy(db->dfrec[fld].fl_fname,eq_pfn);
          else {
            sprintf(eq_pfn,"U%03d",fld+1);
            strcpy(db->dfrec[fld].fl_fname,eq_pfn);
          }
        }
        if (db->dfrec[fld].fl_ftype == '\0')
          if (getnumsysvar(_EQ_PFT) == 0) db->dfrec[fld].fl_ftype = 'C';
          else db->dfrec[fld].fl_ftype = getnumsysvar(_EQ_PFT);
        switch (db->dfrec[fld].fl_ftype) {
          case 'C': gpppush(eq_fld,EQ_STR); break;
          case 'N': gpppush(eq_fld,EQ_NUM); break;
          case 'D': gpppush(eq_fld,EQ_DATE); break;
          case 'L': gpppush(eq_fld,EQ_BOOL); break;
          default:
            gpperror("unknown field type",NULL);
            break;
        }
        fld++;
        break;
      case tkEOR:
        db->dtrec++;
        DynamicDBF[ddbfp++] = tkSOR;
        for (ctr = 1; ctr <= eqstack_ndx; ctr++) {
          /* +4 allows for sof/eof and sor/eor (if this is last field) */
          if (strlen(eqstack[ctr-1].data.value)+ddbfp+4 > recrac) {
            recrac += DYNRECINI * STRMAX * 4;
            if (!(DynamicDBF = (char *) realloc(DynamicDBF,recrac)))
              return(db_MEM);
          }
          DynamicDBF[ddbfp++] = tkSOF;
          for (chr = 0; chr < strlen(eqstack[ctr-1].data.value); chr++)
            DynamicDBF[ddbfp++] = eqstack[ctr-1].data.value[chr];
          DynamicDBF[ddbfp++] = tkEOF;
        }
        DynamicDBF[ddbfp++] = tkEOR;
        if ((ctr - 1) > db->dnfld) db->dnfld = ctr - 1;
        eqstack_ndx = 0;
        break;
      case tkERR:
        gpperror("pattern parse error",NULL);
        break;
    }
  }
  return(0);
}

/*
 * Convert dynamic DBF file to static and complete loading DBF structures
 */

int gppDynamicStop(db) dbase *db; {
int ctr, ptr, rlen, flen, fld, rec;
char eq_pfn[11], value[STRMAX*4] = "", *valuep = value;
  db->dhead->db_nrecs = db->dtrec;
  db->dnrec = 1;
  for (ctr = 0; ctr < db->dnfld; ctr++)
    db->dnrec = db->dnrec + ((db->dfrec[ctr]).fl_fsize /*& 0xFF*/);
  if (!(db->drrec = (char *) malloc(db->dnrec*db->dtrec))) return(db_MEM);
  for (ctr = ptr = rec = 0; rec < db->dtrec; ctr++) {
    switch (DynamicDBF[ctr]) {
      case tkSOR:
        rlen = 0;
        fld = 0;
        break;
      case tkSOF:
        flen = 0;
        break;
      case tkEOF:
        while (flen++ < ((db->dfrec[fld]).fl_fsize /*& 0xFF*/))
          db->drrec[ptr++] = ' ';
        rlen += flen - 1;
        fld++;
        break;
      case tkEOR:
        while (rlen++ < db->dnrec) db->drrec[ptr++] = ' ';
        rec++;
        break;
      default:
        db->drrec[ptr++] = DynamicDBF[ctr];
        flen++;
        break;
    }
  }
  if (rec == 0) db->drrec[ptr] = 0x1A;
  else db->drrec[ptr-1] = 0x1A;
  if (db->dtrec >= 1 && texnamcol) {
    for (fld = 0; fld < db->dnfld; fld++) {
      sprintf(eq_pfn,"U%03d",fld+1);
      if (dbGetf(db,eq_pfn,1,&valuep) != 0) return(1);
      strcpy(value,valuep/*,0xFF*/);
      strip(value);
      for (ctr = 0; ctr < strlen(value) && ctr < 10; ctr++)
        db->dfrec[fld].fl_fname[ctr] = toupper(value[ctr]);
      db->dfrec[fld].fl_fname[ctr] = '\0';
    }
    if (dbDelr(db,1)) return(1);
  }
  if (db->dtrec >= 1 && texdefcol) {
    for (fld = 0; fld < db->dnfld; fld++) {
      if (dbGetf(db,db->dfrec[fld].fl_fname,1,&valuep) != 0) return(1);
      strcpy(value,valuep/*,0xFF*/);
      strip(value);
      if (value[0] == 'C' || value[0] == 'N' || value[0] == 'D'
        || value[0] == 'L') db->dfrec[fld].fl_ftype = value[0];
      else gpperror("unknown field type",value);
    }
    if (dbDelr(db,1)) return(1);
  }
  return(0);
}

/****************************************************************************/
