#ifndef MAIN_WIN_VM
#define MAIN_WIM_VM

#include "image-item.h"

#include "glib-object.h"
#include "gdk/gdk.h"

G_BEGIN_DECLS

typedef struct _DialogService
{
    GPtrArray* (*open_file)(gchar* initial_folder, gpointer user_data);
    gchar* (*save_file)(gchar* initial_folder, gchar** types, gpointer user_data);
    gboolean (*yes_no_dialog)(gchar* msg, gpointer user_data);
    gpointer user_data;
} DialogService; 

enum
{
    CURRENT_ITEM_PROP = 1,
    N_PROPS
};

#define CURRENT_ITEM_PROP_NAME "current-item"

#define VIEWMODELS_TYPE_MAIN_WIN_VM view_models_main_win_vm_get_type()

G_DECLARE_FINAL_TYPE(ViewModelsMainWinVM, view_models_main_win_vm, VIEW_MODELS, MAIN_WIN_VM, GObject)

ViewModelsMainWinVM*
view_models_main_win_vm_new(DialogService* dialog_service);

ViewModelsImageItem*
view_models_main_win_vm_get_current_item(ViewModelsMainWinVM* this);

gboolean
view_models_main_win_vm_open(ViewModelsMainWinVM* this, GError** error);

gboolean
view_models_main_win_vm_open_file(ViewModelsMainWinVM* this, 
                                  const gchar* file_name,
                                  GError** error);

gboolean
view_models_main_win_vm_save(ViewModelsMainWinVM* this, GError** error);

gboolean
view_models_main_win_vm_save_as(ViewModelsMainWinVM* this, GError** error);

gboolean
view_models_main_win_vm_delete(ViewModelsMainWinVM* this, GError** error);

void
view_models_main_win_vm_prev(ViewModelsMainWinVM* this);

void
view_models_main_win_vm_next(ViewModelsMainWinVM* this);

void
view_models_main_win_vm_rotate_item(ViewModelsMainWinVM* this, gint angle);

G_END_DECLS

#endif