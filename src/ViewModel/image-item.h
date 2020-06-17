#ifndef _IMAGE_ITEM_H_
#define _IMAGE_ITEM_H_

#include "glib-object.h"
#include "gdk/gdk.h"

G_BEGIN_DECLS

#define VIEW_MODELS_TYPE_IMAGE_ITEM view_models_image_item_get_type()

#define ANIMATION_TIMEOUT_SIGNAL_NAME "animation_timeout"

G_DECLARE_FINAL_TYPE(ViewModelsImageItem,
                     view_models_image_item,
                     VIEW_MODELS,
                     IMAGE_ITEM,
                     GObject);

ViewModelsImageItem*
view_models_image_item_new_from_file(const gchar* file_name);

GdkPixbuf*
view_models_image_item_get_pixbuf(ViewModelsImageItem* this);

gchar*
view_models_image_item_get_name(ViewModelsImageItem* this);

gint
view_models_image_item_get_rotation(ViewModelsImageItem* this);

void
view_models_image_item_set_rotation(ViewModelsImageItem* this, gint value);

gboolean
view_models_image_item_is_animated(ViewModelsImageItem* this);

gboolean
view_models_image_item_is_modified(ViewModelsImageItem* this);

void
view_models_image_item_set_is_modified(ViewModelsImageItem* this, gboolean value);

void
view_models_image_item_rotate(ViewModelsImageItem* this, gint angle);

G_END_DECLS

#endif