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
#include "gui_cardview.h"
#include "gui_readerview.h"
#include "gui_logview.h"
#include "gui_scratchpad.h"
#include "misc.h"
#include "string.h"

#include "cardpeek_resources.gresource"

/*********************************************************/
/* THE INFAMOUS UGLY GLOBALS *****************************/
/*********************************************************/
  
GtkWidget *MAIN_WINDOW=NULL;

/*********************************************************/
/* GUI_* FUNCTIONS ***************************************/
/*********************************************************/

int gui_question(const char *message, ...)
{
  va_list al;
  const char *item;
  unsigned item_count=0;
  const char** item_table;
  gint retval;

  va_start(al,message);
  while (va_arg(al,const char*)) item_count++;
  va_end(al);

  item_table=g_malloc(sizeof(const char *)*item_count);
  item_count=0;

  va_start(al,message);
  while ((item=va_arg(al,const char*)))
    item_table[item_count++]=item;
  va_end(al);

  retval = gui_question_l(message,item_count,item_table);
  g_free(item_table);
  return retval;
}

int gui_question_l(const char *message, unsigned item_count, const char** items)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *img;
  int result;

  dialog = gtk_dialog_new_with_buttons ("Question",
					GTK_WINDOW(MAIN_WINDOW),
					GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
					NULL);
  for (result=0;result<(int)item_count;result++)
    gtk_dialog_add_button(GTK_DIALOG(dialog),items[result],result);

  img = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
				 GTK_ICON_SIZE_DIALOG);

  label = gtk_label_new(message);

  gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);

  hbox = gtk_hbox_new(FALSE,0);
  
  gtk_box_pack_start(GTK_BOX(hbox), img, FALSE, FALSE, 10);

  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

  gtk_widget_show_all(hbox);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
		     hbox); 

  result = gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  if (result==GTK_RESPONSE_NONE)
    return -1;
  return result;
}

int gui_readline(const char *message, unsigned input_max, char* input)
/* input_max does not include final '\0' */
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *img;
  GtkWidget *entry;
  gint result;

  if (input_max==0)
  {
    log_printf(LOG_ERROR,"'gui_readline' was called with maximum input length set to 0");
    return 0;
  }
  if (input==NULL)
  {
    log_printf(LOG_ERROR,"'gui_readline' was called with a NULL value");
    return 0;
  }

  dialog = gtk_dialog_new_with_buttons("Question",
				       GTK_WINDOW(MAIN_WINDOW),
				       GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
				       GTK_STOCK_OK,
				       GTK_RESPONSE_ACCEPT,
				       NULL);

  img = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
				 GTK_ICON_SIZE_DIALOG);

  vbox = gtk_vbox_new(FALSE,0);

  label = gtk_label_new(message);

  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

  entry = gtk_entry_new_with_max_length(input_max);
  gtk_entry_set_width_chars(GTK_ENTRY(entry),input_max<=80?input_max:80);

  gtk_entry_set_text(GTK_ENTRY(entry),input);

  gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 10);

  hbox = gtk_hbox_new(FALSE,0);
  
  gtk_box_pack_start(GTK_BOX(hbox), img, FALSE, FALSE, 10);

  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 10);

  gtk_widget_show_all(hbox);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
		     hbox);

  result = gtk_dialog_run (GTK_DIALOG (dialog));

  if (result!=GTK_RESPONSE_ACCEPT)
  {
    input[0]=0;
    return 0;
  }
  strncpy(input,gtk_entry_get_text(GTK_ENTRY(entry)),input_max);
  input[input_max]=0; /* this should not be needed */
  gtk_widget_destroy(dialog);
  return 1;
}

char** gui_select_file(const char *title,
                       const char *path,
                       const char *filename)
{
  static char *ret_values[2];
  GtkWidget *filew;

  if (filename==NULL) /* open a file - filename==NULL */
  {
    filew = gtk_file_chooser_dialog_new (title,
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_OPEN,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					 NULL);
  }
  else /* save a file - filename contains the default name */
  {
    filew = gtk_file_chooser_dialog_new (title,
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					 GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					 NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (filew), TRUE);
  }

  if (path)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (filew), path); 
  if (filename)  
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (filew), filename);

  if (gtk_dialog_run (GTK_DIALOG (filew)) == GTK_RESPONSE_ACCEPT)
  {
    ret_values[0] = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (filew));
    ret_values[1] = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filew));
  }
  else
  {
    ret_values[0] = NULL;
    ret_values[1] = NULL;
  }
  gtk_widget_destroy (filew);
  return ret_values;
}

void gui_update(unsigned lag_allowed)
{
  static unsigned ref=0;
  unsigned now = time(NULL);

  if (now-ref<lag_allowed)
    return;
  ref = now;
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

int gui_init(int *argc, char ***argv)
{
  gtk_init(argc,argv);
  return 1;
}

void gui_exit(void)
{
	/* FIXME: UGLY EXIT */
	log_close_file();
	gtk_exit(0);
}

char* gui_select_reader(unsigned list_size, const char** list)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *combo;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *img;
  unsigned i;
  gint result;
  char *retval;


  dialog = gtk_dialog_new_with_buttons("Select card reader",
				       GTK_WINDOW(MAIN_WINDOW),
				       GTK_DIALOG_MODAL,
				       GTK_STOCK_OK,
				       GTK_RESPONSE_ACCEPT,
				       NULL);

  img = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
				 GTK_ICON_SIZE_DIALOG);

  vbox = gtk_vbox_new(FALSE,0);

  label = gtk_label_new("Select a card reader to use");

  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

  combo = gtk_combo_box_new_text();

  gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 10);

  for (i=0;i<list_size;i++)
  {
     gtk_combo_box_append_text(GTK_COMBO_BOX(combo),list[i]);
  }
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),"none");
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo),0);

  hbox = gtk_hbox_new(FALSE,0);
  
  gtk_box_pack_start(GTK_BOX(hbox), img, FALSE, FALSE, 10);

  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 10);

  gtk_widget_show_all(hbox);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
		     hbox);

  result = gtk_dialog_run (GTK_DIALOG (dialog));

  switch (result)
  {
    case GTK_RESPONSE_ACCEPT:
      retval = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));
      log_printf(LOG_INFO,"Selected '%s'",retval);
      if (strcmp(retval,"none")==0)
      {
	g_free(retval);
	retval = NULL;
      }
      break;
    default:
      retval = NULL;
      log_printf(LOG_INFO,"Select empty");
      break;
  }
  gtk_widget_destroy (dialog);
  return retval;
  /*if (retval) g_free(retval);
  return NULL;*/
}

