/********************************************************************** 
*
* This file is part of Cardpeek, the smart card reader utility.
*
* Copyright 2009-2014 by Alain Pannetrat <L1L1@gmx.com>
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

#include <stdlib.h>
#include "ui.h"
#include "gui_readerview.h"
#include "gui_toolbar.h"
#include "smartcard.h"
#include "a_string.h"
#include "misc.h"
#include "pathconfig.h"
#include "lua_ext.h"
#include "string.h"
#include "iso7816.h"

/*********************************************************/
/* THE INFAMOUS UGLY GLOBALS *****************************/
/*********************************************************/
  
GtkTextBuffer* READER_BUFFER=NULL;


/*********************************************************/
/* MENU CALLBACK: only one here, others call lua stuff.  */
/*********************************************************/


static void menu_readerview_save_as_cb(GtkWidget *w, gpointer user_data)
{
  char** select_info;
  a_string_t *command;
  char *filename;
  UNUSED(w);
  UNUSED(user_data);

  select_info = ui_select_file("Save recorded data",path_config_get_string(PATH_CONFIG_FOLDER_REPLAY),"card.clf");
  if (select_info[1])
  {  
    filename = luax_escape_string(select_info[1]);
    command=a_strnew(NULL);
    a_sprintf(command,"card.log_save(\"%s\")",filename);
    luax_run_command(a_strval(command));
    a_strfree(command);
    g_free(select_info[0]);
    g_free(select_info[1]);
    g_free(filename);
  }
}

/*********************************************************/
/* CONSTRUTION OF MAIN UI ********************************/
/*********************************************************/

toolbar_item_t TB_READER_VIEW[] = {
	{ "reader-view-connect", 	"gtk-connect", "Connect", G_CALLBACK(gui_toolbar_run_command_cb), "card.connect()", 
	  "Connect a card to the reader." },
	{ "reader-view-reset", 		"edit-redo", "Reset", G_CALLBACK(gui_toolbar_run_command_cb), "card.warm_reset()", 
	  "Reset the card in the reader." },
	{ "reader-view-disconnect", 	"gtk-disconnect", "Disconnect", G_CALLBACK(gui_toolbar_run_command_cb), "card.disconnect()", 
	  "Diconnect the card in the reader." },
	{ NULL,				TOOLBAR_ITEM_SEPARATOR, NULL, NULL, NULL, NULL },
	{ "reader-view-clear", 		"edit-clear", "Clear", G_CALLBACK(gui_toolbar_run_command_cb), "card.log_clear()", 
	  "Clear the reader view." },
	{ "reader-view-save-as", 	"document-save-as", "Save replay", G_CALLBACK(menu_readerview_save_as_cb), NULL, 
	  "Save card/reader data exchange for later replay\nYou can then select a replay during application startup." },	
	{ NULL, 			NULL, NULL, NULL, NULL, NULL }
};


GtkWidget *gui_readerview_create_window(void)
{
  GtkWidget           *view;
  GtkWidget           *scrolled_window;
  GtkWidget           *base_container;
  GtkWidget           *toolbar;
  PangoFontDescription *font_desc;

  /* Create base window container */

  base_container = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);

  /* Create the toolbar */

  toolbar = gui_toolbar_new(TB_READER_VIEW);

  gtk_box_pack_start (GTK_BOX (base_container), toolbar, FALSE, FALSE, 0);

  /* Create a new scrolled window, with scrollbars only if needed */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  
  gtk_box_pack_end (GTK_BOX (base_container), scrolled_window, TRUE, TRUE, 0);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);


  view = gtk_text_view_new ();

  font_desc = pango_font_description_from_string ("Monospace");
  gtk_widget_override_font (view, font_desc);
  pango_font_description_free (font_desc);

  gtk_container_add (GTK_CONTAINER (scrolled_window), view);

  /* Reader buffer */ 
 
  READER_BUFFER = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  gtk_text_view_set_editable(GTK_TEXT_VIEW (view),FALSE);

  gtk_text_buffer_create_tag(READER_BUFFER,"green_text",
			     "foreground","dark green",
			     NULL);

  gtk_text_buffer_create_tag(READER_BUFFER,"blue_text",
			     "foreground","dark blue",
			     NULL);

  gtk_text_buffer_create_tag(READER_BUFFER,"red_text",
			     "foreground","dark red",
			     NULL);

  return base_container;
}

