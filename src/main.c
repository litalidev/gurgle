/* $Id: main.c,v 1.48 2003/02/04 12:32:15 timc Exp $
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

static char *rcsid = "@(#)$Id: main.c,v 1.48 2003/02/04 12:32:15 timc Exp $ Copyright (C) University of Edinburgh, 1992-1997, 2001, 2003-4, 2008-10. All rights reserved.";

/*
 * GURGLE MAIN ROUTINE
 */

#include <grg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

/*
 * Prints program title, version and rights notice
 */

void gppversion(FILE *file) {
#ifdef RDBMS
  fprintf(file,"GURGLE - GNU REPORT GENERATOR LANGUAGE - Version %s (Built for %s)\n",VERSION,RDBMS);
#else
  fprintf(file,"GURGLE - GNU REPORT GENERATOR LANGUAGE - Version %s\n",VERSION);
#endif
  fprintf(file,"Copyright (C) University of Edinburgh, 1993-1997, 2001, 2003-4, 2008-10.\nAll rights reserved.\n");
  fprintf(file,"The Author, Tim Edward Colles, has exercised his right to be identified\n");
  fprintf(file,"as such under the Copyright, Designs and Patents Act 1988.\n\n");
}

/*
 * Process equate and if it leaves a value on the stack write it
 */

void grg_write_equate(nam,db,rec) char *nam; dbase *db; long rec; {
datumdef datadef = { NULL, 0, STRMAX, sizeof(char), "data" }, *data = &datadef;
  gppequate(nam,db,rec,0);
  if (eqstack_ndx > 1) gpperror("stack overflow",NULL);
  else if (eqstack_ndx == 1) {
    grg_strcpy(data,eqstack[--eqstack_ndx].data.value);
    gppwrite(data->value,db,rec,gpp_out);
  }
  grg_dealloc(data);
}

char gppusage[STRMAX] = "Usage: %s [-d[1234]] file[.grg] [<PDM>]* [<UDCLA>]*\n";

datumdef texclarg = { NULL, 0, TEXCLARGMAX, sizeof(clargdef), "CLARG" };
unsigned short int texclarg_ndx = 0;

