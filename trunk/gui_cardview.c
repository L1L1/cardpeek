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
#include "gui_cardview.h"
#include "gui_toolbar.h"
#include "gui_about.h"
#include "gui_flexi_cell_renderer.h"
#include "lua_ext.h"
#include <dirent.h>
#include "misc.h"
#include "pathconfig.h"
#include <string.h>
#include <stdlib.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#ifndef _WIN32
#include "config.h"
#else
#include "win32/config.h"
#include "win32/win32compat.h"
#endif

typedef struct _ScriptInfo ScriptInfo;

struct _ScriptInfo
{
    char *script_name;
    char *script_file;
    char *script_description;
    char *script_targeted_version;
    ScriptInfo *prev;
};

/*********************************************************/
/* THE INFAMOUS UGLY GLOBALS *****************************/
/*********************************************************/

DyntreeModel *CARDTREE=0;
GtkWidget *CARDVIEW=NULL;
ScriptInfo *SCRIPTS=NULL;

DyntreeModel *gui_cardview_get_store(void)
{
    return CARDTREE;
}

/*********************************************************/
/* LOG FUNCTIONS AND UI CALLBACKS ************************/
/*********************************************************/

static void menu_run_script_cb(GtkWidget *widget,
                               gpointer callback_data,
                               guint callback_action)
{
    ScriptInfo *script = (ScriptInfo *)callback_data;
    UNUSED(widget);
    UNUSED(callback_action);

    if (!script->script_targeted_version)
    {
        log_printf(LOG_WARNING,"The script '%s' does not contain version information, and may use an older incompatible API", script->script_file);
        log_printf(LOG_WARNING,"To remove this warning, add the following line at the begining of '%s':\n\t -- @targets %s", script->script_file, VERSION);
    }

    gui_set_title(script->script_name);
    luax_run_script(script->script_file);
    gtk_tree_view_expand_all (GTK_TREE_VIEW(CARDVIEW));
    gui_update(0);
}

static void menu_cardview_clear_cb(GtkWidget *w, gpointer user_data)
{
    UNUSED(w);
    UNUSED(user_data);

    dyntree_model_iter_remove(CARDTREE,NULL);
    luax_run_command("card.CLA=0");
    log_printf(LOG_INFO,"Cleared card data tree");
}

static void menu_cardview_open_cb(GtkWidget *w, gpointer user_data)
{
    char **select_info;
    a_string_t *command;
    char *filename;
    UNUSED(w);
    UNUSED(user_data);

    select_info = gui_select_file("Load xml card description",path_config_get_string(PATH_CONFIG_FOLDER_WORKING),NULL);

    if (select_info[1])
    {
        path_config_set_string(PATH_CONFIG_FOLDER_WORKING,select_info[0]);
        filename = luax_escape_string(select_info[1]);
        command=a_strnew(NULL);
        a_sprintf(command,"ui.load_view(\"%s\")",filename);
        luax_run_command(a_strval(command));
        a_strfree(command);
        g_free(select_info[0]);
        g_free(select_info[1]);
        g_free(filename);
    }
}

static void menu_cardview_save_as_cb(GtkWidget *w, gpointer user_data)
{
    char **select_info;
    a_string_t *command;
    char *filename;
    UNUSED(w);
    UNUSED(user_data);

    select_info = gui_select_file("Save xml card description",path_config_get_string(PATH_CONFIG_FOLDER_WORKING),"card.xml");
    if (select_info[1])
    {
        path_config_set_string(PATH_CONFIG_FOLDER_WORKING,select_info[0]);
        filename = luax_escape_string(select_info[1]);
        command=a_strnew(NULL);
        a_sprintf(command,"ui.save_view(\"%s\")",filename);
        luax_run_command(a_strval(command));
        a_strfree(command);
        g_free(select_info[0]);
        g_free(select_info[1]);
        g_free(filename);
    }
}

