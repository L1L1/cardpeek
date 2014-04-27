#ifndef GUI_FLEXI_CELL_RENDERER_H
#define GUI_FLEXI_CELL_RENDERER_H

/* This file is WORK IN PROGRESS  */
/* it has not use in cardpeek yet */


#include <gtk/gtk.h>
#include "a_string.h"

#define CUSTOM_TYPE_CELL_RENDERER_FLEXI             (custom_cell_renderer_flexi_get_type())
#define CUSTOM_CELL_RENDERER_FLEXI(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),  CUSTOM_TYPE_CELL_RENDERER_FLEXI, CustomCellRendererFlexi))
#define CUSTOM_CELL_RENDERER_FLEXI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  CUSTOM_TYPE_CELL_RENDERER_FLEXI, CustomCellRendererFlexiClass))
#define CUSTOM_IS_CELL_FLEXI_FLEXI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_CELL_RENDERER_FLEXI))
#define CUSTOM_IS_CELL_FLEXI_FLEXI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_TYPE_CELL_RENDERER_FLEXI))
#define CUSTOM_CELL_RENDERER_FLEXI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_CELL_RENDERER_FLEXI, CustomCellRendererFlexiClass))

typedef struct _CustomCellRendererFlexi CustomCellRendererFlexi;
typedef struct _CustomCellRendererFlexiClass CustomCellRendererFlexiClass;

/* CustomCellRendererFlexi: Our custom cell renderer
 *   structure. Extend according to need */

struct _CustomCellRendererFlexi
{
  GtkCellRenderer   parent;

  gboolean	is_raw;
  a_string_t	*raw_value;
  a_string_t	*alt_text;
  a_string_t  	*mime_type;
  unsigned	rendered_type;
  gpointer	rendered_value;  
  int		default_width; 
  int       default_line_height;
};


struct _CustomCellRendererFlexiClass
{
  GtkCellRendererClass  parent_class;
};

GType                custom_cell_renderer_flexi_get_type (void);

GtkCellRenderer     *custom_cell_renderer_flexi_new (gboolean is_raw);

#endif
