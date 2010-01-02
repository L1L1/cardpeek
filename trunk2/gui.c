/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009 by 'L1L1'
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
#include <dirent.h>
#include "misc.h"
#include "config.h"
#include "smartcard.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
/*#include <sys/time.h>*/

/*****************************************************************/
/* THE INFAMOUS UGLY GLOBALS *************************************/
/*****************************************************************/
  
GtkWidget *MAIN_WINDOW=NULL;
cardtree_t* CARDTREE=0;
GtkTextBuffer* TEXT_BUFFER=NULL;
GtkScrolledWindow* SCROLL_TEXT=NULL;
struct menu_script *SCRIPTS=NULL;
application_callback_t RUN_LUA_SCRIPT = NULL;
application_callback_t RUN_LUA_COMMAND = NULL;
GtkStatusbar *STATUS_BAR=NULL;
guint STATUS_BAR_CONTEXT_ID=0;

/*****************************************************************/
struct menu_script {
        char *script_name;
        char *script_file;
        struct menu_script* prev;
};

void gui_logfunction(int log_level, const char* str)
{
  GtkTextIter iter;
  GtkAdjustment* adj;
  const char* tag;
  char tmp[80];

  if (!TEXT_BUFFER) 
    return;

  switch (log_level) {
    case LOG_INFO:    tag = "green_text";
		      break;
    case LOG_DEBUG:   tag = "green_text";
		      break;
    case LOG_WARNING: tag = "purple_text";
		      break;
    case LOG_ERROR:   tag = "red_text";
		      break;
  }

  gtk_text_buffer_get_iter_at_offset (TEXT_BUFFER,&iter,-1);
  gtk_text_buffer_insert_with_tags_by_name(TEXT_BUFFER,&iter,str,-1,tag,NULL);
  if (SCROLL_TEXT)
  {
    adj = gtk_scrolled_window_get_vadjustment(SCROLL_TEXT);
    adj -> value = adj -> upper;
    gtk_adjustment_value_changed(adj);
  }
  if (STATUS_BAR)
  {
    strncpy(tmp,str,80);
    tmp[79]=0;
    if (tmp[strlen(tmp)-1]<32) tmp[strlen(tmp)-1]=0;
    gtk_statusbar_pop (STATUS_BAR, STATUS_BAR_CONTEXT_ID);
    gtk_statusbar_push (STATUS_BAR, STATUS_BAR_CONTEXT_ID,tmp);
  }
}

/***************************************************************/

void gui_run_script_cb(GtkWidget *widget,
                       gpointer callback_data,
                       guint callback_action)
{
  struct menu_script *script = (struct menu_script *)callback_data;
  RUN_LUA_SCRIPT(script->script_file);
  gui_expand_view();
  gui_update(0);
}

void gui_run_command_cb(GtkWidget *widget,
                        GtkWidget *entry )
{
  const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  RUN_LUA_COMMAND(entry_text);
  gui_update(0);
}

