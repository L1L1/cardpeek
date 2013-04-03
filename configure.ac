#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([cardpeek],[0.8],[L1L1@gmx.com])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])

# silent build by default
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_CANONICAL_HOST

# Our own checks

AM_PATH_GLIB_2_0([2.32.0],,	AC_MSG_ERROR([Please install GLIB dev libraries.]))
AM_PATH_GTK_2_0([2.12.0],, 	AC_MSG_ERROR([Please install GTK+2.0 dev libraries.]))

AC_DEFUN([AM_PATH_PACKAGE],
[
dnl
dnl AM_PATH_PACKAGE(packagename,make packagename,[what to do if found[, what to do if not found]])
dnl
  if test x$PKG_CONFIG = x ; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  fi

  AC_MSG_CHECKING(for $1 libraries)

  if test x$PKG_CONFIG = xno ; then
     echo "*** pkg_config is missing ***"
     AC_MSG_RESULT(no)
  fi

  if test x$PKG_CONFIG != xno ; then
     if `$PKG_CONFIG --exists $1` ; then
      $2_CFLAGS=`$PKG_CONFIG --cflags $1`
      $2_LIBS=`$PKG_CONFIG --libs $1` 
      has_package=yes	
      AC_MSG_RESULT(yes)
     else
      has_package=no
      AC_MSG_RESULT(no)
     fi
  fi

   if test x$has_package = xyes ; then
    ifelse([$3], , :, [$3])
  else
    ifelse([$4], , :, [$4])
  fi
  AC_SUBST([$2_CFLAGS])
  AC_SUBST([$2_LIBS])
])

case "${host}" in
  *-*-darwin*)
    PCSC_CFLAGS='-Wno-deprecated-declarations -I/System/Library/Frameworks/PCSC.framework/Headers'
    PCSC_LIBS='-framework PCSC'
    ;;
  *)
AM_PATH_PACKAGE([libpcsclite],[PCSC],,AC_MSG_ERROR([
Could not find libpcsclite-dev. 
This program requires the libpcsclite library. 
Please install the libpcsclite library first.]))
    ;;
esac

AM_PATH_PACKAGE([lua-5.1],[LUA],,[
  AM_PATH_PACKAGE([lua5.1],[LUA],,[
    AM_PATH_PACKAGE(['lua >= 5.1'],[LUA],,[
AC_MSG_ERROR([Could not find lua 5.1-dev. 
This program requires the lua 5.1 library. 
Please install the lua 5.1 library first.])
])
])
])

##AM_PATH_PACKAGE([openssl],[OPENSSL],,AC_MSG_ERROR([Could not find openssl-dev.
##This program requires the openssl dev. library.
##Please install it first.]))

AC_CHECK_LIB([ssl],[SSL_free],,
	     AC_MSG_ERROR([Could not find openssl-dev.
			   This program requires the openssl dev. library.
			   Please install it first.]))

AC_CHECK_LIB([crypto],[CRYPTO_free],,
	     AC_MSG_ERROR([Could not find openssl-dev.
			  This program requires the openssl dev. library.
			  Please install it first.]))

AC_PATH_PROG(GLIB_COMPILE_RESOURCES,[glib-compile-resources],no)

if test x$GLIB_COMPILE_RESOURCES = xno ; then
AC_MSG_ERROR([glib-compile-resources cannot be found in PATH. 
It is usually included with GTK+2.0 dev. libraries.
Please install this tool first.
])
fi

AC_SUBST(GLIB_COMPILE_RESOURCES)

AC_PATH_PROG(GDK_PIXBUF_PIXDATA,[gdk-pixbuf-pixdata],no)

if test x$GDK_PIXBUF_PIXDATA = xno ; then
AC_MSG_ERROR([gdk-pixbuf-pixdata cannot be found in PATH. 
It is usually included with GTK+2.0 dev. libraries and is needed by glib-compile-resources.
Please install this tool first.
])
fi

# Checks for libraries.

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
CFLAGS="$GLIB_CFLAGS $CFLAGS"
AC_CHECK_TYPES([GStatBuf],[],[],[#include <glib/gstdio.h>])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero getcwd memmove memset select strcasecmp strdup strerror strncasecmp strrchr strtol])

#AC_PATH_PROG(PDFLATEX,[pdflatex],no)
#if test x$PDFLATEX = xno ; then
#AC_MSG_WARN([The pdflatex program could not be found. 
#	This is only a problem if you wish to rebuild the pdf documentation 
#	from the LaTeX source in the doc/ sub-directory.
#	Otherwise this warning can be safely ignored.])
#fi

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

