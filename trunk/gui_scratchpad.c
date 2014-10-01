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

#include "gui.h"
#include "lua_ext.h"
#include "gui_scratchpad.h"
#include "misc.h"

/*********************************************************/
/* THE INFAMOUS UGLY GLOBALS *****************************/
/*********************************************************/

GtkListStore *COMPLETION=NULL;

/*********************************************************/
/* IMPLEMENTATION ****************************************/
/*********************************************************/

static void gui_run_command_cb(GtkWidget *widget,
                        GtkWidget *entry )
{
  GtkTreeIter iter;
  const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  UNUSED(widget);

  luax_run_command(entry_text);
  gtk_list_store_append(COMPLETION,&iter);
  gtk_list_store_set(COMPLETION,&iter,0,entry_text,-1);
  gui_update(0);
}

GtkWidget *gui_scratchpad_create_window(void)
{
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *entry;
  GtkEntryCompletion* compl;
  GtkWidget *icon;

  entry = gtk_entry_new();

  COMPLETION = gtk_list_store_new(1,G_TYPE_STRING);
  compl = gtk_entry_completion_new();
  gtk_entry_completion_set_model(compl, GTK_TREE_MODEL(COMPLETION));
  gtk_entry_completion_set_text_column(compl, 0);
  gtk_entry_set_completion(GTK_ENTRY(entry), compl);

  label = gtk_label_new("Command:");
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  
  if ((icon = gtk_image_new_from_icon_name("system-run",GTK_ICON_SIZE_MENU)))
    gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (gui_run_command_cb),
		    (gpointer) entry);

  return hbox;
}

void gui_scratchpad_cleanup(void)
{
	/* FIXME: check this out */
}
