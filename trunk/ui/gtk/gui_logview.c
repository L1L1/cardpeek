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

#include "ui.h"
#include "gui_core.h"
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
#ifdef CUSTOM_GTK_LOG
guint CUSTOM_LOG_FUNCTION = 0;
#endif

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
    gtk_adjustment_set_value(adj,gtk_adjustment_get_upper(adj));
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
  ui_update();
}

#ifdef CUSTOM_GTK_LOG
static void custom_log_function(const char *domain, 
        GLogLevelFlags log_level, 
        const gchar *message,
        gpointer user_data)
{
    switch (log_level) {
        case G_LOG_LEVEL_ERROR:
            log_printf(LOG_ERROR,"%s",message);
            break;
        case G_LOG_LEVEL_CRITICAL:
            log_printf(LOG_ERROR,"(*critical*) %s",message);
            break;
        case G_LOG_LEVEL_WARNING:
            log_printf(LOG_WARNING,"%s",message);
            break;
        case G_LOG_LEVEL_MESSAGE:
            log_printf(LOG_INFO,"(*message*) %s",message);
            break;
        case G_LOG_LEVEL_INFO:
            log_printf(LOG_INFO,"%s",message);
            break;
        default:
            log_printf(LOG_DEBUG,"%s",message);
    }
}
#endif
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
  gtk_widget_override_font (view, font_desc);
  pango_font_description_free (font_desc);

  LOG_WINDOW = GTK_WIDGET(gtk_scrolled_window_new (NULL, NULL));
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (LOG_WINDOW),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (LOG_WINDOW), view);

  log_set_function(gui_logview_function);

#ifdef CUSTOM_GTK_LOG
  CUSTOM_LOG_FUNCTION = g_log_set_handler(NULL, 
                                          G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, 
                                          custom_log_function, 
                                          NULL); 
#endif

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
	LOG_BUFFER = NULL;
#ifdef CUSTOM_GTK_LOG
    g_log_remove_handler(NULL,CUSTOM_LOG_FUNCTION);
#endif
	log_set_function(NULL);
	LOG_WINDOW=NULL;
	STATUS_BAR=NULL;
}
