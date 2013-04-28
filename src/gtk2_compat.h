/*
 * EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 * Copyright (C) 2012  David King <amigadave@amigadave.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GTK2_COMPAT_H_
#define GTK2_COMPAT_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#if !GTK_CHECK_VERSION(3,0,0)

#define GTK_SCROLLABLE(x) GTK_TREE_VIEW(x)
#define gtk_scrollable_get_vadjustment(x) gtk_tree_view_get_vadjustment(x)

#define GTK_GRID(x) GTK_TABLE(x)
#define GtkGrid GtkTable

#define et_grid_new(r, c) gtk_table_new((r),(c),FALSE)

#define gtk_grid_set_row_spacing gtk_table_set_row_spacings
#define gtk_grid_set_column_spacing gtk_table_set_col_spacings
void gtk_grid_attach (GtkGrid *grid, GtkWidget *child, gint left, gint top,
                      gint width, gint height);

GtkWidget *gtk_box_new(GtkOrientation orientation, gint padding);
GtkWidget *gtk_button_box_new(GtkOrientation orientation);
GtkWidget *gtk_paned_new(GtkOrientation orientation);
GtkWidget *gtk_separator_new(GtkOrientation orientation);

#else /* GTK_CHECK_VERSION(3,0,0) */

#define et_grid_new(r,c) gtk_grid_new()

#endif /* GTK_CHECK_VERSION(3,0,0) */

void et_grid_attach_full (GtkGrid *grid, GtkWidget *child, gint left, gint top,
                          gint width, gint height, gboolean hexpand,
                          gboolean vexpand, gint hmargin, gint vmargin);

void et_grid_attach_margins (GtkGrid *grid, GtkWidget *child, gint left,
                             gint top, gint width, gint height, gint hmargin,
                             gint vmargin);

G_END_DECLS

#endif /* GTK2_COMPAT_H_ */
