#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main-win-vm.h"

#include "pref.h"
#include "image-list.h"

#include "dialog_service.h"

//Temp here
#include "Imaging/jpeg-tran.h"

#include <errno.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#define CLASS_PREFIX(name) view_models_main_win_vm_##name

static gboolean
CLASS_PREFIX(open_internal)(ViewModelsMainWinVM* this, gchar* file_path, GError** error);

static gboolean
CLASS_PREFIX(save_internal)(ViewModelsMainWinVM* this, gchar* file_path, gchar* type, GError** error);

#define FILE_NAME_IS_NULL_MSG "File name is NULL. Cannot open."
#define FILE_OVERRIDING_MSG _("The file name you selected already exists.\nDo you want to overwrite existing file?\n(Warning: The quality of original image might be lost)")
#define FILE_DELETE_MSG _("Are you sure you want to delete current file?\n\nWarning: Once deleted, the file cannot be recovered.")

static GParamSpec *obj_properties[N_PROPS] = { NULL, };

typedef struct _ViewModelsMainWinVM
{
    GObject parent;

    ViewModelsImageItem* current_item;

    ImageList* image_list;
};

G_DEFINE_TYPE(ViewModelsMainWinVM, view_models_main_win_vm, G_TYPE_OBJECT)

static void
CLASS_PREFIX(set_property)(GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    ViewModelsMainWinVM* this = VIEW_MODELS_MAIN_WIN_VM(object);

    switch(property_id)
    {
        
    }
}

static void
CLASS_PREFIX(get_property)(GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    ViewModelsMainWinVM* this = VIEW_MODELS_MAIN_WIN_VM(object);

    switch(property_id)
    {
        case CURRENT_ITEM_PROP:
        {
            g_value_set_pointer(value, this->current_item);
            break;
        }
    }
}

static void
CLASS_PREFIX(class_init)(ViewModelsMainWinVMClass* class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    object_class->set_property = view_models_main_win_vm_set_property;
    object_class->get_property = view_models_main_win_vm_get_property;

    obj_properties[CURRENT_ITEM_PROP] =
        g_param_spec_pointer (CURRENT_ITEM_PROP_NAME,
                              CURRENT_ITEM_PROP_NAME,
                              "Currently shown image.",
                              G_PARAM_READABLE);

    g_object_class_install_properties (object_class,
                                       N_PROPS,
                                       obj_properties);
}

static void
CLASS_PREFIX(init)(ViewModelsMainWinVM* this)
{
    this->current_item = NULL;
}

void
CLASS_PREFIX(open_file_internal)(ViewModelsMainWinVM* this, const gchar* file_name)
{
    if(this->current_item)
        g_object_unref(this->current_item);

    this->current_item = view_models_image_item_new_from_file(file_name);

    g_object_notify(G_OBJECT(this), CURRENT_ITEM_PROP_NAME);    
}

ViewModelsMainWinVM*
CLASS_PREFIX(new)()
{
    ViewModelsMainWinVM* this = VIEW_MODELS_MAIN_WIN_VM(g_object_new(VIEWMODELS_TYPE_MAIN_WIN_VM, NULL));
    
    this->image_list = image_list_new();
    
    return this;
}

ViewModelsImageItem*
CLASS_PREFIX(get_current_item)(ViewModelsMainWinVM* this)
{
    GValue val = G_VALUE_INIT;
    g_value_init(&val, G_TYPE_POINTER);
    g_object_get_property(G_OBJECT(this), CURRENT_ITEM_PROP_NAME, &val);

    return VIEW_MODELS_IMAGE_ITEM(g_value_get_pointer(&val));
}

gboolean
CLASS_PREFIX(open)(ViewModelsMainWinVM* this, GError** error)
{
    GError* innerError = NULL;

    GPtrArray* files = dialog_service_open_file(image_list_get_dir(this->image_list));

    if(files->len != 1)
        return FALSE;

    //TODO: add multiple items.
    gchar* file_path = g_ptr_array_index(files, 0);

    if(file_path == NULL)
    {
        g_set_error(error, 
                    g_quark_from_static_string(FILE_NAME_IS_NULL_MSG), 
                    1, 
                    "%s\n", 
                    FILE_NAME_IS_NULL_MSG);

        return FALSE;
    }

    if(!view_models_main_win_vm_open_file(this, file_path, &innerError))
        g_propagate_error(error, innerError);

    g_free(file_path);

    return innerError != NULL;
}

