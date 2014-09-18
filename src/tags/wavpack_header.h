/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2007 Maarten Maathuis (madman2003@gmail.com)
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ET_WAVPACK_HEADER_H_
#define ET_WAVPACK_HEADER_H_


#include "et_core.h"

gboolean Wavpack_Header_Read_File_Info (const gchar *filename, ET_File_Info *ETFileInfo);
EtFileHeaderFields * Wavpack_Header_Display_File_Info_To_UI (const gchar *filename_utf8, ET_File *ETFile);
void et_wavpack_file_header_fields_free (EtFileHeaderFields *fields);

#endif /* ET_WAVPACK_HEADER_H_ */
