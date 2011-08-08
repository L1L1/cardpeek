/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009-2011 by 'L1L1'
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
#include "pathconfig.h"
#include "smartcard.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "iso7816.h"
#ifndef _WIN32
#include "config.h"
#else
#include "win32/config.h"
#include "win32/win32compat.h"
#endif

#include "icons.c"

/*********************************************************/
/* THE INFAMOUS UGLY GLOBALS *****************************/
/*********************************************************/
  
GtkWidget *MAIN_WINDOW=NULL;
cardtree_t* CARDTREE=0;
GtkTextBuffer* LOG_BUFFER=NULL;
GtkTextBuffer* READER_BUFFER=NULL;
GtkScrolledWindow* SCROLL_TEXT=NULL;
GtkWidget* CARDVIEW=NULL;
GtkStatusbar *STATUS_BAR=NULL;
GtkListStore *COMPLETION=NULL;

struct menu_script *SCRIPTS=NULL;
application_callback_t RUN_LUA_SCRIPT = NULL;
application_callback_t RUN_LUA_COMMAND = NULL;
guint STATUS_BAR_CONTEXT_ID=0;

/*********************************************************/
/* JUST ONE SIMPLE FUNC TO ESCAPE LUA STRINGS ************/
/*********************************************************/

char *lua_escape_string(const char *src)
{
	char *res;
	const char *s;
	char *p;
	unsigned alloc_count = 1;

	for (s=src;*s;s++) 
	{
		if (*s=='\\') 
			alloc_count+=2;
		else
			alloc_count++;
	}
	p = res = malloc(alloc_count);
	while (*src)
	{
		if (*src=='\\')
		{
			*p++='\\';
			*p++='\\';
		}
		else
			*p++=*src;
		src++;
	}
	*p='\0';
	return res;
}

/*********************************************************/
/* LOG FUNCTIONS AND UI CALLBACKS ************************/
/*********************************************************/


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
  gui_update(1);
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
  GtkTreeIter iter;
  const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  RUN_LUA_COMMAND(entry_text);
  gtk_list_store_append(COMPLETION,&iter);
  gtk_list_store_set(COMPLETION,&iter,0,entry_text,-1);
  gui_update(0);
}

void menu_card_view_clear_cb(GtkWidget *w, gpointer user_data)
{
  cardtree_node_delete(CARDTREE,NULL);
  RUN_LUA_COMMAND("card.CLA=0");
  log_printf(LOG_INFO,"Cleared card data tree");
}

void menu_card_view_open_cb(GtkWidget *w, gpointer user_data)
{
  char** select_info;
  a_string_t *command;
  char *filename;

  select_info = gui_select_file("Load xml card description",config_get_string(CONFIG_FOLDER_CARDTREES),NULL);
  if (select_info[1])
  {
    config_set_string(CONFIG_FOLDER_CARDTREES,select_info[0]);
    filename = lua_escape_string(select_info[1]);
    command=a_strnew(NULL);
    a_sprintf(command,"ui.tree_load(\"%s\")",filename);
    RUN_LUA_COMMAND(a_strval(command));
    a_strfree(command);
    g_free(select_info[0]);
    g_free(select_info[1]);
    free(filename);
  }
}

void menu_card_view_save_as_cb(GtkWidget *w, gpointer user_data)
{
  char** select_info;
  a_string_t *command;
  char *filename;

  select_info = gui_select_file("Save xml card description",config_get_string(CONFIG_FOLDER_CARDTREES),"card.xml");
  if (select_info[1])
  {  
    config_set_string(CONFIG_FOLDER_CARDTREES,select_info[0]);
    filename = lua_escape_string(select_info[1]);
    command=a_strnew(NULL);
    a_sprintf(command,"ui.tree_save(\"%s\")",filename);
    RUN_LUA_COMMAND(a_strval(command));
    a_strfree(command);
    g_free(select_info[0]);
    g_free(select_info[1]);
    free(filename);
  }
}