gboolean
CLASS_PREFIX(save)(ViewModelsMainWinVM* this, GError** error)
{
    char* file_name = g_build_filename( image_list_get_dir( this->image_list ),
                                        image_list_get_current( this->image_list ), NULL );

    GdkPixbufFormat* info;
    info = gdk_pixbuf_get_file_info( file_name, NULL, NULL );
    gchar* type = gdk_pixbuf_format_get_name( info );

    /* Confirm save if requested. */
    if (pref.ask_before_save && g_file_test( file_name, G_FILE_TEST_EXISTS ))
    {
        if(!dialog_service_yes_no_dialog(FILE_OVERRIDING_MSG))
            return FALSE;
    }

    if(strcmp("jpeg", type) == 0 || 
       strcmp("jpg", type) == 0)
    {
        gint rotation = view_models_image_item_get_rotation(this->current_item);

        if(!pref.rotate_exif_only)
        {
            // hialan notes:
            // ExifRotate retrun FALSE when
            //   1. Can not read file
            //   2. Exif do not have TAG_ORIENTATION tag
            //   3. Format unknown
            // And then we apply rotate_and_save_jpeg_lossless() ,
            // the result would not effected by EXIF Orientation...
#ifdef HAVE_LIBJPEG
            gint status = rotate_and_save_jpeg_lossless(file_name, rotation);
	        if(status != 0)
            {
                if(status == -1)
                    g_set_error(error, g_quark_from_string("ExifRotate failed"), -1, "ExifRotate failed");
                else
                    g_set_error(error, g_quark_from_string(g_strerror(errno)), errno, g_strerror(errno));
                
                return FALSE;
            }
#else
            g_set_error(error, g_quark_from_string("Libjpeg required for exif is missing"), -1, "Libjpeg required for exif is missing");
            return FALSE;
#endif
        }
        else
        {
            view_models_main_win_vm_save_internal(this, file_name, type, error);
        } 
    }
    else
    {
        view_models_main_win_vm_save_internal(this, file_name, type, error);
    }

    //TODO: Add image processor
    view_models_image_item_set_is_modified(this->current_item, FALSE);
    g_free( file_name );
    g_free( type );

    return TRUE;
}

gboolean
CLASS_PREFIX(save_as)(ViewModelsMainWinVM* this, GError** error)
{
    GError* inner_error = NULL;
    ViewModelsImageItem* item = view_models_main_win_vm_get_current_item(this);

    if( ! item )
        return FALSE;

    gchar* type;
    gchar* file = dialog_service_save_file ( image_list_get_dir( this->image_list ), 
                                             &type );
    if( file )
    {
        char* dir;
        view_models_main_win_vm_save_internal(this, file, type, &inner_error);

        if(inner_error)
        {
            g_propagate_error(error, inner_error);
            return FALSE;
        }
        
        dir = g_path_get_dirname(file);
        const char* name = file + strlen(dir) + 1;

        if( strcmp( image_list_get_dir(this->image_list), dir ) == 0 )
        {
            /* if the saved file is located in the same dir */
            /* simply add it to image list */
            image_list_add_sorted( this->image_list, name, TRUE );
        }
        else /* otherwise reload the whole image list. */
        {
            /* switch to the dir containing the saved file. */
            image_list_open_dir( this->image_list, dir, NULL );
        }

        view_models_main_win_vm_open_file_internal(this, file);

        g_free( dir );
        g_free( file );
        g_free( type );

        return TRUE;
    }

    return FALSE;
}

gboolean
CLASS_PREFIX(delete)(ViewModelsMainWinVM* this, GError** error)
{
    char* file_path = image_list_get_current_file_path( this->image_list );

    if( file_path )
    {
        gboolean resp = TRUE;
	    if ( pref.ask_before_delete )
            resp = dialog_service_yes_no_dialog(FILE_DELETE_MSG);

	    if( resp )
        {
            const gchar* name = image_list_get_current( this->image_list  );

	        if( g_unlink( file_path ) != 0 )
            {
		        g_set_error(error, g_quark_from_static_string(""), errno, g_strerror(errno) );
                return FALSE;
            }
	        else
	        {
		        const gchar* next_name = image_list_get_next( this->image_list );
		        
                if( ! next_name )
                {
		            next_name = image_list_get_prev( this->image_list );
                }
		        else
		        {
		            gchar* next_file_path = image_list_get_current_file_path( this->image_list  );
		            view_models_main_win_vm_open_file_internal(this, next_file_path);
		            g_free( next_file_path );
		        }

		        image_list_remove ( this->image_list , name );
	        }
        }
    }
}

