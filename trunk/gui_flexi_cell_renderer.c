#include "gui_flexi_cell_renderer.h"
#include "misc.h"

/* This is based mainly on GtkCellRendererFlexi
 *  in GAIM, written and (c) 2002 by Sean Egan
 *  (Licensed under the GPL), which in turn is
 *  based on Gtk's GtkCellRenderer[Text|Toggle|Pixbuf]
 *  implementation by Jonathan Blandford */

/* This is taken mainly from the GtkTreeView Tutorial */
/* IT IS NOT USED IN CARDPEEK AT THIS STAGE */


static void     custom_cell_renderer_flexi_init       (CustomCellRendererFlexi      *cellflexi);

static void     custom_cell_renderer_flexi_class_init (CustomCellRendererFlexiClass *klass);

static void     custom_cell_renderer_flexi_get_property  (GObject                    *object,
							  guint                       param_id,
							  GValue                     *value,
							  GParamSpec                 *pspec);

static void     custom_cell_renderer_flexi_set_property  (GObject                    *object,
							  guint                       param_id,
							  const GValue               *value,
							  GParamSpec                 *pspec);

static void     custom_cell_renderer_flexi_finalize (GObject *gobject);


/* These functions are the heart of our custom cell renderer: */

static void     custom_cell_renderer_flexi_get_size   (GtkCellRenderer            *cell,
						       GtkWidget                  *widget,
						       GdkRectangle               *cell_area,
						       gint                       *x_offset,
						       gint                       *y_offset,
						       gint                       *width,
						       gint                       *height);

static void     custom_cell_renderer_flexi_render     (GtkCellRenderer            *cell,
						       GdkWindow                  *window,
						       GtkWidget                  *widget,
						       GdkRectangle               *background_area,
						       GdkRectangle               *cell_area,
						       GdkRectangle               *expose_area,
						       guint                       flags);


enum
{
  PROP_TEXT = 1,
  PROP_MIME_TYPE,
  PROP_FALLBACK
};

static   gpointer parent_class;


/***************************************************************************
 *
 *  custom_cell_renderer_flexi_get_type: here we register our type with
 *                                          the GObject type system if we
 *                                          haven't done so yet. Everything
 *                                          else is done in the callbacks.
 *
 ***************************************************************************/

GType custom_cell_renderer_flexi_get_type (void)
{
  static GType cell_flexi_type = 0;

  if (cell_flexi_type == 0)
  {
    static const GTypeInfo cell_flexi_info =
    {
      sizeof (CustomCellRendererFlexiClass),
      NULL,                                                     /* base_init */
      NULL,                                                     /* base_finalize */
      (GClassInitFunc) custom_cell_renderer_flexi_class_init,
      NULL,                                                     /* class_finalize */
      NULL,                                                     /* class_data */
      sizeof (CustomCellRendererFlexi),
      0,                                                        /* n_preallocs */
      (GInstanceInitFunc) custom_cell_renderer_flexi_init,
    };

    /* Derive from GtkCellRenderer */
    cell_flexi_type = g_type_register_static (GTK_TYPE_CELL_RENDERER,
					      "CustomCellRendererFlexi",
					      &cell_flexi_info,
					      0);
  }

  return cell_flexi_type;
}


/***************************************************************************
 *
 *  custom_cell_renderer_flexi_init: set some default properties of the
 *                                      parent (GtkCellRenderer).
 *
 ***************************************************************************/

static void custom_cell_renderer_flexi_init (CustomCellRendererFlexi *cellrendererflexi)
{
  GTK_CELL_RENDERER(cellrendererflexi)->mode = GTK_CELL_RENDERER_MODE_INERT;
  GTK_CELL_RENDERER(cellrendererflexi)->xpad = 2;
  GTK_CELL_RENDERER(cellrendererflexi)->ypad = 2;
  /* cellrendererflexi->value = { 0 }; */
}


