%{

/* $Id: scanner.l,v 1.3 2001/04/27 10:38:10 timc Exp $
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
 * GURGLE LEX HANDLERS
 */

#include <grg.h>

#ifndef FLEX_SCANNER
#undef  input
#define input()         gppinput(gppfile)
#undef  unput
#define unput(c)        gppunput(c)
#undef  output
#define output(c)       null(c)
#else
#define YY_INPUT(buf,result,maxsize)  \
  { \
    int c = gppinput(gppfile); \
    result = (c == '\0') ? YY_NULL : (buf[0] = c, 1); \
  }
#endif

#undef  yywrap
#define yywrap()        1

#undef  YYLMAX
#define YYLMAX          TEXMAXTEX

/****************************************************************************/

%}

D       [0-9]
A       [a-zA-Z_]
B       [^a-zA-Z_]
L       [a-zA-Z0-9_]
M       [^a-zA-Z0-9_]
W       [ \t\n\r]

%start  TXT ARG TEX EQU COM

%%

        BEGIN TXT;

<TXT>%%{A}{L}*                  { gpptexmacro(yytext);
                                  tmod = 0;
                                  texarg_ndx = 0;
                                  texpage01_ndx = texpagenn_ndx = 0;
                                  switch (gppmode) {
                                    case TEXHEADER: BEGIN ARG; break;
                                    case TEXFOOTER: BEGIN ARG; break;
                                    case TEXBANNER: BEGIN ARG; break;
                                    case TEXRECORD: BEGIN ARG; break;
                                    case TEXDEFINE: BEGIN ARG; break;
                                    case TEXSORTON: BEGIN ARG; break;
                                    case TEXPAGE01: BEGIN ARG; break;
                                    case TEXPAGENN: BEGIN ARG; break;
                                    case TEXDBFFILE: BEGIN ARG; break;
                                    case TEXBLOCK: BEGIN ARG; break;
                                    case TEXFILTER: BEGIN ARG; break;
                                    case TEXEQUATE: BEGIN ARG; break;
                                    case TEXGROUP: BEGIN ARG; break;
                                    case TEXPATTERN: BEGIN ARG; break;
                                    case TEXREVSORT: BEGIN ARG; break;
                                    case TEXEQGUILE: BEGIN ARG; break;
                                    case TEXMASTERDB: BEGIN ARG; break;
                                    case TEXINCLUDE:
                                      texfile_ndx = 0;
                                      BEGIN ARG;
                                      break;
                                    default: BEGIN TXT;
                                  }
                                }
<TXT>%%{W}.*\n                  ;
<TXT>#.*\n                      ;
<TXT>{W}                        { gppdump(yytext); }
<TXT>.                          { gpperror("illegal TXT character",yytext); }

<ARG>{A}{L}*[ \t]+((\\\n)|.)*   { gpptexargdef(yytext,MAC); }
<ARG>{A}{L}*                    { gpptexargdef(yytext,MAC); }
<ARG>{A}{L}*\(.*\)              { gpptexargdef(yytext,DEF); }
<ARG>[-+]?{D}+\.{D}+            { gpptexargdef(yytext,NUM); }
<ARG>[-+]?{D}+                  { gpptexargdef(yytext,NUM); }
<ARG>%[a-zA-Z]{L}*              { gpptexargdef(yytext,FLD); }
<ARG>\"([^\"\n\r]|(\\\"))*\"    { gpptexargdef(yytext,STR); tmod++; }
<ARG>[ \t]                      { gppdump(yytext); }
<ARG>(\\\n)                     ;
<ARG>(\n|\r)                    { switch (gppmode) {
                                    case TEXPAGE01: BEGIN TEX; break;
                                    case TEXPAGENN: BEGIN TEX; break;
                                    case TEXBANNER: BEGIN TEX; break;
                                    case TEXHEADER: BEGIN TEX; break;
                                    case TEXFOOTER: BEGIN TEX; break;
                                    case TEXRECORD: BEGIN TEX; break;
                                    case TEXGROUP: BEGIN TEX; break;
                                    case TEXDBFFILE: BEGIN TEX; break;
                                    case TEXBLOCK: BEGIN TEX; break;
                                    case TEXEQUATE: BEGIN EQU; break;
                                    case TEXEQGUILE: BEGIN TEX; break;
                                    case TEXMASTERDB: BEGIN TEX; break;
                                    case TEXINCLUDE:
                                      BEGIN TXT;
                                      #ifdef FLEX_SCANNER
                                        gppunput(gpplastchar);
                                      #endif
                                      gpppushback(texfile.value);
                                      #ifdef FLEX_SCANNER
                                        YY_FLUSH_BUFFER;
                                      #endif
                                      break;
                                    default: BEGIN TXT;
                                  }
                                  gppdump(yytext);
                                }
