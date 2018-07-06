%if %{?CAINGRES:0}%{!?CAINGRES:1}
%define CAINGRES 0
%endif

Name:           gurgle
Version:        1.61
Release:        1%{?dist}
Summary:        GNU Database Report Generator

Group:          Applications/Productivity
License:        GPLv3
URL:            http://homepages.inf.ed.ac.uk/timc/gurgle
Source0:        http://homepages.inf.ed.ac.uk/timc/gurgle/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  texinfo >= 4.2, mysql-devel, postgresql-devel


%description
Reads record and field information from a dBase 3+ file, delimited ASCII text
file, or an SQL query to a RDBMS and produces a report listing. The program was
designed primarily to produce a TeX/LaTeX formatted output, but plain ASCII
text, troff, PostScript, HTML, or any other kind of ASCII based output format
can be produced also.

The program is ideal for generating large bodies of text where various parts of
the text are substituted with information from a database. It's particularly
useful for generating mainly static web pages with small amounts of dynamic
content.

A definition file controls the formatting process, which holds the report,
page, and record layouts, what fields to display, and where. Other useful
functions include sorting, filtering, and data manipulation of records in the
database. Main features include:

  * Multiple input databases or queries
  * Support for ASCII delimited text, dBase 3+, CA-Ingres, GNU SQL,
    MySQL, and PostgreSQL databases
  * Sorting of database records
  * Automatic banner placement at the start of each sorted group
  * Filters using regular expressions or useer defined function
  * Five main text bodies-- header, footer, record, 1st page, and nth page
  * User defined macros, text bodies, and equations on field contents,
    including the conditionals
  * User configurable input parsing patterns (default is like awk)
  * Include files
  * Environment variables
  * System variables
  * Multiple file output
  * General purpose processing language
  * Optional GUILE support


%if %{CAINGRES}
%define II_SYSTEM /disk/ingres
%endif


%prep
%setup


%build
%if %{CAINGRES}
export MY_II_SYSTEM=${II_SYSTEM:-%{II_SYSTEM}}
sed --in-place "s@II_SYSTEM@${MY_II_SYSTEM}@g" configure.in
%endif
aclocal
autoheader
autoconf
automake
export LDFLAGS=-L/usr/lib/mysql
./configure --enable-mysql --prefix=%{buildroot}/usr
pushd src
make
popd
mv src/gurgle mygurgle
make clean
./configure --enable-postgres --prefix=%{buildroot}/usr
pushd src
make
popd
mv src/gurgle pggurgle
%if %{CAINGRES}
make clean
export LDFLAGS=-L${MY_II_SYSTEM}/ingres/lib
./configure --enable-ingres --prefix=%{buildroot}/usr
pushd src
export II_SYSTEM=${MY_II_SYSTEM}
make
popd
mv src/gurgle cagurgle
%endif
make clean
./configure --prefix=%{buildroot}/usr
make


