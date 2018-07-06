/* $Id: guile.c,v 1.3 2001/04/27 10:38:10 timc Exp $
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
 * GURGLE GUILE SIDE PROCEDURES
 */

#include <grg.h>

#ifdef GUILE

SCM guile_equate(s_nam) SCM s_nam;
{
  char *nam;
  int len = STRMAX;
  SCM s_out;
  nam = gh_scm2newstr(s_nam,&len);
  gppequate(nam,0,0,0);
  if (eqstack_ndx > 1) gpperror("stack overflow",NULL);
  else if (eqstack_ndx == 1) {
    switch (eqstack[--eqstack_ndx].type) {
      case EQ_NUM: case EQ_BOOL:
        s_out = gh_int2scm(atoi(eqstack[eqstack_ndx].data.value));
        break;
      case EQ_DEC:
        s_out = gh_double2scm(atof(eqstack[eqstack_ndx].data.value));
        break;
      case EQ_STR: case EQ_FLD: case EQ_DATE:
        s_out = gh_str2scm(eqstack[eqstack_ndx].data.value,len);
        break;
    }
  }
  return(s_out);
}

SCM guile_getstrsysvar(s_nam) SCM s_nam;
{
  char *nam, *str;
  int len = STRMAX;
  nam = gh_scm2newstr(s_nam,&len);
  str = getstrsysvar(nam);
  return gh_str2scm(str,strlen(str));
}

SCM guile_getnumsysvar(s_nam) SCM s_nam;
{
  char *nam;
  int num, len = STRMAX;
  nam = gh_scm2newstr(s_nam,&len);
  num = getnumsysvar(nam);
  return gh_int2scm(num);
}

SCM guile_putstrsysvar(s_nam,s_str) SCM s_nam, s_str;
{
  char *nam, *str;
  int len = STRMAX;
  nam = gh_scm2newstr(s_nam,&len);
  str = gh_scm2newstr(s_str,&len);
  putstrsysvar(nam,str);
  return gh_str2scm(str,strlen(str));
}

SCM guile_putnumsysvar(s_nam,s_num) SCM s_nam, s_num;
{
  char *nam;
  int num, len = STRMAX;
  nam = gh_scm2newstr(s_nam,&len);
  num = gh_scm2int(s_num);
  putnumsysvar(nam,num);
  return gh_int2scm(num);
}

SCM guile_getfield(s_fld,s_rec,s_dat) SCM s_fld, s_rec, s_dat;
{
  char value[STRMAX] = "", *valuep = value, *dat, *fld;
  int rec, len = STRMAX, ctr;
  dbase *db;
  dat = gh_scm2newstr(s_dat,&len);
  fld = gh_scm2newstr(s_fld,&len);
  rec = gh_scm2int(s_rec);
  for (ctr = 0; ctr < texdbffile_ndx; ctr++) {
    if (strcasecmp(GRGDATA[ctr].name,dat) == 0) {
      db = GRGDATA[ctr].db;
      break;
    }
  }
  if (ctr == texdbffile_ndx) gppeqerror("no such database");
  if (rec < 1 || rec > db->dhead->db_nrecs)
    gppeqerror("record index out of range");
  if (dbGetf(db,fld,rec,&valuep) != 0)
    gpperror("unknown field name",fld);
  strncpy(value,valuep,0xFF);
  strip(value);
  return gh_str2scm(value,strlen(value));

}

#endif

/****************************************************************************/
