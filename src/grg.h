/* $Id: grg.h,v 1.25 2001/04/30 09:34:21 timc Exp $
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

/****************************************************************************/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef GUILE
#include <guile/gh.h>
#endif

#ifdef SQL
#ifdef GNUSQL
#define RDBMS "GNUSQL"
#endif
#ifdef POSTGRES
#define RDBMS "PostgreSQL"
#endif
#ifdef ADD_MYSQL
#define RDBMS "MySQL"
#endif
#ifdef INGRES
#define RDBMS "CA-Ingres"
#endif
#else
#undef RDBMS
#endif

/*
 * Include the dBase3+ DBF file utilities
 */

#include <dbase.h>

#define TEXMAXTEX       4096

#define TEXPBMAX        TEXMAXTEX*4

#define EQUATEESC       '#'
#define MAXEQVARS       384
#define MAXEQSTACK      384
#define MAXLVARS        MAXEQSTACK
#define EQF             0
#define EQN             1
#define EQL             2

/*
 * Equate data types (same as dBase field types), plus additional ones
 */

#define EQ_STR          0           /* character string */
#define EQ_NUM          1           /* integer number */
#define EQ_DATE         2           /* date (dBase format) */
#define EQ_BOOL         3           /* boolean */
#define EQ_FLD          4           /* field of a database */
#define EQ_DEC          5           /* decimal number */
#define EQ_ARR          6           /* dynamic array */

/*
 * Maximum string size (used as a base value for many size definitions)
 */

#define STRMAX          256

/*
 * Predefined TeX macro names
 */

#define BASELINE        0
#define TEXHEADER       1
#define TEXFOOTER       2
#define TEXBANNER       3
#define TEXRECORD       4
#define TEXDEFINE       5
#define TEXSORTON       6
#define TEXPAGE01       7
#define TEXPAGENN       8
#define TEXDBFFILE      9
#define TEXFILTER       10
#define TEXEQUATE       11
#define TEXINCLUDE      12
#define TEXIFDEF        13
#define TEXENDIF        14
#define TEXGROUP        15
#define TEXPATTERN      16
#define TEXREVSORT      17
#define TEXBLOCK        18
#define TEXEQGUILE      19
#define TEXMASTERDB     20

/*
 * Predefined TeX macro argument types
 */

#define MAC     1
#define NUM     2
#define FLD     3
#define STR     4
#define DEF     5

/*
 * Predefined equate names
 */

#define EQ_INIT           "eq_init"
#define EQ_PRE_HEADER     "eq_pre_header"
#define EQ_PRE_FOOTER     "eq_pre_footer"
#define EQ_PRE_PAGE01     "eq_pre_page01"
#define EQ_PRE_PAGENN     "eq_pre_pagenn"
#define EQ_PRE_BANNER     "eq_pre_banner"
#define EQ_PRE_RECORD     "eq_pre_record"
#define EQ_PRE_BLOCK      "eq_pre_block"
#define EQ_POST_HEADER    "eq_post_header"
#define EQ_POST_FOOTER    "eq_post_footer"
#define EQ_POST_PAGE01    "eq_post_page01"
#define EQ_POST_PAGENN    "eq_post_pagenn"
#define EQ_POST_BANNER    "eq_post_banner"
#define EQ_POST_RECORD    "eq_post_record"
#define EQ_POST_BLOCK     "eq_post_block"
#define EQ_EXIT           "eq_exit"
#define EQ_PRE_DATABASE   "eq_pre_database"
#define EQ_POST_DATABASE  "eq_post_database"
#define EQ_ARGS           "eq_args"

/*
 * Deprecated equate names - maintained for backwards compatibility
 */

