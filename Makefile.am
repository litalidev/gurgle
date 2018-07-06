## Process this file with automake to produce Makefile.in

SUBDIRS = src doc

EXTRA_DIST = BUGS gurgle.spec

dist-hook:
	cp doc/gurgle.dvi $(distdir)/doc
	cp doc/gurgle.ps $(distdir)/doc
	cp doc/gurgle.pdf $(distdir)/doc
	cp doc/gurgle.html $(distdir)/doc
	rm -f $(distdir)/src/ingres.c
	rm -f $(distdir)/src/gnusql.c
