/* $Id: parser.c,v 1.5 2001/05/02 16:07:52 timc Exp $
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
 * GURGLE DEFINITION FILE HANDLER
 */

#include <grg.h>

/*
 * Pushback buffer and index
 */

datumdef pbbuffer = { NULL, 0, TEXPBMAX, sizeof(char), "PBBUFFER" };
unsigned long int pbbuffer_ndx = 0;

/*
 * Text bodies
 */

datumdef texheader = { NULL, 0, TEXMAXTEX, sizeof(char), "HEADER" };
datumdef texfooter = { NULL, 0, TEXMAXTEX, sizeof(char), "FOOTER" };
datumdef texrecord = { NULL, 0, TEXMAXTEX, sizeof(char), "RECORD" };
datumdef texpage01 = { NULL, 0, TEXMAXTEX, sizeof(char), "PAGE01" };
datumdef texpagenn = { NULL, 0, TEXMAXTEX, sizeof(char), "PAGENN" };

datumdef texbanner = { NULL, 0, TEXBANNERMAX, sizeof(blockdef), "BANNER" };
unsigned short int texbanner_ndx = 0, banner_ndx = 0;

datumdef texdbffile = { NULL, 0, TEXDBFFILEMAX, sizeof(dbfdef), "DATABASE" };
unsigned short int texdbffile_ndx = 0, dbffile_ndx = 0;
unsigned short int texsqlfile_ndx = 0;

/*
 * Text utilities and indexes
 */

char texsorton_val[TEXSORTONMAX][DBFFIELDMAX];
signed char texsorton_dir[TEXSORTONMAX];


datumdef texblock = { NULL, 0, TEXBLOCKMAX, sizeof(blockdef), "BLOCK" };
unsigned short int texblock_ndx = 0;


short int texfiltermax = TEXFILTERMAX;
filterdef texfilter[TEXFILTERMAX];

groupdef texgroup[TEXGROUPMAX];

unsigned short int texsorton_ndx = 0;
unsigned short int texfilter_ndx = 0;
unsigned short int texgroup_ndx = 0;

unsigned short int tmod = 0;
unsigned short int comnst = 0;

/*
 * Tokens for pattern matchers
 */

struct {
  char token[6];
  int token_id;
} tokens[] = {
  "<nul>", tkNUL,
  "<sor>", tkSOR,
  "<sof>", tkSOF,
  "<fld>", tkFLD,
  "<eof>", tkEOF,
  "<eor>", tkEOR,
  "<err>", tkERR,
};

#define tokens_ndx      (sizeof tokens / sizeof tokens[0])

/*
 * Initialise default dynamic dbf patterns and modes
 */

patterndef texpattern[TEXPATTERNMAX] = {
  "<awk><nul>", "<nul>", "", "<awk><sor>", tkSOR, {0},
  "<awk><sor>", "<wht>", "", "<awk><sor>", tkNUL, {0},
  "<awk><sor>", "<nul>", "", "<awk><sof>", tkSOF, {0},
  "<awk><sof>", "<del>", "", "<awk><sor>", tkEOF, {0},
  "<awk><sof>", "<any>", "", "<awk><sof>", tkFLD, {0},
  "<awk><sof>", "<nul>", "", "<awk><sor>", tkEOF, {0},
  "<awk><sor>", "<new>", "", "<awk><nul>", tkEOR, {0},
};

unsigned short int texpattern_ndx = 7;

/*
 * Environment variables
 */

char texnewpage[STRMAX] = "\\newpage";
char gpptexext[STRMAX] = ".tex";
unsigned short int texfescsub = 0;
unsigned short int texconcat = 0;
unsigned short int texpage01_val = 0xFFFF;
unsigned short int texpagenn_val = 0xFFFF;
unsigned short int texpage_max = 0;
unsigned short int texpage01_ndx = 0;
unsigned short int texpagenn_ndx = 0;
char texdelimiter[8] = "";
char texdfamode[TEXMODEPATMAX] = "<awk><nul>";
unsigned short int texarg_ndx = 0;
char texphysdb[STRMAX] = "";
char texdbhostnm[STRMAX] = "";
char texdbusernm[STRMAX] = "";
char texdbpasswd[STRMAX] = "";
unsigned short int texnamcol = 0;
unsigned short int texdefcol = 0;
char texnull[STRMAX] = "";
unsigned short int texmkdir = 0;
unsigned short int texexpand = 0;