/*
void menu_cardview_row_activated (GtkTreeView        *treeview,
				 GtkTreePath        *path,
				 GtkTreeViewColumn  *col,
				 gpointer            userdata)
{
  unsigned flags;
  char* string_path = gtk_tree_path_to_string(path);
  cardtree_get_flags(CARDTREE,string_path,&flags);
  cardtree_set_flags(CARDTREE,string_path,flags^C_FLAG_DISPLAY_FULL);
  g_free(string_path);
}
*/
void menu_cardview_cursor_changed(GtkTreeView *treeview,
				  gpointer userdata)
{
	GtkTreeViewColumn* column2 = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),2);
	GtkTreeViewColumn* column3 = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),3);
	unsigned flags,size;
	GtkTreePath *gtktreepath;
	char *path;
	GtkTreeViewColumn *column;
	
	gtk_tree_view_get_cursor(treeview,&gtktreepath,&column);
	path = cardtree_path_from_gtktreepath(gtktreepath);
	
	if (path)
	{
		if ((column==column2 || column==column3))
		{
			cardtree_value_strlen(CARDTREE,path,&size);
			if (size>=256) 
			{
				cardtree_flags_get(CARDTREE,path,&flags);
				cardtree_flags_set(CARDTREE,path,flags^C_FLAG_DISPLAY_FULL);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (CARDVIEW),gtktreepath,NULL,TRUE,0,0);
			}
		}
		cardtree_path_free(path);
		gtk_tree_path_free(gtktreepath);
	}
}

void menu_cardview_column_activated(GtkTreeViewColumn *treeviewcolumn,
	       			    gpointer           user_data) 
{
  GtkTreeViewColumn* column2 = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),2);
  GtkTreeViewColumn* column3 = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),3);
  if (treeviewcolumn==column2)
  {
    gtk_tree_view_column_set_visible (column2,FALSE);
    gtk_tree_view_column_set_visible (column3,TRUE);
  }
  else
  {
    gtk_tree_view_column_set_visible (column3,FALSE);
    gtk_tree_view_column_set_visible (column2,TRUE);
  }
}

gboolean menu_cardview_query_tooltip(GtkWidget  *widget,
		   		     gint        x,
				     gint        y,
				     gboolean    keyboard_mode,
				     GtkTooltip *tooltip,
				     gpointer    user_data) 
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  unsigned flags;
  unsigned size;

  if (gtk_tree_view_get_tooltip_context(GTK_TREE_VIEW(widget),&x,&y,keyboard_mode,&model,NULL,&iter))
  {
	gtk_tree_model_get(model,&iter,C_FLAGS,&flags,C_SIZE,&size,-1);
	
	if (size>256)
	{
		if (flags & C_FLAG_DISPLAY_FULL)
			gtk_tooltip_set_text(tooltip,"Click on the data in the last column to reduce\n"
						     "the displayed data to the first 256 bytes");
		else
			gtk_tooltip_set_text(tooltip,"Click on the data in the last column to display\n"
						     "it in full");
	}
	else
	{
		gtk_tooltip_set_text(tooltip,"Right-click for additional tools");
	}
	return TRUE;
  }
  return FALSE;
}


void menu_cardview_context_menu_change_value_type(GtkWidget *menuitem, gpointer userdata)
/* Callback responding to right click context menu item to change 3rd column format */ 
{
  GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
  GtkTreeViewColumn* column2 = gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),2);
  GtkTreeViewColumn* column3 = gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),3);

  if (gtk_tree_view_column_get_visible(column2))
  {
    gtk_tree_view_column_set_visible (column2,FALSE);
    gtk_tree_view_column_set_visible (column3,TRUE);	
  }
  else
  {
    gtk_tree_view_column_set_visible (column2,TRUE);
    gtk_tree_view_column_set_visible (column3,FALSE); 
  }
}


void menu_cardview_context_menu_expand_all(GtkWidget *menuitem, gpointer userdata)
/* Callback responding to right click context menu item to expand current branch */ 
{
  GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
  GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath *path;

  if (gtk_tree_selection_get_selected(selection,&model,&iter))
  {
    path = gtk_tree_model_get_path(model,&iter);
    gtk_tree_view_expand_row(treeview,path,TRUE);
    gtk_tree_path_free(path);
  }
}

