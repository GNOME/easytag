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

#endif /* !GTK_CHECK_VERSION(3,0,0) */
