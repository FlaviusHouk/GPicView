/***************************************************************************
 *   Copyright (C) 2007, 2008 by PCMan (Hong Jen Yee)                      *
 *   pcman.tw@gmail.com                                                    *
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "main-win.h"

#include "ViewModel/image-item.h"
#include "ViewModel/dialog_service.h"

#include "View/working-area.h"
#include "View/file-dlgs.h"

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pref.h"

#include "image-view.h"
#include "ptk-menu.h"

/* For drag & drop */
static GtkTargetEntry drop_targets[] =
{
    {"text/uri-list", 0, 0},
    {"text/plain", 0, 1}
};

static void main_win_init( MainWin* mw );
static void main_win_finalize( GObject* obj );

static void create_nav_bar( MainWin* mw, GtkWidget* box);

static void 
add_nav_btn_img( MainWin* mw, GtkButton* but, GCallback cb, gboolean toggle);
// GtkWidget* add_menu_item(  GtkMenuShell* menu, const char* label, const char* icon, GCallback cb, gboolean toggle=FALSE );
static void rotate_image( MainWin* mw );
static void show_popup_menu( MainWin* mw, GdkEventButton* evt );

/* signal handlers */
static gboolean on_delete_event( GtkWidget* widget, GdkEventAny* evt );
static void on_size_allocate( GtkWidget* widget, GtkAllocation    *allocation );
static gboolean on_win_state_event( GtkWidget* widget, GdkEventWindowState* state );
static void on_scroll_size_allocate(GtkWidget* widget, GtkAllocation* allocation, MainWin* mv);
static void on_zoom_fit( GtkToggleButton* btn, MainWin* mw );
static void on_zoom_fit_menu( GtkMenuItem* item, MainWin* mw );
static void on_full_screen( GtkWidget* btn, MainWin* mw );
static void on_next( GtkWidget* btn, MainWin* mw );
static void on_orig_size( GtkToggleButton* btn, MainWin* mw );
static void on_orig_size_menu( GtkToggleButton* btn, MainWin* mw );
static void on_prev( GtkWidget* btn, MainWin* mw );
static void on_rotate_clockwise( GtkWidget* btn, MainWin* mw );
static void on_rotate_counterclockwise( GtkWidget* btn, MainWin* mw );
static void on_save_as( GtkWidget* btn, MainWin* mw );
static void on_save( GtkWidget* btn, MainWin* mw );
static void cancel_slideshow(MainWin* mw);
static gboolean next_slide(MainWin* mw);
static void on_slideshow_menu( GtkMenuItem* item, MainWin* mw );
static void on_slideshow( GtkToggleButton* btn, MainWin* mw );
static void on_open( GtkWidget* btn, MainWin* mw );
static void on_zoom_in( GtkWidget* btn, MainWin* mw );
static void on_zoom_out( GtkWidget* btn, MainWin* mw );
static void on_preference( GtkWidget* btn, MainWin* mw );
static void on_toggle_toolbar( GtkMenuItem* item, MainWin* mw );
static void on_quit( GtkWidget* btn, MainWin* mw );
static gboolean on_button_press( GtkWidget* widget, GdkEventButton* evt, MainWin* mw );
static gboolean on_button_release( GtkWidget* widget, GdkEventButton* evt, MainWin* mw );
static gboolean on_mouse_move( GtkWidget* widget, GdkEventMotion* evt, MainWin* mw );
static gboolean on_scroll_event( GtkWidget* widget, GdkEventScroll* evt, MainWin* mw );

static gboolean 
on_key_press_event(GtkWidget* widget, GdkEventKey * key, gpointer user_data);

static void on_drag_data_received( GtkWidget* widget, GdkDragContext *drag_context,
                int x, int y, GtkSelectionData* data, guint info, guint time, MainWin* mw );
static void on_delete( GtkWidget* btn, MainWin* mw );
static void on_about( GtkWidget* menu, MainWin* mw );
static void on_animation_timeout( ViewModelsImageItem* this, MainWin* mw );

static void update_title(const char *filename, MainWin *mw );
static void update_btns(MainWin* mw, ViewModelsImageItem* item);

void on_flip_vertical( GtkWidget* btn, MainWin* mw );
void on_flip_horizontal( GtkWidget* btn, MainWin* mw );
static int trans_angle_to_id(int i);
static int get_new_angle( int orig_angle, int rotate_angle );

static void main_win_set_zoom_scale(MainWin* mw, double scale);
static void main_win_set_zoom_mode(MainWin* mw, ZoomMode mode);
static void main_win_update_zoom_buttons_state(MainWin* mw);
static void main_win_adjust_zoom(MainWin* this, ViewModelsImageItem* item);
static void main_win_set_static_styles(MainWin* this);

// Begin of GObject-related stuff

G_DEFINE_TYPE( MainWin, main_win, GTK_TYPE_WINDOW )

void 
main_win_class_init( MainWinClass* klass )
{
    GObjectClass * obj_class;
    GtkWidgetClass *widget_class;

    obj_class = ( GObjectClass * ) klass;
//    obj_class->set_property = _set_property;
//   obj_class->get_property = _get_property;
    obj_class->finalize = main_win_finalize;

    widget_class = GTK_WIDGET_CLASS ( klass );
    widget_class->delete_event = on_delete_event;
    widget_class->size_allocate = on_size_allocate;
    widget_class->window_state_event = on_win_state_event;
}

void main_win_finalize( GObject* obj )
{
    MainWin *mw = MAIN_WIN(obj);

    //TODO: move to dispose.
    if( G_LIKELY(mw->view_model) )
        g_object_unref( mw->view_model );

    g_object_unref( mw->hand_cursor );
}

GtkWidget* main_win_new()
{
    return (GtkWidget*)g_object_new ( MAIN_WIN_TYPE, NULL );
}

// End of GObject-related stuff

