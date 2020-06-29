#include "dialog_service.h"

static GPtrArray* (*open_file_runner)(const gchar* initial_folder, gpointer parent);
static gchar* (*save_file_runner)(const gchar* initial_folder, gchar** types, gpointer parent);
static gboolean (*yes_no_dialog_runner)(const gchar* msg, gpointer parent);
static void (*msg_box_runner)(const gchar* msg, gpointer parent);
static gpointer (*parent_resolver)();
static GHashTable* dialogs;

void
dialog_service_init()
{
    dialogs = g_hash_table_new(g_int_hash, g_int_equal);
}

void
dialog_service_register_open_file_dialog(GPtrArray* (*open_file)(const gchar* initial_folder, gpointer parent))
{
    open_file_runner = open_file;
}

void
dialog_service_register_save_file_dialog(gchar* (*save_file)(const gchar* initial_folder, gchar** types, gpointer parent))
{
    save_file_runner = save_file;
}

void 
dialog_service_register_yes_no_dialog(gboolean (*yes_no_dialog)(const gchar* msg, gpointer parent))
{
    yes_no_dialog_runner = yes_no_dialog;
}

void
dialog_service_register_message_box(void (*msg_box)(const gchar* msg, gpointer parent))
{
    msg_box_runner = msg_box;
}

void
dialog_service_register_dialog(gint dialog_id, WindowRunner window_runner)
{
    gint* key = g_malloc(sizeof(gint));
    *key = dialog_id;

    if(g_hash_table_contains(dialogs, key))
    {
        g_printerr("Dialog %d already registered.\n", dialog_id);
        g_assert(FALSE);
    }

    g_hash_table_insert(dialogs, key, window_runner);
}

void
dialog_service_register_parent_resolver(gpointer (*resolver)())
{
    parent_resolver = resolver;
}

GPtrArray* 
dialog_service_open_file (const gchar* initial_folder)
{
    return open_file_runner(initial_folder, parent_resolver());
}

gchar* 
dialog_service_save_file (const gchar* initial_folder, gchar** types)
{
    return save_file_runner(initial_folder, types, parent_resolver());
}

gboolean 
dialog_service_yes_no_dialog(const gchar* msg)
{
    return yes_no_dialog_runner(msg, parent_resolver());
}

void
dialog_service_show_message(const gchar* msg)
{
    msg_box_runner(msg, parent_resolver());
}

gboolean
dialog_service_show_dialog(gint dialog_id, GObject* view_model)
{
    WindowRunner window_runner = (WindowRunner)g_hash_table_lookup(dialogs, &dialog_id);

    return window_runner(view_model, parent_resolver());
}