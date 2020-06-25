#ifndef _DIALOG_SERVICE_H_
#define _DIALOG_SERVICE_H_

#include "glib.h"
#include "glib-object.h"

typedef gboolean (*WindowRunner)(GObject* view_model, gpointer parent);

void
dialog_service_init();

void
dialog_service_register_open_file_dialog(GPtrArray* (*open_file)(gchar* initial_folder, gpointer parent));

void
dialog_service_register_save_file_dialog(gchar* (*save_file)(gchar* initial_folder, gchar** types, gpointer parent));

void 
dialog_service_register_yes_no_dialog(gboolean (*yes_no_dialog)(gchar* msg, gpointer parent));

void
dialog_service_register_dialog(gint dialog_id, WindowRunner window_runner);

void
dialog_service_register_parent_resolver(gpointer (*resolver)(gpointer user_data));

GPtrArray* 
dialog_service_open_file (gchar* initial_folder);

gchar* 
dialog_service_save_file (gchar* initial_folder, gchar** types);

gboolean 
dialog_service_yes_no_dialog(gchar* msg);

gboolean
dialog_service_show_dialog(gint dialog_id, GObject* view_model);

#endif