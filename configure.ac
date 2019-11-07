#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([cardpeek],[0.8.5],[L1L1@gmx.com])
AM_INIT_AUTOMAKE

#
# Include marcros in m4/
# AC_CONFIG_MACRO_DIR([m4]) does not work on all platforms.
# so we do this 'by hand'
#

m4_include([m4/iconv.m4])
m4_include([m4/lib-ld.m4])
m4_include([m4/lib-link.m4])
m4_include([m4/lib-prefix.m4])
m4_include([m4/ax_lib_readline.m4])
m4_include([m4/ax_check_openssl.m4])

AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])

# silent build by default

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
# AC_PROG_OBJ is only needed on a Mac when --enable-osx-bundle
# Cardpeek does not need objective C to compile, but this directive is needed
# to silence autoconf errors.
AC_PROG_OBJC

#
# Our own checks
#

PKG_CHECK_MODULES([GLIB],[glib-2.0 >= 2.32.0]) 

PKG_CHECK_MODULES([GTK],[gtk+-3.0 >= 3.4])

AC_ARG_ENABLE([osx-bundle],
    AS_HELP_STRING([--enable-osx-bundle], [Compiles for the purpose of creating a MacOSX application bundle.]))

AM_CONDITIONAL([OSX_BUNDLE], [test "x$enable_osx_bundle" = "xyes"])
AM_COND_IF([OSX_BUNDLE], [
    AC_CONFIG_FILES([osx/Info-cardpeek.plist])
])

AC_CANONICAL_HOST
AS_CASE([$host],
    [*-*-darwin*], [HOST_TYPE=darwin],
    [*freebsd*], [HOST_TYPE=freebsd],
    [HOST_TYPE=default]
)

if test "$HOST_TYPE" = "darwin"; then
    if test ! -e "/Applications/Xcode.app/"; then
        AC_MSG_ERROR([Could not find /Applications/Xcode.app/.
        This program requires the PCSC framework that is distributed with XCode on OS X.
        Please install XCode first.])
    fi
    PCSC_HEADERS=`ls -d /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/*.sdk/System/Library/Frameworks/PCSC.framework/Versions/Current/Headers/ | sort | head -1`
    PCSC_CFLAGS="-Wno-deprecated-declarations -I$PCSC_HEADERS"
    PCSC_LIBS="-framework PCSC"
else
    PKG_CHECK_MODULES([PCSC],[libpcsclite])
    AM_COND_IF([OSX_BUNDLE],[
        AC_MSG_ERROR([Bundles are only available on MacOSX (--enable-osx-bunlde).])
    ])
fi

#
# Check for LUA 5.2
#

PKG_CHECK_MODULES([LUA],[lua-5.2],,[
  PKG_CHECK_MODULES([LUA],[lua5.2],,[
    PKG_CHECK_MODULES([LUA],[lua >= 5.2],,[
      AC_SEARCH_LIBS([lua_close], [lua.5.2 lua-5.2 lua5.2], [HAVE_LIBLUA=yes], [HAVE_LIBLUA=no])
      if test x$HAVE_LIBLUA != xyes; then
            AC_MSG_ERROR([Could not find lua 5.2-dev.
                          This program requires the lua 5.2 development library.
                          Please install the lua 5.2 library first.])
      fi
])
])
])

#
# Check for CURL
#

PKG_CHECK_MODULES([CURL],[libcurl],,[
    AC_SEARCH_LIBS([curl_easy_init],[curl], [HAVE_LIBCURL=yes], [HAVE_LIBCURL=no])
    if test x$HAVE_LIBCURL != xyes; then
	AC_MSG_ERROR([Cloud not find libcurl. 
                  This program requires the libcurl development library. 
                  Please install the required library first.]) 
    fi
])

#
# Check for OPENSSL
#

