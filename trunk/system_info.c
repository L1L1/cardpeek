/********************************************************************** 
*
* This file is part of Cardpeek, the smart card reader utility.
*
* Copyright 2009-2013 by Alain Pannetrat <L1L1@gmx.com>
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

/* this hash function is used by SDBM. */
static unsigned int simple_hash(const char *str)
{
    unsigned int hash = 0;
    int c;

    while ((c = *str++)!=0)
        hash = c + (hash << 6) + (hash << 16) - hash;
    return hash;
}

#ifndef _WIN32
#include <string.h>
#include <stdio.h>
#include "config.h"
#include <sys/utsname.h>

const char *system_string_info(void)
{
    static char info[128];
    static struct utsname u;
    
    if (uname(&u)<0)    
    {       
        return "Unknown OS";      
    }
    snprintf(info,128,"%s %s %s",u.sysname,u.release,u.machine);
    
    return info;
}

unsigned int system_name_hash(void)
{
    static struct utsname u;
    if (uname(&u)<0)
    {
        return 0;
    }
    return simple_hash(u.nodename);
}

#else

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "win32/config.h"

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

const char *system_string_info(void)
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

unsigned int system_name_hash() 
{
    static char computer_name[1024]; 
    DWORD size = 1024;     
    GetComputerName(computer_name,&size);           
    return simple_hash(computerName);   
}


#endif

