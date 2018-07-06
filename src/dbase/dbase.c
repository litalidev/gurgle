static char *rcsid = "@(#)$Id: dbase.c,v 1.24 2003/02/04 10:08:24 timc Exp $ Copyright (C) University of Edinburgh, 1992-1997, 2001, 2003-4, 2008-10. All rights reserved.";

/* $Id: dbase.c,v 1.24 2003/02/04 10:08:24 timc Exp $
   Copyright (C) University of Edinburgh, 1992-1997, 2001, 2003-4, 2008-10.
   All rights reserved. */                   

/*
 * DBASE - Utilities to read, write, and manipulate DBASE3+ ".dbf" files
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
#include <stdarg.h>

#include <dbase.h>

/* Macros for direct structure input and output (byte by byte) */

#ifdef WORDS_BIGENDIAN
#define dbRecv(W,X,Y,Z) \
  for (c0=0,c1=Y;c0++<Z;c1+=W) { \
    for (c00=W;c00-->0;*(c1+c00)=fgetc(X)) {} }
#define	dbSend(W,X,Y,Z) \
  for (c0=0,c1=Y;c0++<Z;c1+=W) { \
    for (c00=W;c00-->0;fputc(*(c1+c00),X)) {} }
#else
#define dbRecv(W,X,Y,Z) \
  for (c0=0,c1=Y;c0++<Z;c1+=W) { \
    for (c00=-1;++c00<W;*(c1+c00)=fgetc(X)) {} }
#define	dbSend(W,X,Y,Z) \
  for (c0=0,c1=Y;c0++<Z;c1+=W) { \
    for (c00=-1;++c00<W;fputc(*(c1+c00),X)) {} }
#endif
#define db1Recv(X,Y)    dbRecv(1,db->dfile,X,(Y/1))
#define db2Recv(X,Y)    dbRecv(2,db->dfile,X,(Y/2))
#define db4Recv(X,Y)    dbRecv(4,db->dfile,X,(Y/4))
#define db1Send(X,Y)    dbSend(1,db->dfile,X,(Y/1))
#define db2Send(X,Y)    dbSend(2,db->dfile,X,(Y/2))
#define db4Send(X,Y)    dbSend(4,db->dfile,X,(Y/4))

int db_debug = 0;

/****************************************************************************/

/* Open a DBF file  and read it into  the internal structures. All  operations
are performed internally and for any changes to have lasting effect the dbShut
function must be called. This function takes  the name of the DBF file  (which
should include  the  extension), and the  address  of  a pointer  to  a  dbase
structure which will be replaced with the address of a the created  structure.
It also takes a number representing how many additional record spaces will  be
required. The function returns non zero if an error occured. */