/*
 * Used for include files
 */

datumdef texfile = { NULL, 0, TEXMAXTEX, sizeof(char), "INCLUDE" };
unsigned long int texfile_ndx = 0;

/*
 * Whether to print debugging information
 */

int gppdebug = 0;
int dfadebug = 0;

/*
 * Holds predefined tex macros given on the command line
 */

char gppcommline[TEXMAXTEX];

/*
 * The user defined macros table and indexes
 */

datumdef macrotable = { NULL, 0, MAXMACROS, sizeof(macrodef), "DEFINE" };
unsigned short int macrotable_ndx = 0;

/*
 * The equate expressions table and indexes
 */

datumdef equatetable = { NULL, 0, MAXEQUATES, sizeof(equatedef), "EQUATE" };
unsigned short int equatetable_ndx = 0, equate_ndx = 0;

/*
 * The initial mode
 */

short int gppmode = BASELINE;

/*
 * Flag indicates whether the generic buffer has been copied
 */

unsigned short int gppcopied = 0;

/*
 * Generic text body buffer and index
 */

datumdef texbuffer = { NULL, 0, TEXMAXTEX, sizeof(char), "BUFFER" };
unsigned short int texindex = 0;

/*
 * Flag indicates whether the first gpp predefined macro has been seen yet
 */

unsigned short int gppstarted = 0;

/*
 * Flag indicates whether the TEXRECORD macro was defined (even if empty)
 */

unsigned short int gpprecord = 0;

/*
 * Flag indicates whether the TEXHEADER macro was defined (even if empty)
 */

unsigned short int gppheader = 0;

/*
 * Equate body keyword to operator mappings
 */

static struct {
  char name[16];
  char token[4];
} keywords[] = {
  "while", "[",
  "do", ";",
  "wend", "]",
  "endwhile", "]",
  "roll", "{",
  "through", "}",
  "inputs", "@",
  "outputs", "",
  "not", "~~",
  "and", "&&",
  "or", "||",
  "xor", "^^",
  "if", "",
  "then", "?",
  "else", ":",
  "elif", "::",
  "elseif", "::",
  "endif", ":;",
  "break", "\\b",
  "exit", "\\e",
  "read", ".>",
  "write", ".<",
  "send", "..",
  "exec", "$",
  "expand", "$$",
  "is", "=",
  "into", ">>"
};

/*
 * Called on tex preprocessor parsing errors
 */

void gpperror(str,arg) char *str; char *arg; {
  if (arg) fprintf(stderr,"grg: %s, '%s'.\n",str,arg);
  else fprintf(stderr,"grg: %s.\n",str);
  exit(1);
}

/*
 * Returns new processing mode if str is a predefined macro else returns -1
 */