static void
main_win_vm_property_chagned(GObject* sender,
                             GParamSpec* pspec,
                             gpointer user_data)
{
    ViewModelsMainWinVM* view_model = VIEW_MODELS_MAIN_WIN_VM(sender);
    MainWin* this = MAIN_WIN(user_data);

    if(strcmp(pspec->name, CURRENT_ITEM_PROP_NAME) == 0)
    {
        ViewModelsImageItem* new_item = view_models_main_win_vm_get_current_item(view_model);

        if(new_item == NULL)
            return;

        gchar* disp_name = g_filename_display_name(g_path_get_basename(view_models_image_item_get_name(new_item)));
        update_title(disp_name, this);
        g_free(disp_name);

        update_btns( this, new_item);
        main_win_adjust_zoom(this, new_item);
    }
}

void main_win_init( MainWin*mw )
{
    mw->view_model = view_models_main_win_vm_new();
    g_signal_connect(mw->view_model, "notify", G_CALLBACK(main_win_vm_property_chagned), mw);

    gtk_window_set_title( (GtkWindow*)mw, _("Image Viewer"));
    if (gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "gpicview"))
    {
        gtk_window_set_icon_name((GtkWindow*)mw, "gpicview");
    }
    else
    {
        gtk_window_set_icon_from_file((GtkWindow*)mw, PACKAGE_DATA_DIR "/icons/hicolor/48x48/apps/gpicview.png", NULL);
    }
    gtk_window_set_default_size( (GtkWindow*)mw, 640, 480 );

    GtkWidget* box = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
    gtk_container_add( (GtkContainer*)mw, box);

    // image area
    mw->evt_box = gtk_event_box_new();
    gtk_widget_set_can_focus(mw->evt_box,TRUE);

    gtk_widget_add_events( mw->evt_box,
                           GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|
                           GDK_BUTTON_RELEASE_MASK|GDK_SCROLL_MASK|
                           GDK_KEY_PRESS_MASK);

    g_signal_connect( mw->evt_box, "button-press-event", G_CALLBACK(on_button_press), mw );
    g_signal_connect( mw->evt_box, "button-release-event", G_CALLBACK(on_button_release), mw );
    g_signal_connect( mw->evt_box, "motion-notify-event", G_CALLBACK(on_mouse_move), mw );
    g_signal_connect( mw->evt_box, "scroll-event", G_CALLBACK(on_scroll_event), mw );
    g_signal_connect( mw->evt_box, "key-press-event", G_CALLBACK(on_key_press_event), mw);
    // Set bg color to white

    //gtk_widget_modify_bg( mw->evt_box, GTK_STATE_NORMAL, &pref.bg );
    gtk_widget_set_name(mw->evt_box, "ImageArea");

    GError* err = NULL;
    GtkStyleProvider* provider = GTK_STYLE_PROVIDER(gtk_css_provider_new());
    gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider),
                                    pref_build_dynamic_css(&pref),
                                    -1,
                                    &err);

    if(G_LIKELY(err))
    {
        g_printerr("Cannot parse dynamic css: %d.\n%s\n", err->code, err->message);
        return;
    }

    main_win_set_dynamic_style(mw, provider);
    main_win_set_static_styles(mw);

    mw->img_view = lx_image_view_new();
    gtk_container_add( (GtkContainer*)mw->evt_box, (GtkWidget*)mw->img_view);

    mw->scroll = gtk_scrolled_window_new( NULL, NULL );
    g_signal_connect(G_OBJECT(mw->scroll), "size-allocate", G_CALLBACK(on_scroll_size_allocate), (gpointer) mw);
    gtk_scrolled_window_set_shadow_type( (GtkScrolledWindow*)mw->scroll, GTK_SHADOW_NONE );
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)mw->scroll,
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)mw->scroll);
    gtk_adjustment_set_page_increment(hadj, 10);

    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)mw->scroll);
    gtk_adjustment_set_page_increment(vadj, 10);

    lx_image_view_set_adjustments( LX_IMAGE_VIEW(mw->img_view), hadj, vadj );    // dirty hack :-(
    gtk_container_add ( GTK_CONTAINER (mw->scroll), mw->evt_box );
    
    GtkWidget* viewport = gtk_bin_get_child( (GtkBin*)mw->scroll );
    gtk_viewport_set_shadow_type( (GtkViewport*)viewport, GTK_SHADOW_NONE );
    gtk_container_set_border_width( (GtkContainer*)viewport, 0 );

    gtk_box_pack_start( (GtkBox*)box, mw->scroll, TRUE, TRUE, 0 );

    // build toolbar
    create_nav_bar( mw, box );
    gtk_widget_show_all( box );

    if (pref.show_toolbar)
        gtk_widget_show(gtk_widget_get_parent(mw->nav_bar));
    else
        gtk_widget_hide(gtk_widget_get_parent(mw->nav_bar));


    mw->hand_cursor = gdk_cursor_new_for_display( gtk_widget_get_display((GtkWidget*)mw), GDK_FLEUR );

//    zoom_mode = ZOOM_NONE;
    mw->zoom_mode = ZOOM_FIT;

    // Set up drag & drop
    gtk_drag_dest_set( (GtkWidget*)mw, GTK_DEST_DEFAULT_ALL,
                                                    drop_targets,
                                                    G_N_ELEMENTS(drop_targets),
                                                    GDK_ACTION_COPY | GDK_ACTION_ASK );
    g_signal_connect( mw, "drag-data-received", G_CALLBACK(on_drag_data_received), mw );
}

void create_nav_bar( MainWin* mw, GtkWidget* box )
{
    GtkBuilder* builder = gtk_builder_new_from_file (PACKAGE_DATA_DIR "/gpicview/ui/nav_bar.ui");
    mw->nav_bar = GTK_WIDGET(gtk_builder_get_object(builder, "nav_bar"));

    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "go_prev_butt")), G_CALLBACK(on_prev), FALSE );
    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "go_next_butt")), G_CALLBACK(on_next), FALSE );

    mw->btn_play_stop = GTK_WIDGET(gtk_builder_get_object(builder, "slideshow_butt"));
    add_nav_btn_img( mw, GTK_BUTTON(mw->btn_play_stop), G_CALLBACK(on_slideshow), TRUE );

    mw->img_play_stop = GTK_WIDGET(gtk_builder_get_object(builder, "start_stop_img"));

    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "zoom_out_butt")), G_CALLBACK(on_zoom_out), FALSE );
    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "zoom_in_butt")), G_CALLBACK(on_zoom_in), FALSE );
    
