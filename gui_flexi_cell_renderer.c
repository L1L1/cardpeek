#include "gui_flexi_cell_renderer.h"
#include "bytestring.h"
#include <string.h>
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
        const GdkRectangle         *cell_area,
        gint                       *x_offset,
        gint                       *y_offset,
        gint                       *width,
        gint                       *height);

static void     custom_cell_renderer_flexi_render     (GtkCellRenderer            *cell,
        cairo_t              *cr,
        GtkWidget            *widget,
        const GdkRectangle   *background_area,
        const GdkRectangle   *cell_area,
        GtkCellRendererState  flags);

enum
{
    PROP_RAW_VALUE = 1,
    PROP_MIME_TYPE,
    PROP_ALT_TEXT
};

enum
{
    RENDER_NONE,
    RENDER_TEXT,
    RENDER_PIXBUF
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
    /* default: GTK_CELL_RENDERER(cellrendererflexi)->mode = GTK_CELL_RENDERER_MODE_INERT; */
    gtk_cell_renderer_set_padding(GTK_CELL_RENDERER(cellrendererflexi), 2, 2);

    cellrendererflexi->raw_value = a_strnew(NULL);
    cellrendererflexi->alt_text  = a_strnew(NULL);
    cellrendererflexi->mime_type = a_strnew(NULL);
    cellrendererflexi->rendered_type = RENDER_NONE;
    cellrendererflexi->rendered_value = NULL;
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

static void internal_clear_rendered(CustomCellRendererFlexi *cr)
{
    switch (cr->rendered_type)
    {
        case RENDER_TEXT:
            a_strfree((a_string_t*)cr->rendered_value);
            break;
        case RENDER_PIXBUF:
            g_object_unref(cr->rendered_value);
            break;
    }
    cr->rendered_type = RENDER_NONE;
    cr->rendered_value = NULL;
}

static void custom_cell_renderer_flexi_finalize (GObject *object)
{
    CustomCellRendererFlexi *crflexi = CUSTOM_CELL_RENDERER_FLEXI(object);

    a_strfree(crflexi->raw_value);
    a_strfree(crflexi->mime_type);
    a_strfree(crflexi->alt_text);
    internal_clear_rendered(crflexi);

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
    internal_clear_rendered(cellflexi);
}

/***************************************************************************
 *
 *  custom_cell_renderer_flexi_new: return a new cell renderer instance
 *
 ***************************************************************************/

GtkCellRenderer *custom_cell_renderer_flexi_new (gboolean is_raw)
{
    CustomCellRendererFlexi *cellflexi = g_object_new(CUSTOM_TYPE_CELL_RENDERER_FLEXI, NULL);

    cellflexi -> is_raw = is_raw;

    /*
      if (!is_raw)
      {
    	GSList *formats = gdk_pixbuf_get_formats();
    	GSList* item;
    	char **mime_types;
    	int i;
    	if (formats)
    	{
    		for (item=formats;item!=NULL;item=g_slist_next(item))
    		{
    			mime_types = gdk_pixbuf_format_get_mime_types(item->data);
    			for (i=0;mime_types[i]!=NULL;i++)
    				g_printf("Format(%s): %s\n",
    					 gdk_pixbuf_format_get_name(item->data),
    					 mime_types[i]);
    			g_strfreev(mime_types);
    		}
    		g_slist_free(formats);
    	}
      }
    */
    return GTK_CELL_RENDERER(cellflexi);
}

/***************************************************************************
 *
 * RENDERING
 *
 */

static gboolean internal_render_error(CustomCellRendererFlexi *cr, const char *msg)
{
    cr->rendered_type = RENDER_TEXT;
    cr->rendered_value = a_strnew("<span foreground='#7F0000'>Error: ");
    a_strcat(cr->rendered_value,msg);
    a_strcat(cr->rendered_value,"</span>");
    if (a_strlen(cr->mime_type)>2)
        log_printf(LOG_WARNING,"Rendering error for '%s' mime-type: %s",a_strval(cr->mime_type)+2,msg);
    else
        log_printf(LOG_WARNING,"Rendering error: %s",msg);
    return FALSE;
}

static void internal_format_raw(a_string_t *dst, int len_src, const char *src, int limit)
{
    char tmp[40];

    a_strcpy(dst,"");

    if (len_src>=2)
    {
        a_strcat(dst,"<tt>");

        a_strncat(dst,limit,src+2);

        a_strcat(dst,"</tt>");

        if (limit<len_src-2)
        {
            sprintf(tmp,"[%i bytes follow...]",(len_src-2-limit)/2);
            a_strcat(dst,tmp);
        }

        if (src[2])
        {
            switch (src[0])
            {
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

static void internal_format_alt(a_string_t *dst, int len_src, const char *src)
{
    int i;

    a_strcpy(dst,"");

    if (len_src>=2)
    {
        a_strcat(dst,"<tt><span foreground='#2f2fff'>&gt;</span> ");

        for (i=2; i<len_src; i++)
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

static gboolean internal_format(CustomCellRendererFlexi *cr, const a_string_t *a_src, int limit)
{
    int len_src 	= a_strlen(a_src);
    const char *src = a_strval(a_src);

    cr->rendered_type = RENDER_TEXT;
    cr->rendered_value = a_strnew(NULL);

    if (src==NULL || len_src<2) return FALSE;

    switch (src[0])
    {
        case '8':
        case '4':
        case '1':
            internal_format_raw(cr->rendered_value,len_src,src,limit);
            break;
        case 't':
            internal_format_alt(cr->rendered_value,len_src,src);
            break;
        default:
            return internal_render_error(cr,"Internal format error.");
    }
    return TRUE;
}

static gboolean internal_load_image(CustomCellRendererFlexi *cr, const char *src)
{
    GdkPixbufLoader *loader;
    bytestring_t *bs = bytestring_new_from_string(src);
    GError *err = NULL;

    if (bs==NULL)
    {
        internal_render_error(cr,"No image data.");
        return FALSE;
    }

    if (bs->width!=8)
    {
        internal_render_error(cr,"Image data must be octets.");
        bytestring_free(bs);
        return FALSE;
    }

    loader = gdk_pixbuf_loader_new();

    if (gdk_pixbuf_loader_write(loader,bs->data,bs->len,&err)==FALSE)
    {
        if (err!=NULL)
        {
            internal_render_error(cr,err->message);
            g_error_free(err);
            bytestring_free(bs);
            return FALSE;
        }
    }

    if (gdk_pixbuf_loader_close(loader,&err)==FALSE)
    {
        if (err!=NULL)
        {
            internal_render_error(cr,err->message);
            g_error_free(err);
            bytestring_free(bs);
            return FALSE;
        }
    }

    cr->rendered_type  = RENDER_PIXBUF;
    cr->rendered_value = (GdkPixbuf*) gdk_pixbuf_loader_get_pixbuf(loader);

    g_object_ref(cr->rendered_value);

    g_object_unref(loader);

    bytestring_free(bs);
    return TRUE;
}

static int internal_prepare_rendering(CustomCellRendererFlexi *cr)
{
    if (cr->rendered_type!=RENDER_NONE) return cr->rendered_type;

    if (cr->is_raw)
    {
        internal_format(cr, cr->raw_value, 65536);
    }
    else
    {
        if (a_strlen(cr->mime_type)>2)
        {
            if (strstr(a_strval(cr->mime_type)+2,"image/")!=NULL)
            {
                internal_load_image(cr,a_strval(cr->raw_value));
            }
            else
            {
                internal_render_error(cr,"Unrecognized mime-type");
            }
        }
        else
        {
            if (a_strlen(cr->alt_text)>0)
            {
                internal_format(cr, cr->alt_text,65536);
            }
            else if (a_strlen(cr->raw_value)>0)
            {
                internal_format(cr, cr->raw_value,256);
            }
        }
    }
    return cr->rendered_type;
}

/***************************************************************************
 *
 *  custom_cell_renderer_flexi_get_size: crucial - calculate the size
 *                                          of our cell, taking into account
 *                                          padding and alignment properties
 *                                          of parent.
 *
 ***************************************************************************/


static PangoLayout* internal_text_create_layout(GtkWidget *widget, CustomCellRendererFlexi *cellflexi)
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

    pango_layout_set_wrap(layout,PANGO_WRAP_WORD_CHAR);

    pango_layout_set_markup(layout,a_strval((a_string_t*)cellflexi->rendered_value),-1);

    return layout;
}

static void internal_text_get_size_layout(GtkCellRenderer *cell,
        GtkWidget       *widget,
        PangoLayout     *layout,
        const GdkRectangle    *cell_area,
        gint            *x_offset,
        gint            *y_offset,
        gint            *width,
        gint            *height)
{
    CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (cell);
    PangoRectangle rect;
    gint calc_width;
    gint calc_height;
    gint xpad;
    gint ypad;
    gfloat yalign;

    if (layout==NULL)
        layout = internal_text_create_layout(widget,cellflexi);
    else
        g_object_ref(layout);

    g_assert(layout!=NULL);

    pango_layout_get_pixel_extents(layout,NULL,&rect);

    gtk_cell_renderer_get_padding(cell,&xpad,&ypad);

    calc_width  = xpad * 2 + rect.width;
    calc_height = ypad * 2 + rect.height;

    if (width)
    {
        *width = calc_width;
    }

    if (height)
    {
        *height = calc_height;
    }

    if (cell_area)
    {
        if (x_offset)
        {
            *x_offset = 0;
        }

        if (y_offset)
        {
            gtk_cell_renderer_get_alignment(cell,NULL,&yalign);
            *y_offset = yalign * (cell_area->height - calc_height);
            *y_offset = MAX (*y_offset, 0);
        }
    }
    g_object_unref(layout);
}

static void internal_image_get_size(GtkCellRenderer *cell,
                                    GtkWidget       *widget,
                                    const GdkRectangle    *cell_area,
                                    gint            *x_offset,
                                    gint            *y_offset,
                                    gint            *width,
                                    gint            *height)
{
    CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (cell);
    int image_width = gdk_pixbuf_get_width((GdkPixbuf*)cellflexi->rendered_value);
    int image_height = gdk_pixbuf_get_height((GdkPixbuf*)cellflexi->rendered_value);
    gint calc_width;
    gint calc_height;
    gint xpad;
    gint ypad;

    UNUSED(widget);
    UNUSED(cell_area);

    gtk_cell_renderer_get_padding(cell,&xpad,&ypad);

    calc_width  = xpad * 2 + image_width;
    calc_height = ypad * 2 + image_height;

    if (width)
        *width = calc_width;

    if (height)
        *height = calc_height;

    if (x_offset)
    {
        *x_offset = 0;
    }

    if (y_offset)
    {
        *y_offset = 0;
    }
}



static void custom_cell_renderer_flexi_get_size (GtkCellRenderer *cell,
        GtkWidget       *widget,
        const GdkRectangle    *cell_area,
        gint            *x_offset,
        gint            *y_offset,
        gint            *width,
        gint            *height)
{
    CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (cell);
    int engine = internal_prepare_rendering(cellflexi);

    if (x_offset) *x_offset = 0;
    if (y_offset) *y_offset = 0;
    if (width) *width = 0;
    if (height) *height = 0;

    switch (engine)
    {
        case RENDER_TEXT:
            internal_text_get_size_layout(cell,widget,NULL,cell_area,x_offset,y_offset,width,height);
            break;
        case RENDER_PIXBUF:
            internal_image_get_size(cell,widget,cell_area,x_offset,y_offset,width,height);
            break;
    }
}


/***************************************************************************
 *
 *  custom_cell_renderer_flexi_render: crucial - do the rendering.
 *
 ***************************************************************************/


static void internal_text_render(GtkCellRenderer *cell,
                                 cairo_t         *cr,
                                 GtkWidget       *widget,
                                 const GdkRectangle    *background_area,
                                 const GdkRectangle    *cell_area,
                                 guint            flags)
{
    CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (cell);
    PangoLayout 		*layout;
    GtkStateType          	state;
    gint                  	width, height;
    gint                  	x_offset, y_offset;
    gint 			xpad;
    gint 			ypad;

    UNUSED(background_area);
    UNUSED(flags);

    layout = internal_text_create_layout(widget,cellflexi);

    g_assert(layout!=NULL);

    internal_text_get_size_layout (cell, widget, layout,
                                   cell_area,
                                   &x_offset, &y_offset,
                                   &width, &height);

    if (gtk_widget_has_focus(widget))
        state = GTK_STATE_ACTIVE;
    else
        state = GTK_STATE_NORMAL;

    gtk_cell_renderer_get_padding(cell,&xpad,&ypad);
    width  -= xpad*2;
    height -= ypad*2;

    gtk_render_layout(gtk_widget_get_style_context(widget),
                      cr,
                      cell_area->x + x_offset + xpad,
                      cell_area->y + y_offset + ypad,
                      layout);

    g_object_unref(layout);
}

static void internal_image_render(GtkCellRenderer *cell,
                                  cairo_t         *cr,
                                  GtkWidget       *widget,
                                  const GdkRectangle    *background_area,
                                  const GdkRectangle    *cell_area,
                                  guint            flags)
{
    CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (cell);
    UNUSED(widget);
    UNUSED(background_area);
    UNUSED(flags);


    gdk_cairo_set_source_pixbuf(cr, (GdkPixbuf *)cellflexi->rendered_value,cell_area->x,cell_area->y);
    cairo_paint(cr);
    cairo_fill(cr);
}

static void custom_cell_renderer_flexi_render (GtkCellRenderer *cell,
        cairo_t              *cr,
        GtkWidget            *widget,
        const GdkRectangle   *background_area,
        const GdkRectangle   *cell_area,
        GtkCellRendererState  flags)
{
    CustomCellRendererFlexi *cellflexi = CUSTOM_CELL_RENDERER_FLEXI (cell);
    int engine = internal_prepare_rendering(cellflexi);

    switch (engine)
    {
        case RENDER_TEXT:
            internal_text_render(cell, cr, widget, background_area, cell_area, flags);
            break;
        case RENDER_PIXBUF:
            internal_image_render(cell, cr, widget, background_area, cell_area, flags);
    }
}