short int gppistexmacro(str) char *str; {
short int mode;
  if (strcasecmp(str+2,"TEXHEADER") == 0) mode = TEXHEADER;
  else if (strcasecmp(str+2,"TEXFOOTER") == 0) mode = TEXFOOTER;
  else if (strcasecmp(str+2,"TEXBANNER") == 0) mode = TEXBANNER;
  else if (strcasecmp(str+2,"TEXRECORD") == 0) mode = TEXRECORD;
  else if (strcasecmp(str+2,"TEXPAGE01") == 0) mode = TEXPAGE01;
  else if (strcasecmp(str+2,"TEXPAGENN") == 0) mode = TEXPAGENN;
  else if (strcasecmp(str+2,"TEXDEFINE") == 0) mode = TEXDEFINE;
  else if (strcasecmp(str+2,"TEXSORTON") == 0) mode = TEXSORTON;
  else if (strcasecmp(str+2,"TEXDBFFILE") == 0) mode = TEXDBFFILE;
  else if (strcasecmp(str+2,"TEXBLOCK") == 0) mode = TEXBLOCK;
  else if (strcasecmp(str+2,"TEXFILTER") == 0) mode = TEXFILTER;
  else if (strcasecmp(str+2,"TEXEQUATE") == 0) mode = TEXEQUATE;
  else if (strcasecmp(str+2,"TEXINCLUDE") == 0) mode = TEXINCLUDE;
  else if (strcasecmp(str+2,"TEXIFDEF") == 0) mode = TEXIFDEF;
  else if (strcasecmp(str+2,"TEXENDIF") == 0) mode = TEXENDIF;
  else if (strcasecmp(str+2,"TEXGROUP") == 0) mode = TEXGROUP;
  else if (strcasecmp(str+2,"TEXPATTERN") == 0) mode = TEXPATTERN;
  else if (strcasecmp(str+2,"TEXREVSORT") == 0) mode = TEXREVSORT;
  else if (strcasecmp(str+2,"TEXEQGUILE") == 0) mode = TEXEQGUILE;
  else if (strcasecmp(str+2,"TEXMASTERDB") == 0) mode = TEXMASTERDB;
  else if (strcasecmp(str+2,"TEX") == 0) mode = BASELINE;
  else if (strcasecmp(str+2,"GPPHEADER") == 0) mode = TEXHEADER;
  else if (strcasecmp(str+2,"GPPFOOTER") == 0) mode = TEXFOOTER;
  else if (strcasecmp(str+2,"GPPBANNER") == 0) mode = TEXBANNER;
  else if (strcasecmp(str+2,"GPPRECORD") == 0) mode = TEXRECORD;
  else if (strcasecmp(str+2,"GPPPAGE01") == 0) mode = TEXPAGE01;
  else if (strcasecmp(str+2,"GPPPAGENN") == 0) mode = TEXPAGENN;
  else if (strcasecmp(str+2,"GPPDEFINE") == 0) mode = TEXDEFINE;
  else if (strcasecmp(str+2,"GPPSORTON") == 0) mode = TEXSORTON;
  else if (strcasecmp(str+2,"GPPDBFFILE") == 0) mode = TEXDBFFILE;
  else if (strcasecmp(str+2,"GPPBLOCK") == 0) mode = TEXBLOCK;
  else if (strcasecmp(str+2,"GPPFILTER") == 0) mode = TEXFILTER;
  else if (strcasecmp(str+2,"GPPEQUATE") == 0) mode = TEXEQUATE;
  else if (strcasecmp(str+2,"GPPINCLUDE") == 0) mode = TEXINCLUDE;
  else if (strcasecmp(str+2,"GPPIFDEF") == 0) mode = TEXIFDEF;
  else if (strcasecmp(str+2,"GPPENDIF") == 0) mode = TEXENDIF;
  else if (strcasecmp(str+2,"GPPGROUP") == 0) mode = TEXGROUP;
  else if (strcasecmp(str+2,"GPPPATTERN") == 0) mode = TEXPATTERN;
  else if (strcasecmp(str+2,"GPPREVSORT") == 0) mode = TEXREVSORT;
  else if (strcasecmp(str+2,"GPPEQGUILE") == 0) mode = TEXEQGUILE;
  else if (strcasecmp(str+2,"GPPMASTERDB") == 0) mode = TEXMASTERDB;
  else if (strcasecmp(str+2,"GPP") == 0) mode = BASELINE;
  else if (strcasecmp(str+2,"HEADER") == 0) mode = TEXHEADER;
  else if (strcasecmp(str+2,"FOOTER") == 0) mode = TEXFOOTER;
  else if (strcasecmp(str+2,"BANNER") == 0) mode = TEXBANNER;
  else if (strcasecmp(str+2,"RECORD") == 0) mode = TEXRECORD;
  else if (strcasecmp(str+2,"PAGE01") == 0) mode = TEXPAGE01;
  else if (strcasecmp(str+2,"PAGENN") == 0) mode = TEXPAGENN;
  else if (strcasecmp(str+2,"DEFINE") == 0) mode = TEXDEFINE;
  else if (strcasecmp(str+2,"SORTON") == 0) mode = TEXSORTON;
  else if (strcasecmp(str+2,"DATABASE") == 0) mode = TEXDBFFILE;
  else if (strcasecmp(str+2,"BLOCK") == 0) mode = TEXBLOCK;
  else if (strcasecmp(str+2,"FILTER") == 0) mode = TEXFILTER;
  else if (strcasecmp(str+2,"EQUATE") == 0) mode = TEXEQUATE;
  else if (strcasecmp(str+2,"INCLUDE") == 0) mode = TEXINCLUDE;
  else if (strcasecmp(str+2,"IFDEF") == 0) mode = TEXIFDEF;
  else if (strcasecmp(str+2,"ENDIF") == 0) mode = TEXENDIF;
  else if (strcasecmp(str+2,"GROUP") == 0) mode = TEXGROUP;
  else if (strcasecmp(str+2,"PATTERN") == 0) mode = TEXPATTERN;
  else if (strcasecmp(str+2,"REVSORT") == 0) mode = TEXREVSORT;
  else if (strcasecmp(str+2,"EQGUILE") == 0) mode = TEXEQGUILE;
  else if (strcasecmp(str+2,"MASTERDB") == 0) mode = TEXMASTERDB;
  else if (strcasecmp(str+2,"END") == 0) mode = BASELINE;
  else mode = -1;
  return(mode);
}