//    g_signal_connect( percent, "activate", G_CALLBACK(on_percentage), mw );
//    gtk_widget_set_size_request( percent, 45, -1 );
//    gtk_box_pack_start( (GtkBox*)nav_bar, percent, FALSE, FALSE, 2 );

    mw->btn_fit = GTK_WIDGET(gtk_builder_get_object(builder, "zoom_fit_best_butt"));
    add_nav_btn_img( mw, GTK_BUTTON(mw->btn_fit), G_CALLBACK(on_zoom_fit), TRUE );

    mw->btn_orig = GTK_WIDGET(gtk_builder_get_object(builder, "zoom_orig_butt"));
    add_nav_btn_img( mw, GTK_BUTTON(mw->btn_orig), G_CALLBACK(on_orig_size), TRUE );

    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "fullscreen_butt")), G_CALLBACK(on_full_screen), FALSE );

    mw->btn_rotate_ccw = GTK_WIDGET(gtk_builder_get_object(builder, "rotate_left_butt"));
    add_nav_btn_img( mw, GTK_BUTTON(mw->btn_rotate_ccw), G_CALLBACK(on_rotate_counterclockwise), FALSE );

    mw->btn_rotate_cw = GTK_WIDGET(gtk_builder_get_object(builder, "rotate_right_butt"));
    add_nav_btn_img( mw, GTK_BUTTON(mw->btn_rotate_cw), G_CALLBACK(on_rotate_clockwise), FALSE );

    mw->btn_flip_h = GTK_WIDGET(gtk_builder_get_object(builder, "horr_flip_butt"));
    add_nav_btn_img( mw, GTK_BUTTON(mw->btn_flip_h), G_CALLBACK(on_flip_horizontal), FALSE );
    mw->btn_flip_v = GTK_WIDGET(gtk_builder_get_object(builder, "vert_flip_butt"));
    add_nav_btn_img( mw, GTK_BUTTON(mw->btn_flip_v), G_CALLBACK(on_flip_vertical), FALSE );

    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "open_butt")), G_CALLBACK(on_open), FALSE );
    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "save_butt")), G_CALLBACK(on_save), FALSE );
    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "save_as_butt")), G_CALLBACK(on_save_as), FALSE );
    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "delete_butt")), G_CALLBACK(on_delete), FALSE );

    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "pref_butt")), G_CALLBACK(on_preference), FALSE );
    add_nav_btn_img( mw, GTK_BUTTON(gtk_builder_get_object(builder, "exit_butt")), G_CALLBACK(on_quit), FALSE );

    GtkWidget* holder = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(holder, "NavBarParent");

    gtk_box_pack_start(GTK_BOX(holder), mw->nav_bar, FALSE, FALSE, 0);
    gtk_box_set_homogeneous(GTK_BOX(holder), TRUE);

    gtk_box_pack_start( GTK_BOX(box), holder, FALSE, TRUE, 0 );
}

gboolean on_delete_event( GtkWidget* widget, GdkEventAny* evt )
{
    gtk_widget_destroy( widget );
    return TRUE;
}

static void update_title(const char *filename, MainWin *mw )
{
    static gchar fname[50];
    static gint wid, hei;

    gchar buf[100];

    if(filename != NULL)
    {
        strncpy(fname, filename, 49);
        fname[49] = '\0';

        ViewModelsImageItem* item = view_models_main_win_vm_get_current_item(mw->view_model);
        GdkPixbuf* pix = view_models_image_item_get_pixbuf(item);

        wid = gdk_pixbuf_get_width( pix );
        hei = gdk_pixbuf_get_height( pix );
    }

    snprintf(buf, 100, "%s (%dx%d) %d%%", fname, wid, hei, (int)(mw->scale * 100));
    gtk_window_set_title( (GtkWindow*)mw, buf );
}

static void
on_animation_timeout( ViewModelsImageItem* item, MainWin* mw )
{
    lx_image_view_set_pixbuf(LX_IMAGE_VIEW(mw->img_view), 
                             view_models_image_item_get_pixbuf(item));
}

static void update_btns(MainWin* mw, ViewModelsImageItem* item)
{
    gboolean enable = !view_models_image_item_is_animated(item);

    gtk_widget_set_sensitive(mw->btn_rotate_cw, enable);
    gtk_widget_set_sensitive(mw->btn_rotate_ccw, enable);
    gtk_widget_set_sensitive(mw->btn_flip_v, enable);
    gtk_widget_set_sensitive(mw->btn_flip_h, enable);
}

static void 
main_win_adjust_zoom(MainWin* this, ViewModelsImageItem* item)
{
    GdkPixbuf* pix = view_models_image_item_get_pixbuf(item);

    // select most suitable viewing mode
    if( this->zoom_mode == ZOOM_NONE )
    {
        gint w = gdk_pixbuf_get_width( pix );
        gint h = gdk_pixbuf_get_height( pix );

        GdkRectangle area;
        get_working_area( gtk_widget_get_window(GTK_WIDGET(this)), &area );

        // g_debug("determine best zoom mode: orig size:  w=%d, h=%d", w, h);
        // FIXME: actually this is a little buggy :-(
        if( w < area.width && h < area.height && (w >= 640 || h >= 480) )
        {
            gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(this->scroll), 
                                            GTK_POLICY_NEVER, 
                                            GTK_POLICY_NEVER );

            gtk_widget_set_size_request( GTK_WIDGET(this->img_view), w, h );
            
            GtkRequisition req;
            gtk_widget_get_preferred_size(GTK_WIDGET(this), NULL, &req);

            if( req.width < 640 )   
                req.width = 640;
            
            if( req.height < 480 )   
                req.height = 480;

            gtk_window_resize( GTK_WINDOW(this), req.width, req.height );
            gtk_widget_set_size_request( GTK_WIDGET(this->img_view), -1, -1 );
            gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(this->scroll), 
                                            GTK_POLICY_AUTOMATIC, 
                                            GTK_POLICY_AUTOMATIC );

            this->zoom_mode = ZOOM_ORIG;
            this->scale = 1.0;
        }
        else
        {
            this->zoom_mode = ZOOM_FIT;
        }
    }

    if( this->zoom_mode == ZOOM_FIT )
    {
        main_win_fit_window_size( this, FALSE, GDK_INTERP_BILINEAR );
    }
    else  if( this->zoom_mode == ZOOM_SCALE )  // scale
    {
        main_win_scale_image( this, this->scale, GDK_INTERP_BILINEAR );
    }
    else  if( this->zoom_mode == ZOOM_ORIG )  // original size
    {
        lx_image_view_set_scale( LX_IMAGE_VIEW(this->img_view), this->scale, GDK_INTERP_BILINEAR );
        main_win_center_image( this );
    }

    lx_image_view_set_pixbuf( LX_IMAGE_VIEW(this->img_view), pix );
}