#define EQ_TEXINIT      "eq_texinit"
#define EQ_PRETEXHEADER "eq_pretexheader"
#define EQ_PRETEXFOOTER "eq_pretexfooter"
#define EQ_PRETEXPAGE01 "eq_pretexpage01"
#define EQ_PRETEXPAGENN "eq_pretexpagenn"
#define EQ_PRETEXBANNER "eq_pretexbanner"
#define EQ_PRETEXRECORD "eq_pretexrecord"
#define EQ_PRETEXBLOCK  "eq_pretexblock"
#define EQ_PSTTEXHEADER "eq_psttexheader"
#define EQ_PSTTEXFOOTER "eq_psttexfooter"
#define EQ_PSTTEXPAGE01 "eq_psttexpage01"
#define EQ_PSTTEXPAGENN "eq_psttexpagenn"
#define EQ_PSTTEXBANNER "eq_psttexbanner"
#define EQ_PSTTEXRECORD "eq_psttexrecord"
#define EQ_PSTTEXBLOCK  "eq_psttexblock"
#define EQ_TEXEXIT      "eq_texexit"

/*
 * Predefined system variable names
 */

#define _EQ_TRACE       "_eq_trace"
#define _EQ_VERBOSE     "_eq_verbose"
#define _EQ_VERSION     "_eq_version"
#define _EQ_CLOCK       "_eq_clock"
#define _EQ_DATENOW     "_eq_datenow"
#define _EQ_TIMENOW     "_eq_timenow"
#define _EQ_BANNER_VAL  "_eq_banner_val"
#define _EQ_BANNER_NEST "_eq_banner_nest"
#define _EQ_TOTREC      "_eq_totrec"
#define _EQ_RECNUM      "_eq_currec"
#define _EQ_PFN         "_eq_pfn"
#define _EQ_PFT         "_eq_pft"
#define _EQ_PFL         "_eq_pfl"
#define _EQ_PAT         "_eq_pat"
#define _EQ_FILE        "_eq_file"
#define _EQ_BASE        "_eq_base"
#define _EQ_EXTN        "_eq_extn"
#define _EQ_OUTFILE     "_eq_outfile"
#define _EQ_DBFPATH     "_eq_dbfpath"
#define _EQ_DBFNAME     "_eq_dbfname"
#define _EQ_DBFTYPE     "_eq_dbftype"
#define _EQ_BLOCK       "_eq_block"
#define _EQ_DB_NAME     "_eq_db_name"
#define _EQ_DB_LIMIT    "_eq_db_limit"
#define _EQ_CLARG       "_eq_clarg"

/*
 * Deprecated system variable names - maintained for backwards compatibility
 */

#define _EQ_TEXBASE     "_eq_texbase"
#define _EQ_TEXEXT      "_eq_texext"
#define _GPP_DEBUG      "_gpp_debug"
#define _EQ_PTFILE      "_eq_ptfile"

/*
 * 
 */

#define _TEXMAXTEX      "_texmaxtex"

#define _TEXSORTONMAX   "_texsortonmax"
#define _TEXBANNERMAX   "_texbannermax"
#define _TEXDBFFILEMAX  "_texdbffilemax"
#define _TEXFILTERMAX   "_texfiltermax"

#define _MAXEQUATES     "_maxequates"
#define _MAXMACROS      "_maxmacros"

#define _TEXMAXTEX      "_texmaxtex"
#define _TEXPBMAX       "_texpbmax"

/*
 * Initial values for system resources
 */

#define MAXMACRONAME    64
#define MAXMACRODEF     512

#define MAXMACROS       64
#define MAXEQUATES      32

#define MAXEQUATEDEF    STRMAX

#define TEXSORTONMAX    4
#define TEXBANNERMAX    4
#define TEXDBFFILEMAX   8
#define TEXFILTERMAX    8
#define TEXGROUPMAX     8
#define TEXPATTERNMAX   64
#define TEXBLOCKMAX     8
#define TEXCLARGMAX     4

#define DBFFIELDMAX     16
#define REGEXPMAX       256

#define DYNRECINI       16
#define DYNFLDINI       16

#define TEXMAXUNQCHARS  32
#define TEXMAXUNQMODES  32
#define TEXMAXUNQPOSTS  16

#define TEXTEXTPATMAX   STRMAX/4
#define TEXMODEPATMAX   STRMAX/4

#define TEXMAXCHARSET   128
#define TEXDFAMAXSTATES 16

/*
 * Holds dynamic strings, pointer to allocated space and the number of
 * allocated blocks of STRMAX (not neccessarily the string length)
 */

typedef struct {
  char *value;
  int size;
  int block;
  int bsize;
  char name[MAXMACRONAME];
} datumdef;

