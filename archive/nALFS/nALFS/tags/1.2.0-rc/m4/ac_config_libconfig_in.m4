dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ac_config_libconfig_in.html
dnl
# AC_CONFIG_LIBCONFIG_IN(LIBRARY, DESTINATION, MODULES)
# -----------------------------------------------------
# Generate a custom LIBRARY-config script.  Create the script in the
# DESTINATION directory, including support for MODULES.
AC_DEFUN([AC_CONFIG_LIBCONFIG_IN],
[# create a custom library-config file ($1-config)
m4_if(AC_CONFIG_LIBCONFIG_IN_USEPKGCONFIG, [true],
      [AC_PATH_PROG(PKG_CONFIG, pkg-config)])
pushdef([LIBCONFIG_DIR], [m4_if([$2], , , [$2/])])
LIBCONFIG_FILE="LIBCONFIG_DIR[]$1-config.in"
AC_SUBST(target)dnl
AC_SUBST(host)dnl
AC_SUBST(build)dnl
dnl create directory if it does not preexist
m4_if([$2], , , [AS_MKDIR_P([$2])])
AC_MSG_NOTICE([creating $LIBCONFIG_FILE])
echo '#! /bin/sh' >$LIBCONFIG_FILE
echo "# $1-config library configuration script" >>$LIBCONFIG_FILE
echo '# generated by ac_config_libconfig_in.m4' >>$LIBCONFIG_FILE
echo ' ' >>$LIBCONFIG_FILE
echo 'template_version="1.0.0"' >>$LIBCONFIG_FILE
echo ' ' >>$LIBCONFIG_FILE
echo 'package="@PACKAGE@"' >>$LIBCONFIG_FILE
echo ' ' >>$LIBCONFIG_FILE
echo '# usage instructions if no options given' >>$LIBCONFIG_FILE
echo 'if test "'"\$""#"'" -eq 0; then' >>$LIBCONFIG_FILE
echo '   cat <<EOF' >>$LIBCONFIG_FILE
m4_if($3, ,
      [echo 'Usage: $1-config [[OPTIONS]]' >>$LIBCONFIG_FILE],
      [echo 'Usage: $1-config [[OPTIONS]] [[LIBRARIES]]' >>$LIBCONFIG_FILE])
echo 'Options:' >>$LIBCONFIG_FILE
echo '        [[--prefix[=DIR]]]' >>$LIBCONFIG_FILE
echo '        [[--exec-prefix[=DIR]]]' >>$LIBCONFIG_FILE
echo '        [[--package]]' >>$LIBCONFIG_FILE
echo '        [[--version]]' >>$LIBCONFIG_FILE
echo '        [[--cflags]]' >>$LIBCONFIG_FILE
echo '        [[--libs]]' >>$LIBCONFIG_FILE
echo '        [[--help]]' >>$LIBCONFIG_FILE
m4_if($3, , ,
      [echo 'Libraries:' >>$LIBCONFIG_FILE
       for module in $1 $3 ; do
         echo "        $module" >>$LIBCONFIG_FILE ;
       done])
echo 'EOF' >>$LIBCONFIG_FILE
echo 'fi' >>$LIBCONFIG_FILE
echo ' ' >>$LIBCONFIG_FILE
echo '# parse options' >>$LIBCONFIG_FILE
echo 'o=""' >>$LIBCONFIG_FILE
echo 'h=""' >>$LIBCONFIG_FILE
echo 'for i ; do' >>$LIBCONFIG_FILE
echo '  case $i in' >>$LIBCONFIG_FILE
options="prefix exec-prefix eprefix package version cflags libs bindir sbindir libexecdir datadir sysconfdir sharedstatedir localstatedir libdir infodir mandir target host build pkgdatadir pkglibdir pkgincludedir template-version help"
echo '    --prefix=*) prefix=`echo $i | sed -e "s/--prefix=//"` ;;' >>$LIBCONFIG_FILE
echo '    --exec-prefix=*) exec_prefix=`echo $i | sed -e "s/--exec-prefix=//"` ;;' >>$LIBCONFIG_FILE
echo '    --eprefix=*) exec_prefix=`echo $i | sed -e "s/--eprefix=//"` ;;' >>$LIBCONFIG_FILE
for option in $options ; do
  case $option in
    exec-prefix)  echo "    --$option) echo_exec_prefix=\"yes\" ;;" >>$LIBCONFIG_FILE ;;
    template-version)  echo "    --$option) echo_template_version=\"yes\" ;;" >>$LIBCONFIG_FILE ;;
    *)  echo "    --$option) echo_$option=\"yes\" ;;" >>$LIBCONFIG_FILE ;;
  esac