void menu_cardview_context_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
/* Create a right click context menu */
{
  GtkWidget *menu, *menuitem;
  GtkTreeViewColumn* column2 = gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),2);

  menu = gtk_menu_new();

  /* Menu Item */
  menuitem = gtk_menu_item_new_with_label("Expand all");
  g_signal_connect(menuitem, "activate",
		   (GCallback) menu_cardview_context_menu_expand_all, treeview);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  /* Menu Item */
  if (gtk_tree_view_column_get_visible(column2))
  { 
    menuitem = gtk_menu_item_new_with_label("Show interpreted value");
    g_signal_connect(menuitem, "activate",
		     (GCallback) menu_cardview_context_menu_change_value_type, treeview);
  }
  else
  {
    menuitem = gtk_menu_item_new_with_label("Show raw value");
    g_signal_connect(menuitem, "activate",
		     (GCallback) menu_cardview_context_menu_change_value_type, treeview); 
  }
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);


  gtk_widget_show_all(menu);

  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		 (event != NULL) ? event->button : 0,
		 gdk_event_get_time((GdkEvent*)event));
}

gboolean menu_cardview_button_press_event(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
  GtkTreeSelection *selection;
  GtkTreePath *path;

  if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
  {
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (gtk_tree_selection_count_selected_rows(selection)  <= 1)
    {

      /* Get tree path for row that was clicked */
      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
					(gint) event->x, 
					(gint) event->y,
					&path, NULL, NULL, NULL))
      {
	gtk_tree_selection_unselect_all(selection);
	gtk_tree_selection_select_path(selection, path);
	gtk_tree_path_free(path);
      }
    }
    menu_cardview_context_menu(treeview,event,userdata);
    return TRUE;
  }
  return FALSE;
}

void menu_pos_func(GtkMenu *menu,
		   gint *x,
		   gint *y,
		   gboolean *push_in,
	       	   gpointer user_data)
{
  GtkWidget* button=(GtkWidget *)user_data;
  *push_in = TRUE;
  gdk_window_get_origin(button->window, x, y); 
  *x += button->allocation.x; 
  *y += button->allocation.y;
  *y += button->allocation.height;
}

void menu_card_view_analyzer_cb(GtkWidget *w, gpointer user_data)
{
  GtkWidget* menu=(GtkWidget*)user_data;
  if (menu)
  {
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, 
		   menu_pos_func, w, 
		   0, gtk_get_current_event_time());
  }
  else
    log_printf(LOG_ERROR,"No menu to display");
}

void menu_run_command_cb(GtkWidget *w, gpointer user_data)
{
  if (user_data)
    RUN_LUA_COMMAND((char *)user_data);
  else
    log_printf(LOG_ERROR,"No command to execute");
}

void menu_card_view_analyzer_load_cb(GtkWidget *w, gpointer user_data)
{
  char** select_info;

  select_info = gui_select_file("Load card script",config_get_string(CONFIG_FOLDER_SCRIPTS),NULL);
  if (select_info[1])
  {
    config_set_string(CONFIG_FOLDER_CARDTREES,select_info[0]);
    chdir(select_info[0]);
    RUN_LUA_SCRIPT(select_info[1]);
    g_free(select_info[0]);
    g_free(select_info[1]);
  }
}

void menu_reader_save_as_cb(GtkWidget *w, gpointer user_data)
{
  char** select_info;
  a_string_t *command;
  char *filename;

  select_info = gui_select_file("Save recorded data",config_get_string(CONFIG_FOLDER_LOGS),"card.clf");
  if (select_info[1])
  {  
    filename = lua_escape_string(select_info[1]);
    command=a_strnew(NULL);
    a_sprintf(command,"card.log_save(\"%s\")",filename);
    RUN_LUA_COMMAND(a_strval(command));
    a_strfree(command);
    g_free(select_info[0]);
    g_free(select_info[1]);
    free(filename);
  }
}

