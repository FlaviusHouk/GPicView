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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>

#include "pref.h"
#include "View/main-win.h"
#include "ViewModel/dialog_service.h"

static gchar** files = NULL;
static gboolean should_display_version = FALSE;
static gboolean should_start_slideshow = FALSE;
static GtkApplication* app;

static GOptionEntry opt_entries[] =
{
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &files, NULL, N_("[FILE]")},
    {"version", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &should_display_version,
                 N_("Print version information and exit"), NULL },
    {"slideshow", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &should_start_slideshow,
                 N_("Start slideshow"), NULL },
    { NULL }
};

#define PIXMAP_DIR        PACKAGE_DATA_DIR "/gpicview/pixmaps/"

static void
on_main_window_close(GtkWidget *object, gpointer   user_data)
{
    gtk_main_quit();
}

static GtkWindow*
gpicview_get_active_window()
{
    return gtk_application_get_active_window(app);
}

static void
gpicview_activate(GApplication* app, gpointer user_data)
{
    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), PIXMAP_DIR);

    MainWin* win = MAIN_WIN(main_win_new());
    gtk_application_add_window(app, GTK_WINDOW(win));

    if ( pref.open_maximized )
        gtk_window_maximize( GTK_WINDOW(win) );

    GError *error = NULL;

    // FIXME: need to process multiple files...
    if( files )
    {
        if( G_UNLIKELY( *files[0] != '/' && strstr( files[0], "://" )) )    // This is an URI
        {
            char* path = g_filename_from_uri( files[0], NULL, NULL );
            view_models_main_win_vm_open_file( win->view_model, path, &error);
            g_free( path );            
        }
        else
        {
            view_models_main_win_vm_open_file(win->view_model, files[0], &error);
        }

        if (should_start_slideshow)
            main_win_start_slideshow ( win );
    }
    else
    {
        view_models_main_win_vm_open_file( win->view_model, ".", &error);
    }

    if(error != NULL)
    {
        main_win_show_error(win, error->message);
        return 1;
    }

    gtk_widget_show( GTK_WIDGET(win) );
}

static gint
gpicview_handle_local_options(GApplication *application,
                              GVariantDict *options,
                              gpointer      user_data)
{
    if( should_display_version )
    {
        g_print( "gpicview %s\n", VERSION );
        return 0;
    }

    return -1;
}

int main(int argc, char *argv[])
{
    GOptionContext *context;

    app = gtk_application_new("org.lxde.gpicview", G_APPLICATION_FLAGS_NONE);
    g_application_add_main_option_entries(app, opt_entries);

#ifdef ENABLE_NLS
    bindtextdomain ( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR );
    bind_textdomain_codeset ( GETTEXT_PACKAGE, "UTF-8" );
    textdomain ( GETTEXT_PACKAGE );
#endif

    dialog_service_init();

    g_signal_connect(app, "activate", G_CALLBACK(gpicview_activate), NULL);
    g_signal_connect(app, "handle-local-options", G_CALLBACK(gpicview_handle_local_options), NULL);

    load_preferences();

    g_application_run(app, argc, argv);

    save_preferences();

    return 0;
}
