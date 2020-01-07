#
# Copyright IBM Corp. 2008, 2018
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Common Public License v1.0
# which accompanies this distribution, and is available at
# http://www.ibm.com/developerworks/library/os-cpl.html
#
# Authors:		Andreas Herrmann
#			Sven Schuetz <sven@de.ibm.com>
#
# File:			Makefile.am
#
# Description:
# Input file for automake
#
#

docdir	= $(datadir)/zfcp-hbaapi-$(VERSION)

# libtool versioning
LIB_CURRENT	= 1
LIB_REVISION	= 0
LIB_AGE		= 1

if BACKTRACE
BT_CFLAGS = -rdynamic
endif

if VENDORLIB
SYMFILE = $(srcdir)/vendor.sym
noinst_HEADERS		= vlib.h vlib_aux.h vlib_sysfs.h vlib_sg.h vlib_sg_io.h\
			vlib_events.h vlib_sfhelper.h hbaapi.h \
			fc_tools/include/zfcp_util.h
else
SYMFILE = $(srcdir)/hbaapi.sym
include_HEADERS		= hbaapi.h
noinst_HEADERS		= vlib.h vlib_aux.h vlib_sysfs.h vlib_sg.h \
			vlib_sfhelper.h fc_tools/include/zfcp_util.h
endif

if DEBUG
DEBUG_CFLAGS = -g -DDEBUG
endif

if DOCS
noinst_DATA=docs

dox/man/man3/SupportedHBAAPIs.3: docs
dox/man/man3/UnSupportedHBAAPIs.3: docs
dox/man/man3/hbaapi.h.3: docs

docs: dox

dox:	doxygen.cfg
	(cat $(srcdir)/doxygen.cfg ; echo "INPUT=$(srcdir)" ) | doxygen -

endif

AM_CFLAGS = $(EXTRA_CFLAGS) $(BT_CFLAGS) $(DEBUG_CFLAGS) \
	 	-DVLIBPATH=\"$(exec_prefix)/lib\" -D_REENTRANT

lib_LTLIBRARIES		= libzfcphbaapi.la

libzfcphbaapi_la_SOURCES = vlib.c vlib_callbacks.c vlib_aux.c vlib_sysfs.c \
			vlib_sg.c vlib_sg_io.c vlib_events.c vlib_sfhelper.c
libzfcphbaapi_la_LIBADD = -l@LIBSGUTILS@ -lpthread
libzfcphbaapi_la_LDFLAGS = \
	-version-info $(LIB_CURRENT):$(LIB_REVISION):$(LIB_AGE) \
	-Wl,-init,_initvlib,-fini,_finivlib \
	-export-symbols $(SYMFILE)

bin_PROGRAMS = zfcp_ping zfcp_show

zfcp_ping_SOURCES = fc_tools/zfcp_ping.c
zfcp_show_SOURCES = fc_tools/zfcp_show.c

if VENDORLIB
zfcp_ping_LDADD = -lHBAAPI
zfcp_show_LDADD = -lHBAAPI
else
zfcp_ping_LDADD = -lzfcphbaapi
zfcp_show_LDADD = -lzfcphbaapi
endif


if DOCS
man_MANS = 	dox/man/man3/SupportedHBAAPIs.3 \
		dox/man/man3/UnSupportedHBAAPIs.3 dox/man/man3/hbaapi.h.3
endif

dist_man_MANS		= zfcp_show.8 zfcp_ping.8 libzfcphbaapi.3

dist_doc_DATA = README AUTHORS COPYING

EXTRA_DIST = vendor.sym hbaapi.sym bootstrap doxygen.cfg LICENSE

bootstrap:
	aclocal
	autoheader
	autoconf
	libtoolize --automake
	automake --add-missing

maintainer-clean-local:
	rm -f Makefile.in config.* aclocal.m4 configure install-sh \
		missing mkinstalldirs stamp-h* ltmain.sh depcomp
	rm -rf autom4te*
if DOCS
	rm -rf dox
endif

install-data-local:
if DOCS
	test -d $(DESTDIR)$(docdir)/html || $(mkdir_p) $(DESTDIR)$(docdir)/html
	test -d $(DESTDIR)$(docdir)/latex || $(mkdir_p) $(DESTDIR)$(docdir)/latex
	$(INSTALL) -m 644 dox/html/* $(DESTDIR)$(docdir)/html
	$(INSTALL) -m 644 dox/latex/* $(DESTDIR)$(docdir)/latex
endif

uninstall-local:
if DOCS
	rm -rf $(DESTDIR)$(docdir)/html
	rm -rf $(DESTDIR)$(docdir)/latex
	rm -rf $(DESTDIR)$(docdir)/man
endif


clean-local:
	rm -rf dox
