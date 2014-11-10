#ifndef WIN32_CONFIG_H
#define WIN32_CONFIG_H
#ifdef _WIN32

#define VERSION "0.8.4-win32"

#define ICONV_CONST

#define HAVE_GSTATBUF

#define HAVE_LIBREADLINE 
#define HAVE_READLINE_HISTORY
#define HAVE_READLINE_READLINE_H
#define HAVE_READLINE_HISTORY_H

#else /* not win32 */
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif /* _WIN32 */
#endif