int dbOpen(nm,da,pl) char *nm; dbase **da; short pl; {
int nf = 0, c0, c00, cn, dp;
char *c1, dpstr[8];
long ps;
dbase *db;
    if (!(db = (dbase *) malloc(sizeof(dbase)))) return(db_MEM);
    if (!(db->dfile = fopen(nm,"r+b"))) return(db_FOP);
    db->dname = nm;
    if (db_debug) printf("Opening \"%s\"\n",db->dname);
    if (!(db->dhead = (db_head *) malloc(sizeof(db_head)))) return(db_MEM);
    db4Recv(((void *)db->dhead),sizeof(db_head));
    /* Lose this check - probably isn't a magic number anyway :-)
    if ((db->dhead->db_magcn & 0xF) != db_magic) return(db_MGC);
    */
    if (db_debug) printf("Number of records: %i\n",db->dhead->db_nrecs);
    if ((ps = ftell(db->dfile)) == -1L) return(db_LOC);
    db->dnfld = 0;
    /* An empty DBF file does not have the space, but the cr must start at a
    position which is a multiple of 32, this is safer although may break
    the memo handling */
    /*for (; !(fgetc(db->dfile)=='\r' && fgetc(db->dfile)==' '); db->dnfld++);*/
    for (; !(fgetc(db->dfile)=='\r' && db->dnfld % 32 == 0); db->dnfld++);
    db->dnfld = (db->dnfld + 2) / 32;	/* +2 to cope with possible memos */
    if (!(db->dfrec=(db_field *)calloc(db->dnfld,sizeof(db_field)))) return(db_MEM);
    if (fseek(db->dfile,ps,0)) return(db_LOC);
    if (db_debug) printf("Number of fields/record: %i\n",db->dnfld);
    if (db_debug) printf("Field   Name        Type  Size\n");
    db->dnrec = 1;
    for (cn = 0; cn < db->dnfld; cn++) {
	db1Recv(((void *)(db->dfrec+cn)),12);
	db4Recv(((void *)(&(db->dfrec+cn)->fl_foffs)),(sizeof(db_field)-12));
	/* Skirt around numeric fields with decimal point problem */
	/* Every reference to field size must mask upper byte */
	db->dnrec += (db->dfrec+cn)->fl_fsize & 0xFF;
	dp = ((db->dfrec+cn)->fl_fsize & 0xFF00) >> 8;
	if (dp > 0) sprintf(dpstr,"(.%d)",dp);
	else strcpy(dpstr,"");
	/* mask out upper byte - we never reference it */
	/* we need to do this to support >256 length fields */
	(db->dfrec+cn)->fl_fsize &= 0xFF;
	if (db_debug) {
	    printf("%-8i%-12s%-6c%-6i%s\n",cn+1,(db->dfrec+cn)->fl_fname,
		(db->dfrec+cn)->fl_ftype,(db->dfrec+cn)->fl_fsize&0xFF,dpstr);
	}
    }
    db->dtrec = db->dhead->db_nrecs + pl;
    if (db_debug) printf("Size of record (bytes): %i\n",db->dnrec);
    if (db_debug) printf("Allocated record space: %i*%i\n",db->dnrec,db->dtrec);
    if (!(db->drrec = (char *)malloc(db->dnrec*db->dtrec))) return(db_MEM);
    if (db->dhead->db_nrecs != 0) {
      cn = fgetc(db->dfile);
      for (;fgetc(db->dfile)!=' ';);
      db1Recv(((void *)db->drrec),db->dnrec*db->dhead->db_nrecs);
      if (db_debug) printf("Records in total %i bytes\n",c0-1);
      if (db_debug) printf("Completed\n");
    }
    *da = db;
    return(0);
}

/****************************************************************************/

/* Write out the  internal representation structures of  a DBF file and  close
the dbase strutures.  The arguments are  a pointer to  the dbase structure  to
close and the name of the file the output  is to be written to. If the is  the
same as the filename of the dbase struture then the data will be  overwritten.
Note that even if no  changes are made the output  DBF may be differ  slightly
from the original, since the tail end  of the file (discard) is not used.  The
function returns non-zero if an error occured. */

int dbShut(db,nm) dbase *db; char *nm; {
int cn, c0, c00;
char *c1;
    if (!db) return(db_BDB);
    if (db_debug) printf("Shutting \"%s\" as \"%s\"\n",db->dname,nm);
    if (!strcmp(db->dname,nm)) { if (fseek(db->dfile,0L,0)) return(db_LOC); }
    else if (!(db->dfile = fopen(nm,"wb"))) return(db_FOP);
    db4Send(((void *)db->dhead),sizeof(db_head));
    free(db->dhead);
    for (cn = 0; cn < db->dnfld; cn++) {
	db1Send(((void *)(db->dfrec+cn)),12);
	db4Send(((void *)(&(db->dfrec+cn)->fl_foffs)),(sizeof(db_field)-12));
    }
    free(db->dfrec);
    fputc('\r',db->dfile);
    fputc(' ',db->dfile);
    db1Send(((void *)db->drrec),db->dnrec*db->dhead->db_nrecs);
    free(db->drrec);
    if (fclose(db->dfile) == -1) return(db_FST);
    free(db);
    if (db_debug) printf("Completed\n");
    return(0);
}