static void menu_cardview_switch_column(void)
{
    GtkTreeViewColumn *column2 = gtk_tree_view_get_column(GTK_TREE_VIEW(CARDVIEW),2);
    GtkTreeViewColumn *column3 = gtk_tree_view_get_column(GTK_TREE_VIEW(CARDVIEW),3);

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

static void menu_cardview_context_menu_change_value_type(GtkWidget *menuitem, gpointer userdata)
{
    UNUSED(userdata);
    UNUSED(menuitem);

    menu_cardview_switch_column();
}

static void menu_cardview_column_activated(GtkTreeViewColumn *treeviewcolumn, gpointer userdata)
{
    UNUSED(userdata);
    UNUSED(treeviewcolumn);

    menu_cardview_switch_column();
}


static void menu_cardview_context_menu_expand_all(GtkWidget *menuitem, gpointer userdata)
/* Callback responding to right click context menu item to expand current branch */
{
    GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreePath *path;
    UNUSED(menuitem);

    if (gtk_tree_selection_get_selected(selection,&model,&iter))
    {
        path = gtk_tree_model_get_path(model,&iter);
        gtk_tree_view_expand_row(treeview,path,TRUE);
        gtk_tree_path_free(path);
    }
}

static void menu_cardview_context_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
/* Create a right click context menu */
{
    GtkWidget *menu, *menuitem;
    GtkTreeViewColumn *column2 = gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),2);
    UNUSED(userdata);

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
                   gdk_event_get_time((GdkEvent *)event));
}

static gboolean menu_cardview_button_press_event(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
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
    /*
    else if (event->type == GDK_2BUTTON_PRESS  &&  event->button == 1)
    {
      g_printf("double click\n");
    }
    */
    return FALSE;
}

static void menu_cardview_analyzer_cb_pos_func(GtkMenu *menu,
        gint *x,
        gint *y,
        gboolean *push_in,
        gpointer user_data)
{
    GtkWidget *button=(GtkWidget *)user_data;
    GtkAllocation allocation;
    UNUSED(menu);

    gtk_widget_get_allocation(button,&allocation);

    *push_in = TRUE;

    gdk_window_get_origin(gtk_widget_get_window(button), x, y);
    *x += allocation.x;
    *y += allocation.y;
    *y += allocation.height;
}

static void menu_cardview_analyzer_cb(GtkWidget *w, gpointer user_data)
{
    GtkWidget *menu=(GtkWidget *)user_data;
    if (menu)
    {
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
                       menu_cardview_analyzer_cb_pos_func, w,
                       0, gtk_get_current_event_time());
    }
    else
        log_printf(LOG_ERROR,"No menu to display");
}

static void menu_cardview_analyzer_load_cb(GtkWidget *w, gpointer user_data)
{
    char **select_info;
    UNUSED(w);
    UNUSED(user_data);

    select_info = gui_select_file("Load card script",path_config_get_string(PATH_CONFIG_FOLDER_SCRIPTS),NULL);

    if (select_info[1])
    {
        path_config_set_string(PATH_CONFIG_FOLDER_WORKING,select_info[0]);
        chdir(select_info[0]);
        gui_set_title(select_info[1]);
        luax_run_script(select_info[1]);
        g_free(select_info[0]);
        g_free(select_info[1]);
    }
}

/*********************************************************/
/* CONSTRUTION OF MAIN UI ********************************/
/*********************************************************/

static int select_lua(DIRENT_T *de)
{
    char *ext=rindex(de->d_name,'.');
    if (ext && strcmp(ext,".lua")==0)
        return 1;
    return 0;
}

static void rtrim(char *line)
{
    char *eol = line+strlen(line)-1;
    while (eol>=line && *eol<=' ') *eol--=0;
}

static const char *locate_after_prefix(const char *prefix, const char *line)
{
    char *pos = strstr(line,prefix);
    if (pos)
    {
        pos+=strlen(prefix);
        while (*pos<=' ' && *pos>0) pos++;
        return pos;
    }
    return NULL;
}

