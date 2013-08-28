#include "win32compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "winscard.h"
#include <windows.h>

SCARD_IO_REQUEST g_rgSCardT0Pci = { SCARD_PROTOCOL_T0, 8 };
SCARD_IO_REQUEST g_rgSCardT1Pci = { SCARD_PROTOCOL_T1, 8 };

const char *PCSC_ERROR[] = {
/* 00 */	"No error",
/* 01 */	"Error 01",
/* 02 */ 	"The action was cancelled by an SCardCancel request",
/* 03 */ 	"The supplied handle was invalid",
/* 04 */	"Error 04",
/* 05 */	"Error 05",
/* 06 */	"Error 06",
/* 07 */	"Error 07",
/* 08 */	"Error 08",
/* 09 */	"Error 09",
/* OA */	"The user-specified timeout value has expired",
/* 0B */	"The smart card cannot be accessed because of other connections outstanding",
/* 0C */ 	"The operation requires a smart card, but no smart card is currently in the device",
/* 0D */	"Error 0D",
/* 0E */	"Error 0E",
/* 0F */ 	"The requested protocols are incompatible with the protocol currently in use with the smart card",
/* 10 */	"Error 10",
/* 11 */	"Error 11",
/* 12 */	"Error 12",
/* 13 */	"Error 13",
/* 14 */	"Error 14",
/* 15 */	"Error 15",
/* 16 */ 	"An attempt was made to end a non-existent transaction",
/* 17 */	"The specified reader is not currently available for use",
/* 18 */	"Error 1A",
/* 19 */	"Error 1B",
/* 1A */	"Error 1C",
/* 1B */	"Error 1D",
/* 1C */	"Error 1C",
/* 1D */	"The Smart card resource manager is not running",
/* 1E */	"Error 1E",
/* 1F */	"Error 1F",
/* 20 */	"Error 20",
/* 21 */	"Error 21",
/* 22 */	"Error 22",
/* 23 */	"Error 23",
/* 24 */	"Error 24",
/* 25 */	"Error 25",
/* 26 */	"Error 26",
/* 27 */	"Error 27",
/* 28 */	"Error 28",
/* 29 */	"Error 29",
/* 2A */	"Error 2A",
/* 2B */	"Error 2B",
/* 2C */	"Error 2C",
/* 2D */	"Error 2D",
/* 2E */ 	"Cannot find a smart card reader",
/* 2F */	"Error 2F",

/* 66 */ 	"The smart card is not responding to a reset",
/* 67 */ 	"Power has been removed from the smart card, so that further communication is not possible",
/* 68 */ 	"The smart card has been reset, so any shared state information is invalid",
/* 69 */ 	"The smart card has been removed, so further communication is not possible"
};

static const char *system_stringify_error(DWORD err)
{
	static char error_string[256];
	char *p;

	if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
				NULL, 
				err, 
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
				(LPSTR)error_string, 
				255, 
				NULL))
	{
		sprintf(error_string,"Unknown system error (0x%08X)",(unsigned)err); 
	}
	else
	{
		/* FIXME: not UT8 clean */
		for (p = error_string;*p!='\0';p++) 
			if (*p=='\n' || *p=='\r') 
			{
				*p='\0';
				break;
			}
	}
	return error_string; 
}

const char *pcsc_stringify_error(long err)
{
	static char default_buf[64];
	unsigned index;
	if ((err&0xFFFFFF00)==0x80100000)
	{
		index = err&0xFF;
		if (index<0x30)
			return PCSC_ERROR[index];
		if (index>=0x66 || index<=0x69)
			return PCSC_ERROR[index-0x66+0x30];
	}
	else if (err<1700)
	{
		return system_stringify_error(err);
	}
	sprintf(default_buf,"Unknown error (0x%08X)",(unsigned)err);
	return default_buf;
}


int scandir(const char *dir, struct dirent ***namelist,
		int (*select)(const struct dirent *),
		int (*compar)(const struct dirent **, const struct dirent**))
{
	struct dirent **entries;
	unsigned max;
	DIR* Hdir;
	struct dirent *candidate;
	Hdir=opendir(dir);
	if (Hdir==NULL)
		return 0;
	max = 0;
	while (readdir(Hdir)) max++;
	entries = malloc(sizeof(struct dirent *)*max);
	rewinddir(Hdir);
	max = 0;
	while ((candidate = readdir(Hdir))!=NULL)
	{
		if (select==NULL || select(candidate))
		{
			entries[max]=malloc(sizeof(struct dirent));
			memcpy(entries[max],candidate,sizeof(struct dirent));
			max++;	
		}
	}
	closedir(Hdir);
	*namelist = entries;
	return max;
}

int alphasort (const struct dirent **a, const struct dirent **b)
{
	return strcmp((*a)->d_name,(*b)->d_name);
}


