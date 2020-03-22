#ifndef WORKING_AREA_H
#define WORKING_AREA_H

#include <glib.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

void get_working_area(GdkWindow* gdkWindow, GdkRectangle *rect);

G_END_DECLS

#endif

