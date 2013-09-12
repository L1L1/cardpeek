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

#include "gui_about.h"

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
		    "As an exemption to the GNU General Public License, compiling, linking, and/or using OpenSSL is " 
		    "allowed.\n\n"
		    "CARDPEEK is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without "
		    "even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
		    "General Public License for more details.\n\n"
		    "You should have received a copy of the GNU General Public License along with this program. If "
		    "not, see http://www.gnu.org/licenses/.";
 

void gui_about(void)
{
	GBytes* authors_bytes;
	gsize size;
	const char *authors[2];

	authors_bytes = g_resources_lookup_data("/cardpeek/AUTHORS",G_RESOURCE_LOOKUP_FLAGS_NONE,NULL);
	authors[0] = (const char *)g_bytes_get_data(authors_bytes,&size);
	authors[1] = NULL;

	gtk_show_about_dialog(NULL,
			      "program-name", "cardpeek",
			      "version", VERSION,
			      "license", LICENSE,
			      "wrap-license", TRUE,
			      "authors", authors,
			      "comments", "Cardpeek is a tool to read the contents of smart cards.",
			      "copyright", "Copyright Alain Pannetrat <L1L1@gmx.com>",
			      "website", "https://cardpeek.googlecode.com",
                  "website-label", "https://cardpeek.googlecode.com",  
			      NULL);
}