gboolean
CLASS_PREFIX(open_file)(ViewModelsMainWinVM* this, const gchar* file_path, GError** error)
{
    GError* innerError = NULL;
    ImageList* list = this->image_list;

    if (g_file_test(file_path, G_FILE_TEST_IS_DIR))
    {
        image_list_open_dir( list, file_path, NULL );
        image_list_sort_by_name( list, GTK_SORT_DESCENDING );
        if (image_list_get_first(list))
        {
            gboolean return_value = view_models_main_win_vm_open_file(this, image_list_get_current_file_path(list), &innerError);

            if(innerError != NULL)
                g_propagate_error(error, innerError);

            return return_value;
        }
        
        return FALSE;
    }

    gchar* dir_path = g_path_get_dirname( file_path );
    image_list_open_dir( list, dir_path, NULL );
    image_list_sort_by_name( list, GTK_SORT_DESCENDING );
    g_free( dir_path );

    gchar* base_name = g_path_get_basename( file_path );
    image_list_set_current( list, base_name );
    g_free( base_name );

    view_models_main_win_vm_open_file_internal(this, file_path);

    return TRUE;
}

static gboolean
CLASS_PREFIX(save_internal)(ViewModelsMainWinVM* this, gchar* file_path, gchar* type, GError** error)
{
    gboolean result, gdk_save_supported;
    GSList *gdk_formats;
    GSList *gdk_formats_i;
    GError* err = NULL;

    if( !this->current_item )
        return FALSE;

    GdkPixbuf* pix = view_models_image_item_get_pixbuf(this->current_item);

    /* detect if the current type can be save by gdk_pixbuf_save() */
    gdk_save_supported = FALSE;
    gdk_formats = gdk_pixbuf_get_formats();
    for (gdk_formats_i = gdk_formats; gdk_formats_i;
         gdk_formats_i = g_slist_next(gdk_formats_i))
    {
        GdkPixbufFormat *data;
        data = gdk_formats_i->data;
        if (gdk_pixbuf_format_is_writable(data))
        {
            if ( strcmp(type, gdk_pixbuf_format_get_name(data))==0)
            {
                gdk_save_supported = TRUE;
                break;
            }
        }
    }
    g_slist_free (gdk_formats);

    if (!gdk_save_supported)
    {
        g_set_error(error, g_quark_from_static_string(""), 1, _("Writing this image format is not supported."));
        return FALSE;
    }

    if( strcmp( type, "jpeg" ) == 0 )
    {
        char tmp[32];
        g_sprintf(tmp, "%d", pref.jpg_quality);
        result = gdk_pixbuf_save( pix, file_path, type, &err, "quality", tmp, NULL );
    }
    else if( strcmp( type, "png" ) == 0 )
    {
        char tmp[32];
        g_sprintf(tmp, "%d", pref.png_compression);
        result = gdk_pixbuf_save( pix, file_path, type, &err, "compression", tmp, NULL );
    }
    else
    {
        result = gdk_pixbuf_save( pix, file_path, type, &err, NULL );
    }

    return result;
}

void
CLASS_PREFIX(prev)(ViewModelsMainWinVM* this)
{
    const char* name;
    if( image_list_is_empty( this->image_list ) )
        return;

    name = image_list_get_prev( this->image_list );

    if( !name && image_list_has_multiple_files( this->image_list ) )
        name = image_list_get_last( this->image_list );

    if( name )
    {
        char* file_path = image_list_get_current_file_path( this->image_list );
        view_models_main_win_vm_open_file_internal(this, file_path);
        g_free( file_path );
    }
}

void
CLASS_PREFIX(next)(ViewModelsMainWinVM* this)
{
    if( image_list_is_empty( this->image_list ) )
        return;

    const gchar* name = image_list_get_next( this->image_list );

    if( ! name && image_list_has_multiple_files( this->image_list ) )
    {
        // FIXME: need to ask user first?
        name = image_list_get_first( this->image_list);
    }

    if( name )
    {
        gchar* file_path = image_list_get_current_file_path( this->image_list );
        view_models_main_win_vm_open_file_internal(this, file_path);
        g_free( file_path );
    }
}

void
CLASS_PREFIX(rotate_item)(ViewModelsMainWinVM* this, gint angle)
{
    GError* innerError = NULL;

    //TODO: support periods
    gint corrected_angle = (angle < 0) ? angle : 360 - angle;

    view_models_image_item_rotate(this->current_item, corrected_angle);
    view_models_image_item_set_rotation(this->current_item, angle);

    if(pref.auto_save_rotated)
        view_models_main_win_vm_save(this, &innerError);
}

void
CLASS_PREFIX(edit_prefs)(ViewModelsMainWinVM* this)
{
    dialog_service_show_dialog(GPICVIEW_PREF_WINDOW, NULL);
}