/*
 * Called on the occurence of a tex predefined macro
 */

void gpptexmacro(str) char *str; {
  gppstarted = 1;
  gppmode = gppistexmacro(str);
  if (gppmode < 0) gpperror("unknown predefined macro",str);
  if (gppmode == TEXRECORD) gpprecord = 1;
  else if (gppmode == TEXHEADER) gppheader = 1;
}

/*
 * Called to process arguments to tex predefined macros
 */

void gpptexargdef(str,gpptype) char *str; unsigned short int gpptype; {
int chr, ctr, ndx;
FILE *file;
char texfilename[STRMAX];
  switch (gppmode) {
    case TEXPAGE01:
      if (gpptype != NUM) gpperror("numeric argument expected",str);
      if (texpage01_ndx == 0) texpage01_val = atoi(str);
      else if (texpage01_ndx == 1) texpage_max = atoi(str);
      else gpperror("too many arguments for texpage01",str);
      texpage01_ndx++;
      break;
    case TEXPAGENN:
      if (gpptype != NUM) gpperror("numeric argument expected",str);
      if (texpagenn_ndx == 0) texpagenn_val = atoi(str);
      else if (texpagenn_ndx == 1) texpage_max = atoi(str);
      else gpperror("too many arguments for texpagenn",str);
      texpagenn_ndx++;
      break;
    case TEXSORTON:
      if (gpptype != FLD) gpperror("field argument expected",str);
      if (texsorton_ndx == TEXSORTONMAX) gpperror("too many sort args",str);
      texsorton_dir[texsorton_ndx] = 1;
      strcpy(texsorton_val[texsorton_ndx++],str+1);
      break;
    case TEXBANNER:
      if (gpptype != FLD) gpperror("field argument expected",str);
      grg_alloc(&texbanner,texbanner_ndx);
      strcpy(GRGBANNER[texbanner_ndx].name,str+1);
      sprintf(GRGBANNER[texbanner_ndx].body.name,"BANNER.%s",str+1);
      GRGBANNER[texbanner_ndx].body.block = TEXMAXTEX;
      GRGBANNER[texbanner_ndx].body.bsize = sizeof(char);
      texbanner_ndx++;
      break;
    case TEXDBFFILE:
    case TEXMASTERDB:
      if (gpptype != STR) gpperror("string argument expected",str);
      grg_alloc(&texdbffile,texdbffile_ndx);
      strcpy(GRGDATA[texdbffile_ndx].name,str+1);
      GRGDATA[texdbffile_ndx].name[strlen(str)-2] = '\0';
      for (chr = strlen(GRGDATA[texdbffile_ndx].name) - 1;
        chr >= 0 && GRGDATA[texdbffile_ndx].name[chr] != '.'; chr--);
      if (GRGDATA[texdbffile_ndx].name[chr] == '.' && chr > 0 &&
        GRGDATA[texdbffile_ndx].name[chr-1] != '/') {
        strcpy(GRGDATA[texdbffile_ndx].type,
          GRGDATA[texdbffile_ndx].name+chr);
        GRGDATA[texdbffile_ndx].name[chr] = '\0';
      }
      else GRGDATA[texdbffile_ndx].type[0] = '\0';
      ctr = -1;
      for (chr = 0; GRGDATA[texdbffile_ndx].name[chr] != '\0'; chr++)
        if (GRGDATA[texdbffile_ndx].name[chr] == '/') ctr = chr;
      strncpy(GRGDATA[texdbffile_ndx].path,
        GRGDATA[texdbffile_ndx].name,ctr+1);
      strcpy(texfilename,GRGDATA[texdbffile_ndx].name+ctr+1);
      strcpy(GRGDATA[texdbffile_ndx].name,texfilename);
      if (gppmode == TEXMASTERDB || texdbffile_ndx == 0) {
        for (ctr = 0; ctr < texdbffile_ndx; ctr++) GRGDATA[ctr].master = 0;
        GRGDATA[texdbffile_ndx].master = 1;
      }
      sprintf(GRGDATA[texdbffile_ndx].body.name,"DATABASE.%s",
        GRGDATA[texdbffile_ndx].name);
      GRGDATA[texdbffile_ndx].body.block = TEXMAXTEX;
      GRGDATA[texdbffile_ndx].body.bsize = sizeof(char);
      if (strcmp(GRGDATA[texdbffile_ndx].type,".sql") == 0) texsqlfile_ndx++;
      texdbffile_ndx++;
      break;
    case TEXFILTER:
      if (gpptype != STR) gpperror("string argument expected",str);
      if (texfilter_ndx == texfiltermax) gpperror("too many filter args",str);
      strcpy(texfilter[texfilter_ndx].re,str+1);
      texfilter[texfilter_ndx].re[strlen(str)-2] = '\0';
      if (tmod == 1) {
          texfilter[texfilter_ndx-1].st++;
          tmod++;
      }
      texfilter[texfilter_ndx].st = tmod;
      texfilter_ndx++;
      break;
    case TEXINCLUDE:
      if (gpptype != STR) gpperror("string argument expected",str);
      strcpy(texfilename,str+1);
      texfilename[strlen(str)-2] = '\0';
      if ((file = fopen(texfilename,"r")) == NULL)
        gpperror("couldn't open include file",texfilename);
      for (ctr = texfile_ndx; (chr = fgetc(file)) != EOF; ctr++) {
        grg_alloc(&texfile,ctr);
        texfile.value[ctr] = chr;
      }
      grg_alloc(&texfile,ctr);
      texfile.value[ctr] = '\0';
      texfile_ndx = ctr;
      break;
    case TEXEQUATE:
    case TEXEQGUILE:
      if (gpptype == MAC)
        for (chr = 0; str[chr]!=' ' && str[chr]!='\t' && str[chr]!='\0'; chr++);
      else if (gpptype == DEF)
        for (chr = 0; str[chr]!='(' && str[chr]!='\0'; chr++);
      else gpperror("equate argument expected",str);
      if (str[chr] != '\0') str[chr] = '\0';
      else str[chr+1] = '\0';
      ndx = grg_define_equate(str);
      if (gppmode == TEXEQGUILE) {
        GRGEQUATE[ndx].guile = 1;
        GRGEQUATE[ndx].rev = 1;
      }
      if (gpptype == MAC) {
        for (chr++; str[chr] == ' ' ||  str[chr] == '\t'; chr++);
        if (strlen(str+chr) > MAXEQUATEDEF)
          gpperror("equate definition too long",GRGEQUATE[ndx].name);
        grg_strcpy(&GRGEQUATE[ndx].definition,str+chr);
      }
      else {
        if (strlen(str+chr+1) > MAXEQUATEDEF)
          gpperror("equate definition too long",GRGEQUATE[ndx].name);
        if (str[chr+1] != '\0') {
          grg_sprintf(&GRGEQUATE[ndx].definition,"@(%s",str+chr+1);
          GRGEQUATE[ndx].rev = 1;
        }
      }
      equate_ndx = ndx;
      break;
    case TEXBLOCK:
      if (gpptype != MAC) gpperror("macro argument expected",str);
      for (chr = 0; str[chr]!=' ' && str[chr]!='\t' && str[chr]!='\0'; chr++);
      if (str[chr] != '\0') str[chr] = '\0';
      else str[chr+1] = '\0';
      grg_alloc(&texblock,texblock_ndx);
      strcpy(GRGBLOCK[texblock_ndx].name,str);
      sprintf(GRGBLOCK[texblock_ndx].body.name,"BLOCK.%s",str);
      GRGBLOCK[texblock_ndx].body.block = TEXMAXTEX;
      GRGBLOCK[texblock_ndx].body.bsize = sizeof(char);
      texblock_ndx++;
      break;
    case TEXDEFINE:
      if (gpptype != MAC) gpperror("macro argument expected",str);
      for (chr = 0; str[chr]!=' ' && str[chr]!='\t' && str[chr]!='\0'; chr++);
      if (str[chr] != '\0') str[chr] = '\0';
      else str[chr+1] = '\0';
      grg_alloc(&macrotable,macrotable_ndx);
      for (ndx = 0; ndx < macrotable_ndx; ndx++)
        if (strcmp(GRGMACRO[ndx].name,str) == 0) break;
      if (ndx == macrotable_ndx) macrotable_ndx++;
      strcpy(GRGMACRO[ndx].name,str);
      for (chr++; str[chr] == ' ' ||  str[chr] == '\t'; chr++);
      strcpy(GRGMACRO[ndx].definition,str+chr);
      break;
    case TEXGROUP:
      if (gpptype != STR) gpperror("string argument expected",str);
      if (texgroup_ndx == TEXGROUPMAX) gpperror("too many group args",str);
      strcpy(texgroup[texgroup_ndx].reg,str+1);
      texgroup[texgroup_ndx].reg[strlen(str)-2] = '\0';
      if (tmod == 1) {
          texgroup[texgroup_ndx-1].st++;
          tmod++;
      }
      texgroup[texgroup_ndx].st = tmod;
      texgroup_ndx++;
      break;
    case TEXPATTERN:
      if (gpptype != STR) gpperror("string argument expected",str);
      switch (texarg_ndx) {
        case 0:
          strcpy(texpattern[texpattern_ndx].modep,str+1);
          texpattern[texpattern_ndx].modep[strlen(str)-2] = '\0';
          break;
        case 1:
          strcpy(texpattern[texpattern_ndx].textp,str+1);
          texpattern[texpattern_ndx].textp[strlen(str)-2] = '\0';
          break;
        case 2:
          strcpy(texpattern[texpattern_ndx].eqstr,str+1);
          texpattern[texpattern_ndx].eqstr[strlen(str)-2] = '\0';
          break;
        case 3:
          strcpy(texpattern[texpattern_ndx].nmode,str+1);
          texpattern[texpattern_ndx].nmode[strlen(str)-2] = '\0';
          break;
        case 4:
          for (ndx = 0; ndx < tokens_ndx; ndx++)
            if (strncmp(str+1,tokens[ndx].token,strlen(str)-2) == 0) break;
          if (ndx == tokens_ndx) gpperror("unknown token",str);
          texpattern[texpattern_ndx].token = tokens[ndx].token_id;
          texpattern_ndx++;
          break;
        default:
          gpperror("too many arguments for texpattern",str);
          break;
      }
      texarg_ndx++;
      break;
    case TEXREVSORT:
      if (gpptype != FLD) gpperror("field argument expected",str);
      if (texsorton_ndx == TEXSORTONMAX) gpperror("too many sort args",str);
      texsorton_dir[texsorton_ndx] = -1;
      strcpy(texsorton_val[texsorton_ndx++],str+1);
      break;
    default: gpperror("out of place argument",str);
  }
}

