/***************************************************************************
 *   Copyright (C) 2007 by PCMan (Hong Jen Yee)   *
 *   pcman.tw@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "image-view.h"
#include <math.h>

typedef struct
{
    GtkAdjustment *hadj, *vadj;
    GdkPixbuf* pix;
//    GdkPixmap* buffer;
//    bool cached;
    gdouble scale;
    GdkInterpType interp_type;
    guint idle_handler;
    GdkRectangle img_area;
    GtkAllocation allocation;
}LxImageViewPrivate;

static void lx_image_view_finalize(GObject *iv);

static void lx_image_view_clear( LxImageView* iv );
static void calc_image_area( LxImageView* iv );
static void paint(  LxImageView* iv, GdkRectangle* invalid_rect, GdkInterpType type, cairo_t* cr);

#if GTK_CHECK_VERSION(3, 0, 0)

static void lx_image_view_paint(  LxImageView* iv, cairo_t* cr );

static void on_get_preferred_width( GtkWidget* widget, gint* minimal_width, gint* natural_width );
static void on_get_preferred_height( GtkWidget* widget, gint* minimal_height, gint* natural_height );
static gboolean on_draw_event(GtkWidget* widget, cairo_t* cr);

#endif

static void on_size_allocate( GtkWidget* widget, GtkAllocation    *allocation );

G_DEFINE_TYPE_WITH_PRIVATE( LxImageView, lx_image_view, GTK_TYPE_WIDGET );

void lx_image_view_init( LxImageView* iv )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(iv);

    private->pix = NULL;
    private->scale =  1.0;
    private->interp_type = GDK_INTERP_BILINEAR;
    private->idle_handler = 0;

    gtk_widget_set_has_window( GTK_WIDGET(iv), FALSE );
    gtk_widget_set_can_focus( GTK_WIDGET(iv), TRUE );
    gtk_widget_set_app_paintable( GTK_WIDGET(iv), TRUE );
}


void lx_image_view_class_init( LxImageViewClass* klass )
{
    GObjectClass * obj_class;
    GtkWidgetClass *widget_class;

    obj_class = ( GObjectClass * ) klass;
//    obj_class->set_property = _set_property;
//   obj_class->get_property = _get_property;
    obj_class->finalize = lx_image_view_finalize;

    widget_class = GTK_WIDGET_CLASS ( klass );
    widget_class->get_preferred_width = on_get_preferred_width;
    widget_class->get_preferred_height = on_get_preferred_height;
    widget_class->draw = on_draw_event;
    widget_class->size_allocate = on_size_allocate;

/*
    // set up scrolling support
    klass->set_scroll_adjustments = set_adjustments;
    widget_class->set_scroll_adjustments_signal =
                g_signal_new ( "set_scroll_adjustments",
                        G_TYPE_FROM_CLASS (obj_class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (LxImageView::Class, set_scroll_adjustments),
                        NULL, NULL,
                        _marshal_VOID__OBJECT_OBJECT,
                        G_TYPE_NONE, 2,
                        GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
*/
}

void lx_image_view_finalize(GObject *iv)
{
    lx_image_view_clear( LX_IMAGE_VIEW(iv) );
/*
    if( vadj )
        g_object_unref( vadj );
    if( hadj )
        g_object_unref( hadj );
*/
}

GtkWidget* lx_image_view_new()
{
    return g_object_new ( LX_TYPE_IMAGE_VIEW, NULL );
}

// End of GObject-related stuff

void lx_image_view_set_adjustments( LxImageView* iv, GtkAdjustment* h, GtkAdjustment* v )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(iv);
    
    if( private->hadj != h )
    {
        if( G_LIKELY(private->hadj) )
        {
            g_object_unref( private->hadj );
        }
        if( G_LIKELY(h) )
        {
            private->hadj = (GtkAdjustment*)g_object_ref_sink( h );
        }
        else
        {
            private->hadj = NULL;
        }
    }
    if( private->vadj != v )
    {
        if( G_LIKELY( private->vadj) )
        {
            g_object_unref( private->vadj );
        }
        if( G_LIKELY(v) )
        {
            private->vadj = (GtkAdjustment*)g_object_ref_sink( v );
        }
        else
        {
            private->vadj = NULL;
        }
    }
}

void on_get_preferred_width( GtkWidget* widget, gint* minimal_width, gint* natural_width )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(LX_IMAGE_VIEW(widget));
    *minimal_width = *natural_width = private->img_area.width;
}

void on_get_preferred_height( GtkWidget* widget, gint* minimal_height, gint* natural_height )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(LX_IMAGE_VIEW(widget));
    *minimal_height = *natural_height = private->img_area.height;
}


