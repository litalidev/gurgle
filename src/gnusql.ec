/* $Id: gnusql.ec,v 1.5 2001/04/30 09:31:34 timc Exp $
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
 * GURGLE GNUSQL SERVER HANDLER
 */

#include <grg.h>
 
#ifdef SQL
#ifdef GNUSQL

#include <gnusql/dyn_funcs.h>

#define CHECK(st) if((rc=SQL__##st)!=0){error_rprt(#st,rc,statement_scanner_buffer); goto error; }

static char *statement_scanner_buffer=NULL;

/*
 * Run variable 'select' statement on an GNU SQL Server and load as database
 */

int gppSQLOpen(nm,qry,da,m) char *nm, *qry; dbase **da; unsigned char m;
{
  int l, rc, i, r, j, ptr = 0, stmt = 0;
  dbase *db;
  char fname[32];

  char *b;

  SQL_DESCR in, out;

  sql_descr_element_t *sqv;

  char dbname[51];
  static char stmt_buffer[4096];
  statement_scanner_buffer = stmt_buffer;

  if (!strcmp(qry,"")) {
    strcpy(qry,"SELECT * FROM ");
    strcat(qry,nm);
  }
  strcpy(statement_scanner_buffer,qry);

  l = strlen(statement_scanner_buffer);

  /* put select in parenthesys */
  b = malloc (l+3);
  b[0] = '(';
  strcpy(b+1,statement_scanner_buffer);
  strcat(b+1+l,")");
  statement_scanner_buffer = b;
  
  b = malloc(20);
  sprintf(b,"%dth Stmt",stmt++);

  CHECK(prepare(b,statement_scanner_buffer));
  CHECK(allocate_cursor(b,"CURSOR"));
  in  = SQL__allocate_descr("IN",0);
  out = SQL__allocate_descr("OUT",0);

  CHECK(describe(b,1,in));
  CHECK(describe(b,0,out));

  if (!(db = (dbase *) calloc(1,sizeof(dbase)))) return(db_MEM);
  *da = db;
  db->dname = nm;
  db->dfile = NULL;
  db->dnfld = out->count;
  db->dnrec = 1;
  db->dtrec = 0;
  if (!(db->dhead = (db_head *) calloc(1,sizeof(db_head)))) return(db_MEM);
  db->dhead->db_nrecs = db->dtrec;
  if (!(db->dfrec = (db_field *) calloc(db->dnfld,sizeof(db_field))))
    return(db_MEM);
  for (i = 0; i < out->count; i++)
  {
    sqv = &out->values[i];
    if (texnamcol && !sqv->unnamed)
    {
      for (j = 0; j < strlen(sqv->name) && j < 10; j++)
      {
        db->dfrec[i].fl_fname[j] = toupper(sqv->name[j]);
      }
      db->dfrec[i].fl_fname[j] = '\0';
    }
    else
    {
      sprintf(fname,"U%03d",i+1);
      strcpy(db->dfrec[i].fl_fname,fname);
    }
    switch(sqv->type)
    { 
      case SQL__Int:
      case SQL__Short:
        db->dfrec[i].fl_fsize = 12;
        db->dfrec[i].fl_ftype = 'N';
        break;
      case SQL__Double:
      case SQL__Float:
      case SQL__Real:
        db->dfrec[i].fl_fsize = 14;
        db->dfrec[i].fl_ftype = 'N';
        break;
      case SQL__CharVar:
      case SQL__Char:
      default:
        /*if (sqv->length > 0xFF) db->dfrec[i].fl_fsize = 0xFF;
        else */db->dfrec[i].fl_fsize = sqv->length;
        db->dfrec[i].fl_ftype = 'C';
        break;
    }
    sqv->indicator = 0;
  }
  for (i = 0; i < db->dnfld; i++)
    db->dnrec += db->dfrec[i].fl_fsize;
  if (!(db->drrec = (char *) realloc(NULL,1))) return(db_MEM);

  if(in->count)
  {
    CHECK(open_cursor("CURSOR",in));
  }
  else
  {
    CHECK(open_cursor("CURSOR",NULL));
  }

  while (1) {
    CHECK(fetch("CURSOR",out));
    if(SQLCODE != 0) break;
    db->dtrec++;
    if (!(db->drrec = (char *) realloc(db->drrec,db->dtrec*db->dnrec+1)))
      return(db_MEM);
    for (i = 0; i < out->count; i++)
    {
      sqv = &out->values[i];
      if (sqv->indicator < 0 && sqv->nullable)
      {
        sprintf(&db->drrec[ptr],"%-.*s",db->dfrec[i].fl_fsize,texnull);
      }
      else
      {
        switch (sqv->type)
        {
          case SQL__Int:
            sprintf(&db->drrec[ptr],"%*d",db->dfrec[i].fl_fsize,*(int *)sqv->data);
            break;
          case SQL__Short:
            sprintf(&db->drrec[ptr],"%*d",db->dfrec[i].fl_fsize,(int)*((short*)(sqv->data)));
            break;
          case SQL__Double:
          case SQL__Float:
            sprintf(&db->drrec[ptr],"%*g",db->dfrec[i].fl_fsize,*(double *)sqv->data);
            break;
          case SQL__Real:
            sprintf(&db->drrec[ptr],"%*g",db->dfrec[i].fl_fsize,*(float *)sqv->data);
            break;
          case SQL__Char:
          case SQL__CharVar:
          default:
            sprintf(&db->drrec[ptr],"%-*s",db->dfrec[i].fl_fsize,(char *)sqv->data);
            break;
        }
      }
      ptr += db->dfrec[i].fl_fsize;
    }
    db->drrec[ptr++] = ' ';
  }
  CHECK(close_cursor("CURSOR"));
  CHECK(deallocate_prepare(b));
  db->drrec[ptr-1] = 0x1A;
  db->dhead->db_nrecs = db->dtrec;
  _SQL_rollback();

error:
  SQL__deallocate_descr(&in);
  SQL__deallocate_descr(&out);
  xfree(b);
  b = NULL;
  return(0);
}

error_rprt(char *st,int rc, char *stmt)
{
  fprintf(stderr,"\n#### Error occured in '%s'\n%s\nat \"%s\"\n",
          stmt,gsqlca.errmsg,st);
  exit(1);
}

#endif
#endif

/****************************************************************************/