typedef struct {
  unsigned short int st;
  char re[STRMAX];
} filterdef;

typedef struct {
  char name[STRMAX];
  char path[STRMAX];
  char type[STRMAX];
  datumdef body;
  dbase *db;
  unsigned char master;
} dbfdef;

typedef struct {
  char name[MAXMACRONAME];
  datumdef body;
} blockdef;

typedef struct {
  char name[MAXMACRONAME];
  char definition[MAXMACRODEF];
} macrodef;

typedef struct {
  char name[MAXMACRONAME];
  unsigned char rev;
  unsigned char guile;
  datumdef definition;
  datumdef body;
} equatedef;

typedef struct {
  char modep[TEXMODEPATMAX];
  char textp[TEXTEXTPATMAX];
  char eqstr[STRMAX];
  char nmode[TEXMODEPATMAX];
  int token;
  int modes[TEXMAXUNQMODES];
} patterndef;

typedef struct {
  char value[STRMAX];
} clargdef;

/*
 * Pre-defined tokens
 */

#define tkNUL   0       /* <nul> : used for no match and nay match */
#define tkSOR   1       /* <sor> : start of record */
#define tkEOR   2       /* <eor> : end of record */
#define tkSOF   3       /* <sof> : start of field */
#define tkEOF   4       /* <eof> : end of field */
#define tkFLD   5       /* <fld> : field */
#define tkERR   6       /* <err> : error */
#define tkSOT   7       /* <sot> : start of text */
#define tkEOT   8       /* <eot> : end of text */
#define tkSOL   9       /* <sol> : start of line */
#define tkEOL   10      /* <eol> : end of line */

#define TEXMAXPATTOK    64

/*
 * Array of structures of pre-defined token to character mappings
 */

typedef struct {
  char token[6];
  short int char_rep;
  char char_scmap;
  char char_mcmap[TEXMAXCHARSET];
} tokendef;

extern tokendef pattern[TEXMAXPATTOK];
extern unsigned short int pattern_ndx;

extern unsigned short int texpage01_val;
extern unsigned short int texpagenn_val;

extern char texsorton_val[TEXSORTONMAX][DBFFIELDMAX];


/*
 * Pre-defined blocks
 */

extern datumdef texheader;
extern datumdef texfooter;
extern datumdef texrecord;
extern datumdef texpage01;
extern datumdef texpagenn;

/*
 * Custom command line argument blocks, indexes and dereference/cast macros
 */

extern datumdef texclarg;
extern unsigned short int texclarg_ndx;
#define GRGCLARG ((clargdef *)texclarg.value)

/*
 * Data blocks, indexes and dereference/cast macros
 */

extern datumdef texdbffile;
extern unsigned short int texdbffile_ndx;
#define GRGDATA ((dbfdef *)texdbffile.value)
extern unsigned short int texsqlfile_ndx;

/*
 * Banner blocks, indexes and dereference/cast macros
 */

extern datumdef texbanner;
extern unsigned short int texbanner_ndx, banner_ndx;
#define GRGBANNER ((blockdef *)texbanner.value)

/*
 * User defined blocks, indexes and dereference/cast macros
 */

extern datumdef texblock;
extern unsigned short int texblock_ndx;
#define GRGBLOCK ((blockdef *)texblock.value)

/*
 * User defined macros, indexes and dereference/cast macros
 */

extern datumdef macrotable;
extern unsigned short int macrotable_ndx;
#define GRGMACRO ((macrodef *)macrotable.value)

/*
 * User defined equates, indexes and dereference/cast macros
 */

extern datumdef equatetable;
extern unsigned short int equatetable_ndx, equate_ndx;
#define GRGEQUATE ((equatedef *)equatetable.value)


extern filterdef texfilter[TEXFILTERMAX];
extern patterndef texpattern[TEXPATTERNMAX];
extern unsigned short int tmod;
extern unsigned short int comnst;
extern unsigned short int texpage01_ndx;
extern unsigned short int texpagenn_ndx;
extern unsigned short int texarg_ndx;

typedef struct {
  unsigned short int type;
  datumdef data;
} eqsdef;

extern eqsdef eqstack[MAXEQSTACK];
extern signed short int eqstack_ndx;

