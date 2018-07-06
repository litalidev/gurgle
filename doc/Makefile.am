## Process this file with automake to produce Makefile.in

info_TEXINFOS = gurgle.texi
gurgle_TEXINFOS = fdl.texi

all: gurgle.dvi gurgle.ps gurgle.pdf gurgle.html

SUFFIXES = .pdf .html

gurgle.pdf: gurgle.texi
gurgle.html: gurgle.texi

TEXI2PDF=texi2pdf
.texi.pdf:
	TEXINPUTS=.:$$TEXINPUTS \
          MAKEINFO='$(MAKEINFO) -I $(srcdir)' $(TEXI2PDF) $<

TEXI2HTML=texi2html
HTMLOPTS=-monolithic
.texi.html:
	TEXINPUTS=.:$$TEXINPUTS \
          MAKEINFO='$(MAKEINFO) -I $(srcdir)' $(TEXI2HTML) $(HTMLOPTS) $<

clean-local:
	rm -f gurgle.pdf gurgle.html

distclean-local:
	rm -f version.texi stamp-vti gurgle.info*
