/* $Id: ingres.sc,v 1.7 2001/04/30 09:33:05 timc Exp $
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
 * GURGLE INGRES HANDLER
 */

#include <grg.h>
 
#ifdef SQL
#ifdef INGRES

exec sql include sqlca;
exec sql include sqlda;

exec sql declare stmt statement;

/*
 * Run variable 'select' statement on an Ingres RDBMS and load as database
 * Retains persistent connection for multiple queries,
 * Ie. m = 0 = query, m = 1 = open and query, m = 2 = query and shut,
 * m = 3 = open and query and shut
 */

int gppSQLOpen(nm,qry,da,m) char *nm, *qry; dbase **da; unsigned char m;
{
  int i, r, j, ptr = 0;
  dbase *db;
  char fname[32];
  IISQLDA _sqlda;
  IISQLDA *sqlda = &_sqlda;
  IISQLVAR *sqv;
  exec sql begin declare section;
  char dbname[51], username[9], password[9];
  char stmt_buffer[16384];
  exec sql end declare section;
  sqlda->sqln = IISQ_MAX_COLS;
  if (!strcmp(qry,"")) {
    strcpy(qry,"SELECT * FROM ");
    strcat(qry,nm);
  }
  if (strlen(qry) > (sizeof(stmt_buffer)-1)) {
    printf ("Query statement buffer overflow.\n");
    return(db_FOP);
  }

  strcpy(stmt_buffer,qry);

  if (m == 1 || m == 3) {
    strncpy(dbname,texphysdb,50);
    strncpy(username,texdbusernm,8);
    strncpy(password,texdbpasswd,8);
	  if (!username[0]) {
	    if (!password[0]) {
		    exec sql connect :dbname;
      }
      else {
		    exec sql connect :dbname dbms_password = :password;
      }
    }
	  else {
	    if (!password[0]) {
		    exec sql connect :dbname identified by :username;
      }
      else {
		    exec sql connect :dbname identified by :username dbms_password = :password;
      }
    }
  }

  exec sql prepare stmt from :stmt_buffer;
  exec sql describe stmt into :sqlda;
  if (sqlca.sqlcode < 0 || sqlda->sqld == 0)
  {
    exec sql disconnect;
    return(db_FOP);
  }
  if (!(db = (dbase *) calloc(1,sizeof(dbase)))) return(db_MEM);
  *da = db;
  db->dname = nm;
  db->dfile = NULL;
  db->dnfld = sqlda->sqld;
  db->dnrec = 1;
  db->dtrec = 0;
  if (!(db->dhead = (db_head *) calloc(1,sizeof(db_head)))) return(db_MEM);
  db->dhead->db_nrecs = db->dtrec;
  if (!(db->dfrec = (db_field *) calloc(db->dnfld,sizeof(db_field))))
    return(db_MEM);
  for (i = 0; i < sqlda->sqld; i++)
  {
    sqv = &sqlda->sqlvar[i];
    if (texnamcol)
    {
      for (j = 0; j < sqv->sqlname.sqlnamel && j < 10; j++)
      {
        db->dfrec[i].fl_fname[j] = toupper(sqv->sqlname.sqlnamec[j]);
      }
      db->dfrec[i].fl_fname[j] = '\0';
    }
    else
    {
      sprintf(fname,"U%03d",i+1);
      strcpy(db->dfrec[i].fl_fname,fname);
    }
    switch(abs(sqv->sqltype))
    { 
      case IISQ_INT_TYPE:
        sqv->sqllen = sizeof(long);
        sqv->sqldata = (char *) calloc(1,sizeof(long));
        db->dfrec[i].fl_fsize = 12;
        db->dfrec[i].fl_ftype = 'N';
        break;
      case IISQ_FLT_TYPE:
      case IISQ_MNY_TYPE:
      case IISQ_DEC_TYPE:
        sqv->sqllen = sizeof(double);
        sqv->sqldata = (char *) calloc(1,sizeof(double));
        sqv->sqltype = (sqv->sqltype / abs(sqv->sqltype)) * IISQ_FLT_TYPE;
        db->dfrec[i].fl_fsize = 14;
        db->dfrec[i].fl_ftype = 'N';
        break;
      case IISQ_DTE_TYPE:
        sqv->sqllen = 25;
      case IISQ_VCH_TYPE:
      case IISQ_CHA_TYPE:
        sqv->sqldata = (char *) calloc((sqv->sqllen)+1,sizeof(char));
        sqv->sqltype = (sqv->sqltype / abs(sqv->sqltype)) * IISQ_CHA_TYPE;
        db->dfrec[i].fl_fsize = sqv->sqllen;
        db->dfrec[i].fl_ftype = 'C';
        break;
    }
    if (sqv->sqltype < 0)
    {
      sqv->sqlind = (short *) calloc(1,sizeof(short));
    }
    else
    {
      sqv->sqlind = (short *)0;
    }
  }
  for (i = 0; i < db->dnfld; i++)
    db->dnrec += db->dfrec[i].fl_fsize;
  if (!(db->drrec = (char *) realloc(NULL,1))) return(db_MEM);
  exec sql execute immediate :stmt_buffer using descriptor :sqlda;
  exec sql begin;
    db->dtrec++;
    if (!(db->drrec = (char *) realloc(db->drrec,db->dtrec*db->dnrec+1)))
      return(db_MEM);
    for (i = 0; i < sqlda->sqld; i++)
    {
      sqv = &sqlda->sqlvar[i];
      if (sqv->sqlind && *sqv->sqlind < 0)
      {
        sprintf(&db->drrec[ptr],"%-.*s",db->dfrec[i].fl_fsize,texnull);
      }
      else
      {
        switch (abs(sqv->sqltype))
        {
          case IISQ_INT_TYPE:
            sprintf(&db->drrec[ptr],"%*d",db->dfrec[i].fl_fsize,*(long *)sqv->sqldata);
            break;
          case IISQ_FLT_TYPE:
            sprintf(&db->drrec[ptr],"%*g",db->dfrec[i].fl_fsize,*(double *)sqv->sqldata);
            break;
          case IISQ_CHA_TYPE:
            sprintf(&db->drrec[ptr],"%-*s",db->dfrec[i].fl_fsize,(char *)sqv->sqldata);
            break;
        }
      }
      ptr += db->dfrec[i].fl_fsize;
    }
    db->drrec[ptr++] = ' ';
  exec sql end;
  db->drrec[ptr-1] = 0x1A;
  db->dhead->db_nrecs = db->dtrec;
  exec sql rollback;

  if (m == 2 || m == 3) {
    exec sql disconnect;
  }

  return(0);
}

#endif
#endif

/****************************************************************************/
