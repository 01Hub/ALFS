#!/bin/bash
#
#  Bootstrap script for nALFS, used to get a CVS checkout ready for use.
#
#  Copyright (C) 2003
#
#  Kevin P. Fleming <kpfleming@linuxfromscratch.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

cat <<"EOF"
SUBDIRS = doc
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = CHANGES COPYING README CREDITS
EXTRA_DIST += scripts/flog
EXTRA_DIST += doc/nALFSrc
EXTRA_SCRIPTS =
MOSTLYCLEANFILES = src/.libs/nALFS src/.libs/nALFSS.o
DISTCLEANFILES = nALFS-config.in
# autoconf/automake do not currently offer a way to avoid installing
# the static libraries produced when the user chooses --disable-shared
# (or when the platform does not support shared libraries), so force
# them to be immediately removed after installation until a better
# solution can be found; same is needed for include files
install-exec-hook:
if STATIC_BUILD
	rm -rf $(pkglibdir)
	rm -f $(libdir)/libnALFS.*
	rm -f $(bindir)/nALFS-config
endif

install-data-hook:
if STATIC_BUILD
	rm -f $(includedir)/nALFS.h
endif

AM_CPPFLAGS = -I$(srcdir)/src/include -I$(srcdir)/src
AM_CFLAGS = -W -Wall -Wshadow -Winline
if MAINTAINER_MODE
AM_CFLAGS += -Wundef -Wnested-externs
AM_CFLAGS += -Wredundant-decls -Wcast-align -Wstrict-prototypes
AM_CFLAGS += -Wmissing-prototypes -Wmissing-declarations -Wbad-function-cast
AM_CFLAGS += -Wpointer-arith -Waggregate-return -std=gnu99 -Wfloat-equal
AM_CFLAGS += -Wendif-labels -Wundef -Wcast-qual -Wsign-compare -Wpacked
AM_CFLAGS += -Wdisabled-optimization
endif

noinst_LTLIBRARIES = src/ltdl/ltdl.la
src_ltdl_ltdl_la_LDFLAGS = -module -avoid-version
src_ltdl_ltdl_la_SOURCES = src/ltdl/ltdl.c src/ltdl/ltdl.h
src_ltdl_ltdl_la_LIBADD = $(LIBADD_DL) 
src_ltdl_ltdl_la_CFLAGS = -w

bin_PROGRAMS = src/nALFS
bin_SCRIPTS = @NALFS_CONFIG@
src_nALFS_LDFLAGS = -dlopen self
src_nALFS_CPPFLAGS = $(XML2_INCLUDES) $(AM_CPPFLAGS)
src_nALFS_LDADD = $(XML2_LIB) $(CURL_LIB) $(SSL_LIB) src/lib/libnALFS.la
src_nALFS_LDADD += $(MAIN_LIBS) src/ltdl/ltdl.la
if STATIC_BUILD
src_nALFS_LDFLAGS += -all-static
endif
src_nALFS_SOURCES =
EOF

for file in src/*.{c,h}; do
    echo src_nALFS_SOURCES += ${file} 
done

for handler in ${all_handlers}; do
	echo if HANDLER_${handler}
	echo src_nALFS_LDFLAGS += -dlopen src/handlers/${handler}.la 
	echo endif
done

cat  <<EOF
include_HEADERS = src/include/nALFS.h
EOF

cat  <<"EOF"
lib_LTLIBRARIES = src/lib/libnALFS.la
src_lib_libnALFS_la_LDFLAGS = -avoid-version
src_lib_libnALFS_la_CPPFLAGS = $(SSL_INCLUDES) $(CURL_INCLUDES) $(AM_CPPFLAGS)
src_lib_libnALFS_la_LIBADD = $(CURL_LIB) $(SSL_LIB)
src_lib_libnALFS_la_SOURCES =
EOF

for file in src/lib/*.{c,h}; do
    echo src_lib_libnALFS_la_SOURCES += ${file} 
done

cat  <<EOF
pkglib_LTLIBRARIES =
EOF

for handler in ${all_handlers}; do
    echo if HANDLER_${handler}
    echo pkglib_LTLIBRARIES += src/handlers/${handler}.la 
    echo src_handlers_${handler}_la_SOURCES = src/handlers/${handler}.c 
    echo src_handlers_${handler}_la_LDFLAGS = -module -avoid-version 
    echo endif
done

if test -d profile; then
    for file in `find profile -type f -print`; do
        echo EXTRA_DIST += ${file}
    done	

    for file in `find profile -type l -print`; do
        echo EXTRA_DIST += ${file}
    done	
fi