static GtkWidget *script_info_add(const char *path, const char *fname)
{
    char *fullname;
    ScriptInfo *si;
    FILE *script;
    int i;
    char line[1024];
    const char *str;
    char *dot;
    char *underscore;
    GtkWidget *menuitem = NULL;

    fullname = g_malloc(strlen(path)+strlen(fname)+2);
    sprintf(fullname,"%s/%s",path,fname);

    script = fopen(fullname,"r");
    if (script)
    {
        si = (ScriptInfo *)g_malloc0(sizeof(ScriptInfo));
        si->script_file = strdup(fname);

        rtrim(line);

        for (i=0; i<30; i++)
        {
            if (fgets(line,1024,script)==NULL) break;

            rtrim(line);

            if ((str=locate_after_prefix("@name",line))!=NULL)
            {
                si->script_name = g_strdup(str);
            }
            else if ((str=locate_after_prefix("@description",line))!=NULL)
            {
                si->script_description = g_strdup(str);
            }
            else if ((str=locate_after_prefix("@targets",line))!=NULL)
            {
                si->script_targeted_version = g_strdup(str);
            }
        }

        fclose(script);

        if (si->script_name==NULL)
        {
            si->script_name = g_strdup(fname);

            dot = rindex(si->script_name,'.');
            if (dot) *dot=0;

            for (underscore=si->script_name; *underscore!=0; underscore++)
            {
                if (*underscore=='_') *underscore=' ';
            }
        }

        si->prev = SCRIPTS;
        SCRIPTS = si;

        menuitem = gtk_image_menu_item_new_with_label(si->script_name);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),gtk_image_new_from_stock(GTK_STOCK_EXECUTE,GTK_ICON_SIZE_MENU));
        if (si->script_description) gtk_widget_set_tooltip_text(menuitem,si->script_description);
        g_signal_connect(GTK_WIDGET(menuitem),"activate",G_CALLBACK(menu_run_script_cb),si);
        gtk_widget_show(menuitem);
    }
    else
    {
        log_printf(LOG_ERROR,"Failed to open %s for reading",fullname);
    }
    g_free(fullname);
    return menuitem;
}

static GtkWidget *create_analyzer_menu(GtkAccelGroup *accel_group)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *menuitem = NULL;
    struct dirent **namelist;
    int i,n;
    const char *script_path=path_config_get_string(PATH_CONFIG_FOLDER_SCRIPTS);

    menu = gtk_menu_new();

    n = scandir(script_path, &namelist, select_lua, alphasort);
    if (n > 0)
    {
        for (i=0; i<n; i++)
        {
            log_printf(LOG_INFO,"Adding %s script to menu", namelist[i]->d_name);

            menuitem = script_info_add(script_path, namelist[i]->d_name);

            if (menuitem) gtk_menu_shell_append(GTK_MENU_SHELL(menu),menuitem);

            free(namelist[i]);
        }
        free(namelist);
    }
    else
        log_printf(LOG_WARNING,"No scripts found in %s",script_path);

    menuitem = gtk_separator_menu_item_new();
    gtk_widget_show(menuitem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menuitem);

    menuitem = gtk_image_menu_item_new_with_label("Load a script");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),gtk_image_new_from_stock(GTK_STOCK_OPEN,GTK_ICON_SIZE_MENU));
    g_signal_connect(GTK_WIDGET(menuitem),"activate",G_CALLBACK(menu_cardview_analyzer_load_cb),NULL);

    gtk_widget_add_accelerator(menuitem, "activate", accel_group, GDK_KEY_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_show(menuitem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menuitem);
    return menu;
}

toolbar_item_t TB_CARD_VIEW[] =
{
    {
        "card-view-analyzer", "cardpeek-analyzer", "Analyzer", G_CALLBACK(menu_cardview_analyzer_cb), NULL,
        "Analyze card contents."
    },
    { NULL, 		TOOLBAR_ITEM_SEPARATOR, NULL, NULL, NULL, NULL },
    {
        "card-view-clear", 	GTK_STOCK_CLEAR, "Clear", G_CALLBACK(menu_cardview_clear_cb), NULL,
        "Clear the card view content."
    },
    {
        "card-view-open", 	GTK_STOCK_OPEN, "Open", G_CALLBACK(menu_cardview_open_cb), NULL,
        "Load previously saved card view content (XML fomat)."
    },
    {
        "card-view-save-as",	GTK_STOCK_SAVE_AS, "Save", G_CALLBACK(menu_cardview_save_as_cb), NULL,
        "Save current card view content into a file (XML fomat)."
    },
    { NULL, 		TOOLBAR_ITEM_EXPANDER, NULL, NULL, NULL, NULL },
    {
        "card-view-about",	GTK_STOCK_ABOUT, "About", G_CALLBACK(gui_toolbar_run_command_cb), "ui.about()",
        "About cardpeek " VERSION
    },
    { NULL, 		TOOLBAR_ITEM_SEPARATOR, NULL, NULL, NULL, NULL },
    {
        "card-view-quit",	GTK_STOCK_QUIT, "Quit", G_CALLBACK(gtk_main_quit), NULL,
        "Quit cardpeek"
    },
    /* END MARKER : */
    { NULL, 		NULL, NULL, NULL, NULL, NULL }
};