/*
 * Called to expand tex user macros and equates
 */

char *gpptexdefine(str) char *str; {
unsigned short int macro;
  for (macro = 0; macro < macrotable_ndx; macro++)
    if (strcmp(GRGMACRO[macro].name,str+2) == 0)
      return(GRGMACRO[macro].definition);
  for (macro = 0; macro < equatetable_ndx; macro++)
    if (strcmp(GRGEQUATE[macro].name,str+2) == 0) {
      sprintf(str,"%c%s",EQUATEESC,GRGEQUATE[macro].name);
      return(str);
    }
  gpperror("undefined macro or equate",str);
}

/*
 * Called to add text body characters to the generic text body buffer
 */

void gpptexadd(str) char *str; {
  grg_alloc(&texbuffer,texindex);
  if (str[0] >= 32 && str[0] <= 126)
    texbuffer.value[texindex++] = str[0];
  else if (str[0] == '\n' || str[0] == '\r' || str[0] == '\t')
    texbuffer.value[texindex++] = str[0];
  else gpperror("illegal character",str);
  gppcopied = 0;
}

/*
 * Called to add text body words to the generic text body buffer
 */

void gpptexwordadd(str) char *str; {
short int ndx, chr;
char ch[2];
  for (ndx = 0; ndx < (sizeof(keywords)/sizeof(keywords[0])); ndx++)
    if (strcasecmp(keywords[ndx].name,str) == 0) break;
  if (ndx == (sizeof(keywords)/sizeof(keywords[0]))) {
    for (chr = 0; str[chr] != '\0'; chr++) {
      ch[0] = str[chr]; ch[1] = '\0';
      gpptexadd(ch);
    }
  }
  else {
    for (chr = 0; keywords[ndx].token[chr] != '\0'; chr++) {
      ch[0] = keywords[ndx].token[chr]; ch[1] = '\0';
      gpptexadd(ch);
    }
  }
}

