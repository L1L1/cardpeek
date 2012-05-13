#ifndef _POSIXIFY_H
#define _POSIXIFY_H

#include <dirent.h>

#define rindex strrchr

int scandir (const char *dir, struct dirent ***namelist,int (*select)(const struct dirent *),int (*compar)(const struct dirent **, const struct dirent**));

int alphasort (const struct dirent **a, const struct dirent **b);

const char *pcsc_stringify_error(long err);

#endif
