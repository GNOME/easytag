/* EasyTAG - Tag editor for audio files
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

#include "gtk2_compat.h"

void et_grid_attach_full (GtkGrid *grid, GtkWidget *child, gint left, gint top,
                          gint width, gint height, gboolean hexpand,
                          gboolean vexpand, gint hmargin, gint vmargin)
{
    g_object_set (G_OBJECT(child),
                  "hexpand", hexpand,
                  "vexpand", vexpand,
#if GTK_CHECK_VERSION (3, 12, 0)
                  "margin-start", hmargin,
                  "margin-end", hmargin,
#else
                  "margin-left", hmargin,
                  "margin-right", hmargin,
#endif
                  "margin-top", vmargin,
                  "margin-bottom", vmargin,
                  NULL);
    gtk_grid_attach (grid, child, left, top, width, height);
}

void et_grid_attach_margins (GtkGrid *grid, GtkWidget *child, gint left,
                              gint top, gint width, gint height, gint hmargin,
                              gint vmargin)
{
    et_grid_attach_full (grid, child, left, top, width, height, FALSE, FALSE,
                         hmargin, vmargin);
}