<ARG>.                          { gpperror("illegal ARG character",yytext); }

<TEX>^%%{A}{L}*                 { if (gppistexmacro(yytext) < 0) {
                                    #ifdef FLEX_SCANNER
                                      gppunput(gpplastchar);
                                    #endif
                                    gpppushback(gpptexdefine(yytext));
                                    #ifdef FLEX_SCANNER
                                      YY_FLUSH_BUFFER;
                                    #endif
                                  }
                                  else {
                                    gppcopy();
                                    #ifdef FLEX_SCANNER
                                      gppunput(gpplastchar);
                                    #endif
                                    gpppushback(yytext);
                                    #ifdef FLEX_SCANNER
                                      YY_FLUSH_BUFFER;
                                    #endif
                                    BEGIN TXT;
                                  }
                                }
<TEX>^%%{W}.*\n                 ;
<TEX>%%{A}{L}*                  {
                                  #ifdef FLEX_SCANNER
                                    gppunput(gpplastchar);
                                  #endif
                                  gpppushback(gpptexdefine(yytext));
                                  #ifdef FLEX_SCANNER
                                    YY_FLUSH_BUFFER;
                                  #endif
                                }
<TEX>(.|\n)                     { gpptexadd(yytext); gppdump(yytext); }

<EQU>^%%{A}{L}*                 { if (gppistexmacro(yytext) < 0) {
                                    #ifdef FLEX_SCANNER
                                      gppunput(gpplastchar);
                                    #endif
                                    gpppushback(gpptexdefine(yytext));
                                    #ifdef FLEX_SCANNER
                                      YY_FLUSH_BUFFER;
                                    #endif
                                  }
                                  else {
                                    gppcopy();
                                    #ifdef FLEX_SCANNER
                                      gppunput(gpplastchar);
                                    #endif
                                    gpppushback(yytext);
                                    #ifdef FLEX_SCANNER
                                      YY_FLUSH_BUFFER;
                                    #endif
                                    BEGIN TXT;
                                  }
                                }
<EQU>^%%{W}.*\n                 ;
<EQU>%%{A}{L}*                  { 
                                  #ifdef FLEX_SCANNER
                                    gppunput(gpplastchar);
                                  #endif
                                  /* Hacked to support the expansion of
                                     a user defined macro into a single
                                     double quote (") which in Lex is
                                     not reprocessed but is in Flex. The
                                     Flex behaviour is actually correct
                                     so this hack is purely for ensuring
                                     compatibility and support for the
                                     old behaviour should be deprecated. */
				  if (!strcmp(gpptexdefine(yytext),"\"")) {
				    gpptexadd("\""); gppdump("\"");
				  }
				  else
                                    gpppushback(gpptexdefine(yytext));
                                  #ifdef FLEX_SCANNER
                                    YY_FLUSH_BUFFER;
                                  #endif
                                }
<EQU>{A}{L}*                    { gpptexwordadd(yytext); gppdump(yytext); }
<EQU>\"([^\"\n\r]|(\\\"))*\"    { gpptexwordadd(yytext); gppdump(yytext); }
<EQU>\/\*                       { comnst = 1; BEGIN COM; }
<EQU>(.|\n)                     { gpptexadd(yytext); gppdump(yytext); }

<COM>(.|\n)                     ;
<COM>\/\*                       { comnst++; }
<COM>\*\/                       { if (!--comnst) BEGIN EQU; }

%%

/****************************************************************************/