done
m4_if($3, , ,
      [for module in $1 $3 ; do
         echo "  $module) echo_module_$module=\"yes\" ;" >>$LIBCONFIG_FILE ;
         echo '    echo_module="yes" ;;' >>$LIBCONFIG_FILE ;
       done])
echo '    //*|/*//*|./*//*)        echo_extra="yes" ;;' >>$LIBCONFIG_FILE
echo '    *) eval "echo Unknown option: $i" ; exit 1 ;;' >>$LIBCONFIG_FILE
echo '  esac' >>$LIBCONFIG_FILE
echo 'done' >>$LIBCONFIG_FILE
echo ' ' >>$LIBCONFIG_FILE
# in the order of occurence a standard automake Makefile
echo '# defaults from configure; set only if not set previously' >>$LIBCONFIG_FILE
vars="prefix exec_prefix bindir sbindir libexecdir datadir sysconfdir sharedstatedir localstatedir libdir infodir mandir includedir target host build"
for var in $vars ; do
  echo "if test -z \"\$$var\" ; then" >>$LIBCONFIG_FILE
  echo "  $var=\"@$var@\"" >>$LIBCONFIG_FILE
  echo 'fi' >>$LIBCONFIG_FILE
done
echo ' ' >>$LIBCONFIG_FILE
echo '# generate output' >>$LIBCONFIG_FILE
echo 'if test x$echo_module != xyes ; then' >>$LIBCONFIG_FILE
echo '  echo_module_$1="yes"' >>$LIBCONFIG_FILE
echo 'fi' >>$LIBCONFIG_FILE
AC_CONFIG_LIBCONFIG_IN_MODULES(m4_if([$3], , [$1], [m4_translit([$1 $3], [ ], [,])]))dnl
for option in $options extra; do
  case $option in
    exec-prefix)  echo "if test x\$echo_exec_prefix = xyes ; then" >>$LIBCONFIG_FILE ;;
    template-version)  echo "if test x\$echo_template_version = xyes ; then" >>$LIBCONFIG_FILE ;;
    *)  echo "if test x\$echo_$option = xyes ; then" >>$LIBCONFIG_FILE ;;
  esac
  case $option in
    exec-prefix | eprefix)  echo '  o="$o $exec_prefix"' >>$LIBCONFIG_FILE ;;
    template-version)  echo '  o="$o $template_version"' >>$LIBCONFIG_FILE ;;
    cflags)
      echo '  i=`eval echo "$includedir"`' >>$LIBCONFIG_FILE ;
      echo '  i=`eval echo "$i"`' >>$LIBCONFIG_FILE ;
      echo '  if test "_$i" != "_/usr/include" ; then' >>$LIBCONFIG_FILE ;
      echo '    o="$o -I$includedir"' >>$LIBCONFIG_FILE ;
      echo '  fi' >>$LIBCONFIG_FILE ;
      echo '  o="$o $cflags"' >>$LIBCONFIG_FILE ;;
    libs)  echo '  o="$o -L$libdir $libs"' >>$LIBCONFIG_FILE ;;
    help)  echo '  h="1"' >>$LIBCONFIG_FILE ;;
    pkgdatadir)  echo "  o=\"$o \${datadir}/\${package}\"" >>$LIBCONFIG_FILE ;;
    pkglibdir)  echo "  o=\"$o \${libdir}/\${package}\"" >>$LIBCONFIG_FILE ;;
    pkgincludedir)  echo "  o=\"$o \${includedir}/\${package}\"" >>$LIBCONFIG_FILE ;;
    extra)
      echo '  v=`echo $i | sed -e s://:\$:g`' >>$LIBCONFIG_FILE ;
      echo '  v=`eval "echo $v"`' >>$LIBCONFIG_FILE ;
      echo '  o="$o $v"' >>$LIBCONFIG_FILE ;;
    *)  echo "  o=\"$o \$$option\"" >>$LIBCONFIG_FILE
  esac
  echo 'fi' >>$LIBCONFIG_FILE
