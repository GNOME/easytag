/* EasyTAG - tag editor for audio files
 * Copyright (C) 2015  David King <amigadave@amigadave.com>
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

#ifndef ET_FILE_NAME_H_
#define ET_FILE_NAME_H_

#include <glib.h>

G_BEGIN_DECLS

/*
 * Description of each item of the FileNameList list
 */
typedef struct
{
    guint key;
    gboolean saved; /* Set to TRUE if this filename had been saved */
    gchar *value; /* The filename containing the full path and the extension of the file */
    gchar *value_utf8; /* Same than "value", but converted to UTF-8 to avoid multiple call to the convertion function */
    gchar *value_ck; /* Collate key of "value_utf8" to speed up comparison. */
} File_Name;

File_Name * et_file_name_new (void);
void et_file_name_free (File_Name *file_name);
void ET_Set_Filename_File_Name_Item (File_Name *FileName, const gchar *filename_utf8, const gchar *filename);
gboolean et_file_name_set_from_components (File_Name *file_name, const gchar *new_name, const gchar *dir_name, gboolean replace_illegal);
gboolean et_file_name_detect_difference (const File_Name *a, const File_Name *b);

G_END_DECLS

#endif /* !ET_FILE_NAME_H_ */
