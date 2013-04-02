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

#include <gtk/gtk.h>
#ifndef _WIN32
#include "config.h"
#else
#include "win32/config.h"
/*#include "win32/win32compat.h"*/
#endif


const char* LICENSE="CARDPEEK is free software: you can redistribute it and/or modify it under the terms of the GNU "
	 	    "General Public License as published by the Free Software Foundation, either version 3 of the "
		    "License, or (at your option) any later version.\n\n"
		    "As an exemption to the GNU General Public Licence, compiling, linking, and/or using OpenSSL is " 
		    "allowed.\n\n"
		    "CARDPEEK is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without "
		    "even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
		    "General Public License for more details.\n\n"
		    "You should have received a copy of the GNU General Public License along with this program. If "
		    "not, see http://www.gnu.org/licenses/.";
 

const char* AUTHORS[]= {
	"Main author:",
	"- Alain Pannetrat <L1L1@gmx.com>",
	"",
	"Contributors:",
	"- Ludovic Lange: Initial patch for Mac OS X version.",
	"- Adam Laurie: CVM parsing in EMV.",
	"- Kalev Lember: bug and compatibility fixes.",
	"- Ludovic Rousseau: Lots of code cleanup, better use of autotools.", 
	"- Pascal Terjan: Paris metro/RER name decoding and bug fixes.",
	"- and a few anonymous contributors...",
	NULL 
};

void gui_about(void)
{
	gtk_show_about_dialog(NULL,
			      "program-name", "cardpeek",
			      "version", VERSION,
			      "license", LICENSE,
			      "wrap-license", TRUE,
			      "authors", AUTHORS,
			      "comments", "Cardpeek is a tool to read the contents of smart cards.",
			      "copyright", "Copyright Alain Pannetrat <L1L1@gmx.com>",
			      "website", "https://cardpeek.googlecode.com", 
			      NULL);
}


