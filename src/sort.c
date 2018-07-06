/* $Id: sort.c,v 1.4 2001/04/27 10:38:10 timc Exp $
   Copyright (C) University of Edinburgh, 1992-1997, 2001, 2003-4, 2008-10.
   All rights reserved. */                                                    

/*
 * GURGLE - GNU REPORT GENERATOR LANGUAGE
 *
 * Copyright (C) University of Edinburgh, 1992-1997, 2001, 2003-4, 2008-190.
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
 * GURGLE SORT AND BANNER HANDLING
 */

#include <grg.h>

static int l_gppoffset = 0;
static int r_gppoffset = 0;
static int gppsize = 0;

/****************************************************************************/

/*
 * GURGLE SORT HANDLING
 */

/*
 * Generic sort and reverse sort comparison functions
 */

int gppcomp(a,b) char *a, *b; {
int out;
  if (a[l_gppoffset] == '\0' && b[r_gppoffset] == '\0') return(0);
  else if (a[l_gppoffset] == '\0') return(1);
  else if (b[r_gppoffset] == '\0') return(-1);
  else {
    out = strncmp(a+l_gppoffset,b+r_gppoffset,gppsize);
    if (out == 0) return(0);
    else if (out < 0) return(-1);
    else if (out > 0) return(1);
  }
}

int gpprcomp(a,b) char *a, *b; {
  return(gppcomp(b,a));
}

/*
 * Boolean sort and reverse sort comparison functions
 */

int gppbcomp(a,b) char *a, *b; {
int out, ai, bi;
  if (a[l_gppoffset] == '\0' && b[r_gppoffset] == '\0') return(0);
  else if (a[l_gppoffset] == '\0') return(1);
  else if (b[r_gppoffset] == '\0') return(-1);
  else {
    if (*(a+l_gppoffset) == 'T' || *(a+l_gppoffset) == 'Y') ai = 1;
    else ai = 0;
    if (*(b+r_gppoffset) == 'T' || *(b+r_gppoffset) == 'Y') bi = 1;
    else bi = 0;
    if (ai == bi) return(0);
    else if (ai < bi) return(-1);
    else if (ai > bi) return(1);
  }
}

int gpprbcomp(a,b) char *a, *b; {
  return(gppbcomp(b,a));
}

/*
 * Holds each sort fields offsets, types and direction
 */

struct offset {
  int offset;
  int size;
  char type;
  signed char direction;
  signed char last;
} offsettab[TEXSORTONMAX+1];

/*
 * Perform a record sort at one level
 */

void gppsort(eb,em,es,eoi) char *eb; int em, es, eoi; {
int el, oel, nel;
int (*comp)(const void *,const void *);
  l_gppoffset = r_gppoffset = offsettab[eoi].offset;
  if (offsettab[eoi].last != -1) {
    gppsize = offsettab[eoi].size;
    if (offsettab[eoi].direction < 0) {
      if (offsettab[eoi].type == 'L') comp = gpprbcomp;
      else comp = gpprcomp;
    }
    else
    {
      if (offsettab[eoi].type == 'L') comp = gppbcomp;
      else comp = gppcomp;
    }
    qsort(eb,em,es,comp);
    for (el = 0; el < em-1; el++) {
      for (oel=el, nel=1;
        el != (em-1) && (*comp)(eb+(el*es),eb+((el+1)*es)) == 0; el++, nel++);
      gppsort(eb+(oel*es),nel,es,eoi+1);
      l_gppoffset = r_gppoffset = offsettab[eoi].offset;
      gppsize = offsettab[eoi].size;
    }
  }
}

/*
 * Perform a record sort at any number of multiply nested levels
 */