void on_size_allocate( GtkWidget* widget, GtkAllocation   *allocation )
{
    GTK_WIDGET_CLASS(lx_image_view_parent_class)->size_allocate( widget, allocation );

    LxImageViewPrivate* private = lx_image_view_get_instance_private(LX_IMAGE_VIEW(widget));
    private->allocation = *allocation;

/*
    if( !iv->buffer || allocation->width != widget->allocation.width ||
        allocation->height != widget->allocation.height )
    {
        if( iv->buffer )
            g_object_unref( iv->buffer );
        iv->buffer = gdk_pixmap_new( (GdkDrawable*)widget->window,
                                        allocation->width, allocation->height, -1 );
        g_debug( "off screen buffer created: %d x %d", allocation->width, allocation->height );
    }
*/
    calc_image_area( LX_IMAGE_VIEW(widget) );
}

static cairo_region_t *
eel_cairo_get_clip_region (cairo_t *cr)
/* From nautilus http://git.gnome.org/browse/nautilus/tree/eel/eel-canvas.c */
{
        cairo_rectangle_list_t *list;
        cairo_region_t *region;
        int i;

        list = cairo_copy_clip_rectangle_list (cr);
        if (list->status == CAIRO_STATUS_CLIP_NOT_REPRESENTABLE) {
                cairo_rectangle_int_t clip_rect;

                cairo_rectangle_list_destroy (list);

                if (!gdk_cairo_get_clip_rectangle (cr, &clip_rect))
                        return NULL;
                return cairo_region_create_rectangle (&clip_rect);
        }


        region = cairo_region_create ();
        for (i = list->num_rectangles - 1; i >= 0; --i) {
                cairo_rectangle_t *rect = &list->rectangles[i];
                cairo_rectangle_int_t clip_rect;

                clip_rect.x = floor (rect->x);
                clip_rect.y = floor (rect->y);
                clip_rect.width = ceil (rect->x + rect->width) - clip_rect.x;
                clip_rect.height = ceil (rect->y + rect->height) - clip_rect.y;

                if (cairo_region_union_rectangle (region, &clip_rect) != CAIRO_STATUS_SUCCESS) {
                        cairo_region_destroy (region);
                        region = NULL;
                        break;
                }
        }

        cairo_rectangle_list_destroy (list);
        return region;
}

static gboolean on_draw_event( GtkWidget* widget, cairo_t *cr )
{

    LxImageView* iv = LX_IMAGE_VIEW(widget);

    if ( gtk_widget_get_mapped(widget) ) 
        lx_image_view_paint( iv, cr );
        
    return FALSE;

}

void lx_image_view_paint( LxImageView* iv, cairo_t *cr )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(iv);

    if( private->pix )
    {
        cairo_region_t * region = eel_cairo_get_clip_region(cr);
        int n_rects = cairo_region_num_rectangles(region);

        int i;
        for( i = 0; i < n_rects; ++i )
        {
            cairo_rectangle_int_t rectangle;
            cairo_region_get_rectangle(region, i, &rectangle);
            paint( iv, &rectangle, GDK_INTERP_NEAREST, cr );
        }

        cairo_region_destroy (region);
    }
}

void lx_image_view_clear( LxImageView* iv )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(iv);

    if( G_LIKELY(private->idle_handler) )
    {
        g_source_remove( private->idle_handler );
        private->idle_handler = 0;
    }

    if( G_LIKELY(private->pix) )
    {
        g_object_unref( private->pix );
        private->pix = NULL;
        calc_image_area( iv );
    }
/*
    if( buffer )
    {
        g_object_unref( buffer );
        buffer = NULL;
    }
*/
}

void lx_image_view_set_pixbuf( LxImageView* iv, GdkPixbuf* pixbuf )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(iv);
    
    if( pixbuf != private->pix )
    {
        lx_image_view_clear( iv );
        if( G_LIKELY(pixbuf) )
        {
            private->pix = (GdkPixbuf*)g_object_ref( pixbuf );
        }
        
        calc_image_area( iv );
        gtk_widget_queue_resize( (GtkWidget*)iv );
//        gtk_widget_queue_draw( (GtkWidget*)iv);
    }
}

