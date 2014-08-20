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


#ifndef ET_SCAN_DIALOG_H_
#define ET_SCAN_DIALOG_H_

#include "et_core.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ET_TYPE_SCAN_DIALOG (et_scan_dialog_get_type ())
#define ET_SCAN_DIALOG(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_SCAN_DIALOG, EtScanDialog))

typedef struct _EtScanDialog EtScanDialog;
typedef struct _EtScanDialogClass EtScanDialogClass;
typedef struct _EtScanDialogPrivate EtScanDialogPrivate;

struct _EtScanDialog
{
    /*< private >*/
    GtkDialog parent_instance;
    EtScanDialogPrivate *priv;
};

struct _EtScanDialogClass
{
    /*< private >*/
    GtkDialogClass parent_class;
};

GType et_scan_dialog_get_type (void);
EtScanDialog *et_scan_dialog_new (GtkWindow *parent);
void et_scan_dialog_apply_changes (EtScanDialog *self);
void et_scan_dialog_open (EtScanDialog *self, EtScanMode scanner_type);
void et_scan_dialog_scan_selected_files (EtScanDialog *self);
void et_scan_dialog_update_previews (EtScanDialog *self);

G_END_DECLS


enum {
    MASK_EDITOR_TEXT,
    MASK_EDITOR_COUNT
};

void Scan_Select_Mode_And_Run_Scanner (EtScanDialog *self, ET_File *ETFile);
gchar *Scan_Generate_New_Filename_From_Mask       (ET_File *ETFile, gchar *mask, gboolean no_dir_check_or_conversion);
gchar *Scan_Generate_New_Directory_Name_From_Mask (ET_File *ETFile, gchar *mask, gboolean no_dir_check_or_conversion);

void entry_check_rename_file_mask (GtkEntry *entry, gpointer user_data);

void Scan_Process_Fields_First_Letters_Uppercase (EtScanDialog *self, gchar **str);

#endif /* ET_SCAN_DIALOG_H_ */
