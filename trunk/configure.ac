#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([cardpeek],[0.8.2],[L1L1@gmx.com])
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

PKG_CHECK_MODULES([GLIB],[glib-2.0 >= 2.32.0]) 

PKG_CHECK_MODULES([GTK],[gtk+-3.0 >= 3.4.2])

case "${host}" in
  *-*-darwin*)
    PCSC_CFLAGS='-Wno-deprecated-declarations -I/System/Library/Frameworks/PCSC.framework/Headers'
    PCSC_LIBS='-framework PCSC'
    ;;
  *)
    PKG_CHECK_MODULES([PCSC],[libpcsclite])
    ;;
esac

PKG_CHECK_MODULES([LUA],[lua-5.2],,[
  PKG_CHECK_MODULES([LUA],[lua5.2],,[
    PKG_CHECK_MODULES([LUA],[lua >= 5.2],,[
      for lua_lib_name in lua.5.2 lua-5.2 lua5.2; do 
        AC_CHECK_LIB([$lua_lib_name], [lua_close], [HAVE_LIBLUA=yes], [HAVE_LIBLUA=no])
        if test $HAVE_LIBLUA = yes; then
            AC_SUBST([LUA_CFLAGS], [""])
            AC_SUBST([LUA_LIBS], ["-l$lua_lib_name"])
            break
        fi 
      done
      if test $HAVE_LIBLUA != yes; then
            AC_MSG_ERROR([Could not find lua 5.2-dev. This program requires the lua 5.2 development library. Please install the lua 5.2 library first.])
      fi
])
])
])

PKG_CHECK_MODULES([CURL],[libcurl],,[
    AC_CHECK_LIB([curl],[curl_easy_init],[
        AC_SUBST([CURL_CFLAGS],[""])
        AC_SUBST([CURL_LIBS],[-lcurl])
    ])
])

AC_CHECK_LIB([ssl],[SSL_free],,
	     AC_MSG_ERROR([Could not find openssl-dev.
			   This program requires the openssl dev. library.
			   Please install it first.]))

AC_CHECK_LIB([crypto],[CRYPTO_free],,
	     AC_MSG_ERROR([Could not find openssl-dev.
			  This program requires the openssl dev. library.
			  Please install it first.]))
    
dnl ---------------------------------------------------------
dnl Check if ICONV is in -liconv
dnl otherwise check if directly part of standard library
AC_CHECK_LIB([iconv],[iconv_open])
if test "$HAVE_LIBICONV" != yes; then
    AC_MSG_CHECKING([if iconv is part of standard library])
    AC_TRY_LINK(
    [
#include <stdlib.h>
#include <iconv.h>
    ],
    [ 
        iconv_t cd = iconv_open("",""); 
    ],
    [HAVE_LIBICONV=yes],[HAVE_LIBICONV=no])
    AC_MSG_RESULT([$HAVE_LIBICONV])
fi
if test "$HAVE_LIBICONV" != yes; then
    AC_MSG_ERROR([Could not find the iconv library.
                  This program requires the iconv dev library.
                  Please install it first.])
fi 
dnl ---------------------------------------------------------

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

