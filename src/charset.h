/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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

#ifndef ET_CHARSET_H_
#define ET_CHARSET_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

const char  *get_encoding_from_locale (const char *locale);
const gchar *get_locale               (void);

gchar *convert_string   (const gchar *string, const gchar *from_codeset, const gchar *to_codeset, const gboolean display_error);
gchar *convert_string_1 (const gchar *string, gssize length, const gchar *from_codeset, const gchar *to_codeset, const gboolean display_error);

gchar *filename_from_display (const gchar *string);

gchar *Try_To_Validate_Utf8_String (const gchar *string);

void Charset_Populate_Combobox (GtkComboBox *combo, gint select_charset);
const gchar * et_charset_get_name_from_index (guint index);

void Charset_Insert_Locales_Init    (void);
void Charset_Insert_Locales_Destroy (void);

G_END_DECLS

#endif /* ET_CHARSET_H_ */
