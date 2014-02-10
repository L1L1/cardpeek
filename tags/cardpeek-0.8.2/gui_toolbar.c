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

#include "gui_toolbar.h"
#include "string.h"
#include "misc.h"
#include "lua_ext.h"

/*********************************************************/
/* EXPERIMENTAL ******************************************/
/*********************************************************/
/*
GHashTable *WIDGET_TABLE = NULL;

static guint stringhash(gconstpointer str)
{
	const unsigned char *s = str;
	guint res=0;
	while (*s)
	{
		res = (res*27)+(*s);
		s++;
	}
	return res;
}

static gint stringcompare(gconstpointer a, gconstpointer b)
{
	return (strcmp(a,b)==0);
}

static gboolean gui_widget_table_init(void)
{
	WIDGET_TABLE = g_hash_table_new(stringhash,stringcompare);
	return TRUE;
}

static void gui_widget_table_release(void)
{
	g_hash_table_destroy(WIDGET_TABLE);
}

#ifdef _DEAD_CODE_
static GtkWidget *gui_widget_table_lookup(const gchar *name)
{
	return (GtkWidget *)g_hash_table_lookup(WIDGET_TABLE,name);
}
#endif

static void gui_widget_table_insert(const gchar *name, const GtkWidget *widget)
{
	g_hash_table_insert(WIDGET_TABLE,(gpointer)name,(gpointer)widget);	
}
*/


GtkWidget *gui_toolbar_new(toolbar_item_t *tbitems)
{
	GtkWidget	*toolbar;
	GtkToolItem	*item;
	int 		i;

	toolbar = gtk_toolbar_new();

	gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar), GTK_ORIENTATION_HORIZONTAL );
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);

	for (i=0; tbitems[i].icon!=NULL; i++)
	{
		if (strcmp(tbitems[i].icon,TOOLBAR_ITEM_SEPARATOR)==0)
		{
			item = gtk_separator_tool_item_new();
			gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);
			
		}
		else if (strcmp(tbitems[i].icon,TOOLBAR_ITEM_EXPANDER)==0)
		{
			item = gtk_separator_tool_item_new();
			gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM(item),FALSE);
			gtk_tool_item_set_expand(GTK_TOOL_ITEM(item),TRUE);
			gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

		}
		else
		{
			item = gtk_tool_button_new_from_stock (tbitems[i].icon);
			if (tbitems[i].text) 
				gtk_tool_button_set_label(GTK_TOOL_BUTTON(item),tbitems[i].text);
			if (tbitems[i].callback)
				g_signal_connect(G_OBJECT(item),"clicked",G_CALLBACK(tbitems[i].callback),(gpointer)tbitems[i].callback_data);
			if (tbitems[i].tooltip)
				gtk_widget_set_tooltip_text(GTK_WIDGET(item),tbitems[i].tooltip);
			gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);
		}
		/*
		if (tbitems[i].id)
			gui_widget_table_insert(tbitems[i].id, GTK_WIDGET(item));
		*/
	}
	return toolbar;
}

void gui_toolbar_run_command_cb(GtkWidget *w, gconstpointer user_data)
{
  UNUSED(w);

  if (user_data)
    luax_run_command((char *)user_data);
  else
    log_printf(LOG_ERROR,"No command to execute");
}

