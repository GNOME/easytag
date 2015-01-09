/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ET_CDDB_DIALOG_H_
#define ET_CDDB_DIALOG_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ET_TYPE_CDDB_DIALOG (et_cddb_dialog_get_type ())
#define ET_CDDB_DIALOG(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_CDDB_DIALOG, EtCDDBDialog))

typedef struct _EtCDDBDialog EtCDDBDialog;
typedef struct _EtCDDBDialogClass EtCDDBDialogClass;

struct _EtCDDBDialog
{
    /*< private >*/
    GtkDialog parent_instance;
};

struct _EtCDDBDialogClass
{
    /*< private >*/
    GtkDialogClass parent_class;
};

GType et_cddb_dialog_get_type (void);
EtCDDBDialog *et_cddb_dialog_new (void);
gboolean et_cddb_dialog_search_from_selection (EtCDDBDialog *self);

G_END_DECLS

#endif /* ET_CDDB_H_ */