static void internal_cell_renderer_icon_cb (GtkTreeViewColumn *col,
        GtkCellRenderer   *renderer,
        GtkTreeModel      *model,
        GtkTreeIter       *iter,
        gpointer           user_data)
{
    char *classname;
    const char *icon_name = NULL;
    UNUSED(col);
    UNUSED(user_data);

    gtk_tree_model_get(GTK_TREE_MODEL(model),
                       iter,
                       CC_CLASSNAME, &classname,
                       -1);

    if (classname!=NULL && classname[0]=='t')
    {
        switch (classname[2])
        {
            case 'a':
                if (strcmp("t:application",classname)==0) icon_name="cardpeek-application";
                else if (strcmp("t:atr",classname)==0)    icon_name="cardpeek-atr";
                break;
            case 'b':
                if (strcmp("t:block",classname)==0)       icon_name="cardpeek-block";
                else if (strcmp("t:body",classname)==0)   icon_name="cardpeek-body";
                break;
            case 'c':
                if (strcmp("t:card",classname)==0)        icon_name="cardpeek-smartcard";
                break;
            case 'f':
                if (strcmp("t:file",classname)==0)        icon_name="cardpeek-file";
                else if (strcmp("t:folder",classname)==0) icon_name="cardpeek-folder";
                break;
            case 'h':
                if (strcmp("t:header",classname)==0)      icon_name="cardpeek-header";
                break;
            case 'r':
                if (strcmp("t:record",classname)==0)      icon_name="cardpeek-record";
                break;
        }
    }

    if (icon_name==NULL)
        icon_name="cardpeek-item";

    g_object_set(renderer, "stock-id", icon_name, NULL);

    g_free(classname);
}

static void internal_cell_renderer_markup_cb (GtkTreeViewColumn *col,
        GtkCellRenderer   *renderer,
        GtkTreeModel      *model,
        GtkTreeIter       *iter,
        gpointer           user_data)
{
    a_string_t *markup_label_id;
    char *classname;
    char *id;
    char *label;
    UNUSED(col);
    UNUSED(user_data);

    gtk_tree_model_get(GTK_TREE_MODEL(model),
                       iter,
                       CC_CLASSNAME, &classname,
                       CC_LABEL, &label,
                       CC_ID, &id,
                       -1);

    /* TODO: escape markup characters that may appear in classname, id and label */

    markup_label_id = a_strnew(NULL);

    if (label && label[0]=='t')
        a_sprintf(markup_label_id,"<b>%s</b>",label+2);
    else
    {
        if (classname && classname[0]=='t')
            a_sprintf(markup_label_id,"<b>%s</b>",classname+2);
        else
            a_sprintf(markup_label_id,"<b>item</b>");
    }

    if (id && id[0]=='t')
    {
        a_strcat(markup_label_id," ");
        a_strcat(markup_label_id,id+2);
    }

    g_object_set(renderer, "markup", a_strval(markup_label_id), NULL);

    a_strfree(markup_label_id);

    g_free(classname);
    g_free(label);
    g_free(id);
}

static void internal_cell_renderer_size_cb (GtkTreeViewColumn *col,
        GtkCellRenderer   *renderer,
        GtkTreeModel      *model,
        GtkTreeIter       *iter,
        gpointer           user_data)
{
    char *size;
    UNUSED(col);
    UNUSED(user_data);

    gtk_tree_model_get(GTK_TREE_MODEL(model),
                       iter,
                       CC_SIZE, &size,
                       -1);


    if (size && size[0]=='t')
        g_object_set(renderer, "text", size+2, NULL);
    else
        g_object_set(renderer, "text", "", NULL);

    g_free(size);
}


