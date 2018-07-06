## Process this file with automake to produce Makefile.in

SUBDIRS = dbase

INCLUDES = -I./dbase

bin_PROGRAMS = gurgle

EXTRA_DIST = gnusql.ec ingres.sc

gurgle_SOURCES = config.h grg.h gnusql.c postgres.c mysql.c ingres.c pattern.c sort.c scanner.l parser.c filter.c equate.c utility.c main.c guile.c debug.c

gurgle_LDADD = dbase/libdbase.a @LEXLIB@

clean-local:
	rm -f ingres.c

distclean-local:
	rm -f gnusql.c ingres.c

SUFFIXES = .sc .sc~ .ec .ec~

## ESQL Preprocessor language section (CA-Ingres).

ESQL=@ESQL@
ESQLFLAGS=
ESQLDEFS=@ESQLDEFS@
ESQLDEBUG=-d
ESQLC=$(ESQL) $(ESQLFLAGS)
.sc:
	$(ESQLC) $< $(ESQLDEFS)
.sc.c:
	$(ESQLC) $< $(ESQLDEFS)


## GSQL Preprocessor language section (GNUSQL)

GSQL=@GSQL@
GSQLFLAGS=
GSQLDEFS=@GSQLDEFS@
GSQLDEBUG=-d
GSQLC=$(GSQL) $(GSQLFLAGS)
.ec:
	$(GSQLC) $< $(GSQLDEFS)
.ec.c:
	$(GSQLC) $< $(GSQLDEFS)