void grg_main(argc,argv) int argc; char *argv[]; {
int ctr, record = 0, mode, prec, arg, i, sm = 0, sc = 0;
dbase *master_db, *db;
char gpppretex[64];
FILE *gpp_pt;
time_t clock;
struct tm *tm_time;
char temp[5], date_now[STRMAX], time_now[STRMAX], value[STRMAX];
char filename[STRMAX];

  /*
   * Process command line arguments
   */

  gpppretex[0] = '\0';
  if (argc == 1 || (argc == 2 && (argv[1][0] == '-' || argv[1][0] == '%'))) {
    gppversion(stderr);
    fprintf(stderr,gppusage,argv[0]);
    exit(1);
  }
  else if (argc == 2) {
    strcpy(gpppretex,argv[1]);
  }
  else if (argc >= 3) {
    for (arg = 1; arg < argc; arg++) {
      if (strncmp(argv[arg],"-d",2) == 0) {
        if (strlen(argv[arg]) == 2) gppdebug = 1;
        else if (strlen(argv[arg]) == 3 && argv[arg][2] == '1') dfadebug = 1;
        else if (strlen(argv[arg]) == 3 && argv[arg][2] == '2') dfadebug = 2;
        else if (strlen(argv[arg]) == 3 && argv[arg][2] == '3') gppdebug = 2;
        else if (strlen(argv[arg]) == 3 && argv[arg][2] == '4') db_debug = 1; 
        else {
          gppversion(stderr);
          fprintf(stderr,gppusage,argv[0]);
          exit(1);
        }
      }
      else if (strncasecmp(argv[arg],"%%TEX",5) == 0) {
        sprintf(gppcommline,"%s%s\n",gppcommline,argv[arg]);
        argv[arg][0] = '\0';
      }
      else if (strncasecmp(argv[arg],"%%GPP",5) == 0) {
        sprintf(gppcommline,"%s%s\n",gppcommline,argv[arg]);
        argv[arg][0] = '\0';
      }
      else if (strncasecmp(argv[arg],"%%",2) == 0) {
        sprintf(gppcommline,"%s%s\n",gppcommline,argv[arg]);
        argv[arg][0] = '\0';
      }
      else if (argv[arg][0] == '-' || argv[arg][0] == '%') {
        gppversion(stderr);
        fprintf(stderr,gppusage,argv[0]);
        exit(1);
      }
      else {
        if (gpppretex[0] == '\0') strcpy(gpppretex,argv[arg]);
        else {
/*          gppversion(stderr);
          fprintf(stderr,gppusage,argv[0]);
          exit(1); */
          grg_alloc(&texclarg,texclarg_ndx);
          strncpy(GRGCLARG[texclarg_ndx++].value,argv[arg],STRMAX);
        }
      }
    }
  }
  if (gpppretex[0] == '\0') {
    gppversion(stderr);
    fprintf(stderr,gppusage,argv[0]);
    exit(1);
  }

  /*
   * Parse PreTeX file name for extension, start building output filename
   */

  if (strncmp(gpppretex+(strlen(gpppretex)-4),".grg",4) == 0) {
    strncpy(gppoutput,gpppretex,strlen(gpppretex)-4);
    gppoutput[strlen(gpppretex)-4] = '\0';
  }
  else if (strncmp(gpppretex+(strlen(gpppretex)-4),".gpp",4) == 0) {
    strncpy(gppoutput,gpppretex,strlen(gpppretex)-4);
    gppoutput[strlen(gpppretex)-4] = '\0';
  }
  else if (strncmp(gpppretex+(strlen(gpppretex)-3),".pt",3) == 0) {
    strncpy(gppoutput,gpppretex,strlen(gpppretex)-3);
    gppoutput[strlen(gpppretex)-3] = '\0';
  }
  else {
    strcpy(gppoutput,gpppretex);
  }

  /*
   * Initialise built in equates to null
   */
  
  grg_define_equate(EQ_TEXINIT);    /* deprecated */
  grg_define_equate(EQ_INIT);
  grg_define_equate(EQ_PRETEXHEADER);   /* deprecated */
  grg_define_equate(EQ_PRE_HEADER);
  grg_define_equate(EQ_PRETEXFOOTER);   /* deprecated */
  grg_define_equate(EQ_PRE_FOOTER);
  grg_define_equate(EQ_PRETEXPAGE01);   /* deprecated */
  grg_define_equate(EQ_PRE_PAGE01);
  grg_define_equate(EQ_PRETEXPAGENN);   /* deprecated */
  grg_define_equate(EQ_PRE_PAGENN);
  grg_define_equate(EQ_PRETEXBANNER);   /* deprecated */
  grg_define_equate(EQ_PRE_BANNER);
  grg_define_equate(EQ_PRETEXRECORD);   /* deprecated */
  grg_define_equate(EQ_PRE_RECORD);
  grg_define_equate(EQ_PRETEXBLOCK);    /* deprecated */
  grg_define_equate(EQ_PRE_BLOCK);
  grg_define_equate(EQ_PSTTEXHEADER);   /* deprecated */
  grg_define_equate(EQ_POST_HEADER);
  grg_define_equate(EQ_PSTTEXFOOTER);   /* deprecated */
  grg_define_equate(EQ_POST_FOOTER);
  grg_define_equate(EQ_PSTTEXPAGE01);   /* deprecated */
  grg_define_equate(EQ_POST_PAGE01);
  grg_define_equate(EQ_PSTTEXPAGENN);   /* deprecated */
  grg_define_equate(EQ_POST_PAGENN);
  grg_define_equate(EQ_PSTTEXBANNER);   /* deprecated */
  grg_define_equate(EQ_POST_BANNER);
  grg_define_equate(EQ_PSTTEXRECORD);   /* deprecated */
  grg_define_equate(EQ_POST_RECORD);
  grg_define_equate(EQ_PSTTEXBLOCK);    /* deprecated */
  grg_define_equate(EQ_POST_BLOCK);
  grg_define_equate(EQ_TEXEXIT);    /* deprecated */
  grg_define_equate(EQ_EXIT);
  grg_define_equate(EQ_PRE_DATABASE);
  grg_define_equate(EQ_POST_DATABASE);
  grg_define_equate(EQ_ARGS);

  /*
   * Get the current time into a dBase date format
   */

  clock = time(NULL);
  tm_time = localtime(&clock);
  /* We really want "%Y%m%d" in one command but cannot do this as SCCS treats
  the <percent>Y<percent> sequence as an ID keyword, and there seems to be no
  way to escape this behaviour */
  (void) strftime(date_now,sizeof(date_now),"%Y",tm_time);
  (void) strftime(temp,sizeof(date_now),"%m%d",tm_time);
  (void) strcat(date_now,temp);
  (void) strftime(time_now,sizeof(time_now),"%T",tm_time);

  /*
   * Initialise built in system variables
   */

  putsysvar(_EQ_TRACE,EQ_NUM,0);
  putsysvar(_EQ_VERBOSE,EQ_NUM,1);
  putsysvar(_EQ_VERSION,EQ_NUM,1);
  putsysvar(_EQ_CLOCK,EQ_NUM,clock);
  putsysvar(_EQ_DATENOW,EQ_DATE,date_now);
  putsysvar(_EQ_TIMENOW,EQ_STR,time_now);
  putsysvar(_EQ_PTFILE,EQ_STR,gpppretex);     /* deprecated */
  putsysvar(_EQ_FILE,EQ_STR,gpppretex);
  putsysvar(_EQ_TEXBASE,EQ_STR,gppoutput);    /* deprecated */
  putsysvar(_EQ_BASE,EQ_STR,gppoutput);

  /*
   * Prepend any command line arguments to the .pt file
   */

  if (gppcommline[0] != '\0') gpppushback(gppcommline);
  
  /*
   * Initialise dynamic memory structures
   */
  
  grg_alloc_init(&texbuffer);
  grg_alloc_init(&texheader);
  grg_alloc_init(&texfooter);
  grg_alloc_init(&texrecord);
  grg_alloc_init(&texpage01);
  grg_alloc_init(&texpagenn);

  /*
   * Parse the PreTeX (*.pt) file
   */

  gppparse(gpppretex);

  /*
   * Reverse equate definitions where neccessary (include definition)
   */
  
  /*
   * WARNING WARNING NO CHECKS MADE ON SIZE WHEN WRITING INTO TEXBUFFER
   * MUST HAVE HAD AT LEAST ONE TEXT BODY TO INIT OR WILL CRASH
   * * WHAT IF EQUAT EBODY VALUE UNINITIALISED AT TEST POINTS BELOW?
   */

  for (ctr = 0; ctr < equatetable_ndx; ctr++) {
    if (GRGEQUATE[ctr].guile == 1) {
#ifdef GUILE
      grg_sprintf(&texbuffer,"(define (%s %s) %s)",GRGEQUATE[ctr].name,
        GRGEQUATE[ctr].definition.value,GRGEQUATE[ctr].body.value);
      gh_eval_str(texbuffer.value);
#endif
    }
    else if (GRGEQUATE[ctr].body.value[0] != '\0'
        || GRGEQUATE[ctr].rev == 1) {
      grg_reverse(&GRGEQUATE[ctr].definition,&texbuffer);
      grg_strcpy(&GRGEQUATE[ctr].definition,texbuffer.value);
      grg_reverse(&GRGEQUATE[ctr].body,&texbuffer);
      grg_strcat(&GRGEQUATE[ctr].definition,texbuffer.value);
      GRGEQUATE[ctr].rev = 1;
    }
  }
  
  /*
   * Initialise equate stack data
   */
  
  eqstack_init();

  /*
   * Process custom command line arguments and reparse
   */

  if (texclarg_ndx > 0) {
    gppcommline[0] = '\0';
    for (ctr = 0; ctr < texclarg_ndx; ctr++) {
      putsysvar(_EQ_CLARG,EQ_STR,GRGCLARG[ctr].value);
      gppequate(EQ_ARGS,0,0,0);
      if (eqstack_ndx > 1) gpperror("stack overflow",NULL);
      else if (eqstack_ndx == 1)
        sprintf(gppcommline,"%s%s\n",gppcommline,eqstack[--eqstack_ndx].data.value);
    }
    if (gppcommline[0] != '\0') {
      gpppushback(gppcommline);
      gppparse(gpppretex);
    }
  }

  /*
   * Update any environment settings
   */

  for (ctr = 0; ctr < macrotable_ndx; ctr++)
    if (strcmp(GRGMACRO[ctr].name,"NPAGE") == 0)
      strcpy(texnewpage,GRGMACRO[ctr].definition);
    else if (strcmp(GRGMACRO[ctr].name,"FESCSUB") == 0)
      texfescsub = 1;
    else if (strcmp(GRGMACRO[ctr].name,"TEXEXT") == 0)
      strcpy(gpptexext,GRGMACRO[ctr].definition);
    else if (strcmp(GRGMACRO[ctr].name,"DELIM") == 0) {
      strcpy(texdelimiter,GRGMACRO[ctr].definition);
      if (strcmp(texdelimiter,"") != 0) {
        for (i = 0; strcmp(pattern[i].token,"<del>") != 0; i++);
        if (strlen(texdelimiter) > 1)
          strcpy(pattern[i].char_mcmap,texdelimiter);
        else {
          pattern[i].char_scmap = texdelimiter[0];
          pattern[i].char_mcmap[0] = '\0';
        }
      }
    }
    else if (strcmp(GRGMACRO[ctr].name,"DFAMODE") == 0)
      strcpy(texdfamode,GRGMACRO[ctr].definition);
    else if (strcmp(GRGMACRO[ctr].name,"CONCAT") == 0)
      texconcat = 1;
    else if (strcmp(GRGMACRO[ctr].name,"PHYSDB") == 0)
      strcpy(texphysdb,GRGMACRO[ctr].definition);
    else if (strcmp(GRGMACRO[ctr].name,"DBHOSTNM") == 0)
      strcpy(texdbhostnm,GRGMACRO[ctr].definition);
    else if (strcmp(GRGMACRO[ctr].name,"DBUSERNM") == 0)
      strcpy(texdbusernm,GRGMACRO[ctr].definition);
    else if (strcmp(GRGMACRO[ctr].name,"DBPASSWD") == 0)
      strcpy(texdbpasswd,GRGMACRO[ctr].definition);
    else if (strcmp(GRGMACRO[ctr].name,"NAMCOL") == 0)
      texnamcol = 1;
    else if (strcmp(GRGMACRO[ctr].name,"DEFCOL") == 0)
      texdefcol = 1;
    else if (strcmp(GRGMACRO[ctr].name,"NULL") == 0)
      strcpy(texnull,GRGMACRO[ctr].definition);
    else if (strcmp(GRGMACRO[ctr].name,"MKDIR") == 0)
      texmkdir = 1;
    else if (strcmp(GRGMACRO[ctr].name,"EXPAND") == 0)
      texexpand = 1;

  /*
   * Complete building the output file name
   */

  strcat(gppoutput,gpptexext);

  /*
   * Initialise the output file name built in system variables
   */

  putsysvar(_EQ_TEXEXT,EQ_STR,gpptexext);   /* deprecated */
  putsysvar(_EQ_EXTN,EQ_STR,gpptexext);
  putsysvar(_EQ_OUTFILE,EQ_STR,gppoutput);

  /*
   * Initialise the master dbf file name built in system variables
   */

  if (texdbffile_ndx > 0) {
    for (ctr = 0; ctr < texdbffile_ndx && !GRGDATA[ctr].master; ctr++);
    if (ctr == texdbffile_ndx) ctr = 0;
    putsysvar(_EQ_DBFPATH,EQ_STR,GRGDATA[ctr].path);
    putsysvar(_EQ_DBFNAME,EQ_STR,GRGDATA[ctr].name);
    putsysvar(_EQ_DBFTYPE,EQ_STR,GRGDATA[ctr].type);
  }
  else gpperror("no database defined",NULL);

#ifdef GUILE

  /*
   * Install GUILE side GURGLE procedures
   */

  gh_new_procedure1_0("grg_equate",guile_equate);
  gh_new_procedure1_0("grg_getstrsysvar",guile_getstrsysvar);
  gh_new_procedure1_0("grg_getnumsysvar",guile_getnumsysvar);
  gh_new_procedure2_0("grg_putstrsysvar",guile_putstrsysvar);
  gh_new_procedure2_0("grg_putnumsysvar",guile_putnumsysvar);
  gh_new_procedure3_0("grg_getfield",guile_getfield);

  /*
   * Install GURGLE equates as GUILE procedures
   * (only those with no args will work from GUILE)
   */
  
  /*
   * WARNING WARNING - ALSO NO CHECK ON BUFFER SIZE
   */

  for (ctr = 0; ctr < equatetable_ndx; ctr++) {
    if (GRGEQUATE[ctr].guile != 1) {
      grg_sprintf(&texbuffer,"(define (%s) (grg_equate \"%s\"))",
        GRGEQUATE[ctr].name,GRGEQUATE[ctr].name);
      gh_eval_str(texbuffer.value);
    }
  }

#endif

  /*
   * Interpret the initialisation equate with no database defined
   */

  gppequate(EQ_TEXINIT,0,0,0);    /* deprecated */
  gppequate(EQ_INIT,0,0,0);

  /*
   * Update the output file name, as the texinit equate may have changed it
   */

  strcpy(gppoutput,getstrsysvar(_EQ_OUTFILE));

  /*
   * Update the master dbf file name, as the texinit equate may have changed it
   */

  for (ctr = 0; ctr < texdbffile_ndx && !GRGDATA[ctr].master; ctr++);
  if (ctr == texdbffile_ndx) ctr = 0;
  putsysvar(_EQ_DBFPATH,EQ_STR,GRGDATA[ctr].path);
  putsysvar(_EQ_DBFNAME,EQ_STR,GRGDATA[ctr].name);
  putsysvar(_EQ_DBFTYPE,EQ_STR,GRGDATA[ctr].type);

  /*
   * Display the program title and copyright banner
   */

  if (getsysvar(_EQ_VERSION))
    gppversion(stdout);

#ifdef GUILE

  /*
   * Display the GUILE version banner
   */

  if (getsysvar(_EQ_VERSION))
  {
    gh_eval_str("(display \"Loaded GUILE \")");
    gh_eval_str("(display (version)) (display \" \")");
    gh_eval_str("(display (libguile-config-stamp))");
    gh_eval_str("(newline)");
  }

#endif

  /*
   * Say what we're going to do
   */

  if (getsysvar(_EQ_VERBOSE))
    fprintf(stdout,"Running: %s --> %s\n",gpppretex,gppoutput);

  /*
   * Open output file (*.pt --> *.tex)
   */

  if (strcmp(gppoutput,"-") == 0) gpp_out = stdout;
  else {
    if (texmkdir) makepathfor(gppoutput);
    if (texconcat) {
      if ((gpp_out = fopen(gppoutput,"a")) == NULL) {
        fprintf(stderr,"gpp: couldn't open \"%s\" (concat): ",gppoutput);
        perror("");
        exit(1);
      }
    }
    else {
      if ((gpp_out = fopen(gppoutput,"w")) == NULL) {
        fprintf(stderr,"gpp: couldn't open \"%s\": ",gppoutput);
        perror("");
        exit(1);
      }
    }
  }

  /*
   * If in debug mode print results of this parse
   */
  
  grg_debug();

  /*
   * Open the source databases so as field contents can be accessed
   */

  sc = texsqlfile_ndx;
  if (gppdebug && texexpand) fprintf(stderr,"EXPANSIONS:\n");
  for (ctr = 0; ctr < texdbffile_ndx; ctr++) {
    if (GRGDATA[ctr].name[0] != '\0') {
      putsysvar(_EQ_DB_NAME,EQ_STR,GRGDATA[ctr].name);
      putsysvar(_EQ_DB_LIMIT,EQ_NUM,0);
      grg_write_equate(EQ_PRE_DATABASE,db,0);
      strcpy(filename,GRGDATA[ctr].path);
      strcat(strcat(filename,GRGDATA[ctr].name),GRGDATA[ctr].type);
      if (strcmp(GRGDATA[ctr].type,".dbf") == 0) {
        if (dbOpen(filename,&db,0)) {
          gpperror("couldn't open dbf file",filename);
        }
      }
#ifdef SQL
      else if (strcmp(GRGDATA[ctr].type,".sql") == 0) {
        if (texexpand) {
          datumdef bodydef = { NULL, 0, STRMAX, sizeof(char), "body" }, *body = &bodydef;
          grg_expand(&GRGDATA[ctr].body,0,0,body);
          grg_strcpy(&GRGDATA[ctr].body,body->value);
          if (gppdebug)
            fprintf(stderr,"%d: %s%s%s\n%s",ctr,GRGDATA[ctr].path,
              GRGDATA[ctr].name,GRGDATA[ctr].type,GRGDATA[ctr].body.value);
          grg_dealloc(body);
        }
        if (sc == 1) {
          if (texsqlfile_ndx == 1) sm = 3;
          else sm = 2;
        }
        else if (sc > 1) {
          if (texsqlfile_ndx == sc) sm = 1;
          else sm = 0;
        }
        sc--;
        if (gppSQLOpen(GRGDATA[ctr].name,GRGDATA[ctr].body.value,&db,sm)) {
          gpperror("couldn't open dynamic SQL dbf file",filename);
        }
      }
#endif
      else {
        if (gppDynamicOpen(filename,&db)) {
          gpperror("couldn't open dynamic dbf file",filename);
        }
        if (gppDynamicFill(db)) {
          gpperror("couldn't fill dynamic dbf file",filename);
        }
        if (gppDynamicStop(db)) {
          gpperror("couldn't stop dynamic dbf file",filename);
        }
      }
      GRGDATA[ctr].db = db;
      putsysvar(_EQ_DB_LIMIT,EQ_NUM,db->dhead->db_nrecs);
      grg_write_equate(EQ_POST_DATABASE,db,0);
    }
    else {
      GRGDATA[ctr].db = 0;
      break;
    }
  }
  for (ctr = 0; ctr < texdbffile_ndx && !GRGDATA[ctr].master; ctr++);
  if (ctr == texdbffile_ndx) master_db = GRGDATA[0].db;
  else master_db = GRGDATA[ctr].db;

  /*
   * Build the default record body (all fields delimited) and header body
   * (field names delimited) if none has been defined
   */
  
  /*
   * WARNING - THESE ASSUME TEXHEADER HAS BEEN DEFINED WITH SOME SPACE
   * WILL CRASH IF THIS IS NOT THE CASE
   */

  if (!gpprecord) {
    for (i = 0; strcmp(pattern[i].token,"<del>") != 0; i++);
    for (ctr = 0; ctr < master_db->dnfld; ctr++) {
      if (ctr > 0) {
        if (pattern[i].char_scmap) {
          grg_sprintf(&texrecord,"%s%c",texrecord.value,pattern[i].char_scmap);
          if (!gppheader)
            grg_sprintf(&texheader,"%s%c",texheader.value,pattern[i].char_scmap);
        }
        else
        {
          grg_strncat(&texrecord,pattern[i].char_mcmap,1);
          if (!gppheader)
            grg_strncat(&texheader,pattern[i].char_mcmap,1);
        }
      }
      grg_sprintf(&texrecord,"%s%%%s",texrecord.value,(master_db->dfrec+ctr)->fl_fname);
      if (!gppheader)
        grg_sprintf(&texheader,"%s%s",texheader.value,(master_db->dfrec+ctr)->fl_fname);
    }
    grg_strcat(&texrecord,"\n");
    if (!gppheader) grg_strcat(&texheader,"\n");
  }

  /*
   * Set system variable to number of records in master database
   */

  putsysvar(_EQ_TOTREC,EQ_NUM,master_db->dhead->db_nrecs);

  /*
   * Group the records internally according to the %%TEXGROUP list
   */

  /* ??? */

  /*
   * Sort the records internally according to the %%TEXSORTON list
   */

  gppmainsort(master_db);

  /*
   * Write out the pre-processed header
   */
  
  /* WARNING - WOULD CRASH WITH NO HEADER DEFINED */

  grg_write_equate(EQ_PRETEXHEADER,master_db,1);   /* deprecated */
  grg_write_equate(EQ_PRE_HEADER,master_db,1);
  if (texheader.value[0] != '\0')
    gppwrite(texheader.value,master_db,1,gpp_out);
  grg_write_equate(EQ_PSTTEXHEADER,master_db,1);   /* deprecated */
  grg_write_equate(EQ_POST_HEADER,master_db,1);


  /*
   * Write out the pre-processed records
   */

  gpppage1 = 1;
  mode = 0;
  gpppage = 0;
  grg_write_equate(EQ_PRETEXPAGE01,master_db,1);   /* deprecated */
  grg_write_equate(EQ_PRE_PAGE01,master_db,1);
  if (texpage01.value[0] != '\0')
    gppwrite(texpage01.value,master_db,1,gpp_out);
  grg_write_equate(EQ_PSTTEXPAGE01,master_db,1);   /* deprecated */
  grg_write_equate(EQ_POST_PAGE01,master_db,1);
  for (record = prec = 1; record <= master_db->dhead->db_nrecs; record++) {
    if (!gppfilter(master_db,record)) continue;
    putsysvar(_EQ_RECNUM,EQ_NUM,record);
    if (gpppage) {
      gpppage = 0;
      fprintf(gpp_out,texnewpage);
      grg_write_equate(EQ_PRETEXPAGENN,master_db,record);   /* deprecated */
      grg_write_equate(EQ_PRE_PAGENN,master_db,record);
      if (texpagenn.value[0] != '\0')
        gppwrite(texpagenn.value,master_db,record,gpp_out);
      grg_write_equate(EQ_PSTTEXPAGENN,master_db,record);   /* deprecated */
      grg_write_equate(EQ_POST_PAGENN,master_db,record);
    }
    mode = gppbanner(master_db,record,mode,gpp_out);
    grg_write_equate(EQ_PRETEXRECORD,master_db,record);   /* deprecated */
    grg_write_equate(EQ_PRE_RECORD,master_db,record);
    if (texrecord.value[0] != '\0')
      gppwrite(texrecord.value,master_db,record,gpp_out);
    grg_write_equate(EQ_PSTTEXRECORD,master_db,record);   /* deprecated */
    grg_write_equate(EQ_POST_RECORD,master_db,record);
    if (gpppage1 && texpage01_val != 0 && prec % texpage01_val == 0) {
      gpppage1 = 0;
      gpppage = 1;
    }
    else if (!gpppage1 && texpage01_val != 0 && (prec-texpage01_val) % texpagenn_val == 0) {
      gpppage = 1;
    }
    prec++;
  }

  /*
   * Write out the pre-processed footer
   */

  grg_write_equate(EQ_PRETEXFOOTER,master_db,master_db->dhead->db_nrecs);   /* deprecated */
  grg_write_equate(EQ_PRE_FOOTER,master_db,master_db->dhead->db_nrecs);
  if (texfooter.value[0] != '\0')
    gppwrite(texfooter.value,master_db,master_db->dhead->db_nrecs,gpp_out);
  grg_write_equate(EQ_PSTTEXFOOTER,master_db,master_db->dhead->db_nrecs);   /* deprecated */
  grg_write_equate(EQ_POST_FOOTER,master_db,master_db->dhead->db_nrecs);

  /*
   * Interpret the exit equate
   */

  grg_write_equate(EQ_TEXEXIT,master_db,master_db->dhead->db_nrecs);    /* deprecated */
  grg_write_equate(EQ_EXIT,master_db,master_db->dhead->db_nrecs);

  /*
   * Close the output file
   */

  fclose(gpp_out);

  /*
   * Time to go byebye (make sure we exit with a zero code)
   */

  exit(0);

}

main(argc,argv) int argc; char *argv[];
{
#ifdef GUILE
  gh_enter(argc,argv,grg_main);
#else
  grg_main(argc,argv);
#endif
}

/****************************************************************************/