/****************************************************************************/

/* This function  gets the  current value  of a  field from  the internal  DBF
representation, this must be created  first using dbOpen. Its arguments  are a
pointer to  the dbase  structure reffered  to, the  name of  the field  to  be
checked (this should be in capitals), the  record number to look at (these  go
from one), and the address of a pointer  to a string. This will be updated  to
hold the current  value of  the requested  field. The  function will  return a
non-zero value if any errors occured. */

int dbGetf(db,fn,rc,vl) dbase *db; char *fn; long rc; char **vl; {
int of = 0, sz, ct, fl = 1;
char *st;
    if (!db) return(db_BDB);
    if (db_debug) printf("Get field [%s] of \"%s\"\n",fn,db->dname);
    if (rc < 1) return(db_LST);
    if (rc > db->dhead->db_nrecs) return(db_GRT);
    for (ct = 0; ct < db->dnfld; ct++) {
	if (!strcmp((db->dfrec+ct)->fl_fname,fn)) {
	    sz = (db->dfrec+ct)->fl_fsize/* & 0xFF*/;
	    fl = !fl;
	    break;
	}
    	of += (db->dfrec+ct)->fl_fsize/* & 0xFF*/;
    }
    if (fl) return(db_BFL);
    if (db_debug) printf("Offset: %i, Size: %i, Record %i\n",of,sz,rc);
    if (!(st = (char *) calloc(1,sz+1))) return(db_MEM);
    strncpy(st,(db->drrec+((db->dnrec*(rc-1))+of)),sz);
    *vl = st;
    if (db_debug) printf("Completed, contents: %s\n",st);
    return(0);
}

/****************************************************************************/

/* This function changes  the value of  a selected field  in the internal  DBF
representation. Note that the function dbShut  must be called for any  changes
to be written back to the DBF  file. The functions arguments are a pointer  to
the dbase  structure  to be  referred,  the field  name  (must be  in  capital
letters) of  the  field to  change,  the record  number  of the  record  being
altered, and a  string containing the  value to  be put in  the field.  Only N
characters will be taken from this string (where N is the size of the  field).
The function will return non zero if any errors occured. */

int dbPutf(db,fn,rc,vl) dbase *db; char *fn; long rc; char *vl; {
int of = 0, sz, ct, cu, fl = 1;
char *st;
    if (!db) return(db_BDB);
    if (db_debug) printf("Put field [%s] of \"%s\" with: %s\n",fn,db->dname,vl);
    if (rc < 1) return(db_LST);
    if (rc > db->dhead->db_nrecs) return(db_GRT);
    for (ct = 0; ct < db->dnfld; ct++) {
	if (!strcmp((db->dfrec+ct)->fl_fname,fn)) {
	    sz = (db->dfrec+ct)->fl_fsize/* & 0xFF*/;
	    fl = !fl;
	    break;
	}
    	of += (db->dfrec+ct)->fl_fsize/* & 0xFF*/;
    }
    if (fl) return(db_BFL);
    if (db_debug) printf("Offset: %i, Size: %i, Record %i\n",of,sz,rc);
    /*strncpy((db->drrec+((db->dnrec*(rc-1))+of)),vl,sz);*/
    for (cu = 0; cu < sz; cu++) {
	if (cu < strlen(vl)) *(db->drrec+((db->dnrec*(rc-1))+of+cu)) = *(vl+cu);
	else *(db->drrec+((db->dnrec*(rc-1))+of+cu)) = ' ';
    }
    if (db_debug) printf("Completed\n");
    return(0);
}

/****************************************************************************/