void main_win_start_slideshow( MainWin* mw )
{
    on_slideshow_menu(NULL, mw);
}

void on_size_allocate( GtkWidget* widget, GtkAllocation    *allocation )
{
    GTK_WIDGET_CLASS(main_win_parent_class)->size_allocate( widget, allocation );

    if(gtk_widget_get_realized (widget) )
    {
        MainWin* mw = (MainWin*)widget;

        if( mw->zoom_mode == ZOOM_FIT )
        {
            while(gtk_events_pending ())
                gtk_main_iteration(); // makes it more fluid

            main_win_fit_window_size( mw, FALSE, GDK_INTERP_BILINEAR );
        }
    }
}

gboolean on_win_state_event( GtkWidget* widget, GdkEventWindowState* state )
{
    MainWin* mw = (MainWin*)widget;
    GtkStyleContext* context = gtk_widget_get_style_context(mw->evt_box);
    if( (state->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0 )
    {
        gtk_style_context_remove_class(context, "ImageArea");
        gtk_style_context_add_class(context, "ImageAreaFullScreen");

        //gtk_widget_modify_bg( mw->evt_box, GTK_STATE_NORMAL, &pref.bg_full );

        gtk_widget_hide( gtk_widget_get_parent(mw->nav_bar) );
        mw->full_screen = TRUE;
    }
    else
    {
        gtk_style_context_remove_class(context, "ImageArea");
        gtk_style_context_add_class(context, "ImageArea");

        //gtk_widget_modify_bg( mw->evt_box, GTK_STATE_NORMAL, &pref.bg );
        if (pref.show_toolbar)
            gtk_widget_show( gtk_widget_get_parent(mw->nav_bar) );
        mw->full_screen = FALSE;
    }

    int previous = pref.open_maximized;
    pref.open_maximized = (state->new_window_state == GDK_WINDOW_STATE_MAXIMIZED);
    if (previous != pref.open_maximized)
        save_preferences();
    return TRUE;
}

void main_win_fit_size( MainWin* mw, int width, int height, gboolean can_strech, GdkInterpType type )
{
    ViewModelsImageItem* item = view_models_main_win_vm_get_current_item(mw->view_model);

    if( ! item )
        return;

    GdkPixbuf* pix = view_models_image_item_get_pixbuf(item);

    gint orig_w = gdk_pixbuf_get_width( pix );
    gint orig_h = gdk_pixbuf_get_height( pix );

    if( can_strech || (orig_w > width || orig_h > height) )
    {
        gdouble xscale = ((gdouble)width) / orig_w;
        gdouble yscale = ((gdouble)height)/ orig_h;
        gdouble final_scale = xscale < yscale ? xscale : yscale;

        main_win_scale_image( mw, final_scale, type );
    }
    else    // use original size if the image is smaller than the window
    {
        mw->scale = 1.0;
        lx_image_view_set_scale( (LxImageView*)mw->img_view, 1.0, type );

        update_title(NULL, mw);
    }
}

void main_win_fit_window_size(  MainWin* mw, gboolean can_strech, GdkInterpType type )
{
    mw->zoom_mode = ZOOM_FIT;

    ViewModelsImageItem* item = view_models_main_win_vm_get_current_item(mw->view_model);

    if( ! item )
        return;

    GdkPixbuf* pix = view_models_image_item_get_pixbuf(item);

    if( pix == NULL )
        return;

    main_win_fit_size( mw, mw->scroll_allocation.width, mw->scroll_allocation.height, can_strech, type );
}

static void
add_nav_btn_img(MainWin* mw, GtkButton* btn, GCallback cb, gboolean toggle )
{
    if( G_UNLIKELY(toggle) )
        g_signal_connect( btn, "toggled", cb, mw );
    else
        g_signal_connect( btn, "clicked", cb, mw );
}

void on_scroll_size_allocate(GtkWidget* widget, GtkAllocation* allocation, MainWin* mv)
{
    mv->scroll_allocation = *allocation;
}

void on_zoom_fit_menu( GtkMenuItem* item, MainWin* mw )
{
    gtk_button_clicked( (GtkButton*)mw->btn_fit );
}

void on_zoom_fit( GtkToggleButton* btn, MainWin* mw )
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)))
        main_win_set_zoom_mode(mw, ZOOM_FIT);

    main_win_update_zoom_buttons_state(mw);
}

void on_full_screen( GtkWidget* btn, MainWin* mw )
{
    if( ! mw->full_screen )
        gtk_window_fullscreen( (GtkWindow*)mw );
    else
        gtk_window_unfullscreen( (GtkWindow*)mw );
}

void on_orig_size_menu( GtkToggleButton* btn, MainWin* mw )
{
    gtk_button_clicked( (GtkButton*)mw->btn_orig );
}

void on_orig_size( GtkToggleButton* btn, MainWin* mw )
{
    // this callback could be called from activate signal of menu item.
    if( GTK_IS_MENU_ITEM(btn) )
    {
        gtk_button_clicked( (GtkButton*)mw->btn_orig );
        return;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)))
        main_win_set_zoom_mode(mw, ZOOM_ORIG);

    main_win_update_zoom_buttons_state(mw);
}

void on_prev( GtkWidget* btn, MainWin* mw )
{
    view_models_main_win_vm_prev(mw->view_model);
}

