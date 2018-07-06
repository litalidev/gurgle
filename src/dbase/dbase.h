/****************************************************************************/

/* $Id: dbase.h,v 1.6 2001/04/26 13:06:40 timc Exp $
   Copyright (C) University of Edinburgh, 1992-1997, 2001, 2003-4, 2008-10.
   All rights reserved. */

/* 
 * DBASE - Include file for dbase.c utilities
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

/* 1 for debug on, 0 otherwise */

extern int db_debug;

/*#define	db_magic	0x0F015C03	*/
#define	db_magic	0x03		/* DBF file magic number value */

/* Error return value types */

#define	db_MEM		0x01		/* Memory allocation failed */
#define	db_FOP		0x02		/* Opening a file failed */
#define	db_MGC		0x03		/* DBF file magic number wrong */
#define	db_LOC		0x04		/* Locating in a file failed */
#define	db_BDB		0x05		/* Bad DBF pointer given */
#define	db_FST		0x06		/* Closing a file failed */
#define	db_BFL		0x07		/* Bad DBF field name given */
#define	db_OVR		0x08		/* Record space overflow */
#define	db_UND		0x09		/* Record space underflow */
#define	db_GRT		0x0A		/* Record number too big */
#define	db_LST		0x0B		/* Record number too small */
#define	db_FGT		0x0C		/* Field number too big */
#define	db_FLT		0x0D		/* Field number too small */
#define	db_BAD		0x0E		/* Function not implemented */
#define	db_PER		0x0F		/* Parse Error */

/* The DBF file is represented internally by the structures below. A DBF  file
has a 32 byte header, and N 32 byte field descriptors. These are terminated by
a carriage return (\r) space sequence. Each record then follows as  characters
(spaces if  not all  the  field length  unused).  Character strings  are  left
justified and numbers are right justified. Dates are always eight  characters,
and booleans one character.  Each record is followed  by one space  character,
except the last record which is followed by the sub (\0x1A) character. */

/* Structure for the header of a DBF file. Header is 32 bytes. */

typedef struct {
    long db_magcn;	/* Magic number */
    long db_nrecs;	/* Number of records */
    long db_ignr1;
    long db_ignr2;
    long db_ignr3;
    long db_ignr4;
    long db_ignr5;
    long db_ignr6;
} db_head;

/* Structure for each field descriptor in a DBF file, each is 32 bytes. */

typedef struct {
    char fl_fname[11];	/* Field name (max 10 chars) */
    char fl_ftype;	/* Field type (ascii C, N, D, or B) */
    long fl_foffs;	/* Field offset (to next field) */
    long fl_fsize;	/* Size of field (D always 8, B always 1) */
    long fl_ignr1;
    long fl_ignr2;
    long fl_ignr3;
} db_field;

/* The  database file  structure. This  is what  is created  and used  by  the
dbase.c utilities. It has pointers to the other structures above. There can be
any number of these, and hence more than one DBF file open at a time. */

typedef struct {
    char *dname;	/* File name of DBF file */
    FILE *dfile;	/* Stream for database */
    short dnfld;	/* Number of fields per record */
    short dnrec;	/* Total size (in bytes) of a record */
    short dtrec;	/* Maximum number of accessable records */
    db_head *dhead;	/* Pointer to DBF header structure */
    db_field *dfrec;	/* Pointer to array of DBF field structures */
    char *drrec;	/* Pointer to array of all the DBF records */
} dbase;

/* Function declarations for the dbase.c utilities. All return an error  code.
This is NULL if the function succeeded and an error value otherwise. */

extern int dbOpen();	/* Open and read in a DBF file */
extern int dbShut();	/* Write out and close a DBF file */
extern int dbGetf();	/* Get the contents of a field */
extern int dbPutf();	/* Put the contents of a field */
extern int dbAddr();	/* Add a new record to the DBF representation */
extern int dbDelr();	/* Delete a record from the DBF representation */
extern int dbList();	/* List the contents of record with fieldnames */
extern int dbMake();	/* Make a new record and add to DBF representation */
extern int dbAscw();	/* Write DBF representation as ASCII file */
extern int dbAscr();	/* Read ASCII file into a DBF representation */

/****************************************************************************/