/*
 * Called to copy the generic text body buffer to named text body buffers
 */

void gppcopy() {
int chr;
  texbuffer.value[texindex] = '\0';
  switch(gppmode) {
    case TEXHEADER:
      grg_strcpy(&texheader,texbuffer.value); break;
    case TEXFOOTER:
      grg_strcpy(&texfooter,texbuffer.value); break;
    case TEXRECORD:
      grg_strcpy(&texrecord,texbuffer.value); break;
    case TEXPAGE01:
      grg_strcpy(&texpage01,texbuffer.value); break;
    case TEXPAGENN:
      grg_strcpy(&texpagenn,texbuffer.value); break;
    case TEXBANNER:
      for (; banner_ndx < texbanner_ndx; banner_ndx++)
        grg_strcpy(&GRGBANNER[banner_ndx].body,texbuffer.value);
      break;
    case TEXGROUP:
      strcpy(texgroup[texgroup_ndx-1].def,texbuffer.value); break;
    case TEXDBFFILE:
    case TEXMASTERDB:
      for (; dbffile_ndx < texdbffile_ndx; dbffile_ndx++)
        grg_strcpy(&GRGDATA[dbffile_ndx].body,texbuffer.value);
      break;
    case TEXBLOCK:
      grg_strcpy(&GRGBLOCK[texblock_ndx-1].body,texbuffer.value); break;
    case TEXEQUATE:
    case TEXEQGUILE:
      for (chr = 0; texbuffer.value[chr] != '\0'; chr++)
        if (texbuffer.value[chr] != ' ' && texbuffer.value[chr] != '\t'
          && texbuffer.value[chr] != '\n' && texbuffer.value[chr] != '\r')
          break;
      if (chr != strlen(texbuffer.value))
        grg_strcpy(&GRGEQUATE[equate_ndx].body,texbuffer.value);
      if (gppdebug)
        fprintf(stdout,"%s:\n%s\n%s\n",GRGEQUATE[equate_ndx].name,
          GRGEQUATE[equate_ndx].definition.value,
          GRGEQUATE[equate_ndx].body.value);
      break;
    default:
      gpperror("unknown mode",NULL);
  }
  texindex = 0;
  texbuffer.value[texindex] = '\0';
  gppcopied = 1;
}