char HEX_CHAR[17]="0123456789ABCDEF";
static const char* hex_pretty_print(int indent, const bytestring_t *bs,int add_ascii)
{
  static char retval[1500];
  int i;
  int offset;
  int line;
  int sizebs=bytestring_get_size(bs);
  int linesize=indent+48+4+16+1;
  unsigned char e;

  if (sizebs==0)
    return "(nil)\n";

  for (i=0;i<sizebs;i++)
  {
    offset = (i&0x0F);
    line = (i&0xFFF0)>>4;
    if (offset==0)
    {
      memset(retval+line*linesize,' ',linesize);
      retval[line*linesize+linesize-1]='\n';
      retval[line*linesize+linesize]=0;
    }
    bytestring_get_element(&e,bs,i);
    retval[line*linesize+indent+offset*3]=HEX_CHAR[(e>>4)];
    retval[line*linesize+indent+offset*3+1]=HEX_CHAR[e&0xF];
    if (add_ascii)
    {
      if (e>=' ' && e<=126)
	retval[line*linesize+indent+52+offset]=e;
      else
	retval[line*linesize+indent+52+offset]='.';
    }
  }
  return retval+indent;
}

void gui_readerview_print(unsigned event,
                          const bytestring_t *command,
                          unsigned short sw,
                          const bytestring_t *response,
                          void *extra_data)
{
    const char* text;
    char buf[200];
    GtkTextIter iter;
    char *string_sw = NULL;
    UNUSED(extra_data);


    if (event==CARDREADER_EVENT_RESET || event==CARDREADER_EVENT_CONNECT)
    {
        gtk_text_buffer_get_iter_at_offset (READER_BUFFER,&iter,-1);
        gtk_text_buffer_insert_with_tags_by_name(READER_BUFFER,&iter,"RSET ",-1,"red_text",NULL);
        text = hex_pretty_print(5,command,0);
        gtk_text_buffer_get_iter_at_offset (READER_BUFFER,&iter,-1);
        gtk_text_buffer_insert_with_tags_by_name(READER_BUFFER,&iter,text,-1,"red_text",NULL);
    }
    else if (event==CARDREADER_EVENT_TRANSMIT)
    {
        gtk_text_buffer_get_iter_at_offset (READER_BUFFER,&iter,-1);
        gtk_text_buffer_insert_with_tags_by_name(READER_BUFFER,&iter,"SEND ",-1,"green_text",NULL);
        text = hex_pretty_print(5,command,0);
        gtk_text_buffer_get_iter_at_offset (READER_BUFFER,&iter,-1);
        gtk_text_buffer_insert_with_tags_by_name(READER_BUFFER,&iter,text,-1,"green_text",NULL);

        luax_variable_call("card.stringify_sw","u>s",sw,&string_sw);
        snprintf(buf,200,"RECV %04X                                                # %s\n     ",sw,string_sw);
        if (string_sw) free(string_sw);

        gtk_text_buffer_get_iter_at_offset (READER_BUFFER,&iter,-1);
        gtk_text_buffer_insert_with_tags_by_name(READER_BUFFER,&iter,buf,-1,"blue_text",NULL);
        text = hex_pretty_print(5,response,1);
        if (text) {
            gtk_text_buffer_get_iter_at_offset (READER_BUFFER,&iter,-1);
            gtk_text_buffer_insert_with_tags_by_name(READER_BUFFER,&iter,text,-1,"blue_text",NULL);
        }
    } 
    else if (event==CARDREADER_EVENT_CLEAR_LOG)
    {
        gtk_text_buffer_set_text(READER_BUFFER,"",0);
    }
    ui_update();
}

void gui_readerview_cleanup(void)
{
	READER_BUFFER=NULL;
}