void on_next( GtkWidget* btn, MainWin* mw )
{
    view_models_main_win_vm_next(mw->view_model);
}

void cancel_slideshow(MainWin* mw)
{
    mw->slideshow_cancelled = TRUE;
    mw->slideshow_running = FALSE;
    if (mw->slide_timeout != 0)
        g_source_remove(mw->slide_timeout);
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(mw->btn_play_stop), FALSE );
}

gboolean next_slide(MainWin* mw)
{
    /* Timeout causes an implicit "Next". */
    if (mw->slideshow_running)
        view_models_main_win_vm_next(mw->view_model);

    return mw->slideshow_running;
}

void on_slideshow_menu( GtkMenuItem* item, MainWin* mw )
{
    gtk_button_clicked( (GtkButton*)mw->btn_play_stop );
}

void on_slideshow( GtkToggleButton* btn, MainWin* mw )
{
    if ((mw->slideshow_running) || (mw->slideshow_cancelled))
    {
        mw->slideshow_running = FALSE;
        mw->slideshow_cancelled = FALSE;
        gtk_image_set_from_icon_name( GTK_IMAGE(mw->img_play_stop), "media-playback-start", GTK_ICON_SIZE_SMALL_TOOLBAR );
        gtk_widget_set_tooltip_text( GTK_WIDGET(btn), _("Start Slideshow") );
        gtk_toggle_button_set_active( btn, FALSE );
    }
    else
    {
        gtk_toggle_button_set_active( btn, TRUE );
        mw->slideshow_running = TRUE;
        gtk_image_set_from_icon_name( GTK_IMAGE(mw->img_play_stop), "media-playback-stop", GTK_ICON_SIZE_SMALL_TOOLBAR );
        gtk_widget_set_tooltip_text( GTK_WIDGET(btn), _("Stop Slideshow") );
        mw->slide_timeout = g_timeout_add(1000 * pref.slide_delay, (GSourceFunc) next_slide, mw);
    }
}

//////////////////// rotate & flip

void on_rotate_clockwise( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);

    view_models_main_win_vm_rotate_item(mw->view_model, 90);
    
    rotate_image( mw );
}

void on_rotate_counterclockwise( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);

    view_models_main_win_vm_rotate_item(mw->view_model, 270);
    rotate_image( mw );
}

void on_flip_vertical( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    
    view_models_main_win_vm_rotate_item(mw->view_model, -180);
    rotate_image( mw );
}

void on_flip_horizontal( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);

    view_models_main_win_vm_rotate_item(mw->view_model, -90);
    rotate_image( mw );
}

/* end of rotate & flip */

void on_save_as( GtkWidget* btn, MainWin* mw )
{
    GError* error = NULL;
    cancel_slideshow(mw);

    view_models_main_win_vm_save_as(mw->view_model, &error);

    if (error != NULL)
    {
        dialog_service_show_message (error->message);
        g_error_free(error);
    }
}

void on_save( GtkWidget* btn, MainWin* mw )
{
    GError* error = NULL;
    cancel_slideshow(mw);

    view_models_main_win_vm_save(mw->view_model, &error);

    if (error != NULL)
    {
        dialog_service_show_message (error->message);
        g_error_free(error);
    }
}

void on_open( GtkWidget* btn, MainWin* mw )
{
    GError* error = NULL;
    cancel_slideshow(mw);
 
    view_models_main_win_vm_open(mw->view_model, &error);

    if(error != NULL)
    {
        dialog_service_show_message (error->message);
        g_error_free(error);
        return;
    }

    main_win_update_zoom_buttons_state(mw);
}

void on_zoom_in( GtkWidget* btn, MainWin* mw )
{
    double scale = mw->scale;
    scale *= 1.05;
    main_win_set_zoom_scale(mw, scale);
}

void on_zoom_out( GtkWidget* btn, MainWin* mw )
{
    double scale = mw->scale;
    scale /= 1.05;
    main_win_set_zoom_scale(mw, scale);
}

void on_preference( GtkWidget* btn, MainWin* mw )
{
    edit_preferences( (GtkWindow*)mw );
}

void on_quit( GtkWidget* btn, MainWin* mw )
{
    cancel_slideshow(mw);
    gtk_widget_destroy( (GtkWidget*)mw );
}

gboolean on_button_press( GtkWidget* widget, GdkEventButton* evt, MainWin* mw )
{
    if( ! gtk_widget_has_focus( widget ) )
        gtk_widget_grab_focus( widget );

    if( evt->type == GDK_BUTTON_PRESS)
    {
        if( evt->button == 1 )    // left button
        {
            ViewModelsImageItem* item = view_models_main_win_vm_get_current_item(mw->view_model);

            if( ! item )
                return FALSE;

            mw->dragging = TRUE;
            gdk_window_get_device_position ( gtk_widget_get_window(GTK_WIDGET(mw)), 
                                             evt->device,
                                             &mw->drag_old_x,
                                             &mw->drag_old_y,
                                             NULL );
            gdk_window_set_cursor( gtk_widget_get_window(widget), mw->hand_cursor );
        }
        else if( evt->button == 3 )   // right button
        {
            show_popup_menu( mw, evt );
        }
    }
    else if( evt->type == GDK_2BUTTON_PRESS && evt->button == 1 )    // double clicked
    {
         on_full_screen( NULL, mw );
    }

    return FALSE;
}

