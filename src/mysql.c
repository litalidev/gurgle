/* $Id: mysql.c,v 1.5 2001/05/01 10:39:31 timc Exp $
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
 * GURGLE MYSQL HANDLER
 * Contributed by: Stuart Whitman <swhitman@s3i.com>
 * TEC modified
 */

#include <grg.h>
 
#ifdef SQL
#ifdef ADD_MYSQL

#include <mysql/mysql.h>

MYSQL *sock;

/*
 * Run variable 'select' statement on an MySQL RDBMS and load as database
 * Retains persistent connection for multiple queries,
 * Ie. m = 0 = query, m = 1 = open and query, m = 2 = query and shut,
 * m = 3 = open and query and shut
 */

int gppSQLOpen(nm,qry,da,m) char *nm, *qry; dbase **da; unsigned char m;
{
  int i, r, j, ptr = 0;
  dbase *db;
  char fname[32];

  MYSQL_RES *	results;
  MYSQL_FIELD *	f;
  int		numRows;
  MYSQL_ROW	row;

  if (m == 1 || m == 3) {
    char *mydb = NULL, *myhost = NULL, *myuser = NULL, *mypass = NULL;
    if (strcmp(texphysdb,"")) mydb = texphysdb;
    if (strcmp(texdbhostnm,"")) myhost = texdbhostnm;
    if (strcmp(texdbusernm,"")) myuser = texdbusernm;
    if (strcmp(texdbpasswd,"")) mypass = texdbpasswd;

    sock = mysql_init(NULL);
    if (!mysql_real_connect(sock,myhost,myuser,mypass,mydb,NULL,NULL,NULL)) {
      printf ("%s\n",mysql_error(sock));
      return (db_FOP);
    }
  }

  if (!strcmp(qry,"")) {
    strcpy(qry,"SELECT * FROM ");
    strcat(qry,nm);
  }

  numRows = mysql_query (sock, qry);
  if (numRows < 0)
  {
    printf ("%s\n", mysql_error(sock));
    return(db_FOP);
  }

  results = mysql_store_result (sock);

  if (!(db = (dbase *) calloc(1,sizeof(dbase)))) return(db_MEM);
  *da = db;
  db->dname = nm;
  db->dfile = NULL;
  db->dnfld = mysql_num_fields (results);
  db->dnrec = 1;
  db->dtrec = 0;
  if (!(db->dhead = (db_head *) calloc(1,sizeof(db_head)))) return(db_MEM);
  db->dhead->db_nrecs = db->dtrec;
  if (!(db->dfrec = (db_field *) calloc(db->dnfld,sizeof(db_field))))
    return(db_MEM);

  for (i = 0; i < db->dnfld; i++)
  {
    f = mysql_fetch_field (results);
    if (texnamcol)
    {
      for (j = 0; j < strlen (f->name) && j < 10; j++)
      {
        db->dfrec[i].fl_fname[j] = toupper(f->name[j]);
      }
      db->dfrec[i].fl_fname[j] = '\0';
    }
    else
    {
      sprintf(fname,"U%03d",i+1);
      strcpy(db->dfrec[i].fl_fname,fname);
    }

/* FIELD types from mysql.h
 * enum enum_field_types { FIELD_TYPE_DECIMAL, FIELD_TYPE_TINY,
 *                    FIELD_TYPE_SHORT,  FIELD_TYPE_LONG,
 *                    FIELD_TYPE_FLOAT,  FIELD_TYPE_DOUBLE,
 *                    FIELD_TYPE_NULL,   FIELD_TYPE_TIMESTAMP,
 *                    FIELD_TYPE_LONGLONG,FIELD_TYPE_INT24,
 *                    FIELD_TYPE_DATE,   FIELD_TYPE_TIME,
 *                    FIELD_TYPE_DATETIME, FIELD_TYPE_YEAR,
 *                    FIELD_TYPE_NEWDATE,
 *                    FIELD_TYPE_ENUM=247,
 *                    FIELD_TYPE_SET=248,
 *                    FIELD_TYPE_TINY_BLOB=249,
 *                    FIELD_TYPE_MEDIUM_BLOB=250,
 *                    FIELD_TYPE_LONG_BLOB=251,
 *                    FIELD_TYPE_BLOB=252,
 *                    FIELD_TYPE_VAR_STRING=253,
 *                    FIELD_TYPE_STRING=254
 */
    switch (f->type) {
      case FIELD_TYPE_DECIMAL:
      case FIELD_TYPE_TINY:
      case FIELD_TYPE_SHORT:
      case FIELD_TYPE_LONG:
      case FIELD_TYPE_LONGLONG:
      case FIELD_TYPE_INT24:
        db->dfrec[i].fl_fsize = 12;
        db->dfrec[i].fl_ftype = 'N';
	break;

      case FIELD_TYPE_FLOAT:
      case FIELD_TYPE_DOUBLE:
        db->dfrec[i].fl_fsize = 14;
        db->dfrec[i].fl_ftype = 'N';
	break;

      case FIELD_TYPE_STRING:
      case FIELD_TYPE_VAR_STRING:
        db->dfrec[i].fl_fsize = f->length;
        db->dfrec[i].fl_ftype = 'C';
	break;

      default:
        db->dfrec[i].fl_fsize = f->length;
        db->dfrec[i].fl_ftype = 'C';
        break;
    }

  }

  for (i = 0; i < db->dnfld; i++)
    db->dnrec += db->dfrec[i].fl_fsize;
  if (!(db->drrec = (char *) realloc(NULL,1))) return(db_MEM);

  row = mysql_fetch_row (results);
  while (row != NULL) {
    db->dtrec++;
    if (!(db->drrec = (char *) realloc(db->drrec,db->dtrec*db->dnrec+1)))
      return(db_MEM);

    for (i = 0; i < db->dnfld; i++)
    {
      if (row[i] == NULL)
      {
        sprintf(&db->drrec[ptr],"%-.*s",db->dfrec[i].fl_fsize,texnull);
      }
      else
      {
        switch (db->dfrec[i].fl_ftype)
        {
          case 'N':
            sprintf(&db->drrec[ptr],"%*s",db->dfrec[i].fl_fsize,row[i]);
            break;

          case 'C':
            sprintf(&db->drrec[ptr],"%-*s",db->dfrec[i].fl_fsize,row[i]);
            break;
        }
      }
      ptr += db->dfrec[i].fl_fsize;
    }
    db->drrec[ptr++] = ' ';
    
    row = mysql_fetch_row (results);
  }

  db->drrec[ptr-1] = 0x1A;
  db->dhead->db_nrecs = db->dtrec;
  mysql_free_result (results);

  if (m == 2 || m == 3) {
    mysql_close (sock);
  }

  return(0);
}

#endif
#endif

/****************************************************************************/
