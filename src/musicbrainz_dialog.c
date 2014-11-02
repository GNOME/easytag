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

#ifdef ENABLE_MUSICBRAINZ

#include "musicbrainz_dialog.h"

#include <glib/gi18n.h>

#include "musicbrainz.h"

typedef struct
{
    EtMusicbrainz *mb;
} EtMusicbrainzDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtMusicbrainzDialog, et_musicbrainz_dialog, GTK_TYPE_DIALOG)

static void
et_musicbrainz_dialog_finalize (GObject *object)
{
    EtMusicbrainzDialog *self;
    EtMusicbrainzDialogPrivate *priv;

    self = ET_MUSICBRAINZ_DIALOG (object);
    priv = et_musicbrainz_dialog_get_instance_private (self);

    g_clear_object (&priv->mb);

    G_OBJECT_CLASS (et_musicbrainz_dialog_parent_class)->finalize (object);
}

static void
et_musicbrainz_dialog_init (EtMusicbrainzDialog *self)
{
    EtMusicbrainzDialogPrivate *priv;

    priv = et_musicbrainz_dialog_get_instance_private (self);

    gtk_widget_init_template (GTK_WIDGET (self));

    priv->mb = et_musicbrainz_new ();
}

static void
et_musicbrainz_dialog_class_init (EtMusicbrainzDialogClass *klass)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS (klass);
    widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/EasyTAG/musicbrainz_dialog.ui");

    gobject_class->finalize = et_musicbrainz_dialog_finalize;
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

#endif /* ENABLE_MUSICBRAINZ */
