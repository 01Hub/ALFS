dnl
dnl configure.in
dnl - Process this file with autoconf to produce a configure script.
dnl
dnl Copyright (C) 2002
dnl
dnl Neven Has <haski@sezampro.yu>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
dnl


AC_INIT(src/nalfs.c)


AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(LD, ld)
AC_CHECK_TOOL(RANLIB, ranlib, :)

AC_CHECK_LIB(ncurses, initscr)
AC_CHECK_LIB(form, form_driver)
AC_CHECK_LIB(dl, dlopen)


dnl Check for libxml2 and its fetaures.

AC_ARG_WITH(libxml2,
[  --with-libxml2=PREFIX   search for xml2-config in PREFIX/bin],
[
	case "$withval" in
		n|no|y|ye|yes) ;; dnl Do nothing, we need it.

		*) libxml2_prefix=$withval ;;
	esac
])

AC_PATH_PROG([XML2_CONFIG], [xml2-config], [no],
	[$libxml2_prefix/bin /bin /usr/bin /usr/local/bin $PATH $prefix/bin])

if test "$XML2_CONFIG" != "no" ; then
	LIBS="$LIBS `$XML2_CONFIG --libs`"
	CFLAGS="$CFLAGS `$XML2_CONFIG --cflags`"
else
	echo "*** Can't find xml2-config. Make sure you have libxml2 installed."
	echo "*** If you do, try using --with-libxml2=PREFIX."
	echo "*** If not, download it from http://xmlsoft.org/."
	exit 1
fi

AC_CHECK_LIB(xml2, xmlXIncludeProcess, CPPFLAGS="$CPPFLAGS -DHAVE_XINCLUDE")


dnl Check for ssl library.

AC_ARG_WITH(ssl,
[  --with-ssl=PREFIX       specify prefix for ssl files (ssl will be used
                          for checking non-MD5 digests in <unpack> elements)],
[
	case "$withval" in
		n|no)
			;;
		y|ye|yes)
			enable_ssl=yes
			;;
		*)
			SSL_DIR="$withval"
			LIBS="$LIBS -L$SSL_DIR/lib"
			enable_ssl=yes
			;;
	esac
])

if test "$enable_ssl" == "yes" ; then
	AC_CHECK_LIB(ssl, OpenSSL_add_all_digests, [
		CPPFLAGS="$CPPFLAGS -I$SSL_DIR/include -DHAVE_LIBSSL"
		LIBS="$LIBS -lcrypto -lssl"], [], -lcrypto)
fi


dnl Check for curl library.

AC_ARG_WITH(curl,
[  --with-curl=PREFIX      search for curl-config in PREFIX/bin (curl will
                          be used for fetching files, instead of wget)],
[
	case "$withval" in
		n|no)
			disable_curl=yes
			;;
		y|ye|yes)
			;;

		*)
			curl_prefix=$withval
			;;
	esac
])

if test "$disable_curl" != "yes" ; then
	AC_PATH_PROG([CURL_CONFIG], [curl-config], [no],
	[$curl_prefix/bin /bin /usr/bin /usr/local/bin $PATH $prefix/bin])

	if test "$CURL_CONFIG" != "no" ; then
		LIBS="$LIBS `$CURL_CONFIG --libs`"
		CFLAGS="$CFLAGS `$CURL_CONFIG --cflags`"
		CPPFLAGS="$CPPFLAGS -DHAVE_LIBCURL"
	fi
fi


dnl Link the program statically, or not.

dnl AC_ARG_ENABLE(static-link,
dnl 	[  --enable-static-link    link the program statically],
dnl	[ LDFLAGS="$LDFLAGS -static" ])


AC_OUTPUT([
Rules.mak
Makefile
src/Makefile
src/lib/Makefile
src/handlers/Makefile
])