/***************************************************************************
 *
 *  custom_cell_renderer_flexi_class_init:
 *
 *  set up our own get_property and set_property functions, and
 *  override the parent's functions that we need to implement.
 *  And make our new "percentage" property known to the type system.
 *  If you want cells that can be activated on their own (ie. not
 *  just the whole row selected) or cells that are editable, you
 *  will need to override 'activate' and 'start_editing' as well.
 *
 ***************************************************************************/

static void custom_cell_renderer_flexi_class_init (CustomCellRendererFlexiClass *klass)
{
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS(klass);
  GObjectClass         *object_class = G_OBJECT_CLASS(klass);

  parent_class           = g_type_class_peek_parent (klass);
  object_class->finalize = custom_cell_renderer_flexi_finalize;

  /* Hook up functions to set and get our
   *   custom cell renderer properties */
  object_class->get_property = custom_cell_renderer_flexi_get_property;
  object_class->set_property = custom_cell_renderer_flexi_set_property;

  /* Override the two crucial functions that are the heart
   *   of a cell renderer in the parent class */
  cell_class->get_size = custom_cell_renderer_flexi_get_size;
  cell_class->render   = custom_cell_renderer_flexi_render;

  /* Install our very own properties */
  g_object_class_install_property (object_class,
				   PROP_TEXT,
				   g_param_spec_string ("text",
							"Text",
							"The main source of data to display",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_MIME_TYPE,
				   g_param_spec_string ("mime-type",
							"MimeType",
							"The format of data to display",
							NULL,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_FALLBACK,
				   g_param_spec_string ("fallback",
							"Fallback",
							"The alternative source of data to display if the main one is unavailable",
							NULL,
							G_PARAM_READWRITE));
}


/***************************************************************************
 *
 *  custom_cell_renderer_flexi_finalize: free any resources here
 *
 ***************************************************************************/

static void custom_cell_renderer_flexi_finalize (GObject *object)
{
  CustomCellRendererFlexi *crflexi = CUSTOM_CELL_RENDERER_FLEXI(object);
  
  /* g_free() simply return if ptr is NULL */
  g_free(crflexi->text);
  g_free(crflexi->mime_type);
  g_free(crflexi->fallback); 

  (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


/***************************************************************************
 *
 *  custom_cell_renderer_flexi_get_property: as it says
 *
 ***************************************************************************/

static void custom_cell_renderer_flexi_get_property (GObject    *object,
					 guint       param_id,
					 GValue     *value,
					 GParamSpec *psec)
{
  CustomCellRendererFlexi  *cellflexi = CUSTOM_CELL_RENDERER_FLEXI(object);

  switch (param_id)
  {
    case PROP_TEXT:
      g_value_set_string(value, cellflexi->text);
      break;

    case PROP_FALLBACK:
      g_value_set_string(value, cellflexi->fallback);
      break;

    case PROP_MIME_TYPE:
      g_value_set_string(value, cellflexi->mime_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
      break;
  }
}


/***************************************************************************
 *
 *  custom_cell_renderer_flexi_set_property: as it says
 *
 ***************************************************************************/

static void custom_cell_renderer_flexi_set_property (GObject      *object,
						     guint         param_id,
						     const GValue *value,
						     GParamSpec   *pspec)
{
  CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (object);

  switch (param_id)
  {
    case PROP_TEXT:
      g_free(cellflexi->text);
      cellflexi->text = g_value_dup_string(value);
      break;

    case PROP_FALLBACK:
      g_free(cellflexi->fallback);
      cellflexi->fallback = g_value_dup_string(value);
      break;

    case PROP_MIME_TYPE:
      g_free(cellflexi->mime_type);
      cellflexi->mime_type = g_value_dup_string(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
  }
}

/***************************************************************************
 *
 *  custom_cell_renderer_flexi_new: return a new cell renderer instance
 *
 ***************************************************************************/

GtkCellRenderer *custom_cell_renderer_flexi_new (gboolean interpret)
{
  CustomCellRendererFlexi *cellflexi = g_object_new(CUSTOM_TYPE_CELL_RENDERER_FLEXI, NULL);

  cellflexi->interpret = interpret;
  return GTK_CELL_RENDERER(cellflexi);
}


/***************************************************************************
 *
 *  custom_cell_renderer_flexi_get_size: crucial - calculate the size
 *                                          of our cell, taking into account
 *                                          padding and alignment properties
 *                                          of parent.
 *
 ***************************************************************************/

static void internal_get_size_layout(GtkCellRenderer *cell,
			      GtkWidget       *widget,
			      PangoLayout     *layout,
			      GdkRectangle    *cell_area,
			      gint            *x_offset,
			      gint            *y_offset,
			      gint            *width,
			      gint            *height)
{
  CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (cell);
  PangoRectangle rect;
  gint calc_width;
  gint calc_height;

  if (layout==NULL)
    layout = gtk_widget_create_pango_layout(widget,cellflexi->text);
  else
    g_object_ref(layout);
  g_assert(layout!=NULL);

  pango_layout_get_pixel_extents(layout,NULL,&rect);

  calc_width  = (gint) cell->xpad * 2 + rect.width;
  calc_height = (gint) cell->ypad * 2 + rect.height;

  if (width)
    *width = calc_width;

  if (height)
    *height = calc_height;

  if (cell_area)
  {
    if (x_offset)
    {
      *x_offset = cell->xalign * (cell_area->width - calc_width);
      *x_offset = MAX (*x_offset, 0);
    }

    if (y_offset)
    {
      *y_offset = cell->yalign * (cell_area->height - calc_height);
      *y_offset = MAX (*y_offset, 0);
    }
  }
  g_object_unref(layout);
}

/*
int internal_get_size(GtkCellRenderer *cell,
		      GtkWidget       *widget,
		      PangoLayout     *layout,
		      GdkRectangle    *cell_area,
		      gint            *x_offset,
		      gint            *y_offset,
		      gint            *width,
		      gint            *height)
{
    CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (cell);

     FIXME 
    
}
*/

static void custom_cell_renderer_flexi_get_size (GtkCellRenderer *cell,
						 GtkWidget       *widget,
						 GdkRectangle    *cell_area,
						 gint            *x_offset,
						 gint            *y_offset,
						 gint            *width,
						 gint            *height)
{
  internal_get_size_layout(cell,widget,NULL,cell_area,x_offset,y_offset,width,height);
}


/***************************************************************************
 *
 *  custom_cell_renderer_flexi_render: crucial - do the rendering.
 *
 ***************************************************************************/

static void custom_cell_renderer_flexi_render (GtkCellRenderer *cell,
					       GdkWindow       *window,
					       GtkWidget       *widget,
					       GdkRectangle    *background_area,
					       GdkRectangle    *cell_area,
					       GdkRectangle    *expose_area,
					       guint            flags)
{
  CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (cell);
  PangoLayout 		*layout;
  GtkStateType          state;
  gint                  width, height;
  gint                  x_offset, y_offset;

  
  layout = gtk_widget_create_pango_layout(widget,cellflexi->text);
	
  g_assert(layout!=NULL);

  internal_get_size_layout (cell, widget, layout,
		     cell_area,
		     &x_offset, &y_offset,
		     &width, &height);

  if (GTK_WIDGET_HAS_FOCUS (widget))
    state = GTK_STATE_ACTIVE;
  else
    state = GTK_STATE_NORMAL;

  width  -= cell->xpad*2;
  height -= cell->ypad*2;

  gtk_paint_layout (widget->style,
		    window,
		    state,
		    FALSE, 
		    expose_area, 
		    widget, 
		    "cellrendererflexi",
		    cell_area->x + x_offset + cell->xpad,
		    cell_area->y + y_offset + cell->ypad,
		    layout);
  g_object_unref(layout);
}