const char *icon_resources[] = {
    "/cardpeek/icons/analyzer.png", 	"cardpeek-analyzer",
    "/cardpeek/icons/application.png",	"cardpeek-application",
    "/cardpeek/icons/block.png", 	"cardpeek-block",
    "/cardpeek/icons/record.png", 	"cardpeek-record",
    "/cardpeek/icons/cardpeek.png", 	"cardpeek-cardpeek",
    "/cardpeek/icons/file.png", 	"cardpeek-file",
    "/cardpeek/icons/folder.png", 	"cardpeek-folder",
    "/cardpeek/icons/item.png", 	"cardpeek-item", 
    "/cardpeek/icons/smartcard.png", 	"cardpeek-smartcard",
    "/cardpeek/icons/atr.png", 		"cardpeek-atr",
    "/cardpeek/icons/header.png", 	"cardpeek-header",
    "/cardpeek/icons/body.png", 	"cardpeek-body",
    NULL
};

static int gui_load_icons(void)
{
  GResource* cardpeek_resources;
  GBytes* icon_bytes;
  unsigned char *icon_bytes_start;
  gsize icon_bytes_size;
  GtkIconSet* icon_set;
  GdkPixbuf* pixbuf;
  GtkIconFactory* icon_factory;
  unsigned u;
  int is_ok = 1; 

  cardpeek_resources = cardpeek_resources_get_resource();
  if (cardpeek_resources == NULL)
  {
        log_printf(LOG_ERROR,"Could not load cardpeek internal resources. This is not good.");
        return 0; 
  }
  
  icon_factory = gtk_icon_factory_new();

  u=0;
  while (icon_resources[u*2])
  {
	  icon_bytes = g_resources_lookup_data(icon_resources[u*2],G_RESOURCE_LOOKUP_FLAGS_NONE,NULL);
	  if (icon_bytes == NULL)
	  {
		  log_printf(LOG_ERROR,"Could not load icon %s",icon_resources[u*2]);
		  is_ok = 0;
	  }

	  icon_bytes_start = (unsigned char *)g_bytes_get_data(icon_bytes,&icon_bytes_size);  

	  pixbuf = gdk_pixbuf_new_from_inline (-1, icon_bytes_start, FALSE, NULL);
	  icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
	  gtk_icon_factory_add(icon_factory,icon_resources[u*2+1],icon_set);

	  if (strcmp(icon_resources[u*2+1],"cardpeek-cardpeek")==0)
		  gtk_window_set_default_icon (pixbuf);

	  g_bytes_unref(icon_bytes);

	  u++;
  }

  gtk_icon_factory_add_default(icon_factory);
  return is_ok;
} 

int gui_create(void)
{ 
  GtkWidget *widget;
  GtkWidget *cardview;
  GtkWidget *logview;
  GtkWidget *readerview;
  GtkWidget *statusbar;
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *tabs;
  GtkWidget *label;
  
  /* gui_widget_table_init(); */

  /* Build icon sets */

  gui_load_icons();

  /* main window start */

  MAIN_WINDOW = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(MAIN_WINDOW),600,400);
  gtk_container_set_border_width (GTK_CONTAINER (MAIN_WINDOW), 0);
  g_signal_connect (MAIN_WINDOW, "delete_event", gtk_main_quit, NULL); /* dirty */

  /* log frame */

  widget = gui_logview_create_window();
  logview = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(logview),GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER(logview),widget);

  /* reader view frame */

  widget = gui_readerview_create_window();
  readerview = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(readerview),GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER(readerview),widget);

  /* tree view frame */

  widget = gui_cardview_create_window();
  cardview = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(cardview),GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER(cardview),widget);
 
  /* command entry */

  entry = gui_scratchpad_create_window ();

  /* status bar */
  
  statusbar = gui_logview_create_status_bar();
  
  /* notebook */

  tabs = gtk_notebook_new ();
  g_object_set(G_OBJECT (tabs), "tab-border", 4, NULL);

  label = gtk_label_new ("card view");
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), cardview, label);

  label = gtk_label_new ("reader");
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), readerview, label);

  label = gtk_label_new ("logs");
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), logview, label);

  /* vertical packing */

  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), tabs, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);
 
  /* main window finish */

  gtk_container_add (GTK_CONTAINER (MAIN_WINDOW), vbox);

  gtk_widget_show_all (MAIN_WINDOW);

  return 1;
}


int gui_run(void)
{
  gtk_main ();
  
  gui_scratchpad_cleanup();
  gui_logview_cleanup();
  gui_readerview_cleanup();
  gui_cardview_cleanup();
  return 1;
}


