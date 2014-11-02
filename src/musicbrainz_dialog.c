/* EasyTAG - tag editor for audio files
 * Copyright (C) 2015  David King <amigadave@amigadave.com>
 * Copyright (C) 2014  Abhinav Jangda <abhijangda@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "musicbrainz_dialog.h"

#include <glib/gi18n.h>

typedef struct
{
    gpointer unused;
} EtMusicbrainzDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtMusicbrainzDialog, et_musicbrainz_dialog, GTK_TYPE_DIALOG)

static void
et_musicbrainz_dialog_init (EtMusicbrainzDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

static void
et_musicbrainz_dialog_class_init (EtMusicbrainzDialogClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/EasyTAG/musicbrainz_dialog.ui");
}

/*
 * et_musicbrainz_dialog_new:
 *
 * Create a new EtMusicbrainzDialog instance.
 *
 * Returns: a new #EtMusicbrainzDialog
 */
EtMusicbrainzDialog *
et_musicbrainz_dialog_new (GtkWindow *parent)
{
    g_return_val_if_fail (GTK_WINDOW (parent), NULL);

    return g_object_new (ET_TYPE_MUSICBRAINZ_DIALOG, "transient-for", parent,
                         NULL);
}
