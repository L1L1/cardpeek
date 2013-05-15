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

#include "gui.h"
#include "gui_logview.h"
#include "misc.h"
#include <string.h>

/*********************************************************/
/* THE INFAMOUS UGLY GLOBALS *****************************/
/*********************************************************/
  
GtkTextBuffer* LOG_BUFFER=NULL;
GtkWidget* LOG_WINDOW=NULL;
GtkStatusbar *STATUS_BAR=NULL;
guint STATUS_BAR_CONTEXT_ID=0;

/*********************************************************/
/* LOG FUNCTIONS AND UI CALLBACKS ************************/
/*********************************************************/

static void gui_logview_function(int log_level, const char* str)
{
  GtkTextIter iter;
  GtkAdjustment* adj;
  const char* tag;
  char status_bar_text[160];

  if (!LOG_BUFFER) 
    return;

  switch (log_level) {
    case LOG_INFO:    tag = "green_text";
		      break;
    case LOG_DEBUG:   tag = "black_text";
		      break;
    case LOG_WARNING: tag = "purple_text";
		      break;
    case LOG_ERROR:   
    default:
		      tag = "red_text";
  }

  gtk_text_buffer_get_iter_at_offset (LOG_BUFFER,&iter,-1);
  gtk_text_buffer_insert_with_tags_by_name(LOG_BUFFER,&iter,str,-1,tag,NULL);
  if (LOG_WINDOW)
  {
    adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(LOG_WINDOW));
    adj -> value = adj -> upper;
    gtk_adjustment_value_changed(adj);
  }
  if (STATUS_BAR)
  {
    strncpy(status_bar_text,str,160);
    status_bar_text[159]=0;
    
    if (status_bar_text[strlen(status_bar_text)-1]<' ') 
	    status_bar_text[strlen(status_bar_text)-1]=0;

    gtk_statusbar_pop (STATUS_BAR, STATUS_BAR_CONTEXT_ID);
    gtk_statusbar_push (STATUS_BAR, STATUS_BAR_CONTEXT_ID,status_bar_text);
  }
  gui_update(1);
}

GtkWidget *gui_logview_create_window(void)
{
  GtkWidget *view;
  PangoFontDescription *font_desc;

  view = gtk_text_view_new ();
  
  LOG_BUFFER = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  gtk_text_view_set_editable(GTK_TEXT_VIEW (view),FALSE);

  gtk_text_buffer_create_tag(LOG_BUFFER,"black_text",
			     "foreground","black",
			     NULL);

  gtk_text_buffer_create_tag(LOG_BUFFER,"green_text",
			     "foreground","dark green",
			     NULL);

  gtk_text_buffer_create_tag(LOG_BUFFER,"purple_text",
			     "foreground","purple",
			     NULL);

  gtk_text_buffer_create_tag(LOG_BUFFER,"red_text",
			     "foreground","dark red",
			     NULL);

  font_desc = pango_font_description_from_string ("Monospace");
  gtk_widget_modify_font (view, font_desc);
  pango_font_description_free (font_desc);

  LOG_WINDOW = GTK_WIDGET(gtk_scrolled_window_new (NULL, NULL));
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (LOG_WINDOW),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (LOG_WINDOW), view);

  log_set_function(gui_logview_function);

  return GTK_WIDGET(LOG_WINDOW);
}

GtkWidget *gui_logview_create_status_bar(void)
{
  GtkWidget* status = gtk_statusbar_new ();
  STATUS_BAR = GTK_STATUSBAR(status);
  STATUS_BAR_CONTEXT_ID = gtk_statusbar_get_context_id(STATUS_BAR,"main");
  return status;
}

void gui_logview_cleanup(void)
{
	gtk_statusbar_pop (STATUS_BAR, STATUS_BAR_CONTEXT_ID);
	LOG_BUFFER = NULL;
	log_set_function(NULL);
}