GtkWidget *gui_cardview_create_window(GtkAccelGroup *accel_group)
{
    GtkCellRenderer     *renderer;
    GtkWidget           *scrolled_window;
    GtkTreeViewColumn   *column;
    GtkWidget           *base_container;
    GtkWidget           *toolbar;
    GtkWidget	      *colheader;
    GtkWidget	      *colitem;

    /* Create base window container */

    base_container = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);

    /* Create the toolbar */

    TB_CARD_VIEW[0].callback_data = create_analyzer_menu(accel_group);

    toolbar = gui_toolbar_new(TB_CARD_VIEW);

    gtk_box_pack_start (GTK_BOX (base_container), toolbar, FALSE, FALSE, 0);

    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);

    gtk_box_pack_end (GTK_BOX (base_container), scrolled_window, TRUE, TRUE, 0);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);


    CARDVIEW = gtk_tree_view_new ();

    g_signal_connect(CARDVIEW,
                     "button-press-event", (GCallback) menu_cardview_button_press_event, NULL);


    gtk_container_add (GTK_CONTAINER (scrolled_window), CARDVIEW);

    /* --- Column #0 --- */

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column,"Items");
    gtk_tree_view_column_set_resizable(column,TRUE);

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, internal_cell_renderer_icon_cb, NULL, NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, internal_cell_renderer_markup_cb, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(CARDVIEW), column);

    /* --- Column #1 --- */

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column,"Size");

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, internal_cell_renderer_size_cb, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(CARDVIEW), column);

    g_object_set(renderer,
                 "foreground", "blue",
                 NULL);
    /* --- Column #2 --- */

    renderer = custom_cell_renderer_flexi_new(TRUE);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (CARDVIEW),
            -1,
            NULL,
            renderer,
            "raw-value", CC_VAL,
            NULL);
    column = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),2);
    gtk_tree_view_column_set_resizable(column,TRUE);
    gtk_tree_view_column_set_visible (column,FALSE);
    gtk_tree_view_column_set_clickable(column,TRUE);
    g_signal_connect(column,"clicked",(GCallback)menu_cardview_column_activated,NULL);

    colheader = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
    gtk_box_pack_start (GTK_BOX (colheader), gtk_label_new("Raw value"), FALSE, FALSE, 0);
    if ((colitem = gtk_image_new_from_stock(GTK_STOCK_CONVERT,GTK_ICON_SIZE_MENU)))
    {
        gtk_box_pack_start (GTK_BOX (colheader), colitem, FALSE, FALSE, 0);
    }
    gtk_widget_show_all(colheader);
    gtk_widget_set_tooltip_text(colheader,"Click to switch to 'interpreted' data.");
    gtk_tree_view_column_set_widget(column,colheader);

    /* --- Column #3 --- */

    renderer = custom_cell_renderer_flexi_new(FALSE);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (CARDVIEW),
            -1,
            NULL,
            renderer,
            "raw-value", CC_VAL,
            "alt_text", CC_ALT,
            "mime-type", CC_MIME_TYPE,
            NULL);

    column = gtk_tree_view_get_column(GTK_TREE_VIEW (CARDVIEW),3);
    gtk_tree_view_column_set_resizable(column,TRUE);
    gtk_tree_view_column_set_clickable(column,TRUE);
    g_signal_connect(column,"clicked",(GCallback)menu_cardview_column_activated,NULL);

    colheader = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
    gtk_box_pack_start (GTK_BOX (colheader), gtk_label_new("Interpreted value"), FALSE, FALSE, 0);
    if ((colitem = gtk_image_new_from_stock(GTK_STOCK_CONVERT,GTK_ICON_SIZE_MENU)))
    {
        gtk_box_pack_start (GTK_BOX (colheader), colitem, FALSE, FALSE, 0);
    }
    gtk_widget_show_all(colheader);
    gtk_widget_set_tooltip_text(colheader,"Click to switch to 'raw' data.");
    gtk_tree_view_column_set_widget(column,colheader);

    /* add the dat model */

    CARDTREE = dyntree_model_new();

    gtk_tree_view_set_model(GTK_TREE_VIEW(CARDVIEW),GTK_TREE_MODEL(CARDTREE));

    g_object_unref(CARDTREE);

    return base_container;
}

void gui_cardview_cleanup(void)
{
    ScriptInfo *prev;

    while (SCRIPTS)
    {
        prev=SCRIPTS->prev;
        g_free(SCRIPTS->script_name);
        g_free(SCRIPTS->script_file);
        if (SCRIPTS->script_description) g_free(SCRIPTS->script_description);
        if (SCRIPTS->script_targeted_version) g_free(SCRIPTS->script_targeted_version);
        free(SCRIPTS);
        SCRIPTS=prev;
    }
    CARDTREE=NULL;
    CARDVIEW=NULL;
}