AX_CHECK_OPENSSL([],
                 if test "$HOST_TYPE" = "darwin"; then
                     AC_MSG_ERROR([

OpenSSL headers cannot be located. Consider using the --with-openssl option to specify an appropriate path.

Note: 

OpenSSL has been removed on newer versions of Mac OS X. 
You may need to install your own version of OpenSSL and then specify the root of the OpenSSL directory with the option --with-openssl=...

For example if you are using howebrew:

    brew install openssl
    ./configure --with-openssl=/usr/local/opt/openssl

                     ])
                 else
                     AC_MSG_ERROR([

OpenSSL headers cannot be located. Consider using the --with-openssl option to specify an appropriate path.
                     
                     ])
                 fi)

#AC_CHECK_LIB([ssl],[SSL_free],,
#	     AC_MSG_ERROR([Could not find openssl-dev.
#                       This program requires the openssl dev. library.
#                       Please install it first.]))
#
#AC_CHECK_LIB([crypto],[CRYPTO_free],,
#	     AC_MSG_ERROR([Could not find openssl-dev.
#                       This program requires the openssl dev. library.
#                       Please install it first.]))

#old_CPPFLAGS="$CPPFLAGS"
#CPPFLAGS="$OPENSSL_INCLUDES"
#AC_CHECK_HEADERS([openssl/des.h openssl/sha.h openssl/err.h openssl/ssl.h],[],[AC_MSG_ERROR([OpenSSL headers cannot be located. Consider using the --with-openssl option to specify an appropriate path.])])
#CPPFLAGS="$old_CPPFLAGS"

#
# Check for BACKTRACE lib (optional)
#

AC_CHECK_LIB([execinfo],[backtrace],,)
AC_CHECK_DECLS([backtrace], [], [], [[#include <execinfo.h>]])


# 
# Check for ICONV
# 

AC_ARG_VAR(ICONV_LIBS,[linker flags for ICONV (e.g. -l iconv), overriding the detected value.])
AM_ICONV
if test ! $ICONV_LIBS; then
	AC_SUBST([ICONV_LIBS],[$LIBICONV])	
fi

# 
# Check for GLIB_COMPILE_RESOURCES
#

AC_PATH_PROG(GLIB_COMPILE_RESOURCES,[glib-compile-resources],no)

if test x$GLIB_COMPILE_RESOURCES = xno ; then
AC_MSG_ERROR([glib-compile-resources cannot be found in PATH. 
              It is usually included with GTK+2.0 dev. libraries.
              Please install this tool first.
])
fi

AC_SUBST(GLIB_COMPILE_RESOURCES)

#
# Check for GDK_PIXBUF_PIXDATA
#

AC_PATH_PROG(GDK_PIXBUF_PIXDATA,[gdk-pixbuf-pixdata],no)

if test x$GDK_PIXBUF_PIXDATA = xno ; then
AC_MSG_ERROR([gdk-pixbuf-pixdata cannot be found in PATH. 
              It is usually included with GTK+2.0 dev. libraries and is needed by glib-compile-resources.
              Please install this tool first.
])
fi

#
# Check for READLINE
# 

AX_LIB_READLINE
if test -z "$ax_cv_lib_readline"; then
    AC_MSG_ERROR([Could not find readline library.
                  This program requires the readline dev. library.
                  Please install it first.])
fi

#
# Checks for header files.
#
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/time.h termios.h unistd.h])

#
# Checks for typedefs, structures, and compiler characteristics.
# 
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
CFLAGS="$GLIB_CFLAGS $CFLAGS"
AC_CHECK_TYPES([GStatBuf],[],[],[#include <glib/gstdio.h>])

#
# Checks for library functions.
#
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero getcwd memmove memset select strcasecmp strdup strerror strncasecmp strrchr strtol])

AC_CONFIG_FILES([Makefile])

if test "$HOST_TYPE" = "freebsd" -a ! "$ICONV_LIBS"; then
    AC_MSG_WARN([
********

On FreeBSD, you may need to specify the environement variable ICONV_LIBS='-l iconv' before running 'configure', as detailed in doc/build-freebsd.md. Otherwise 'make' may fail.

********])
fi

AC_OUTPUT

