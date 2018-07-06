/* $Id: postgres.c,v 1.5 2003/08/21 11:31:21 timc Exp $
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
 * GURGLE POSTGRESQL HANDLER
 * Contributed by: Anthony Symons <a.symons@citr.com.au>
 * TEC Modified
 */

#include <grg.h>
 
#ifdef SQL
#ifdef POSTGRES

#include <libpq-fe.h>
#include <ctype.h>

#define BOOLOID                 16
#define BYTEAOID                17
#define CHAROID 18
#define NAMEOID 19
#define INT2OID                 21
#define INT28OID                22
#define INT4OID                 23
#define TEXTOID 25
#define OIDOID                  26
#define FLOAT4OID 700
#define FLOAT8OID 701
#define UNKNOWNOID              705
#define CASHOID 790
#define BPCHAROID               1042
#define VARCHAROID              1043
#define DATEOID                 1082
#define TIMEOID                 1083
#define DATETIMEOID             1184
#define TIMESPANOID             1186
#define TIMESTAMPOID    1296
#define NUMERICOID		1700

/*
 * Run variable 'select' statement on an Ingres RDBMS and load as database
 * Retains persistent connection for multiple queries,
 * Ie. m = 0 = query, m = 1 = open and query, m = 2 = query and shut,
 * m = 3 = open and query and shut
 */

PGconn *connection = NULL;  

