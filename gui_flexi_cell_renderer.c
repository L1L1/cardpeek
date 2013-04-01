#include "gui_flexi_cell_renderer.h"
#include <string.h>

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
  PROP_RAW_VALUE = 1,
  PROP_MIME_TYPE,
  PROP_ALT_TEXT
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
      sizeof (CustomCellRendererFlexiClass),			    /* class size */
      NULL,							    /* base_init */
      NULL,							    /* base_finalize */
      (GClassInitFunc) custom_cell_renderer_flexi_class_init,	    /* class_init */
      NULL,                                                         /* class_finalize */
      NULL,                                                         /* class_data */
      sizeof (CustomCellRendererFlexi),				    /* instance size */
      0,                                                            /* n_preallocs */
      (GInstanceInitFunc) custom_cell_renderer_flexi_init,	    /* instance init */
      NULL,
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
  
  cellrendererflexi->raw_value = a_strnew(NULL);
  cellrendererflexi->alt_text  = a_strnew(NULL);
  cellrendererflexi->mime_type = a_strnew(NULL);
  cellrendererflexi->rendering = a_strnew(NULL);
  cellrendererflexi->default_width = -1;
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
				   PROP_RAW_VALUE,
				   g_param_spec_string ("raw-value",
							"RawValue",
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
				   PROP_ALT_TEXT,
				   g_param_spec_string ("alt-text",
							"AltText",
							"The alternative text to display if available",
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
  
  a_strfree(crflexi->raw_value);
  a_strfree(crflexi->mime_type);
  a_strfree(crflexi->alt_text); 
  a_strfree(crflexi->rendering); 

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
    case PROP_RAW_VALUE:
      g_value_set_string(value, a_strval(cellflexi->raw_value));
      break;

    case PROP_ALT_TEXT:
      g_value_set_string(value, a_strval(cellflexi->alt_text));
      break;

    case PROP_MIME_TYPE:
      g_value_set_string(value, a_strval(cellflexi->mime_type));
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
	  case PROP_RAW_VALUE:
		  if (value)
		   	a_strcpy(cellflexi->raw_value, g_value_get_string(value));
		  else
			a_strcpy(cellflexi->raw_value,"");
		  break;

	  case PROP_ALT_TEXT:
		  if (value)
		   	a_strcpy(cellflexi->alt_text, g_value_get_string(value));
		  else
			a_strcpy(cellflexi->alt_text,"");
		  break;

	  case PROP_MIME_TYPE:
		  if (value)
		   	a_strcpy(cellflexi->mime_type, g_value_get_string(value));
		  else
			a_strcpy(cellflexi->mime_type,"");
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

GtkCellRenderer *custom_cell_renderer_flexi_new (gboolean is_raw)
{
  CustomCellRendererFlexi *cellflexi = g_object_new(CUSTOM_TYPE_CELL_RENDERER_FLEXI, NULL);

  cellflexi->is_raw = is_raw;
  return GTK_CELL_RENDERER(cellflexi);
}

static void internal_format_raw(a_string_t *dst, const a_string_t *a_src)
{
	const char *src = a_strval(a_src);
	int len_src;
	
	a_strcpy(dst,"");

	if (src && strlen(src)>=2)
	{
		len_src = strlen(src)-2;
		
		a_strcat(dst,"<tt>");
	
		a_strcat(dst,src+2);

		a_strcat(dst,"</tt>");
		if (src[2])
		{
			switch (src[0]) {
				case '8':
					a_strcat(dst,"<span foreground='#2222ff'>h</span>");
					break;
				case '4':
					a_strcat(dst,"<span foreground='#2222ff'>q</span>");
					break;
				case '1':
					a_strcat(dst,"<span foreground='#2222ff'>b</span>");
					break;
				default:
					a_strcat(dst,"<span foreground='#2222ff'>?</span>");
			}
		}
		else
		{
			a_strcat(dst,"<span foreground='#2222ff'>-</span>");
		}
	}
}

static void internal_format_alt(a_string_t *dst, const a_string_t *a_src)
{
	const char *src;
	int i;
	int len_src = a_strlen(a_src);
	
	a_strcpy(dst,"");

	if (len_src)
	{
		src = a_strval(a_src);
		
		a_strcat(dst,"<tt><span foreground='#2f2fff'>&gt;</span> ");
		
		for (i=0;i<len_src;i++)
		{
			if (src[i]=='<') 	
				a_strcat(dst,"&lt;");
			else if (src[i]=='>')
				a_strcat(dst,"&gt;");
			else if (src[i]=='&')
				a_strcat(dst,"&amp;");
			else
				a_strpushback(dst,src[i]);
		}
		a_strcat(dst,"</tt>");
	}
}


static PangoLayout* internal_create_layout(GtkWidget *widget, CustomCellRendererFlexi *cellflexi)
{
	PangoContext* p_context = gtk_widget_get_pango_context(widget);
	PangoFontDescription* font_desc = NULL;
        PangoLayout* layout = NULL;
	PangoRectangle rect;

	if (cellflexi->default_width < 0)
	{
		layout = gtk_widget_create_pango_layout(widget,"0123456789ABCDEF");
		
		font_desc = pango_font_description_from_string("Monospace");
		if (font_desc)
			pango_layout_set_font_description(layout, font_desc);		
		
		pango_layout_get_pixel_extents(layout,NULL,&rect);

		cellflexi->default_width = rect.width*4;
	
		/* catch-all, if any conditions fails */
		if (cellflexi->default_width <= 0)
		{
			cellflexi->default_width = 400;
		}

		if (font_desc)
			pango_font_description_free(font_desc);

		g_object_unref(layout);		
	}

	layout = pango_layout_new(p_context);

	pango_layout_set_width(layout,cellflexi->default_width*PANGO_SCALE);

	if (!cellflexi->is_raw && a_strlen(cellflexi->alt_text))
	{
		internal_format_alt(cellflexi->rendering, cellflexi->alt_text);
		pango_layout_set_wrap(layout,PANGO_WRAP_WORD_CHAR);
	}
	else
	{
		internal_format_raw(cellflexi->rendering, cellflexi->raw_value);
		pango_layout_set_wrap(layout,PANGO_WRAP_CHAR);
	}
	pango_layout_set_markup(layout,a_strval(cellflexi->rendering),-1);

	return layout;
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
	  layout = internal_create_layout(widget,cellflexi);
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
      *x_offset = 0;
    }

    if (y_offset)
    {
      *y_offset = cell->yalign * (cell_area->height - calc_height);
      *y_offset = MAX (*y_offset, 0);
    }
  }
  g_object_unref(layout);
}


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

 
  layout = internal_create_layout(widget,cellflexi);
	
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


void custom_cell_renderer_flexi_set_format(GtkCellRenderer *crenderer, gboolean is_raw)
{
	CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (crenderer);

	cellflexi->is_raw = is_raw;
}

gboolean custom_cell_renderer_flexi_get_format(GtkCellRenderer *crenderer)
{
	CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (crenderer);
	
	return cellflexi->is_raw;
}