void menu_cb(GtkWidget *w, gint info, GtkWidget *menuitem)
{
  char** select_info;
  a_string_t *command;

  switch(info) {
	case 1: 
	  cardtree_delete_node(CARDTREE,NULL);
	  RUN_LUA_COMMAND("card.CLA=0");
	  log_printf(LOG_INFO,"Cleared card data tree");
	  break;

	case 2:
	  select_info = gui_select_file("Load xml card description",config_get_string(CONFIG_FOLDER_CARDTREES),NULL);
	  if (select_info[1])
	  {  
	    config_set_string(CONFIG_FOLDER_CARDTREES,select_info[0]);
	    command=a_strnew(NULL);
	    a_sprintf(command,"ui.tree_load(\"%s\")",select_info[1]);
	    RUN_LUA_COMMAND(a_strval(command));
	    a_strfree(command);
	    g_free(select_info[0]);
	    g_free(select_info[1]);
	  }	  
	  break;

	case 3:
	  select_info = gui_select_file("Save xml card description",config_get_string(CONFIG_FOLDER_CARDTREES),"card.xml");
	  if (select_info[1])
	  {  
	    config_set_string(CONFIG_FOLDER_CARDTREES,select_info[0]);
	    command=a_strnew(NULL);
	    a_sprintf(command,"ui.tree_save(\"%s\")",select_info[1]);
	    RUN_LUA_COMMAND(a_strval(command));
	    a_strfree(command);
	    g_free(select_info[0]);
	    g_free(select_info[1]);
	  }
	  break;

	case 4:
	  select_info = gui_select_file("Load script",config_get_string(CONFIG_FOLDER_SCRIPTS_OTHER),NULL);
	  if (select_info[1])
	  {  
	    config_set_string(CONFIG_FOLDER_SCRIPTS_OTHER,select_info[0]);
	    RUN_LUA_SCRIPT(select_info[1]);
	    g_free(select_info[0]);
	    g_free(select_info[1]);
	  }
	  break;
 
	case 11:
	  RUN_LUA_COMMAND("card.connect()");
	  break;

	case 12:
	  RUN_LUA_COMMAND("card.warm_reset()");
	  break;

	case 13:
	  RUN_LUA_COMMAND("card.disconnect()");
	  break;

	case 14:
	  if (GTK_CHECK_MENU_ITEM(menuitem)->active)
	  {
	    if (!cardlog_exists())
	      log_printf(LOG_INFO,
			 "Begin recording card data exchange.");
	    else
	      log_printf(LOG_INFO,
			 "Resume recording card data exchange (%d records).",
			 cardlog_count_records()); 
	    cardlog_start();
	  }
	  else
	  {
	    log_printf(LOG_INFO,
		       "Pause recording card data exchange (%d records).",
		       cardlog_count_records());
	    cardlog_stop();
	  }
	  break;

	case 15:
	  select_info = gui_select_file("Save recorded data",config_get_string(CONFIG_FOLDER_LOGS),"card.clf");
	  if (select_info[1])
	  {  
	    cardlog_save(select_info[1]);
	    g_free(select_info[0]);
	    g_free(select_info[1]);
	  }
	  break;


	case 99:
	  RUN_LUA_COMMAND("ui.about()");
	  break;

	default:
	  log_printf(LOG_INFO,"menu option : %i",info);
  }
}

GtkWidget *create_view(cardtree_t *cardtree)
{
  GtkCellRenderer     *renderer;
  GtkWidget           *view;
  GtkWidget           *scrolled_window;
  GtkTreeViewColumn   *column;

  /* Create a new scrolled window, with scrollbars only if needed */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);


  view = gtk_tree_view_new ();
  /* "enable-tree-lines" */
  /* gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW(view),TRUE);*/

  gtk_container_add (GTK_CONTAINER (scrolled_window), view);

  /* --- Column #1 --- */

  column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(column,"Items");

  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_set_attributes(column,renderer,
				      "pixbuf", C_ICON,
				      NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start(column, renderer, TRUE);
  gtk_tree_view_column_set_attributes(column,renderer,
				      "markup", C_MARKUP_NAME_ID,
				      NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

  /* --- Column #2 --- */

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                               -1,
                                               "Value",  
                                               renderer,
                                               "markup", C_MARKUP_VALUE,
                                               NULL);

  /* --- Column #3 --- */

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                               -1,      
                                               "Size",  
                                               renderer,
                                               "text", C_LENGTH,
                                               NULL);

   /* --- Column #4 --- */

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                               -1,      
                                               "Information",  
                                               renderer,
                                               "text", C_COMMENT,
                                               NULL);
  g_object_set(renderer,
               "style-set", TRUE,
               "style", PANGO_STYLE_ITALIC,
               NULL);


  cardtree_bind_to_treeview(cardtree,view);
  /* gtk_widget_set_size_request (scrolled_window,-1,200);*/

  return scrolled_window;
}

GtkWidget *create_text()
{
  GtkWidget *scrolled_window;
  GtkWidget *view;
  PangoFontDescription *font_desc;


  view = gtk_text_view_new ();
  
  TEXT_BUFFER = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

  gtk_text_view_set_editable(GTK_TEXT_VIEW (view),FALSE);

  gtk_text_buffer_create_tag(TEXT_BUFFER,"green_text",
			     "foreground","dark green",
			     NULL);

  gtk_text_buffer_create_tag(TEXT_BUFFER,"purple_text",
			     "foreground","purple",
			     NULL);

  gtk_text_buffer_create_tag(TEXT_BUFFER,"red_text",
			     "foreground","dark red",
			     NULL);

  font_desc = pango_font_description_from_string ("Monospace");
  gtk_widget_modify_font (view, font_desc);
  pango_font_description_free (font_desc);


  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (scrolled_window), view);
/*  insert_text (buffer);

    gtk_widget_show_all (scrolled_window);
    gtk_widget_set_size_request (scrolled_window,-1,100);
*/
  return scrolled_window;
}