/*
 * LEX character unput (pushback) routine
 */

char gppunput(chr) char chr; {
  grg_alloc(&pbbuffer,pbbuffer_ndx);
  pbbuffer.value[pbbuffer_ndx++] = chr;
  if (chr == EOF) return('\0');
  else return(chr);
}

/*
 * Called to reverse and pushback a string
 */

void gpppushback(str) char *str; {
unsigned long int chr;
  for (chr = strlen(str); chr > 0; chr--)
    gppunput(str[chr-1]);
}

/*
 * LEX character input routine (with pushback buffer support)
 */

char gpplastchar = '\0';

char gppinput(file) FILE *file; {
  if (pbbuffer_ndx > 0) gpplastchar = pbbuffer.value[--pbbuffer_ndx];
  else gpplastchar = fgetc(file);
  if (gpplastchar == EOF) {
    if (strcmp(gppcommline,"") == 0) return('\0');
    else {
      gpppushback(gppcommline);
      strcpy(gppcommline,"");
      return(gppinput(file));
    }
  }
  else return(gpplastchar);
}

/*
 * Produces the preprocessed output
 */

void gppdump(str) char *str; {
  if (gppdebug) fputs(str,stdout);
}

/*
 * The one and only familiar
 */

void null(c) int c; {}

/*
 * File descriptor for the PreTex file to be parsed
 */

FILE *gppfile;

/*
 * File name for the generated file
 */

char gppoutput[196];

/*
 * File descriptor for the generated file
 */

FILE *gpp_out;

/*
 * Called to parse a PreTex file
 */

void gppparse(gppfilename) char *gppfilename; {
  if ((gppfile = fopen(gppfilename,"r")) == NULL) {
    strcat(gppfilename,".grg");
    putsysvar(_EQ_PTFILE,EQ_STR,gppfilename);   /* deprecated */
    putsysvar(_EQ_FILE,EQ_STR,gppfilename);
    if ((gppfile = fopen(gppfilename,"r")) == NULL) {
      gppversion(stderr);
      gpperror("couldn't open file",gppfilename);
    }
  }
  while (yylex() != '\0');
  if (!gppcopied) gppcopy();
  fclose(gppfile);
}

/****************************************************************************/