/*********************************************************/
/* CONSTRUTION OF MAIN UI ********************************/
/*********************************************************/

int select_lua(const struct dirent* de)
{
  char *ext=rindex(de->d_name,'.');
  if (ext && strcmp(ext,".lua")==0)
    return 1;
  return 0;
}

GtkWidget *create_analyzer_menu()
{
  GtkItemFactory *item_factory;
  GtkItemFactoryEntry fentry;
  struct dirent **namelist;
  int n,i;
  const char *script_path=config_get_string(CONFIG_FOLDER_SCRIPTS);
  struct menu_script* menu_item;
  char *dot;
  char *underscore;

  /* Make an ItemFactory */
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Analyzer>",NULL);

  if (item_factory==NULL)
  {
	log_printf(LOG_ERROR,"Could not create menu for scripts in %s",script_path);
	return NULL;
  }

  if (script_path)
  {
    n = scandir(script_path, &namelist, select_lua, alphasort);
    if (n > 0)
    {
      for (i=0;i<n;i++) 
      {
	log_printf(LOG_INFO,"Adding %s script to menu", namelist[i]->d_name);

        menu_item=(struct menu_script *)malloc(sizeof(struct menu_script));
	menu_item->script_name = (char* )malloc(strlen(namelist[i]->d_name)+11);
	strcpy(menu_item->script_name,"/");
	strcat(menu_item->script_name,namelist[i]->d_name);

	dot = rindex(menu_item->script_name,'.');
	if (dot)
	  *dot=0;

	underscore=menu_item->script_name;
	do 
	  if (*underscore=='_') *underscore=' '; 
	while (*underscore++);

	menu_item->script_file = strdup(namelist[i]->d_name);
	menu_item->prev=SCRIPTS;
	SCRIPTS = menu_item;

	memset(&fentry,0,sizeof(fentry));
	fentry.path=menu_item->script_name;
	fentry.callback=G_CALLBACK(gui_run_script_cb);
	fentry.callback_action=0;
	fentry.item_type="<StockItem>";
	fentry.extra_data=GTK_STOCK_PROPERTIES;
	gtk_item_factory_create_item(item_factory,&fentry,menu_item,2);
	free(namelist[i]);
      }
      free(namelist);
    }
    else
      log_printf(LOG_WARNING,"No scripts found in %s",script_path);
  }
 
  memset(&fentry,0,sizeof(fentry));
  fentry.path="/Sep1";
  fentry.callback=NULL;
  fentry.callback_action=0;
  fentry.item_type="<Separator>";
  fentry.extra_data=NULL;
  gtk_item_factory_create_item(item_factory,&fentry,NULL,2);

  memset(&fentry,0,sizeof(fentry));
  fentry.path="/Load a script";
  fentry.callback=G_CALLBACK(menu_card_view_analyzer_load_cb);
  fentry.callback_action=0;
  fentry.item_type="<StockItem>";
  fentry.extra_data=GTK_STOCK_OPEN;
  gtk_item_factory_create_item(item_factory,&fentry,NULL,2);
  
  return gtk_item_factory_get_widget (item_factory, "<Analyzer>");
}