/* This function adds  a new record to  the internal DBF representation.  Note
that the representation  must be shut  with dbShut for  these changes to  take
affect. Also note that  extra record space must  be allocated with the  dbOpen
function (or records deleted)  for this function to  work. Note that if  space
runs out  the DBF  can  always be  shut then  reopened  with more  space.  The
functions arguments are a pointer to the dbase structure to be referred  and a
character string  representing the  record.  Only the  record size  number  of
characters will  be taken  from this  string,  but the  string must  have  the
correct format.  That is  each field  must be  its given  length (with  padded
spaces as  neccessary). The  record  need not  be  terminated with  the  'sub'
character this will be done  automatically by this function. Returns  non-zero
on any error condition. */

int dbAddr(db,vl) dbase *db; char *vl; {
    if (!db) return(db_BDB);
    if (db_debug) printf("Append record to \"%s\"\n",db->dname);
    if (db->dhead->db_nrecs == db->dtrec) return(db_OVR);
    if (db_debug) {
	printf("Size: %i, Record: %i\n",db->dnrec,db->dhead->db_nrecs+1);
    }
    *(db->drrec+(db->dnrec*db->dhead->db_nrecs)-1) = 0x20;
    strncpy((db->drrec+((db->dnrec*db->dhead->db_nrecs))),vl,db->dnrec);
    *(db->drrec+(db->dnrec*(++db->dhead->db_nrecs))-1) = 0x1A;
    if (db_debug) printf("Completed\n");
    return(0);
}

/****************************************************************************/

/* This function deletes  a record from the  internal DBF representation.  The
record is permanently deleted (not just  marked for deletion). For the  change
to be  recorded the  internal representation  must be  written out  using  the
dbShut function. The  arguments are  a pointer to  the dbase  structure to  be
referred to and  the record  to be  deleted. All  the records  after this  are
shifted down  one, overwriting  the record  being deleted.  The function  will
return non-zero if an error occurs. */

int dbDelr(db,rc) dbase *db; long rc; {
long fo, lo, sz;
    if (!db) return(db_BDB);
    if (db_debug) printf("Delete record %i from \"%s\"\n",rc,db->dname);
    if (db->dhead->db_nrecs == 0) return(db_UND);
    if (rc < 1) return(db_LST);
    if (rc > db->dhead->db_nrecs) return(db_GRT);
    fo = db->dnrec * (rc - 1);
    lo = db->dnrec * rc;
    sz = db->dnrec * (db->dhead->db_nrecs - rc);
    if (rc == db->dhead->db_nrecs) {
	if (db_debug) printf("Deleting last record\n");
	*(db->drrec+fo) = 0x1A;
	db->dhead->db_nrecs--;
	return(0);
    }
    if (db_debug) {
	printf("From: %i, To: %i, Number: %i, Block: %i\n",lo,fo,rc,sz);
    }
    strncpy(db->drrec+fo,db->drrec+lo,sz);
    db->dhead->db_nrecs--;
    if (db_debug) printf("Completed\n");
    return(0);
}

/****************************************************************************/

/* This is  a more complex  function. It basically  adds a new  record to  the
database like dbAddr. It also makes up the record string from a list of  field
value arguments which can be numbers or strings and then formats the arguments
in the correct  way to  make up  a record  string. Its  fixed arguments  are a
pointer to the  dbase structure  to be referred  to (from  which the  function
derives the types of the  field), the address of a  pointer to a string  which
will be  replaced  by the  created  record string,  and  any number  of  field
initialiser arguments which should all be strings. The number of  initialisers
must be the same as the total number of fields in the dbase structure referred
to. Each field argument is the value  to initialise a field with in the  order
specified in the  dbase structure.  Behaviour is  undefined if  the number  of
field arguments is less  than the number of  fields, or anything other  than a
string is given as a field argument.  The field arguments can be NULL  strings
and need not be the complete  length of the field (additional characters  will
be stripped). Any characters  less than the field  length will be padded  with
spaces, strings  left  justified and  numbers  right justified.  The  function
returns non-zero if any detectable error occured. */