done
echo ' ' >>$LIBCONFIG_FILE
echo '# output data' >>$LIBCONFIG_FILE
echo 'o=`eval "echo $o"`' >>$LIBCONFIG_FILE
echo 'o=`eval "echo $o"`' >>$LIBCONFIG_FILE
echo 'if test -n "$o" ; then ' >>$LIBCONFIG_FILE
echo '  eval "echo $o"' >>$LIBCONFIG_FILE
echo 'fi' >>$LIBCONFIG_FILE
echo ' ' >>$LIBCONFIG_FILE
echo '# help text' >>$LIBCONFIG_FILE
echo 'if test ! -z "$h" ; then ' >>$LIBCONFIG_FILE
echo '  cat <<EOF' >>$LIBCONFIG_FILE
echo 'All available options:' >>$LIBCONFIG_FILE
echo '  --prefix=DIR and   change \$prefix and \$exec-prefix' >>$LIBCONFIG_FILE
echo '  --exec-prefix=DIR  (affects all other options)' >>$LIBCONFIG_FILE
echo '  --prefix           \$prefix        $prefix' >>$LIBCONFIG_FILE
echo '  --exec_prefix  or... ' >>$LIBCONFIG_FILE
echo '  --eprefix          \$exec_prefix   $exec_prefix' >>$LIBCONFIG_FILE
echo '  --version          \$version       $version' >>$LIBCONFIG_FILE
echo '  --cflags           -I\$includedir  unless it is /usr/include' >>$LIBCONFIG_FILE
echo '  --libs             -L\$libdir \$LIBS $libs' >>$LIBCONFIG_FILE
echo '  --package          \$package       $package' >>$LIBCONFIG_FILE
echo '  --bindir           \$bindir        $bindir' >>$LIBCONFIG_FILE
echo '  --sbindir          \$sbindir       $sbindir' >>$LIBCONFIG_FILE
echo '  --libexecdir       \$libexecdir    $libexecdir' >>$LIBCONFIG_FILE
echo '  --datadir          \$datadir       $datadir' >>$LIBCONFIG_FILE
echo '  --sysconfdir       \$sysconfdir    $sysconfdir' >>$LIBCONFIG_FILE
echo '  --sharedstatedir   \$sharedstatedir$sharedstatedir' >>$LIBCONFIG_FILE
echo '  --localstatedir    \$localstatedir $localstatedir' >>$LIBCONFIG_FILE
echo '  --libdir           \$libdir        $libdir' >>$LIBCONFIG_FILE
echo '  --infodir          \$infodir       $infodir' >>$LIBCONFIG_FILE
echo '  --mandir           \$mandir        $mandir' >>$LIBCONFIG_FILE
echo '  --target           \$target        $target' >>$LIBCONFIG_FILE
echo '  --host             \$host          $host' >>$LIBCONFIG_FILE
echo '  --build            \$build         $build' >>$LIBCONFIG_FILE
echo '  --pkgdatadir       \$datadir/\$package    ${datadir}/${package}'    >>$LIBCONFIG_FILE
echo '  --pkglibdir        \$libdir/\$package     ${libdir}/${package}' >>$LIBCONFIG_FILE
echo '  --pkgincludedir    \$includedir/\$package ${includedir}/${package}' >>$LIBCONFIG_FILE
echo '  --template-version \$template_version     $template_version' >>$LIBCONFIG_FILE
echo '  --help' >>$LIBCONFIG_FILE
echo 'EOF' >>$LIBCONFIG_FILE
echo 'fi' >>$LIBCONFIG_FILE
m4_pushdef([LIBCONFIG_UP], [m4_translit([$1], [a-z], [A-Z])])dnl
LIBCONFIG_UP[]_CONFIG="LIBCONFIG_DIR[]$1-config"
AC_SUBST(LIBCONFIG_UP[]_CONFIG)
AC_CONFIG_FILES(LIBCONFIG_DIR[]$1[-config], [chmod +x ]LIBCONFIG_DIR[]$1[-config])
m4_popdef([LIBCONFIG_DIR])
m4_popdef([LIBCONFIG_UP])
])


