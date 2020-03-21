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

#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/**
    @author PCMan (Hong Jen Yee) <pcman.tw@gmail.com>
*/

G_BEGIN_DECLS

#define LX_TYPE_IMAGE_VIEW lx_image_view_get_type ()

G_DECLARE_DERIVABLE_TYPE(LxImageView, lx_image_view, LX, IMAGE_VIEW, GtkWidget);


struct _LxImageViewClass
{
    GtkWidgetClass parent_class;
    void (*set_scroll_adjustments)( GtkWidget* w, GtkAdjustment* h, GtkAdjustment* v );
};

GtkWidget* lx_image_view_new();

void lx_image_view_set_pixbuf( LxImageView* iv, GdkPixbuf* pixbuf );

gdouble lx_image_view_get_scale( LxImageView* iv );
void lx_image_view_set_scale( LxImageView* iv, gdouble new_scale, GdkInterpType type );
void lx_image_view_get_size( LxImageView* iv, int* w, int* h );

void lx_image_view_set_adjustments( LxImageView* iv, GtkAdjustment* h, GtkAdjustment* v );

G_END_DECLS

#endif