GtkWidget *create_card_view(cardtree_t *cardtree)
{
  GtkCellRenderer     *renderer;
  GtkWidget           *scrolled_window;
  GtkTreeViewColumn   *column;
  GtkWidget           *base_container;
  GtkWidget           *toolbar;
  GtkToolItem         *item;
  GtkWidget           *analyzer_menu;

  /* Create base window container */
  
  base_container = gtk_vbox_new(FALSE,0);

  /* Create the toolbar */

  toolbar = gtk_toolbar_new();
  
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
 /*  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5); */

  item = gtk_tool_button_new_from_stock ("cardpeek-smartcard");
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(item),"Analyzer");
  analyzer_menu = create_analyzer_menu();
  g_signal_connect(G_OBJECT(item),"clicked",G_CALLBACK(menu_card_view_analyzer_cb),analyzer_menu);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_separator_tool_item_new();
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_CLEAR);
  g_signal_connect(G_OBJECT(item),"clicked",G_CALLBACK(menu_card_view_clear_cb),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_OPEN);
  g_signal_connect(G_OBJECT(item),"clicked",G_CALLBACK(menu_card_view_open_cb),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_SAVE_AS);
  g_signal_connect(G_OBJECT(item),"clicked",G_CALLBACK(menu_card_view_save_as_cb),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM(item),FALSE);
  gtk_tool_item_set_expand(GTK_TOOL_ITEM(item),TRUE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_ABOUT);
  g_signal_connect(G_OBJECT(item),"clicked",
		   G_CALLBACK(menu_run_command_cb),
		   "ui.about()");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_separator_tool_item_new();
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_QUIT);
  g_signal_connect(G_OBJECT(item),"clicked",G_CALLBACK(gtk_main_quit),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  gtk_box_pack_start (GTK_BOX (base_container), toolbar, FALSE, FALSE, 0);

  /* Create a new scrolled window, with scrollbars only if needed */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  
  gtk_box_pack_end (GTK_BOX (base_container), scrolled_window, TRUE, TRUE, 0);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);


  CARDVIEW = gtk_tree_view_new ();
  /* "enable-tree-lines" */
  /* gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW(view),TRUE);*/
  g_object_set(CARDVIEW,
	       "has-tooltip",TRUE, NULL);

/*
  g_signal_connect(CARDVIEW, 
		   "row-activated", (GCallback) menu_cardview_row_activated, NULL);
*/
  g_signal_connect(CARDVIEW, 
		   "cursor-changed", (GCallback) menu_cardview_cursor_changed, NULL);  
  g_signal_connect(CARDVIEW, 
 		   "query-tooltip", (GCallback) menu_cardview_query_tooltip, NULL);
  g_signal_connect(CARDVIEW,
		   "button-press-event", (GCallback) menu_cardview_button_press_event, NULL);


  gtk_container_add (GTK_CONTAINER (scrolled_window), CARDVIEW);

  /* --- Column #0 --- */

  column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(column,"Items");
  gtk_tree_view_column_set_resizable(column,TRUE);

  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_set_attributes(column,renderer,
				      "stock-id", C_ICON,
				      NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start(column, renderer, TRUE);
  gtk_tree_view_column_set_attributes(column,renderer,
				      "markup", C_MARKUP_CLASSNAME_DESCRIPTION_ID,
				      NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(CARDVIEW), column);

  /* --- Column #1 --- */

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (CARDVIEW),
                                               -1,      
                                               "Size",  
                                               renderer,
                                               "text", C_SIZE,
                                               NULL);

  column = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),1);
  gtk_tree_view_column_set_resizable(column,TRUE);

  g_object_set(renderer,
               "foreground", "blue",
               NULL);
  /* --- Column #2 --- */

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (CARDVIEW),
                                               -1,
                                               "Raw Value",  
                                               renderer,
                                               "markup", C_MARKUP_VALUE,
                                               NULL);
  column = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),2);
  gtk_tree_view_column_set_resizable(column,TRUE);
  gtk_tree_view_column_set_visible (column,FALSE);
  gtk_tree_view_column_set_clickable(column,TRUE);
  g_signal_connect(column,"clicked",(GCallback)menu_cardview_column_activated,NULL);

   /* --- Column #3 --- */

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (CARDVIEW),
                                               -1,
                                               "Interpreted Value",  
                                               renderer,
                                               "markup", C_MARKUP_ALT_VALUE,
                                               NULL);
  column = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),3);
  gtk_tree_view_column_set_resizable(column,TRUE);
  gtk_tree_view_column_set_clickable(column,TRUE);
  g_signal_connect(column,"clicked",(GCallback)menu_cardview_column_activated,NULL);


/*
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (CARDVIEW),
                                               -1,      
                                               "Information",  
                                               renderer,
                                               "text", C_COMMENT,
                                               NULL);
  column = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),3);
  gtk_tree_view_column_set_resizable(column,TRUE);

  g_object_set(renderer,
               "style-set", TRUE,
               "style", PANGO_STYLE_ITALIC,
               NULL);
    */

  cardtree_bind_to_treeview(cardtree,CARDVIEW);
  /* gtk_widget_set_size_request (scrolled_window,-1,200);*/

  return base_container;
}