gboolean on_mouse_move( GtkWidget* widget, GdkEventMotion* evt, MainWin* mw )
{
    if( ! mw->dragging )
        return FALSE;

    int cur_x, cur_y;
    gdk_window_get_device_position (  gtk_widget_get_window(GTK_WIDGET(mw)),
                                      evt->device, 
                                      &cur_x,
                                      &cur_y,
                                      NULL );

    int dx = (mw->drag_old_x - cur_x);
    int dy = (mw->drag_old_y - cur_y);

    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)mw->scroll);
    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)mw->scroll);

    GtkRequisition req;
    gtk_widget_get_preferred_size( (GtkWidget*)mw->img_view, NULL, &req );

    gdouble hadj_page_size = gtk_adjustment_get_page_size(hadj);
    gdouble hadj_lower = gtk_adjustment_get_lower(hadj);
    gdouble hadj_upper = gtk_adjustment_get_upper(hadj);

    if( ABS(dx) > 4 )
    {
        mw->drag_old_x = cur_x;
        if( req.width > hadj_page_size )
        {
            gdouble value = gtk_adjustment_get_value (hadj);
            gdouble x = value + dx;
            if( x < hadj_lower )
                x = hadj_lower;
            else if( (x + hadj_page_size) > hadj_upper )
                x = hadj_upper - hadj_page_size;

            if( x != value )
                gtk_adjustment_set_value (hadj, x );
        }
    }

    gdouble vadj_page_size = gtk_adjustment_get_page_size(vadj);
    gdouble vadj_lower = gtk_adjustment_get_lower(vadj);
    gdouble vadj_upper = gtk_adjustment_get_upper(vadj);

    if( ABS(dy) > 4 )
    {
        if( req.height > vadj_page_size )
        {
            mw->drag_old_y = cur_y;
            gdouble value = gtk_adjustment_get_value (vadj);
            gdouble y = value + dy;
            if( y < vadj_lower )
                y = vadj_lower;
            else if( (y + vadj_page_size) > vadj_upper )
                y = vadj_upper - vadj_page_size;

            if( y != value )
                gtk_adjustment_set_value (vadj, y );
        }
    }
    return FALSE;
}

gboolean on_button_release( GtkWidget* widget, GdkEventButton* evt, MainWin* mw )
{
    mw->dragging = FALSE;
    gdk_window_set_cursor( gtk_widget_get_window(widget), NULL );
    return FALSE;
}

gboolean on_scroll_event( GtkWidget* widget, GdkEventScroll* evt, MainWin* mw )
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    switch( evt->direction )
    {
    case GDK_SCROLL_UP:
        if ((evt->state & modifiers) == GDK_CONTROL_MASK)
            on_zoom_in( NULL, mw );
        else
            on_prev( NULL, mw );
        break;
    case GDK_SCROLL_DOWN:
        if ((evt->state & modifiers) == GDK_CONTROL_MASK)
            on_zoom_out( NULL, mw );
        else
            on_next( NULL, mw );
        break;
    case GDK_SCROLL_LEFT:
        if( gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL )
            on_next( NULL, mw );
        else
            on_prev( NULL, mw );
        break;
    case GDK_SCROLL_RIGHT:
        if( gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL )
            on_prev( NULL, mw );
        else
            on_next( NULL, mw );
        break;
    }
    return TRUE;
}

static gboolean 
on_key_press_event(GtkWidget* widget, 
                   GdkEventKey * key,
                   gpointer user_data)
{
    MainWin* mw = MAIN_WIN(user_data);

    switch( key->keyval )
    {
        case GDK_Right:
        case GDK_KP_Right:
        case GDK_rightarrow:
            if( gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL )
                on_prev( NULL, mw );
            else
                on_next( NULL, mw );

            break;
        case GDK_Return:
        case GDK_space:
        case GDK_Next:
        case GDK_KP_Down:
        case GDK_Down:
        case GDK_downarrow:
            on_next( NULL, mw );
            break;
        case GDK_Left:
        case GDK_KP_Left:
        case GDK_leftarrow:
            if( gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL )
                on_next( NULL, mw );
            else
                on_prev( NULL, mw );
            break;
        case GDK_Prior:
        case GDK_BackSpace:
        case GDK_KP_Up:
        case GDK_Up:
        case GDK_uparrow:
            on_prev( NULL, mw );
            break;
        case GDK_w:
        case GDK_W:
            on_slideshow_menu( NULL, mw );
            break;
        case GDK_KP_Add:
        case GDK_plus:
        case GDK_equal:
            on_zoom_in( NULL, mw );
            break;
        case GDK_KP_Subtract:
        case GDK_minus:
            on_zoom_out( NULL, mw );
            break;
        case GDK_s:
        case GDK_S:
            on_save( NULL, mw );
            break;
        case GDK_a:
        case GDK_A:
            on_save_as( NULL, mw );
            break;
        case GDK_l:
        case GDK_L:
            on_rotate_counterclockwise( NULL, mw );
            break;
        case GDK_r:
        case GDK_R:
            on_rotate_clockwise( NULL, mw );
            break;
        case GDK_f:
        case GDK_F:
            if( mw->zoom_mode != ZOOM_FIT )
                gtk_button_clicked((GtkButton*)mw->btn_fit );
            break;
        case GDK_g:
        case GDK_G:
            if( mw->zoom_mode != ZOOM_ORIG )
                gtk_button_clicked((GtkButton*)mw->btn_orig );
            break;
        case GDK_h:
        case GDK_H:
            on_flip_horizontal( NULL, mw );
            break;
        case GDK_v:
        case GDK_V:
            on_flip_vertical( NULL, mw );
            break;
        case GDK_o:
        case GDK_O:
            on_open( NULL, mw );
            break;
        case GDK_Delete:
        case GDK_d:
        case GDK_D:
            on_delete( NULL, mw );
            break;
        case GDK_p:
        case GDK_P:
            on_preference( NULL, mw );
	    break;
        case GDK_t:
        case GDK_T:
            on_toggle_toolbar( NULL, mw );
	    break;
        case GDK_Escape:
            if( mw->full_screen )
                on_full_screen( NULL, mw );
            else
                on_quit( NULL, mw );
            break;
        case GDK_q:
	    case GDK_Q:
            on_quit( NULL, mw );
            break;
        case GDK_F11:
            on_full_screen( NULL, mw );
            break;
    }

    return FALSE;
}

void main_win_center_image( MainWin* mw )
{
    GtkAdjustment *hadj, *vadj;
    hadj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)mw->scroll);
    vadj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)mw->scroll);

    GtkRequisition req;
    gtk_widget_get_preferred_size( (GtkWidget*)mw->img_view, NULL, &req );

    gdouble hadj_page_size = gtk_adjustment_get_page_size(hadj);
    gdouble hadj_upper = gtk_adjustment_get_upper(hadj);

    if( req.width > hadj_page_size )
        gtk_adjustment_set_value(hadj, ( hadj_upper - hadj_page_size ) / 2 );

    gdouble vadj_page_size = gtk_adjustment_get_page_size(vadj);
    gdouble vadj_upper = gtk_adjustment_get_upper(vadj);

    if( req.height > vadj_page_size )
        gtk_adjustment_set_value(vadj, ( vadj_upper - vadj_page_size ) / 2 );
}

