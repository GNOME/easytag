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

#ifndef ET_MISC_H_
#define ET_MISC_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Combobox misc functions
 */
gboolean Add_String_To_Combo_List(GtkListStore *liststore, const gchar *string);

gchar *Convert_Duration (gulong duration);

gboolean et_run_audio_player (GList *files, GError **error);
gboolean et_run_program (const gchar *program_name, GList *args_list, GError **error);

gchar * et_disc_number_to_string (const guint disc_number);
gchar * et_track_number_to_string (const guint track_number);

G_END_DECLS

#endif /* ET_MISC_H_ */
