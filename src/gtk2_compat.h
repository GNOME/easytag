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

void et_grid_attach_full (GtkGrid *grid, GtkWidget *child, gint left, gint top,
                          gint width, gint height, gboolean hexpand,
                          gboolean vexpand, gint hmargin, gint vmargin);

void et_grid_attach_margins (GtkGrid *grid, GtkWidget *child, gint left,
                             gint top, gint width, gint height, gint hmargin,
                             gint vmargin);

G_END_DECLS

#endif /* GTK2_COMPAT_H_ */