//TODO: handler to react to change of item
void rotate_image( MainWin* mw )
{
    ViewModelsImageItem* item = view_models_main_win_vm_get_current_item(mw->view_model);

    if(!item)
        return;

    GdkPixbuf* pix = view_models_image_item_get_pixbuf(item);

    lx_image_view_set_pixbuf( (LxImageView*)mw->img_view, pix );

    if( mw->zoom_mode == ZOOM_FIT )
        main_win_fit_window_size( mw, FALSE, GDK_INTERP_BILINEAR );
}

gboolean main_win_scale_image( MainWin* mw, double new_scale, GdkInterpType type )
{
    if( G_UNLIKELY( new_scale == 1.0 ) )
    {
        gtk_toggle_button_set_active( (GtkToggleButton*)mw->btn_orig, TRUE );
        mw->scale = 1.0;
        return TRUE;
    }
    mw->scale = new_scale;
    lx_image_view_set_scale( (LxImageView*)mw->img_view, new_scale, type );

    update_title( NULL, mw );

    return TRUE;
}

gboolean main_win_save( MainWin* mw, const char* file_path, const char* type, gboolean confirm )
{
    GError* error;
    gboolean result = view_models_main_win_vm_save(mw->view_model, &error);

    if( ! result )
    {
        dialog_service_show_message( error->message );
        g_error_free(error);
        return FALSE;
    }
}

void on_delete( GtkWidget* btn, MainWin* mw )
{
    GError* error = NULL;
    cancel_slideshow(mw);

    if(!view_models_main_win_vm_delete(mw->view_model, &error))
    {
        dialog_service_show_message( error->message );
        g_error_free(error);
    }
}

void on_toggle_toolbar( GtkMenuItem* item, MainWin* mw )
{
    pref.show_toolbar = !pref.show_toolbar;

    if (pref.show_toolbar)
        gtk_widget_show(gtk_widget_get_parent(mw->nav_bar));
    else
        gtk_widget_hide(gtk_widget_get_parent(mw->nav_bar));

    save_preferences();
}


void show_popup_menu( MainWin* mw, GdkEventButton* evt )
{
    static PtkMenuItemEntry menu_def[] =
    {
        PTK_IMG_MENU_ITEM( N_( "Previous" ), "go-previous", on_prev, GDK_leftarrow, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Next" ), "go-next", on_next, GDK_rightarrow, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Start/Stop Slideshow" ), "media-playback-start", on_slideshow_menu, GDK_W, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_( "Zoom Out" ), "zoom-out", on_zoom_out, GDK_minus, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Zoom In" ), "zoom-in", on_zoom_in, GDK_plus, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Fit Image To Window Size" ), "zoom-fit-best", on_zoom_fit_menu, GDK_F, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Original Size" ), "zoom-original", on_orig_size_menu, GDK_G, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_( "Full Screen" ), "view-fullscreen", on_full_screen, GDK_F11, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_( "Rotate Counterclockwise" ), "object-rotate-left", on_rotate_counterclockwise, GDK_L, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Rotate Clockwise" ), "object-rotate-right", on_rotate_clockwise, GDK_R, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Flip Horizontal" ), "object-flip-horizontal", on_flip_horizontal, GDK_H, 0 ),
        PTK_IMG_MENU_ITEM( N_( "Flip Vertical" ), "object-flip-vertical", on_flip_vertical, GDK_V, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_("Open File"), "document-open", G_CALLBACK(on_open), GDK_O, 0 ),
        PTK_IMG_MENU_ITEM( N_("Save File"), "document-save", G_CALLBACK(on_save), GDK_S, 0 ),
        PTK_IMG_MENU_ITEM( N_("Save As"), "document-save-as", G_CALLBACK(on_save_as), GDK_A, 0 ),
//        PTK_IMG_MENU_ITEM( N_("Save As Other Size"), GTK_STOCK_SAVE_AS, G_CALLBACK(on_save_as), GDK_A, 0 ),
        PTK_IMG_MENU_ITEM( N_("Delete File"), "edit-delete", G_CALLBACK(on_delete), GDK_Delete, 0 ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_("Preferences"), "preferences-system", G_CALLBACK(on_preference), GDK_P, 0 ),
        PTK_IMG_MENU_ITEM( N_("Show/Hide toolbar"), NULL, G_CALLBACK(on_toggle_toolbar), GDK_T, 0 ),
        PTK_STOCK_MENU_ITEM( "help-about", on_about ),
        PTK_SEPARATOR_MENU_ITEM,
        PTK_IMG_MENU_ITEM( N_("Quit"), "application-exit", G_CALLBACK(on_quit), GDK_Q, 0 ),
        PTK_MENU_END
    };
    GtkWidget* rotate_cw;
    GtkWidget* rotate_ccw;
    GtkWidget* flip_v;
    GtkWidget* flip_h;

    menu_def[10].ret = &rotate_ccw;
    menu_def[11].ret = &rotate_cw;
    menu_def[12].ret = &flip_h;
    menu_def[13].ret = &flip_v;

    // mw accel group is useless. It's only used to display accels in popup menu
    GtkAccelGroup* accel_group = gtk_accel_group_new();
    GtkMenuShell* popup = (GtkMenuShell*)ptk_menu_new_from_data( menu_def, mw, accel_group );

    gtk_widget_show_all( (GtkWidget*)popup );
    g_signal_connect( popup, "selection-done", G_CALLBACK(gtk_widget_destroy), NULL );

    gtk_menu_popup_at_pointer( (GtkMenu*)popup, (GdkEvent*)evt );
}