# AC_CONFIG_LIBCONFIG_IN_MODULES [(MODULE1 [, MODULE2 [, ...]])]
# --------------------------------------------------------------
#Output shell script using custom module variables.
AC_DEFUN([AC_CONFIG_LIBCONFIG_IN_MODULES],
[m4_if([$1], , ,
       [# create module definition for $1
dnl we're going to need uppercase, lowercase and user-friendly versions of the
dnl string `MODULE'
m4_pushdef([MODULE_UP], m4_translit([$1], [a-z], [A-Z]))dnl
m4_pushdef([MODULE_DOWN], m4_translit([$1], [A-Z], [a-z]))dnl
if test -z "$MODULE_DOWN[]_cflags" ; then
  if test -n "$MODULE_UP[]_CFLAGS" ; then
      MODULE_DOWN[]_cflags="$MODULE_UP[]_CFLAGS"
  else
dnl    AC_MSG_WARN([variable `MODULE_DOWN[]_cflags' undefined])
    MODULE_DOWN[]_cflags=''
  fi
fi
AC_SUBST(MODULE_DOWN[]_cflags)dnl
if test -z "$MODULE_DOWN[]_libs" ; then
  if test -n "$MODULE_UP[]_LIBS" ; then
    MODULE_DOWN[]_libs="$MODULE_UP[]_LIBS"
  else
    AC_MSG_WARN([variable `MODULE_DOWN[]_libs' and `MODULE_UP[]_LIBS' undefined])
    MODULE_DOWN[]_libs='-l$1'
  fi
  if test -n "$MODULE_UP[]_LIBDEPS" ; then
    MODULE_DOWN[]_libs="$MODULE_DOWN[]_libs $MODULE_UP[]_LIBDEPS"
  fi
fi
AC_SUBST(MODULE_DOWN[]_libs)dnl
if test -z "$MODULE_UP[]_VERSION" ; then
  AC_MSG_WARN([variable `MODULE_UP[]_VERSION' undefined])
  MODULE_UP[]_VERSION="$VERSION"
fi
AC_SUBST(MODULE_UP[]_VERSION)dnl
echo 'if test x$echo_module_$1 = xyes ; then' >>$LIBCONFIG_FILE
AC_CONFIG_LIBCONFIG_IN_MODULES_VARS([cflags], [MODULE_DOWN[]_cflags], [cflags])
AC_CONFIG_LIBCONFIG_IN_MODULES_VARS([libs], [MODULE_DOWN[]_libs], [libs])
AC_CONFIG_LIBCONFIG_IN_MODULES_VARS([version], [MODULE_UP[]_VERSION], [modversion])
echo 'fi' >>$LIBCONFIG_FILE
m4_popdef([MODULE_UP])dnl
m4_popdef([MODULE_DOWN])dnl
AC_CONFIG_LIBCONFIG_IN_MODULES(m4_shift($@))])dnl
])


# AC_CONFIG_LIBCONFIG_IN_MODULES_VARS [(VAR, SUBSTITUTION,
# PKGCONFIG_ARGS)]
# --------------------------------------------------------
# Output AC_CONFIG_LIBCONFIG_IN_MODULES variables.
#   VAR = variable to set
#   SUBSTITUTION = set if pkg-config is not available
#   PKGCONFIG_ARGS = args for pkg-config
AC_DEFUN([AC_CONFIG_LIBCONFIG_IN_MODULES_VARS],
[m4_if(AC_CONFIG_LIBCONFIG_IN_USEPKGCONFIG, [true],
[
echo 'if test -x "`which pkg-config`" ; then' >>$LIBCONFIG_FILE
echo '  if pkg-config --atleast-pkgconfig-version=0.7 --exists "MODULE_DOWN"; then' >>$LIBCONFIG_FILE
echo '    $1="@S|@$1 `pkg-config --$3 MODULE_DOWN`"' >>$LIBCONFIG_FILE
echo '  fi' >>$LIBCONFIG_FILE
echo 'else' >>$LIBCONFIG_FILE
echo '  $1="@S|@$1 @$2@"' >>$LIBCONFIG_FILE
echo 'fi' >>$LIBCONFIG_FILE
],
[echo '  $1="@S|@$1 @$2@"' >>$LIBCONFIG_FILE
])])


# AC_CONFIG_LIBCONFIG_IN_PKGCONFIG
# --------------------------------
# Enable pkgconfig support in libconfig script (default)
AC_DEFUN([AC_CONFIG_LIBCONFIG_IN_PKGCONFIG],
[m4_define([AC_CONFIG_LIBCONFIG_IN_USEPKGCONFIG], [true])
])dnl


# AC_CONFIG_LIBCONFIG_IN_STATIC
# -----------------------------
# Disable pkgconfig support in libconfig script
AC_DEFUN([AC_CONFIG_LIBCONFIG_IN_STATIC],
[m4_define([AC_CONFIG_LIBCONFIG_IN_USEPKGCONFIG], [false])
])dnl
