/* charset.h - 2001/12/04 */
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef __CHARSET_H__
#define __CHARSET_H__

#include <gtk/gtk.h>

/***************
 * Declaration *
 ***************/

typedef struct
{
    gchar *charset_title;
    gchar *charset_name;
} CharsetInfo;

/* translated charset titles */
extern const CharsetInfo charset_trans_array[];



/**************
 * Prototypes *
 **************/

const char  *get_encoding_from_locale (const char *locale);
const gchar *get_locale               (void);


gchar *convert_string   (const gchar *string, const gchar *from_codeset, const gchar *to_codeset, const gboolean display_error);
gchar *convert_string_1 (const gchar *string, gssize length, const gchar *from_codeset, const gchar *to_codeset, const gboolean display_error);

/* Used for Ogg Vorbis and FLAC tags */
gchar *convert_to_utf8   (const gchar *string);

gchar *filename_to_display   (const gchar *string);
gchar *filename_from_display (const gchar *string);

gchar *Try_To_Validate_Utf8_String (const gchar *string);

void   Charset_Populate_Combobox   (GtkComboBox *combo, gchar *select_charset);
gchar *Charset_Get_Name_From_Title (const gchar *charset_title);


void Charset_Insert_Locales_Init    (void);
void Charset_Insert_Locales_Destroy (void);


#endif /* __CHARSET_H__ */