void on_about( GtkWidget* menu, MainWin* mw )
{
    GtkWidget * about_dlg;
    const gchar *authors[] =
    {
        "洪任諭 Hong Jen Yee <pcman.tw@gmail.com>",
        "Martin Siggel <martinsiggel@googlemail.com>",
        "Hialan Liu <hialan.liu@gmail.com>",
        "Marty Jack <martyj19@comcast.net>",
        "Louis Casillas <oxaric@gmail.com>",
        "Will Davies",
        "Vladyslav Stovmanenko <flaviusglamfenix@gmail.com>",
        _(" * Refer to source code of EOG image viewer and GThumb"),
        NULL
    };
    /* TRANSLATORS: Replace this string with your names, one name per line. */
    gchar *translators = _( "translator-credits" );

    about_dlg = gtk_about_dialog_new ();

    gtk_container_set_border_width ( ( GtkContainer*)about_dlg , 2 );
    gtk_about_dialog_set_version ( (GtkAboutDialog*)about_dlg, VERSION );
    gtk_about_dialog_set_program_name ( (GtkAboutDialog*)about_dlg, _( "GPicView" ) );
    gtk_about_dialog_set_logo_icon_name ( (GtkAboutDialog*)about_dlg, "gpicview" );
    gtk_about_dialog_set_copyright ( (GtkAboutDialog*)about_dlg, _( "Copyright (C) 2007 - 2011" ) );
    gtk_about_dialog_set_comments ( (GtkAboutDialog*)about_dlg, _( "Lightweight image viewer from LXDE project" ) );
    gtk_about_dialog_set_license ( (GtkAboutDialog*)about_dlg, "GPicView\n\nCopyright (C) 2007 Hong Jen Yee (PCMan)\n\nThis program is free software; you can redistribute it and/or\nmodify it under the terms of the GNU General Public License\nas published by the Free Software Foundation; either version 2\nof the License, or (at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program; if not, write to the Free Software\nFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA." );
    gtk_about_dialog_set_website ( (GtkAboutDialog*)about_dlg, "http://wiki.lxde.org/en/GPicView" );
    gtk_about_dialog_set_authors ( (GtkAboutDialog*)about_dlg, authors );
    gtk_about_dialog_set_translator_credits ( (GtkAboutDialog*)about_dlg, translators );
    gtk_window_set_transient_for( (GtkWindow*) about_dlg, GTK_WINDOW( mw ) );

    gtk_dialog_run( ( GtkDialog*)about_dlg );
    gtk_widget_destroy( about_dlg );
}

void on_drag_data_received( GtkWidget* widget, GdkDragContext *drag_context,
                int x, int y, GtkSelectionData* data, guint info, guint time, MainWin* mw )
{
    if( !data)
        return;

    int data_length = gtk_selection_data_get_length(data);

    if( data_length <= 0)
        return;

    char* file = NULL;
    if( info == 0 )    // text/uri-list
    {
        char** uris = gtk_selection_data_get_uris( data );
        if( uris )
        {
            file = g_filename_from_uri(*uris, NULL, NULL);
            g_strfreev( uris );
        }
    }
    else if( info == 1 )    // text/plain
    {
        file = (char*)gtk_selection_data_get_text( data );
    }
    if( file )
    {
        GError* error = NULL;
        view_models_main_win_vm_open(mw->view_model, &error);

        if(error != NULL)
        {
            dialog_service_show_message(error->message);
            g_error_free(error);
            return;
        }

        mw->zoom_mode = ZOOM_NONE;

        g_free( file );
    }
}

static void main_win_set_zoom_scale(MainWin* mw, double scale)
{
    main_win_set_zoom_mode(mw, ZOOM_SCALE);

    if (scale > 20.0)
        scale = 20.0;
    if (scale < 0.02)
        scale = 0.02;

    if (mw->scale != scale)
        main_win_scale_image(mw, scale, GDK_INTERP_BILINEAR);
}

static void main_win_set_zoom_mode(MainWin* mw, ZoomMode mode)
{
    if (mw->zoom_mode == mode)
       return;

    mw->zoom_mode = mode;

    main_win_update_zoom_buttons_state(mw);

    if (mode == ZOOM_ORIG)
    {
        mw->scale = 1.0;
        
        ViewModelsImageItem* item = view_models_main_win_vm_get_current_item(mw->view_model);

        if( ! item )
            return;

        update_title(NULL, mw);

        lx_image_view_set_scale( (LxImageView*)mw->img_view, 1.0, GDK_INTERP_BILINEAR );

        while (gtk_events_pending ())
            gtk_main_iteration ();

        main_win_center_image( mw ); // FIXME:  mw doesn't work well. Why?
    }
    else if (mode == ZOOM_FIT)
    {
        main_win_fit_window_size( mw, FALSE, GDK_INTERP_BILINEAR );
    }
}

static void main_win_update_zoom_buttons_state(MainWin* mw)
{
    gboolean button_fit_active = mw->zoom_mode == ZOOM_FIT;
    gboolean button_orig_active = mw->zoom_mode == ZOOM_ORIG;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mw->btn_fit)) != button_fit_active)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mw->btn_fit), button_fit_active);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mw->btn_orig)) != button_orig_active)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mw->btn_orig), button_orig_active);
}

void main_win_set_dynamic_style (MainWin* mw, GtkStyleProvider* provider)
{
    GtkStyleContext* context = gtk_widget_get_style_context(GTK_WIDGET(mw));

    if(G_LIKELY(mw->dynamic_style))
    {
        gtk_style_context_remove_provider(context, mw->dynamic_style);
        g_object_unref(mw->dynamic_style);
    }

    mw->dynamic_style = provider;
    
    if(G_LIKELY(provider))
    {
        gtk_style_context_add_provider(context, provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
}

static void main_win_set_static_styles(MainWin* this)
{
    GdkDisplay *display;
    GdkScreen *screen;
    GError* err = NULL;

    GtkStyleProvider* provider = GTK_STYLE_PROVIDER(gtk_css_provider_new ());
    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display);
    gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gboolean val = gtk_css_provider_load_from_path(GTK_CSS_PROVIDER(provider), PACKAGE_DATA_DIR "/gpicview/ui/styles.css", &err);
    g_object_unref (provider);

    if(G_LIKELY(err))
    {
        g_printerr("Cannot load application css: %d.\n%s\n", err->code, err->message);
        return;
    }
}