GtkWidget *create_reader_view()
{
  GtkWidget           *view;
  GtkWidget           *scrolled_window;
  GtkWidget           *base_container;
  GtkWidget           *toolbar;
  GtkToolItem         *item;
  PangoFontDescription *font_desc;

  /* Create base window container */

  base_container = gtk_vbox_new(FALSE,0);

  /* Create the toolbar */

  toolbar = gtk_toolbar_new();
  
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
 /*  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5); */


  item = gtk_tool_button_new_from_stock (GTK_STOCK_CONNECT);
  g_signal_connect(G_OBJECT(item),"clicked",
		   G_CALLBACK(menu_run_command_cb),
		   "card.connect()");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_REDO);
  g_signal_connect(G_OBJECT(item),"clicked",
		   G_CALLBACK(menu_run_command_cb),
		   "card.warm_reset()");
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(item),"Reset");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_DISCONNECT);
  g_signal_connect(G_OBJECT(item),"clicked",
		   G_CALLBACK(menu_run_command_cb),
		   "card.disconnect()");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_separator_tool_item_new();
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_CLEAR);
  g_signal_connect(G_OBJECT(item),"clicked",
		   G_CALLBACK(menu_run_command_cb),
		   "card.log_clear()");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_SAVE_AS);
  g_signal_connect(G_OBJECT(item),"clicked",
		   G_CALLBACK(menu_reader_save_as_cb),
		   NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item),-1);

  gtk_box_pack_start (GTK_BOX (base_container), toolbar, FALSE, FALSE, 0);

  /* Create a new scrolled window, with scrollbars only if needed */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  
  gtk_box_pack_end (GTK_BOX (base_container), scrolled_window, TRUE, TRUE, 0);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);


  view = gtk_text_view_new ();

  font_desc = pango_font_description_from_string ("Monospace");
  gtk_widget_modify_font (view, font_desc);
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


GtkWidget *create_log_view()
{
  GtkWidget *scrolled_window;
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


  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (scrolled_window), view);
  
  return scrolled_window;
}

GtkWidget *create_command_entry()
{
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *entry;
  GtkEntryCompletion* compl;

  entry = gtk_entry_new();

  COMPLETION = gtk_list_store_new(1,G_TYPE_STRING);
  compl = gtk_entry_completion_new();
  gtk_entry_completion_set_model(compl, GTK_TREE_MODEL(COMPLETION));
  gtk_entry_completion_set_text_column(compl, 0);
  gtk_entry_set_completion(GTK_ENTRY(entry), compl);

  label = gtk_label_new("Command :");
  hbox = gtk_hbox_new (FALSE, 4);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 2);

  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (gui_run_command_cb),
		    (gpointer) entry);

  return hbox;
}

/*********************************************************/
/* GUI_* FUNCTIONS ***************************************/
/*********************************************************/