/* Variable arglist causes problems, not used in gurgle anyway ...
int dbMake(dbase *db,char **vl,...) {
va_list ap;
char *st;
char *ns;
int ct, cu, of, sz = 0;
    if (!db) return(db_BDB);
    if (db_debug) printf("Making record for \"%s\"\n",db->dname);
    if (!(st = (char *) malloc(db->dnrec))) return(db_MEM);
    for (ct = 0; ct < db->dnrec; ct++) { st[ct] = ' '; }
    if (db_debug) printf("Record size %i padded\n",ct);
    va_start(ap,vl);
    for (ct = 0; ct < db->dnfld; ct++) {
	ns = va_arg(ap,char *);
	if (db_debug) printf("Field [%s]: %s\n",(db->dfrec+ct)->fl_fname,ns);
	if (strlen(ns) < ((db->dfrec+ct)->fl_fsize & 0xFF)) {
	    if ((db->dfrec+ct)->fl_ftype == 'N') {
		of = ((db->dfrec+ct)->fl_fsize & 0xFF) - strlen(ns);
	    }
	    else of = 0;
	}
	else of = 0;
	for (cu = 0; cu < ((db->dfrec+ct)->fl_fsize & 0xFF); cu++) {
	    if (cu < strlen(ns)) *(st+sz+cu+of) = *(ns+cu);
	    else *(st+sz+cu+of) = ' ';
	}
	sz += (db->dfrec+ct)->fl_fsize & 0xFF;
    }
    va_end(ap);
    if (ct = dbAddr(db,st)) return(ct);
    *vl = st;
    if (db_debug) printf("Completed\n");
    return(0);
}
*/

/****************************************************************************/

/* This function lists the contents of one record by fieldname, the fieldnames
are also displayed. Its arguments are a  pointer to the dbase structure to  be
referred to, and the number of the  record to display. It returns non-zero  if
there was an error. */

int dbList(db,rc) dbase *db; long rc; {
int ct, sz = 0;
char st[256];
    if (!db) return(db_BDB);
    if (db_debug) printf("List record %i from \"%s\"\n",rc,db->dname);
    if (rc < 1) return(db_LST);
    if (rc > db->dhead->db_nrecs) return(db_GRT);
    rc = db->dnrec * (rc - 1);
    for (ct = 0; ct < db->dnfld; ct++) {
	strncpy(st,db->drrec+rc+sz,((db->dfrec+ct)->fl_fsize & 0xFF));
	st[(db->dfrec+ct)->fl_fsize & 0xFF] = '\0';
	printf("%-14s: %s\n",(db->dfrec+ct)->fl_fname,st);
	sz += (db->dfrec+ct)->fl_fsize & 0xFF;
    }
    if (db_debug) printf("Completed\n");
    return(0);
}

/****************************************************************************/

/* This function lists all the records in a database into a file. The ascii 
file representation of the database can then be edited and read back in by
using the dbAscr function. The arguments to the function are a pointer to the
dbase structure to be referred to, and the filename to write the output to. It
returns non-zero if there was an error. */

int dbAscw(db,fn) dbase *db; char *fn; {
int ct, sz, rc, mc;
FILE *os;
char st[256];
    if (!db) return(db_BDB);
    if (!(os = fopen(fn,"w"))) return(db_FOP);
    if (db_debug) printf("Write all records from \"%s\"\n",db->dname);
    for (mc = 1; mc <= db->dhead->db_nrecs; mc++) {
        if (db_debug) printf("Write record %i from \"%s\"\n",mc,db->dname);
        rc = db->dnrec * (mc - 1);
        for (sz = 0, ct = 0; ct < db->dnfld; ct++) {
	    strncpy(st,db->drrec+rc+sz,((db->dfrec+ct)->fl_fsize & 0xFF));
	    st[(db->dfrec+ct)->fl_fsize & 0xFF] = '\0';
	    fprintf(os,"%-14s: %s\n",(db->dfrec+ct)->fl_fname,st);
	    sz += (db->dfrec+ct)->fl_fsize & 0xFF;
        }
	if (mc != db->dhead->db_nrecs) fprintf(os,"\n");
    }
    if (fclose(os) == -1) return(db_FST);
    if (db_debug) printf("Completed\n");
    return(0);
}