%install
rm -rf %{buildroot}
make install prefix=%{buildroot}/usr
install -m 755 mygurgle %{buildroot}%{_bindir}
install -m 755 pggurgle %{buildroot}%{_bindir}
%if %{CAINGRES}
install -m 755 cagurgle %{buildroot}%{_bindir}
%endif


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%doc AUTHORS BUGS COPYING TODO NEWS
%doc doc/gurgle.ps doc/gurgle.pdf doc/gurgle.html
%{_infodir}/*
%{_bindir}/gurgle
%{_libdir}/libdbase.a


%package mysql
Summary: gurgle with RDBMS suppoprt for MySQL
Group: Applications/Productivity
%description mysql
GNU Database Report Generator with RDBMS suppoprt for MySQL.
%files mysql
%defattr(-,root,root)
%{_bindir}/mygurgle


%package postgresql
Summary: gurgle with RDBMS suppoprt for PostgreSQL
Group: Applications/Productivity
%description postgresql
GNU Database Report Generator with RDBMS suppoprt for PostgreSQL.
%files postgresql
%defattr(-,root,root)
%{_bindir}/pggurgle


%if %{CAINGRES}
%package caingres
Summary: gurgle with RDBMS suppoprt for CA Ingres 
Group: Applications/Productivity
%description caingres
GNU Database Report Generator with RDBMS suppoprt for CA Ingres.
%files caingres
%defattr(-,root,root)
%{_bindir}/cagurgle
%endif


%changelog
* Thu Jan 21 2010 Tim Colles <timc@inf.ed.ac.uk> - 1.61-1
- fixed more valgrind detected problems

* Fri Nov 20 2009  Tim Colles <timc@inf.ed.ac.uk> - 1.60-1
- fixed numeric field handling under PostgreSQL

* Wed Nov 18 2009  Tim Colles <timc@inf.ed.ac.uk> - 1.59-1
- fixed date/time field handling under PostgreSQL

* Wed Nov 18 2009  Tim Colles <timc@inf.ed.ac.uk> - 1.58-1
- fixed enumerate field handling under PostgreSQL

* Sat Nov 14 2009 Tim Colles <timc@inf.ed.ac.uk> - 1.57-1
- updated version with GPLv3 license and FDL license for documentation
- fixed single valgrind identified error in strcpy (same src/dst)
- fixed support for boolean handling under PostgreSQL

* Mon Mar 31 2008 Tim Colles <timc@inf.ed.ac.uk> - 1.55-1
- updated version

* Mon Feb 11 2008 Tim Colles <timc@inf.ed.ac.uk> - 1.54-1
- updated specfile for Fedora Packaging Conventions
- build for EL5
- patches rolled back into version

* Thu May 10 2007 Tim Colles <timc@inf.ed.ac.uk> - 0:1.53-3.dice.3
- built for FC6
- some specfile refinements

* Wed Nov 29 2006 Tim Colles <timc@inf.ed.ac.uk> 0:1.53-3.dice.2
- patch for Ingres on FC5 needing libcrypt specified

* Tue Apr 25 2006 Tim Colles <timc@inf.ed.ac.uk> 1.53-3
- Port to FC5
- Force MySQL version 3 for now

* Wed May 04 2005 Tim Colles <timc@inf.ed.ac.uk> 1.53-2
- Port to FC3
- Patch to replace varargs.h usage with stdarg.h instead
- Made cagurgle optional (rpmbuild -ba gurgle.spec --define "CAINGRES 1")

* Tue Mar 23 2004 Tim Colles <timc@inf.ed.ac.uk>
- updates for CVS version, including full build from master source
- builds separate package for each rdbms supported
- need II_SYSTEM defined in environment for esqlc to work properly

* Wed Mar 17 2004 Ken Dawson <ktd@inf.ed.ac.uk>
- New minor release.
- Modify configure.in in sources to insert II_SYSTEM string which is replaced
  by sed during the build stage.
- Derive a suitable substitution for II_SYSTEM based on ${II_SYSTEM} and
  %{II_SYSTEM}.

* Tue Mar 16 2004 Ken Dawson <ktd@inf.ed.ac.uk>
- Modify configure.in so that esqlc is searched for in /disk/ingres/ingres/bin.
- Modify Makefile.am in source so that a make clean deletes the src/ingres.c.
- Change source file so that it doesn't contain prebuilt docs, configure and
  Makefile.in.
- Change build section so that configure is generated by autoconf from
  configure.in.
- Change build section so that Makefile.in is generated by automake from
  Makefile.am.
- Build cagurgle (gurgle with CA  Ingres RDBMS support) and install it in a
  separate package (not all hosts will have ingres installed).

* Thu Aug 21 2003 Tim Colles <timc@inf.ed.ac.uk>
- updated to 1.52 (for newer autoconf/automake/aclocal/Postgres under RH9)
- only build doc tree once

* Tue Feb  4 2003 Tim Colles <timc@inf.ed.ac.uk>
- Initial rpmification of gurgle
- Specific binaries for MySQL and PostgreSQL