void gppmainsort(db) dbase *db; {
unsigned short int sortndx, ctr, ofs, fsz;
char fty;
  for (sortndx = 0; sortndx < TEXSORTONMAX; sortndx++) {
    if (texsorton_val[sortndx][0] == '\0') break;
    else {
      for (ctr = 0, ofs = 0; ctr < db->dnfld; ctr++) {
        if (strcmp((db->dfrec+ctr)->fl_fname,texsorton_val[sortndx])==0) {
          fsz = (db->dfrec+ctr)->fl_fsize;
          fty = (db->dfrec+ctr)->fl_ftype;
          break;
        }
        ofs += (db->dfrec+ctr)->fl_fsize;
      }
      if (ctr==db->dnfld) gpperror("unknown sort field",texsorton_val[sortndx]);
      else {
        offsettab[sortndx].offset = ofs;
        offsettab[sortndx].size = fsz;
        offsettab[sortndx].type = fty;
        offsettab[sortndx].direction = texsorton_dir[sortndx];
      }
    }
  }
  offsettab[sortndx].last = -1;
  gppsort(db->drrec,db->dhead->db_nrecs,db->dnrec,0);
}

/****************************************************************************/

/*
 * GURGLE BANNER HANDLING
 */

/*
 * Write out banner text body, keep track of banners
 */

typedef struct {
  int offset;
  int size;
  char type;
  datumdef field;
} bannerdef;

datumdef bannertab = { NULL, 0, TEXBANNERMAX, sizeof(bannerdef), "bannertab" };

#define BANNER ((bannerdef *)bannertab.value)

int gppbanner(db,rc,md,file) dbase *db; int rc, md; FILE *file; {
unsigned short int bannndx, ctr, ofs, fsz, fty;
  if (!md) {
    grg_alloc(&bannertab,texbanner_ndx);
    for (bannndx = 0; bannndx < texbanner_ndx; bannndx++) {
      if (GRGBANNER[bannndx].name[0] == '\0') break;
      else {
        for (ctr = 0, ofs = 0; ctr < db->dnfld; ctr++) {
          if (strcmp((db->dfrec+ctr)->fl_fname,GRGBANNER[bannndx].name)==0) {
            fsz = (db->dfrec+ctr)->fl_fsize;
            fty = (db->dfrec+ctr)->fl_ftype;
            break;
          }
          ofs += (db->dfrec+ctr)->fl_fsize;
        }
        if (ctr == db->dnfld)
          gpperror("unknown banner field",GRGBANNER[bannndx].name);
        else {
          BANNER[bannndx].offset = ofs;
          BANNER[bannndx].size = fsz;
          BANNER[bannndx].type = fty;
          sprintf(BANNER[bannndx].field.name,"bannertab.field.%s",
            GRGBANNER[bannndx].name);
          BANNER[bannndx].field.block = STRMAX;
          BANNER[bannndx].field.bsize = sizeof(char);
          grg_alloc(&BANNER[bannndx].field,0);
        }
      }
    }
    BANNER[bannndx].offset = -1;
    md = 1;
  }
  for (bannndx = 0; BANNER[bannndx].offset != -1; bannndx++) {
    l_gppoffset = BANNER[bannndx].offset;
    r_gppoffset = 0;
    gppsize = BANNER[bannndx].size;
    if ((BANNER[bannndx].type == 'L' && gppbcomp(db->drrec+((rc-1)*db->dnrec),BANNER[bannndx].field.value) != 0) ||
      (BANNER[bannndx].type != 'L' && gppcomp(db->drrec+((rc-1)*db->dnrec),BANNER[bannndx].field.value) != 0)) {
      grg_strncpy(&BANNER[bannndx].field,
        db->drrec+((rc-1)*db->dnrec)+l_gppoffset,gppsize);
      /* Reset all banners below this one */
      for (ctr = bannndx + 1; BANNER[ctr].offset != -1; ctr++)
        grg_strcpy(&BANNER[ctr].field,"");
      putsysvar(_EQ_BANNER_VAL,EQ_STR,BANNER[bannndx].field.value);
      putsysvar(_EQ_BANNER_NEST,EQ_NUM,bannndx+1);
      gppequate(EQ_PRETEXBANNER,db,rc,0);   /* deprecated */
      gppequate(EQ_PRE_BANNER,db,rc,0);
      if (GRGBANNER[bannndx].body.value[0] != '\0')
        gppwrite(GRGBANNER[bannndx].body.value,db,rc,file);
      gppequate(EQ_PSTTEXBANNER,db,rc,0);   /* deprecated */
      gppequate(EQ_POST_BANNER,db,rc,0);
    }
  }
  return(md);
}

/****************************************************************************/
