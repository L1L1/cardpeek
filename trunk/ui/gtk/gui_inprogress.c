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

#include "ui.h"
#include "gui_inprogress.h"
#include "misc.h"

/*********************************************************/
/* LOG FUNCTIONS AND UI CALLBACKS ************************/
/*********************************************************/

typedef struct {
	unsigned in_progress;
	GtkWidget *progress_bar;
} progress_info_t; 

static void internal_inprogress_response_cb(GtkDialog *dialog,
                                            gint       response_id,
                                            gpointer   user_data)
{
   progress_info_t *progress = user_data;
   UNUSED(dialog);
   UNUSED(response_id);

   progress->in_progress = 0;
}

void* gui_inprogress_new(const char *title, const char *message)
{
   GtkWidget *dialog;
   GtkWidget *label;
   GtkWidget *content_area;
   GtkWidget *progress_bar;
   GtkWidget *vbox;
   progress_info_t *progress;
   

   dialog = gtk_dialog_new_with_buttons (title,
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         NULL);

   progress = g_malloc(sizeof(progress_info_t));
   progress->in_progress = 1;
   g_object_set_data(G_OBJECT(dialog),"progress",progress);

   content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
   
   vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,16);
   gtk_container_set_border_width(GTK_CONTAINER(vbox),16);   
 
   label = gtk_label_new (message);
   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
   
   progress_bar = gtk_progress_bar_new();
   gtk_box_pack_end (GTK_BOX (vbox), progress_bar, FALSE, FALSE, 0);
   progress->progress_bar = progress_bar;

   g_signal_connect (dialog,
                     "response",
                     G_CALLBACK (internal_inprogress_response_cb),
                     progress);

   g_signal_connect (dialog, 
		     "delete-event", 
		     G_CALLBACK (gtk_true), 
		     NULL);

   gtk_container_add (GTK_CONTAINER (content_area), vbox);
   gtk_widget_show_all (dialog);

   return (void *)dialog;
}

unsigned gui_inprogress_pulse(void *pulser)
{
    GtkWidget *dialog = GTK_WIDGET(pulser);
	progress_info_t *progress = g_object_get_data(G_OBJECT(dialog),"progress");
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress->progress_bar));
	ui_update();
	return progress->in_progress; 
}

unsigned gui_inprogress_set_fraction(void *pulser, gdouble level)
{
    GtkWidget *dialog = GTK_WIDGET(pulser);
	progress_info_t *progress = g_object_get_data(G_OBJECT(dialog),"progress");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->progress_bar),level);
	ui_update();
	return progress->in_progress; 
}

void gui_inprogress_free(void *pulser)
{
    GtkWidget *dialog = GTK_WIDGET(pulser);
	progress_info_t *progress = g_object_get_data(G_OBJECT(dialog),"progress");

	g_free(progress);
	gtk_widget_destroy(dialog);
}
