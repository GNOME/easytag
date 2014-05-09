/* EasyTAG - tag editor for audio files
 * Copyright (C) 2013  David King <amigadave@amigadave.com>
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

#ifndef ET_PLAYLIST_DIALOG_H_
#define ET_PLAYLIST_DIALOG_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#include "et_core.h"

#define ET_TYPE_PLAYLIST_DIALOG (et_playlist_dialog_get_type ())
#define ET_PLAYLIST_DIALOG(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_PLAYLIST_DIALOG, EtPlaylistDialog))

typedef struct _EtPlaylistDialog EtPlaylistDialog;
typedef struct _EtPlaylistDialogClass EtPlaylistDialogClass;
typedef struct _EtPlaylistDialogPrivate EtPlaylistDialogPrivate;

/* FIXME: Used by setting.c, remove when porting to GSettings. */
GtkWidget *playlist_use_mask_name;
GtkWidget *playlist_use_dir_name;
GtkWidget *playlist_only_selected_files;
GtkWidget *playlist_full_path;
GtkWidget *playlist_relative_path;
GtkWidget *playlist_create_in_parent_dir;
GtkWidget *playlist_use_dos_separator;


struct _EtPlaylistDialog
{
    /*< private >*/
    GtkDialog parent_instance;
    EtPlaylistDialogPrivate *priv;
};

struct _EtPlaylistDialogClass
{
    /*< private >*/
    GtkDialogClass parent_class;
};

GType et_playlist_dialog_get_type (void);
EtPlaylistDialog *et_playlist_dialog_new (void);
void et_playlist_dialog_apply_changes (EtPlaylistDialog *self);

G_END_DECLS

#endif /* !ET_PLAYLIST_DIALOG_H_ */
