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

#include "gtk2_compat.h"

#if !GTK_CHECK_VERSION(3,0,0)

void et_grid_attach_full (GtkGrid *grid, GtkWidget *child, gint left, gint top,
                          gint width, gint height, gboolean hexpand,
                          gboolean vexpand, gint hmargin, gint vmargin)
{
    GtkAttachOptions xoptions = GTK_FILL;
    GtkAttachOptions yoptions = GTK_FILL;

    if (hexpand)
    {
        xoptions |= GTK_EXPAND;
    }
    if (vexpand)
    {
        yoptions |= GTK_EXPAND;
    }

    gtk_table_attach (grid, child, left, left + width, top, top + height,
                      xoptions, yoptions, hmargin, vmargin);
}

void gtk_grid_attach (GtkGrid *grid, GtkWidget *child, gint left, gint top,
                      gint width, gint height)
{
    et_grid_attach_full (grid, child, left, top, width, height, FALSE, FALSE,
                         0, 0);
}

GtkWidget *gtk_box_new(GtkOrientation orientation,gint padding)
{
    if (orientation==GTK_ORIENTATION_HORIZONTAL)
        return gtk_hbox_new(FALSE,padding);
    return gtk_vbox_new(FALSE,padding);
}

GtkWidget *gtk_button_box_new(GtkOrientation orientation)
{
    if (orientation==GTK_ORIENTATION_HORIZONTAL)
        return gtk_hbutton_box_new();
    return gtk_vbutton_box_new();
}

GtkWidget *gtk_paned_new(GtkOrientation orientation)
{
    if (orientation==GTK_ORIENTATION_HORIZONTAL)
        return gtk_hpaned_new();
    return gtk_vpaned_new();
}

GtkWidget *gtk_separator_new(GtkOrientation orientation)
{
    if (orientation==GTK_ORIENTATION_HORIZONTAL)
        return gtk_hseparator_new();
    return gtk_vseparator_new();
}

#else /* GTK_CHECK_VERSION(3,0,0) */

void et_grid_attach_full (GtkGrid *grid, GtkWidget *child, gint left, gint top,
                          gint width, gint height, gboolean hexpand,
                          gboolean vexpand, gint hmargin, gint vmargin)
{
    g_object_set (G_OBJECT(child),
                  "hexpand", hexpand,
                  "vexpand", vexpand,
                  "margin-left", hmargin,
                  "margin-right", hmargin,
                  "margin-top", vmargin,
                  "margin-bottom", vmargin,
                  NULL);
    gtk_grid_attach (grid, child, left, top, width, height);
}

#endif /* GTK_CHECK_VERSION(3,0,0) */
