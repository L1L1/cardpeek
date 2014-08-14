#ifndef _WIN32COMPAT_H
#define _WIN32COMPAT_H
#ifdef _WIN32

#include <dirent.h>

#define rindex strrchr

int scandir (const char *dir, struct dirent ***namelist,int (*select)(const struct dirent *),int (*compar)(const struct dirent **, const struct dirent**));

int alphasort (const struct dirent **a, const struct dirent **b);

const char *pcsc_stringify_error(long err);

#endif
#endif