/****************************************************************************/

/* This function reads in a file created by dbAscw (or manually produced to
the same format as dbAscw produces) and overwrites the database contents with
the records in the file. The functions arguments are a pointer to the database
structure to be referred to, the filename of the file to read in, and a mode
flag - this should be 0 to append records and 1 to overwrite. The function
returns non-zero if any errors occured. */

int dbAscr(db,fn,md) dbase *db; char *fn; unsigned char md; {
int ct, sz, rc, mc, of, cu;
FILE *is;
char st[256], *sp, *ds, ch;
    if (!db) {
        if (db_debug) printf("Bad Database\n");
	return(db_BDB);
    }
    if (!(is = fopen(fn,"r"))) return(db_FOP);
    if (db_debug) printf("Read all records from \"%s\"\n",db->dname);
    if (db_debug) printf("%s mode set.\n",md?"OVERWRITE":"APPEND");
    if (md) mc = 1;
    else {
	mc = db->dhead->db_nrecs+1;
    	if (db_debug) printf("Making record for \"%s\"\n",db->dname);
    	if (!(ds = (char *) malloc(db->dnrec))) return(db_MEM);
    	for (ct = 0; ct < db->dnrec; ct++) { ds[ct] = ' '; }
    	if (db_debug) printf("Record size %i padded\n",ct);
    }
    for (; mc <= db->dtrec; mc++) {
        rc = db->dnrec * (mc - 1);
        if ((ch = fgetc(is)) != EOF) {
	    ungetc(ch,is);
	    if (!md) {
	        if ((cu = dbAddr(db,ds))) return(cu);
	    }
	}
	else break;
        if (db_debug) printf("Read record %i from \"%s\"\n",mc,fn);
        for (sz = 0, ct = 0; ct < db->dnfld; ct++) {
	    fgets(st,sizeof(st),is);
	    if (feof(is)) break;
	    if (strncmp(st,(db->dfrec+ct)->fl_fname,strlen((db->dfrec+ct)->fl_fname))) {
		ct -= 1;
	    }
	    else {
		for (sp=st, cu=0; *sp != '\0'; sp++) {
		    if (*sp == ':') cu = 1;
		    else if (cu && *sp != ' ' && *sp != '\t') {
			cu = 2;
			break;
		    }
		}
	        if (cu != 2) {
                    if (db_debug)
			printf("Parse Error in \"%s\": Bad Field\n",fn);
	            return(db_PER);
	        }
		else {
		    for (cu = 0; sp[cu] != '\n'; cu++);
		    sp[cu] = '\0';
		}
	        if (db_debug)
		    printf("Field [%s]: %s\n",(db->dfrec+ct)->fl_fname,sp);
	        if (strlen(sp) < ((db->dfrec+ct)->fl_fsize & 0xFF)) {
	            if ((db->dfrec+ct)->fl_ftype == 'N') {
		        of = ((db->dfrec+ct)->fl_fsize & 0xFF) - strlen(sp);
	            }
	            else of = 0;
	        }
	        else of = 0;
	        for (cu = 0; cu < ((db->dfrec+ct)->fl_fsize & 0xFF); cu++) {
	            if (cu < strlen(sp)) *(db->drrec+rc+sz+cu+of) = *(sp+cu);
	            else *(db->drrec+rc+sz+cu+of) = ' ';
	        }
	        sz += (db->dfrec+ct)->fl_fsize & 0xFF;
	    }
        }
	if (ct != db->dnfld && ct != 0) {
            if (db_debug) printf("Parse Error in \"%s\": Missing Field\n",fn);
	    return(db_PER);
	}
	else if (feof(is)) break;
    }
    if (!feof(is) && fgetc(is) != EOF) {
        if (db_debug) printf("Database Overrun\n");
	return(db_OVR);
    }
    if (fclose(is) == -1) return(db_FST);
    if (db_debug) printf("Completed\n");
    return(0);
}

/****************************************************************************/
