/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 David King <amigadave@amigadave.com>
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

#ifndef ET_SCAN_H_
#define ET_SCAN_H_

#include <glib.h>

G_BEGIN_DECLS

void Scan_Convert_Underscore_Into_Space (gchar *string);
void Scan_Convert_P20_Into_Space (gchar *string);
void Scan_Convert_Space_Into_Underscore (gchar *string);
void Scan_Process_Fields_Remove_Space (gchar *string);
gchar* Scan_Process_Fields_Insert_Space (const gchar *string);
void Scan_Process_Fields_Keep_One_Space (gchar *string);
void Scan_Remove_Spaces (gchar *string);
gchar* Scan_Process_Fields_All_Uppercase (const gchar *string);
gchar* Scan_Process_Fields_All_Downcase (const gchar *string);
gchar* Scan_Process_Fields_Letter_Uppercase (const gchar *string);
void Scan_Process_Fields_First_Letters_Uppercase (gchar **str, gboolean uppercase_preps, gboolean handle_roman);

G_END_DECLS

#endif /* !ET_SCAN_H_ */