static GtkItemFactoryEntry menu_items[] = {
  { "/_Data",                    NULL,         NULL,                  0, "<Branch>" },
  { "/Data/_New",                "<control>N", G_CALLBACK(menu_cb),   1, "<StockItem>", GTK_STOCK_NEW },
  { "/Data/_Open",               "<control>O", G_CALLBACK(menu_cb),   2, "<StockItem>", GTK_STOCK_OPEN },
  { "/Data/_Save",               "<control>S", G_CALLBACK(menu_cb),   3, "<StockItem>", GTK_STOCK_SAVE },
  { "/Data/sep1",                NULL,         NULL,                  0, "<Separator>" },
  { "/Data/_Quit",               "<CTRL>Q",    gtk_main_quit,         0, "<StockItem>", GTK_STOCK_QUIT }, 
  { "/_Analyzer",                NULL,         NULL,                  0, "<Branch>" },
  { "/Analyzer/_Load",           "<control>L", G_CALLBACK(menu_cb),   4, "<StockItem>", GTK_STOCK_OPEN },
  { "/Analyzer/sep1",            NULL,         NULL,                  0, "<Separator>" },
  { "/_Reader",                  NULL,         NULL,                  0, "<Branch>" },
  { "/_Reader/Connect",          NULL,         G_CALLBACK(menu_cb),  11, "<StockItem>", GTK_STOCK_CONNECT },
  { "/_Reader/Reset",            NULL,         G_CALLBACK(menu_cb),  12, "<StockItem>", GTK_STOCK_REDO },
  { "/_Reader/Disconnect",       NULL,         G_CALLBACK(menu_cb),  13, "<StockItem>", GTK_STOCK_DISCONNECT },
  { "/_Reader/sep1",             NULL,         NULL,                  0, "<Separator>" },
  { "/_Reader/Record card data", NULL,         G_CALLBACK(menu_cb),  14, "<CheckItem>" }, 
  { "/_Reader/Save card data",   NULL,         G_CALLBACK(menu_cb),  15, "<Item>" }, 
  { "/_Help",                    NULL,         NULL,                  0, "<LastBranch>" },
  { "/_Help/About",              NULL,         G_CALLBACK(menu_cb),  99, "<StockItem>", GTK_STOCK_INFO },
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

int select_lua(const struct dirent* de)
{
  char *ext=rindex(de->d_name,'.');
  if (ext && strcmp(ext,".lua")==0)
    return 1;
  return 0;
}

GtkWidget *create_menubar(GtkWidget *window)
{
  GtkItemFactory *item_factory;
  GtkAccelGroup *accel_group;
  struct dirent **namelist;
  int n;
  const char *script_path=config_get_string(CONFIG_FOLDER_SCRIPTS);
  GtkItemFactoryEntry fentry;
  struct menu_script* menu_item;
  char *dot;


  /* Make an accelerator group (shortcut keys) */
  accel_group = gtk_accel_group_new ();

  /* Make an ItemFactory (that makes a menubar) */
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
                                       accel_group);

  /* This function generates the menu items. Pass the item factory,
     the number of items in the array, the array itself, and any
     callback data for the the menu items. */
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

  if (script_path)
  {
    n = scandir(script_path, &namelist, select_lua, alphasort);
    if (n > 0)
    {
      while(n--) 
      {
	log_printf(LOG_INFO,"Adding %s script to menu", namelist[n]->d_name);

        menu_item=(struct menu_script *)malloc(sizeof(struct menu_script));
	menu_item->script_name = (char* )malloc(strlen(namelist[n]->d_name)+11);
	strcpy(menu_item->script_name,"/Analyzer/");
	strcat(menu_item->script_name,namelist[n]->d_name);
	dot = rindex(menu_item->script_name,'.');
	if (dot)
	  *dot=0;
	menu_item->script_file = strdup(namelist[n]->d_name);
	menu_item->prev=SCRIPTS;
	SCRIPTS = menu_item;

	fentry.path=menu_item->script_name;
	fentry.callback=G_CALLBACK(gui_run_script_cb);
	fentry.callback_action=0;
	fentry.item_type="<StockItem>";
	fentry.extra_data=GTK_STOCK_PROPERTIES;
	gtk_item_factory_create_item(item_factory,&fentry,menu_item,2);
	free(namelist[n]);
      }
      free(namelist);
    }
    else
      log_printf(LOG_WARNING,"No scripts found in %s",script_path);
  }

  /* Attach the new accelerator group to the window. */
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  /* Finally, return the actual menu bar created by the item factory. */
  return gtk_item_factory_get_widget (item_factory, "<main>");
}

