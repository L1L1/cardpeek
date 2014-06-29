#ifndef WIN32_CONFIG_H
#define WIN32_CONFIG_H
#ifdef _WIN32

#define VERSION "0.8.3-win32"
#define ICONV_CONST

#else /* not win32 */
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif /* _WIN32 */
#endif
