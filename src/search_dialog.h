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

#ifndef ET_SEARCH_DIALOG_H_
#define ET_SEARCH_DIALOG_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#include "et_core.h"

#define ET_TYPE_SEARCH_DIALOG (et_search_dialog_get_type ())
#define ET_SEARCH_DIALOG(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_SEARCH_DIALOG, EtSearchDialog))

typedef struct _EtSearchDialog EtSearchDialog;
typedef struct _EtSearchDialogClass EtSearchDialogClass;

struct _EtSearchDialog
{
    /*< private >*/
    GtkDialog parent_instance;
};

struct _EtSearchDialogClass
{
    /*< private >*/
    GtkDialogClass parent_class;
};

GType et_search_dialog_get_type (void);
EtSearchDialog *et_search_dialog_new (GtkWindow *parent);
void et_search_dialog_apply_changes (EtSearchDialog *self);

G_END_DECLS

#endif /* !ET_SEARCH_DIALOG_H_ */