int gppSQLOpen(nm,qry,da,m) char *nm, *qry; dbase **da; unsigned char m;
{
    int i,  j, ptr = 0;
    int tuple;
    
    dbase *db;
    char fname[32];
    char stmt_buffer[16384];
    PGresult * qry_res = NULL;
 
    if (!strcmp(qry,"")) {
        strcpy(qry,"SELECT * FROM ");
        strcat(qry,nm);
    }

    if (strlen(qry) > sizeof(stmt_buffer)) gpperror("query statement buffer overflow",NULL);
    strcpy(stmt_buffer,qry);

    if (m == 1 || m == 3) {
      char *pgdb = NULL, *pghost = NULL, *pguser = NULL, *pgpass = NULL;
      if (strcmp(texphysdb,"")) pgdb = texphysdb;
      if (strcmp(texdbhostnm,"")) pghost = texdbhostnm;
      if (strcmp(texdbusernm,"")) pguser = texdbusernm;
      if (strcmp(texdbpasswd,"")) pgpass = texdbpasswd;

      /* make connection */
      if ( NULL == (connection =
                    PQsetdbLogin(pghost,NULL,NULL,NULL,pgdb,pguser,pgpass)))
      {
          /* did not connect */
          fprintf(stderr,"%s:%d: gppSQLOpen could not make connection to %s \n",
                __FILE__,__LINE__,
                pgdb);
          return(db_FOP);
      }
      if (CONNECTION_BAD == PQstatus(connection))
      {
        fprintf(stderr,"%s:%d: gppSQLOpen could not make connection %s\n",
                __FILE__,__LINE__,
                PQerrorMessage(connection));
        return(db_FOP);
      }
    }
    
    /* now execute the sql query */
    if (NULL == (qry_res = PQexec(connection, stmt_buffer)))
    {
        fprintf(stderr,"%s:%d: gppSQLOpen got null result %s \n",
                __FILE__,__LINE__,
                PQerrorMessage(connection));
        PQfinish(connection);
        
        return(db_FOP);
    }
    /* was the query ok? */
    if (!( PGRES_COMMAND_OK == PQresultStatus(qry_res) 
           || PGRES_TUPLES_OK == PQresultStatus(qry_res))
        )
    {
        fprintf(stderr,"%s:%d: gppSQLOpen  query failed %s\n",
                __FILE__,__LINE__,
                PQerrorMessage(connection));
        PQclear(qry_res);
        PQfinish(connection);
        
        return(db_FOP);
    }
		
    

    
    if (!(db = (dbase *) calloc(1,sizeof(dbase)))) return(db_MEM);
    *da = db;
    db->dname = nm;
    db->dfile = NULL;
    db->dnfld = PQnfields(qry_res);
    db->dnrec = 1;
    db->dtrec = 0;
    if (!(db->dhead = (db_head *) calloc(1,sizeof(db_head)))) return(db_MEM);
    db->dhead->db_nrecs = db->dtrec;
    if (!(db->dfrec = (db_field *) calloc(db->dnfld,sizeof(db_field))))
        return(db_MEM);
    for (i = 0; i < db->dnfld; i++)
    {
        char * qry_fname = PQfname(qry_res,i);
        
        if (texnamcol)
        {
            for (j = 0; j < strlen(qry_fname) && j < 10; j++)
            {
                db->dfrec[i].fl_fname[j] = toupper(qry_fname[j]);
            }
            db->dfrec[i].fl_fname[j] = '\0';
        }
        else
        {
            sprintf(fname,"U%03d",i+1);
            strcpy(db->dfrec[i].fl_fname,fname);
        }
        switch(PQftype(qry_res, i))
        { 
        case INT2OID:
        case INT4OID:
        case INT28OID:
        case OIDOID:
            /* int types */
            db->dfrec[i].fl_fsize = 12;
            db->dfrec[i].fl_ftype = 'N';
            break;
        


        case CASHOID:    
        case FLOAT4OID:
        case FLOAT8OID:
        case NUMERICOID:
            /* float types */
            db->dfrec[i].fl_fsize = 14;
            db->dfrec[i].fl_ftype = 'N';
            break;

        case DATEOID:
        case TIMEOID:
        case DATETIMEOID:
            /* date and time types */
            db->dfrec[i].fl_fsize = 25;
            db->dfrec[i].fl_ftype = 'C';
            break;

        case BOOLOID:       
        case BYTEAOID:
        case CHAROID:
        case NAMEOID:
        case TEXTOID:
        case VARCHAROID:
        case TIMESPANOID:
        case TIMESTAMPOID:
        
            
            db->dfrec[i].fl_fsize = PQfsize(qry_res, i);
            db->dfrec[i].fl_ftype = 'C';
            break;
        default:
            db->dfrec[i].fl_fsize = -1;
            db->dfrec[i].fl_ftype = 'C';
            break;
        }
        
    }
    /* now check if any had a size -1 */
    for (i = 0; i < db->dnfld; i++)
        if (db->dfrec[i].fl_fsize < 0)
        {
            /* find the maximum field size */
            int k,  mx = 1 /* some default? */, tmp;
            for(k = 0; k < PQntuples(qry_res); k++)
            {
                
                tmp = PQgetlength(qry_res,k,i);
                if (tmp > mx)
                    mx = tmp;
            }
            db->dfrec[i].fl_fsize = mx;
        }
    
            
            
            
    for (i = 0; i < db->dnfld; i++)
        db->dnrec += db->dfrec[i].fl_fsize;
    if (!(db->drrec = (char *) realloc(NULL,1)))
        return(db_MEM);
    for (tuple = 0; tuple < PQntuples(qry_res); tuple ++)
    {
        
        db->dtrec++;
        if (!(db->drrec = (char *) realloc(db->drrec,db->dtrec*db->dnrec+1)))
            return(db_MEM);
        for (i = 0; i < db->dnfld; i++)
        {
            char * qry_data = PQgetvalue(qry_res,tuple,i);
        
       
            if (NULL == qry_data)
            {
                if (strlen(texnull) > db->dfrec[i].fl_fsize)
                  gpperror("field length overflow",NULL);
                sprintf(&db->drrec[ptr],"%-*.*s",db->dfrec[i].fl_fsize,
                        db->dfrec[i].fl_fsize,texnull);
            }
            else
            {
           
                if (strlen(qry_data) > db->dfrec[i].fl_fsize)
                  gpperror("field length overflow",NULL);
                sprintf(&db->drrec[ptr],"%*.*s",db->dfrec[i].fl_fsize,
                        db->dfrec[i].fl_fsize,qry_data);
                
            }
            ptr += db->dfrec[i].fl_fsize;
        }
        db->drrec[ptr++] = ' ';
    }
    
    if (tuple == 0) db->drrec[ptr] = 0x1A;
    else db->drrec[ptr-1] = 0x1A;
    db->dhead->db_nrecs = db->dtrec;
    PQclear(qry_res);

    if (m == 2 || m == 3) {
      PQfinish(connection);
    }
    
    return(0);
}

#endif
#endif

/****************************************************************************/
