/*
 * EasyTAG - Tag editor for audio files
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ET_PREFERENCES_DIALOG_H_
#define ET_PREFERENCES_DIALOG_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ET_TYPE_PREFERENCES_DIALOG (et_preferences_dialog_get_type ())
#define ET_PREFERENCES_DIALOG(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_PREFERENCES_DIALOG, EtPreferencesDialog))

typedef struct _EtPreferencesDialog EtPreferencesDialog;
typedef struct _EtPreferencesDialogClass EtPreferencesDialogClass;

struct _EtPreferencesDialog
{
    /*< private >*/
    GtkDialog parent_instance;
};

struct _EtPreferencesDialogClass
{
    /*< private >*/
    GtkDialogClass parent_class;
};

GType et_preferences_dialog_get_type (void);
EtPreferencesDialog *et_preferences_dialog_new (GtkWindow *parent);
void et_preferences_dialog_show_scanner (EtPreferencesDialog *self);

G_END_DECLS

#endif /* ET_PREFERENCES_DIALOG_H_ */