void gui_expand_view()
{
  gtk_tree_view_expand_all (GTK_TREE_VIEW(CARDVIEW));
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

void gui_about()
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_INFO,
				   GTK_BUTTONS_OK,
				   "cardpeek, version %s\n"
				   "Copyright 2009-2011, by 'L1L1'\n"
				   "Licenced under the GPL 3\n\n"
				   "Path to scripts is:\n%s",
				   VERSION,config_get_string(CONFIG_FOLDER_SCRIPTS));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

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
  GtkWidget *reader;
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *frame1;
  GtkWidget *frame2;
  GtkWidget *frame3;
  GtkWidget *status;
  GtkWidget *tabs;
  GtkWidget *label;
  GtkIconSet* icon_set;
  GdkPixbuf* pixbuf;
  GtkIconFactory* icon_factory;

  RUN_LUA_SCRIPT = run_script_cb;
  RUN_LUA_COMMAND = run_command_cb;

  /* Build icon sets */

  icon_factory = gtk_icon_factory_new();
  
  pixbuf = gdk_pixbuf_new_from_inline (-1, icon_block, FALSE, NULL);
  icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
  gtk_icon_factory_add(icon_factory,"cardpeek-block",icon_set);

  pixbuf = gdk_pixbuf_new_from_inline (-1, icon_record, FALSE, NULL);
  icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
  gtk_icon_factory_add(icon_factory,"cardpeek-record",icon_set);

  pixbuf = gdk_pixbuf_new_from_inline (-1, icon_application, FALSE, NULL);
  icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
  gtk_icon_factory_add(icon_factory,"cardpeek-application",icon_set);

  pixbuf = gdk_pixbuf_new_from_inline (-1, icon_file, FALSE, NULL);
  icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
  gtk_icon_factory_add(icon_factory,"cardpeek-file",icon_set);

  pixbuf = gdk_pixbuf_new_from_inline (-1, icon_smartcard, FALSE, NULL);
  icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
  gtk_icon_factory_add(icon_factory,"cardpeek-smartcard",icon_set);

  pixbuf = gdk_pixbuf_new_from_inline (-1, icon_item, FALSE, NULL);
  icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
  gtk_icon_factory_add(icon_factory,"cardpeek-item",icon_set);

  gtk_icon_factory_add_default(icon_factory);

  /* main window start */

  MAIN_WINDOW = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(MAIN_WINDOW),600,400);
  gtk_container_set_border_width (GTK_CONTAINER (MAIN_WINDOW), 0);
  g_signal_connect (MAIN_WINDOW, "delete_event", gtk_main_quit, NULL); /* dirty */

  pixbuf = gdk_pixbuf_new_from_inline (-1, icon_cardpeek, FALSE, NULL);
  gtk_window_set_default_icon (pixbuf);

  /* log frame */

  text = create_log_view ();
  SCROLL_TEXT = GTK_SCROLLED_WINDOW(text);
  frame3 = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame3),GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER(frame3),text);

  log_set_function(gui_logfunction);

  /* reader view frame */

  reader = create_reader_view ();
  frame2 = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame2),GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER(frame2),reader);

  /* tree view frame */

  CARDTREE = cardtree_new();

  view = create_card_view (CARDTREE);

  frame1 = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame1),GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER(frame1),view);
 
  /* command entry */

  entry = create_command_entry ();
  
  /* status bar */

  status = gtk_statusbar_new ();
  STATUS_BAR = GTK_STATUSBAR(status);
  STATUS_BAR_CONTEXT_ID = gtk_statusbar_get_context_id(STATUS_BAR,"main");

  /* notebook */

  tabs = gtk_notebook_new ();
  g_object_set(G_OBJECT (tabs), "tab-border", 4, NULL);

  label = gtk_label_new ("card view");
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), frame1, label);

  label = gtk_label_new ("reader");
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), frame2, label);

  label = gtk_label_new ("logs");
  gtk_notebook_append_page (GTK_NOTEBOOK (tabs), frame3, label);

  /* vertical packing */

  vbox = gtk_vbox_new (FALSE, 0);

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

  LOG_BUFFER=NULL;

  while (SCRIPTS)
  {
     it=SCRIPTS->prev;
     free(SCRIPTS->script_name);
     free(SCRIPTS->script_file);
     free(SCRIPTS);
     SCRIPTS=it;
  }

  /* FIXME: do some more cleanup */
 
  return 1;
}

char HEX_CHAR[17]="0123456789ABCDEF";
const char* hex_pretty_print(int indent, const bytestring_t *bs,int add_ascii)
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

void gui_reader_print_data(unsigned event,
                           const bytestring_t *command,
                           unsigned short sw,
                           const bytestring_t *response,
                           void *extra_data)
{
  const char* text;
  char buf[200];
  GtkTextIter iter;

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


    sprintf(buf,"RECV %04X                                                # %s\n     ",sw,iso7816_stringify_sw(sw));
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
  gui_update(1);
}

