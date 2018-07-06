/* $Id: debug.c,v 1.3 2001/04/27 10:38:10 timc Exp $
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
 * GURGLE DEBUG ROUTINE
 */

#include <grg.h>

void grg_debug() {
int ctr,m,tm;
  if (gppdebug) {

    fprintf(stderr,"\n");
    fprintf(stderr,"OPERATING LIMITS (PRE-PROCESSING)\n");
    fprintf(stderr,"---------------------------------\n\n");
    
    fprintf(stderr,"DYNAMIC:\n");
    tm = 0;
    fprintf(stderr,"  %-16s  %-12s  %-6s  %-10s  %-10s\n",
      "NAME","CURRENT SIZE","BLOCK","BLOCK SIZE","ALLOCATED");
    m = pbbuffer.size * pbbuffer.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      pbbuffer.name,pbbuffer.size,pbbuffer.block,pbbuffer.bsize,m);
    m = texfile.size * texfile.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texfile.name,texfile.size,texfile.block,texfile.bsize,m);
    m = texbuffer.size * texbuffer.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texbuffer.name,texbuffer.size,texbuffer.block,texbuffer.bsize,m);
    m = texheader.size * texheader.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texheader.name,texheader.size,texheader.block,texheader.bsize,m);
    m = texfooter.size * texfooter.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texfooter.name,texfooter.size,texfooter.block,texfooter.bsize,m);
    m = texrecord.size * texrecord.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texrecord.name,texrecord.size,texrecord.block,texrecord.bsize,m);
    m = texpage01.size * texpage01.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texpage01.name,texpage01.size,texpage01.block,texpage01.bsize,m);
    m = texpagenn.size * texpagenn.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texpagenn.name,texpagenn.size,texpagenn.block,texpagenn.bsize,m);
    m = texbanner.size * texbanner.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texbanner.name,texbanner.size,texbanner.block,texbanner.bsize,m);
    m = texdbffile.size * texdbffile.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texdbffile.name,texdbffile.size,texdbffile.block,texdbffile.bsize,m);
    m = texblock.size * texblock.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texblock.name,texblock.size,texblock.block,texblock.bsize,m);
    m = macrotable.size * macrotable.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      macrotable.name,macrotable.size,macrotable.block,macrotable.bsize,m);
    m = equatetable.size * equatetable.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      equatetable.name,equatetable.size,equatetable.block,equatetable.bsize,m);
    m = texclarg.size * texclarg.bsize; tm += m;
    fprintf(stderr,"  %-16s  %12d  %6d  %10d  %10d\n",
      texclarg.name,texclarg.size,texclarg.block,texclarg.bsize,m);
    fprintf(stderr,"  TOTAL ALLOCATED = %10d\n",tm);

    fprintf(stderr,"\nSTATIC:\n");

    fprintf(stderr,"TEXMAXTEX=%06d   TEXPBMAX=%06d    MAXMACRONAME=%04d\n",
      TEXMAXTEX,TEXPBMAX,MAXMACRONAME);
    fprintf(stderr,"MAXMACRODEF=%04d   MAXMACROS=%04d     MAXEQUATES=%04d\n",
      MAXMACRODEF,MAXMACROS,MAXEQUATES);
    fprintf(stderr,"TEXSORTONMAX=%04d  TEXBANNERMAX=%04d  TEXDBFFILEMAX=%04d\n",
      TEXSORTONMAX,TEXBANNERMAX,TEXDBFFILEMAX);
    fprintf(stderr,"TEXFILTERMAX=%04d  DBFFIELDMAX=%04d   STRMAX=%04d\n",
      texfiltermax,DBFFIELDMAX,STRMAX);
    fprintf(stderr,"TEXBLOCKMAX=%04d   MAXEQUATEDEF=%04d  TEXCLARGMAX=%04d\n",
      TEXBLOCKMAX,MAXEQUATEDEF,TEXCLARGMAX);
    fprintf(stderr,"REGEXPMAX=%04d     DYNFLDINI=%04d     DYNRECINI=%04d\n",
      REGEXPMAX,DYNFLDINI,DYNRECINI);

    fprintf(stderr,"\n");
    fprintf(stderr,"ENVIRONMENT VARIABLE SETTINGS\n");
    fprintf(stderr,"-----------------------------\n\n");

    fprintf(stderr,"NPAGE=%s\n",texnewpage);
    fprintf(stderr,"FESCSUB=%s\n",texfescsub?"YES":"NO");
    fprintf(stderr,"TEXEXT=%s\n",gpptexext);
    fprintf(stderr,"PAGE1=%d\n",texpage01_val);
    fprintf(stderr,"PAGEN=%d\n",texpagenn_val);
    fprintf(stderr,"MAXPAGE=%d\n",texpage_max);
    fprintf(stderr,"DELIM=%s\n",texdelimiter);
    fprintf(stderr,"DFAMODE=%s\n",texdfamode);
    fprintf(stderr,"CONCAT=%s\n",texconcat?"YES":"NO");
    fprintf(stderr,"PHYSDB=%s\n",texphysdb);
    fprintf(stderr,"DBHOSTNM=%s\n",texdbhostnm);
    fprintf(stderr,"DBUSERNM=%s\n",texdbusernm);
    if (!texdbpasswd[0]) fprintf(stderr,"DBPASSWD=\n");
    else fprintf(stderr,"DBPASSWD=(HIDDEN)\n");
    fprintf(stderr,"NAMCOL=%s\n",texnamcol?"YES":"NO");
    fprintf(stderr,"DEFCOL=%s\n",texdefcol?"YES":"NO");
    fprintf(stderr,"NULL=%s\n",texnull);
    fprintf(stderr,"MKDIR=%s\n",texmkdir?"YES":"NO");
    fprintf(stderr,"EXPAND=%s\n",texexpand?"YES":"NO");

    fprintf(stderr,"\n");
    fprintf(stderr,"COMMAND LINE ARGUMENTS\n");
    fprintf(stderr,"----------------------\n\n");
    for (ctr = 0; ctr < texclarg_ndx; ctr++)
      fprintf(stderr,"%d: %s\n",ctr,GRGCLARG[ctr].value);

    fprintf(stderr,"\n");
    fprintf(stderr,"USER DEFINED MACROS\n");
    fprintf(stderr,"-------------------\n\n");
    for (ctr = 0; ctr < macrotable_ndx; ctr++)
      fprintf(stderr,"%d: %s=%s\n",
        ctr,GRGMACRO[ctr].name,GRGMACRO[ctr].definition);

    fprintf(stderr,"\n");
    fprintf(stderr,"USER DEFINED EQUATES\n");
    fprintf(stderr,"--------------------\n\n");
    for (ctr = 0; ctr < equatetable_ndx; ctr++)
      fprintf(stderr,"%d: %s=%s\n",
        ctr,GRGEQUATE[ctr].name,GRGEQUATE[ctr].definition.value);

    fprintf(stderr,"\n");
    fprintf(stderr,"TEXT BLOCKS\n");
    fprintf(stderr,"-----------\n\n");

    fprintf(stderr,"HEADER:\n%s",texheader.value);
    fprintf(stderr,"FOOTER:\n%s",texfooter.value);
    fprintf(stderr,"RECORD:\n%s",texrecord.value);
    fprintf(stderr,"PAGE01:\n%s",texpage01.value);
    fprintf(stderr,"PAGENN:\n%s",texpagenn.value);

    fprintf(stderr,"BANNER:\n");
    for (ctr = 0; ctr < texbanner_ndx; ctr++)
      fprintf(stderr,"%d: %s\n%s",ctr,GRGBANNER[ctr].name,
        GRGBANNER[ctr].body.value);

    fprintf(stderr,"BLOCK:\n");
    for (ctr = 0; ctr < texblock_ndx; ctr++)
      fprintf(stderr,"%d: %s\n%s",ctr,GRGBLOCK[ctr].name,
        GRGBLOCK[ctr].body.value);

    fprintf(stderr,"DATABASE:\n");
    for (ctr = 0; ctr < texdbffile_ndx; ctr++)
      fprintf(stderr,"%d: %s%s%s%s\n%s",ctr,GRGDATA[ctr].path,
        GRGDATA[ctr].name,GRGDATA[ctr].type,GRGDATA[ctr].master?" [MASTER]":"",
        GRGDATA[ctr].body.value);

    fprintf(stderr,"TEXSORTON:\n");
    for (ctr = 0; ctr < TEXSORTONMAX; ctr++)
      fprintf(stderr,"%d: %s\n",ctr,texsorton_val[ctr]);
    fprintf(stderr,"TEXFILTER:\n");
    for (ctr = 0; ctr < texfiltermax; ctr++)
      fprintf(stderr,"%d: (%d) %s\n",ctr,texfilter[ctr].st,texfilter[ctr].re);
    fprintf(stderr,"TEXGROUP:\n");
    for (ctr = 0; ctr < TEXGROUPMAX; ctr++)
      fprintf(stderr,"%d: (%d) %s\n%s",ctr,texgroup[ctr].st,texgroup[ctr].reg,
        texgroup[ctr].def);
    fprintf(stderr,"TEXPATTERN:\n");
    for (ctr = 0; ctr < texpattern_ndx; ctr++)
      fprintf(stderr,"%d: %s = %s\t%s\t{%s}\n",ctr,texpattern[ctr].nmode,
        texpattern[ctr].modep,texpattern[ctr].textp,texpattern[ctr].eqstr);
  }
}

/****************************************************************************/