GtkWidget *create_command_entry()
{
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *entry;

  entry = gtk_entry_new();
  label = gtk_label_new("Command :");
  hbox = gtk_hbox_new (FALSE, 4);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 2);

  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (gui_run_command_cb),
		    (gpointer) entry);

  return hbox;
}

void gui_expand_view()
{
  gtk_tree_view_expand_all (CARDTREE->_view);
}

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

  item_table=malloc(sizeof(const char *)*item_count);
  item_count=0;

  va_start(al,message);
  while ((item=va_arg(al,const char*)))
    item_table[item_count++]=item;
  va_end(al);

  retval = gui_question_l(message,item_count,item_table);
  free(item_table);
  return retval;
}

int gui_question_l(const char *message, unsigned item_count, const char** items)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *img;
  gint result;

  dialog = gtk_dialog_new_with_buttons ("Question",
					GTK_WINDOW(MAIN_WINDOW),
					GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
					NULL);
  for (result=0;result<item_count;result++)
    gtk_dialog_add_button(GTK_DIALOG(dialog),items[result],result);

  img = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
				 GTK_ICON_SIZE_DIALOG);

  label = gtk_label_new(message);

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

void gui_about()
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_INFO,
				   GTK_BUTTONS_OK,
				   "%s",
				   "cardpeek, version 0.1\nCopyright 2009, by 'L1L1'\nLicenced under the GPL 3");
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/* A clock with a resolution of 1/10th of a second */
/*
unsigned time10()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return ((tv.tv_sec%60)*10)+(tv.tv_usec/100000);
}
*/

void gui_update(int lag_allowed)
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

int gui_create(application_callback_t run_script_cb,
	       application_callback_t run_command_cb)
{
  GtkWidget *view;
  GtkWidget *text;
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *frame1;
  GtkWidget *frame2;
  GtkWidget *status;
  GtkWidget *menu;
  GtkWidget *tabs;
  GtkWidget *label;

  RUN_LUA_SCRIPT = run_script_cb;
  RUN_LUA_COMMAND = run_command_cb;

  /* main window start */

  MAIN_WINDOW = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(MAIN_WINDOW),600,400);
  gtk_container_set_border_width (GTK_CONTAINER (MAIN_WINDOW), 0);
  g_signal_connect (MAIN_WINDOW, "delete_event", gtk_main_quit, NULL); /* dirty */
 
  /* log text zone */

  text = create_text ();
  SCROLL_TEXT = GTK_SCROLLED_WINDOW(text);
  frame2 = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame2),GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER(frame2),text);

  log_set_function(gui_logfunction);

  /* tree view */

  CARDTREE = cardtree_new();

  view = create_view (CARDTREE);

  frame1 = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame1),GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER(frame1),view);

  /* menu bar */

  menu = create_menubar(MAIN_WINDOW);
  
 /* command entry */

  entry = create_command_entry ();
  
  /* status bar */

  status = gtk_statusbar_new ();
  STATUS_BAR = GTK_STATUSBAR(status);
  STATUS_BAR_CONTEXT_ID = gtk_statusbar_get_context_id(STATUS_BAR,"main");

  /* notebook */

  tabs = gtk_notebook_new ();
  gtk_notebook_set_tab_border(GTK_NOTEBOOK (tabs),4);

  label = gtk_label_new ("card view");
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), frame1, label);

  label = gtk_label_new ("log");
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), frame2, label);

  /* vertical packing */

  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), menu, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), tabs, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), status, FALSE, TRUE, 0);
 
  /* main window finish */
 
  gtk_container_add (GTK_CONTAINER (MAIN_WINDOW), vbox);

  gtk_widget_show_all (MAIN_WINDOW);

  /*gtk_widget_size_request(vpaned,&requisition);*/

  /*gtk_paned_set_position(GTK_PANED (vpaned), 256);*/

  return 1;
}


int gui_run()
{
  struct menu_script *it;
  
  gtk_main ();

  TEXT_BUFFER=NULL;

  while (SCRIPTS)
  {
     it=SCRIPTS->prev;
     free(SCRIPTS->script_name);
     free(SCRIPTS->script_file);
     free(SCRIPTS);
     SCRIPTS=it;
  }
  
  if (cardlog_exists())
    cardlog_clear();

  /* FIXME: do some more cleanup */
 
  return 1;
}