void lx_image_view_set_scale( LxImageView* iv, gdouble new_scale, GdkInterpType type )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(iv);

    if( new_scale == private->scale )
        return;
    
    gdouble initZoom = private->scale;
    gint xPos, yPos;
    gdouble oldRelativePositionX,
            oldRelativePositionY,
            visibleAreaX,
            visibleAreaY;

    GdkDisplay* display = gdk_display_get_default();
    GdkSeat* seat = gdk_display_get_default_seat(display);
    GdkDevice* device = gdk_seat_get_pointer(seat);

    gdk_window_get_device_position ( gtk_widget_get_window(GTK_WIDGET(iv)),
                                     device,
                                     &xPos, 
                                     &yPos,
                                     NULL);
    
    //g_print("Mouse: %d,%d\n", xPos, yPos);
    oldRelativePositionX =  xPos * 1.0 / private->img_area.width; 
    oldRelativePositionY = yPos * 1.0 / private->img_area.height;
    visibleAreaX = xPos - gtk_adjustment_get_value(private->hadj);
    visibleAreaY = yPos - gtk_adjustment_get_value(private->vadj);

    private->scale = new_scale;
    if( G_LIKELY(private->pix) )
    {
        calc_image_area( iv );
        
        gdouble newPosX = oldRelativePositionX * private->img_area.width - visibleAreaX;
        gdouble newPosY = oldRelativePositionY * private->img_area.height - visibleAreaY;
        //g_print("Adjustment: %f,%f\n\n\n", newPosX, newPosY);

        gtk_adjustment_set_value(private->hadj, newPosX > 0 ? newPosX : 0);
        gtk_adjustment_set_value(private->vadj, newPosY > 0 ? newPosY : 0);

        gtk_widget_queue_resize( (GtkWidget*)iv );
    }
}

void calc_image_area( LxImageView* iv )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(iv);

    if( G_LIKELY( private->pix ) )
    {
        GtkAllocation allocation = private->allocation;
        private->img_area.width = (int)floor( ((gdouble)gdk_pixbuf_get_width( private->pix )) * private->scale + 0.5 );
        private->img_area.height = (int)floor( ((gdouble)gdk_pixbuf_get_height( private->pix )) * private->scale + 0.5 );
        // g_debug( "width=%d, height=%d", width, height );

        int x_offset = 0, y_offset = 0;
        if( private->img_area.width < allocation.width )
            x_offset = (int)floor( ((gdouble)(allocation.width - private->img_area.width)) / 2 + 0.5);

        if( private->img_area.height < allocation.height )
            y_offset = (int)floor( ((gdouble)(allocation.height - private->img_area.height)) / 2 + 0.5);

        private->img_area.x = x_offset;
        private->img_area.y = y_offset;
    }
    else
    {
        private->img_area.x = private->img_area.y = private->img_area.width = private->img_area.height = 0;
    }
}

void paint( LxImageView* iv, GdkRectangle* invalid_rect, GdkInterpType type, cairo_t* cr )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(iv);
    
    GdkRectangle rect;
    if( ! gdk_rectangle_intersect( invalid_rect, &private->img_area, &rect ) )
        return;

    int dest_x;
    int dest_y;

    GdkPixbuf* src_pix = NULL;
    if( private->scale == 1.0 )  // original size
    {
        src_pix = (GdkPixbuf*)g_object_ref( private->pix );
        dest_x = private->img_area.x;
        dest_y = private->img_area.y;
    }
    else    // scaling is needed
    {
        dest_x = rect.x;
        dest_y = rect.y;

        rect.x -= private->img_area.x;
        rect.y -= private->img_area.y;

        GdkPixbuf* scaled_pix = NULL;
        int src_x = (int)floor( ((gdouble)rect.x) / private->scale + 0.5 );
        int src_y = (int)floor( ((gdouble)rect.y) / private->scale + 0.5 );
        int src_w = (int)floor( ((gdouble)rect.width) / private->scale + 0.5 );
        int src_h = (int)floor( ((gdouble)rect.height) / private->scale + 0.5 );
        
        if( src_y > gdk_pixbuf_get_height( private->pix ) )
            src_y = gdk_pixbuf_get_height( private->pix );

        if( src_x + src_w > gdk_pixbuf_get_width( private->pix ) )
            src_w = gdk_pixbuf_get_width( private->pix ) - src_x;

        if( src_y + src_h > gdk_pixbuf_get_height( private->pix ) )
            src_h = gdk_pixbuf_get_height( private->pix ) - src_y;
            
        /*g_print("orig src: x=%d, y=%d, w=%d, h=%d\n",
                src_x, src_y, src_w, src_h );*/

        if ((src_w > 0) && (src_h > 0))
        {
            src_pix = gdk_pixbuf_new_subpixbuf( private->pix, src_x, src_y,  src_w, src_h );
            scaled_pix = gdk_pixbuf_scale_simple( src_pix, rect.width, rect.height, type );
            g_object_unref( src_pix );
            src_pix = scaled_pix;
        }

    }

    if( G_LIKELY(src_pix) )
    {
        GtkWidget* widget = (GtkWidget*)iv;
        gdk_cairo_set_source_pixbuf (cr, src_pix, dest_x, dest_y);
        cairo_paint (cr);

        g_object_unref( src_pix );
    }
}

void lx_image_view_get_size( LxImageView* iv, int* w, int* h )
{
    LxImageViewPrivate* private = lx_image_view_get_instance_private(iv);

    if( G_LIKELY(w) )
        *w = private->img_area.width;

    if( G_LIKELY(h) )
        *h = private->img_area.height;
}
