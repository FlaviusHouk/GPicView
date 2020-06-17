#include "image-item.h"

// defined in exif.c
extern int ExifRotateFlipMapping[9][9];

static int trans_angle_to_id(int i)
{
    if(i == 0) 		return 1;
    else if(i == 90)	return 6;
    else if(i == 180)	return 3;
    else if(i == 270)	return 8;
    else if(i == -45)	return 7;
    else if(i == -90)	return 2;
    else if(i == -135)	return 5;
    else     /* -180 */ return 4;
}

static int get_new_angle( int orig_angle, int rotate_angle )
{
    // defined in exif.c
    static int angle_trans_back[] = {0, 0, -90, 180, -180, -135, 90, -45, 270};

    orig_angle = trans_angle_to_id(orig_angle);
    rotate_angle = trans_angle_to_id(rotate_angle);

    return angle_trans_back[ ExifRotateFlipMapping[orig_angle][rotate_angle] ];
}

typedef struct _ViewModelsImageItem
{
    GObject parent;

    gchar* file_name;
    gint rotation_angle;
    gboolean is_modified;

    GdkPixbuf* pix;
    GdkPixbufAnimation* animation;
    GdkPixbufAnimationIter* animation_iter;
    guint animation_timeout;
};

#define CLASS_PREFIX(name) view_models_image_item_##name

static guint animation_timeout_signal_id = 0;

G_DEFINE_TYPE(ViewModelsImageItem, view_models_image_item, G_TYPE_OBJECT);

static void
CLASS_PREFIX(animation_timeout)(ViewModelsImageItem* this)
{
    if ( gdk_pixbuf_animation_iter_advance( this->animation_iter, NULL ) )
    {
        this->pix = gdk_pixbuf_animation_iter_get_pixbuf( this->animation_iter );

        g_signal_emit(this, 
                      animation_timeout_signal_id, 
                      g_quark_from_static_string(ANIMATION_TIMEOUT_SIGNAL_NAME),
                      NULL);
    }

    gint delay = gdk_pixbuf_animation_iter_get_delay_time( this->animation_iter );
    this->animation_timeout = g_timeout_add(delay, G_SOURCE_FUNC(view_models_image_item_animation_timeout), this );    
}

static void
CLASS_PREFIX(dispose)(GObject* obj)
{
    ViewModelsImageItem* this = VIEW_MODELS_IMAGE_ITEM(obj);

    if( this->animation )
    {
        g_object_unref( this->animation );
        if( this->animation_timeout );
        {
            g_source_remove( this->animation_timeout );
            this->animation_timeout = 0;
        }
    }
    else if( this->pix )
    {
        g_object_unref( this->pix );
    }

    G_OBJECT_CLASS(view_models_image_item_parent_class)->dispose(obj);
}

static void
CLASS_PREFIX(class_init)(ViewModelsImageItemClass* class)
{
    //TODO: Save in class object
    //TODO: use
    animation_timeout_signal_id = g_signal_new(ANIMATION_TIMEOUT_SIGNAL_NAME,
                                               VIEW_MODELS_TYPE_IMAGE_ITEM,
                                               G_SIGNAL_RUN_FIRST,
                                               0,
                                               NULL,
                                               NULL,
                                               NULL,
                                               G_TYPE_NONE,
                                               0);

    GObjectClass* g_object_class = G_OBJECT_CLASS(class);

    g_object_class->dispose = view_models_image_item_dispose;
}

static void
CLASS_PREFIX(init)(ViewModelsImageItem* this)
{
    this->rotation_angle = 0;
    this->pix = NULL;
}

gint
CLASS_PREFIX(get_rotation)(ViewModelsImageItem* this)
{
    return this->rotation_angle;
}

void
CLASS_PREFIX(set_rotation)(ViewModelsImageItem* this, gint value)
{
    this->rotation_angle = get_new_angle(this->rotation_angle, value);
}

gboolean
CLASS_PREFIX(is_animated)(ViewModelsImageItem* this)
{
    return this->animation != NULL && !gdk_pixbuf_animation_is_static_image( this->animation );
}

gboolean
CLASS_PREFIX(is_modified)(ViewModelsImageItem* this)
{
    return this->is_modified;
}

void
CLASS_PREFIX(set_is_modified)(ViewModelsImageItem *this, gboolean value)
{
    if(!value)
        view_models_image_item_set_rotation(this, 0);

    this->is_modified = value;
}

ViewModelsImageItem*
CLASS_PREFIX(new_from_file)(const gchar* file_name)
{
    GError* error = NULL;
    ViewModelsImageItem* this = VIEW_MODELS_IMAGE_ITEM(g_object_new(VIEW_MODELS_TYPE_IMAGE_ITEM, NULL));

    //rework
    this->file_name = g_strdup(file_name);

    this->animation = gdk_pixbuf_animation_new_from_file( file_name, &error );
    if( ! this->animation )
    {     
        return NULL;
    }

    /* tests if the file is actually just a normal picture */
    if ( gdk_pixbuf_animation_is_static_image( this->animation ) )
    {
       this->pix = gdk_pixbuf_animation_get_static_image( this->animation );
       g_object_ref(this->pix);
       g_object_unref(this->animation);
       this->animation = NULL;
    }
    else
    {
        int delay;
        /* we found an animation */
        this->animation_iter = gdk_pixbuf_animation_get_iter( this->animation, NULL );
        this->pix = gdk_pixbuf_animation_iter_get_pixbuf( this->animation_iter );
        delay = gdk_pixbuf_animation_iter_get_delay_time( this->animation_iter );
        
        this->animation_timeout = g_timeout_add( delay, 
                                                 G_SOURCE_FUNC(view_models_image_item_animation_timeout), 
                                                 this );
    }

    GdkPixbufFormat* info;
    info = gdk_pixbuf_get_file_info( file_name, NULL, NULL );
    gchar* type = ((info != NULL) ? gdk_pixbuf_format_get_name(info) : "");

    if(!strcmp(type,"jpeg"))
    {
        GdkPixbuf* tmp;
        // Only jpeg should rotate by EXIF
        tmp = gdk_pixbuf_apply_embedded_orientation(this->pix);
        g_object_unref(this->pix);
        this->pix = tmp;
    }

    return this;
}

GdkPixbuf*
CLASS_PREFIX(get_pixbuf)(ViewModelsImageItem* this)
{
    return this->pix;
}

gchar*
CLASS_PREFIX(get_name)(ViewModelsImageItem* this)
{
    return this->file_name;
}

void
CLASS_PREFIX(rotate)(ViewModelsImageItem* this, gint angle)
{
    GdkPixbuf* rpix = NULL;

    if(angle > 0)
    {
        rpix = gdk_pixbuf_rotate_simple( this->pix, angle );
    }
    else
    {
        //TODO: rework
        if(angle == -90)
            rpix = gdk_pixbuf_flip( this->pix, TRUE );
        else if(angle == -180)
            rpix = gdk_pixbuf_flip( this->pix, FALSE );
    }

    if (!rpix) {
        return;
    }

    g_object_unref( this->pix );

    this->pix = rpix;
}