extern void gppparse();
extern void gppwrite();
extern void gppreverse();
extern void grg_expand();

typedef unsigned short int bool;

/*
 * For LEX purposes only
 */

extern short int gppmode;
extern unsigned short int gppstarted;
extern unsigned short int gppcopied;
extern void gpperror();
extern short int gppistexmacro();
extern void gpptexmacro();
extern void gpptexargdef();
extern char *gpptexdefine();
extern void gpptexadd();
extern void gpptexwordadd();
extern void gppcopy();
extern char gppinput();
extern char gppunput();
extern void gpppushback();
extern void gppdump();
extern void putsysvar();
extern char gpplastchar;

extern datumdef texfile;
extern unsigned long int texfile_ndx;

/*
 * For regular expression filter matching
 */

#define INIT            extern char *regexp_ndx;
#define GETC()          (*regexp_ndx++)
#define PEEKC()         (*regexp_ndx)
#define UNGETC(c)       (--regexp_ndx)
#define RETURN(c)       return;
#define ERROR(c)        gpperror("couldn't compile regular expression",NULL);

extern signed char texsorton_dir[TEXSORTONMAX];

extern short int texfiltermax;

typedef struct {
  char reg[REGEXPMAX];
  char def[STRMAX];
  unsigned short int st;
  unsigned long int rec;
} groupdef;

extern groupdef texgroup[TEXGROUPMAX];
extern unsigned short int texgroup_ndx;

extern unsigned short int texsorton_ndx;
extern unsigned short int texfilter_ndx;
extern unsigned short int texpattern_ndx;


extern char texnewpage[STRMAX];
extern char gpptexext[STRMAX];
extern unsigned short int texfescsub;
extern unsigned short int texconcat;
extern unsigned short int texpage_max;
extern char texdelimiter[8];
extern char texdfamode[TEXMODEPATMAX];
extern char texphysdb[STRMAX];
extern char texdbhostnm[STRMAX];
extern char texdbusernm[STRMAX];
extern char texdbpasswd[STRMAX];
extern unsigned short int texnamcol;
extern unsigned short int texdefcol;
extern char texnull[STRMAX];
extern unsigned short int texmkdir;
extern unsigned short int texexpand;

extern char gppcommline[TEXMAXTEX];

extern datumdef texbuffer;
extern unsigned short int texindex;

extern unsigned short int gpprecord;
extern unsigned short int gppheader;
extern int gppdebug;
extern int dfadebug;

extern FILE *gppfile;
extern char gppoutput[196];
extern FILE *gpp_out;

extern unsigned short int gppline;
extern unsigned short int gpppage;
extern unsigned short int gpppage1;

extern datumdef pbbuffer;
extern unsigned long int pbbuffer_ndx;

/*
 * Function Declarations
 */

extern char *grg_strcpy();
extern char *grg_strncpy();
extern char *grg_strcat();
extern char *grg_strncat();
extern int grg_sprintf(datumdef *datum,const char *str,...);

extern unsigned int grg_define_equate();
extern void grg_equate();
extern void grg_reverse();

extern void eqstack_init();
extern int getsysvar();
extern void gpperror();
extern void gppequate();
extern int gppbanner();
extern int gppfilter();
extern void putsysvar();
extern void gppversion();
extern int gppDynamicFill();
extern int gppDynamicOpen();
extern int gppDynamicStop();
extern char *getstrsysvar();
extern void gpppushback();
extern void gppmainsort();

#ifdef GUILE
extern SCM guile_equate();
extern SCM guile_getstrsysvar();
extern SCM guile_getnumsysvar();
extern SCM guile_putstrsysvar();
extern SCM guile_putnumsysvar();
extern SCM guile_getfield();
#endif

/*
 * Write the value of a numeric system variable
 */

#define putnumsysvar(X,Y)       putsysvar(X,EQ_NUM,Y)

/*
 * Write the value of a string system variable
 */

#define putstrsysvar(X,Y)       putsysvar(X,EQ_STR,Y)

/*
 * Retrieve the value of a numeric system variable
 */

#define getnumsysvar(X)         getsysvar(X,EQ_NUM)

/****************************************************************************/
