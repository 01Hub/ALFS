cat <<EOF
ACLOCAL_FLAGS=-I m4
EXTRA_DIST = CHANGES COPYING README CREDITS
EXTRA_DIST += scripts/flog
EXTRA_DIST += doc/nALFSrc
EXTRA_SCRIPTS =
# autoconf/automake do not currently offer a way to avoid installing
# the static libraries produced when the user chooses --disable-shared
# (or when the platform does not support shared libraries), so force
# them to be immediately removed after installation until a better
# solution can be found; same is needed for include files
install-exec-hook:
if STATIC_BUILD
	rm -rf \$(pkglibdir)
	rm -f \$(libdir)/libnALFS.*
	rm -f \$(bindir)/nALFS-config
endif

install-data-hook:
if STATIC_BUILD
	rm -f \$(includedir)/nALFS.h
endif

AM_CPPFLAGS = -I\$(srcdir)/src/include -I\$(srcdir)/src
AM_CFLAGS = -W -Wall -Wshadow -Winline

noinst_LTLIBRARIES = src/ltdl/ltdl.la
src_ltdl_ltdl_la_LDFLAGS = -module -avoid-version
src_ltdl_ltdl_la_SOURCES = src/ltdl/ltdl.c src/ltdl/ltdl.h
src_ltdl_ltdl_la_CFLAGS =

bin_PROGRAMS = src/nALFS
bin_SCRIPTS = @NALFS_CONFIG@
src_nALFS_LDFLAGS = -dlopen self
src_nALFS_CPPFLAGS = \$(XML2_INCLUDES) \$(AM_CPPFLAGS)
src_nALFS_LDADD = \$(XML2_LIB) \$(CURL_LIB) \$(SSL_LIB) src/lib/libnALFS.la
src_nALFS_LDADD += \$(LIBADD_DL) \$(MAIN_LIBS) src/ltdl/ltdl.la
if STATIC_BUILD
src_nALFS_LDFLAGS += -all-static
endif
src_nALFS_SOURCES =
EOF

for file in src/*.{c,h}; do
    echo src_nALFS_SOURCES += ${file} 
done

for syntax in ${all_syntaxes}; do
    echo if HANDLERS_${syntax} 
    eval "handlers=\"\${handlers_${syntax}}\""
    for handler in ${handlers}; do
	echo src_nALFS_LDFLAGS += -dlopen src/handlers/${handler}.la 
    done
    echo endif 
done

cat  <<EOF
include_HEADERS = src/include/nALFS.h
EOF

cat  <<EOF
lib_LTLIBRARIES = src/lib/libnALFS.la
src_lib_libnALFS_la_LDFLAGS = -avoid-version
src_lib_libnALFS_la_CPPFLAGS = \$(SSL_INCLUDES) \$(CURL_INCLUDES) \$(AM_CPPFLAGS)
src_lib_libnALFS_la_LIBADD = \$(CURL_LIB) \$(SSL_LIB)
src_lib_libnALFS_la_SOURCES =
EOF

for file in src/lib/*.{c,h}; do
    echo src_lib_libnALFS_la_SOURCES += ${file} 
done

cat  <<EOF
pkglib_LTLIBRARIES =
EOF

for syntax in ${all_syntaxes}; do
    echo if HANDLERS_${syntax} 
    eval "handlers=\"\${handlers_${syntax}}\""
    for handler in ${handlers}; do
	echo pkglib_LTLIBRARIES += src/handlers/${handler}.la 
    done
    echo endif 
done

for handler in ${all_handlers}; do
    echo src_handlers_${handler}_la_SOURCES = src/handlers/${handler}.c 
    echo src_handlers_${handler}_la_LDFLAGS = -module -avoid-version 
done
