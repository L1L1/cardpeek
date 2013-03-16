/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009-2013 by 'L1L1'
*
* Cardpeek is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Cardpeek is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Cardpeek.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "system_info.h"

#ifndef _WIN32
#include <string.h>
#include <stdio.h>
#include "config.h"

const char *system_string_info()
{
	static char info[128];
	char first_line[128];
	FILE *cmd;
	char *p;

	if ((cmd=popen("uname -smr","r")))
	{
		if (fgets(first_line,127,cmd))
		{
			for (p=first_line;*p!='\0';p++)
				if (*p=='\r' || *p=='\n') 
				{ 
					*p='\0'; 
					break; 
				} 	
		}
		else 
		{
			strcpy(first_line,"Unknown os");
		}
		if (pclose(cmd),0)
		{
			strcpy(first_line,"Unknown os");
		}
	}
	else
	{
		strcpy(first_line,"Unknown os");
	}
	snprintf(info,128,"Cardpeek %s on %s", 
		VERSION,
		first_line);
	return info;	
}


#else

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "win32/config.h"

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

const char *system_string_info()
{
	static char info[128];
	SYSTEM_INFO si;
	OSVERSIONINFOEX osvi;
	PGNSI GetNativeSystemInfo_func;

	memset(&si,0,sizeof(si));
	memset(&osvi,0,sizeof(osvi));

	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx((OSVERSIONINFO*)&osvi);

	GetNativeSystemInfo_func = (PGNSI) GetProcAddress(
			GetModuleHandle(TEXT("kernel32.dll")),
			"GetNativeSystemInfo");

	if (GetNativeSystemInfo_func!=NULL)
		GetNativeSystemInfo_func(&si);
	else
		GetSystemInfo(&si);

	snprintf(info,110,"Cardpeek %s on Windows NT_%u.%u (%s) ",
			VERSION,
			(unsigned)osvi.dwMajorVersion,
			(unsigned)osvi.dwMinorVersion,
			osvi.szCSDVersion);

	switch (si.wProcessorArchitecture) {
		case 9: strcat(info,"AMD64");
			break;
		case 6: strcat(info,"IA64");
			break;
		case 0: strcat(info,"x86");
			break;
		default: strcat(info,"Unknown platform");
	};
	return info;
}

#endif

