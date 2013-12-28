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

#ifndef ET_APPLICATION_WINDOW_H_
#define ET_APPLICATION_WINDOW_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#include "et_core.h"

#define ET_TYPE_APPLICATION_WINDOW (et_application_window_get_type ())
#define ET_APPLICATION_WINDOW(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_APPLICATION_WINDOW, EtApplicationWindow))

typedef struct _EtApplicationWindow EtApplicationWindow;
typedef struct _EtApplicationWindowClass EtApplicationWindowClass;
typedef struct _EtApplicationWindowPrivate EtApplicationWindowPrivate;

struct _EtApplicationWindow
{
    /*< private >*/
    GtkApplicationWindow parent_instance;
    EtApplicationWindowPrivate *priv;
};

struct _EtApplicationWindowClass
{
    /*< private >*/
    GtkApplicationWindowClass parent_class;
};

GType et_application_window_get_type (void);
EtApplicationWindow *et_application_window_new (GtkApplication *application);
void et_application_window_tag_area_set_sensitive (EtApplicationWindow *self, gboolean sensitive);
void et_application_window_file_area_set_sensitive (EtApplicationWindow *self, gboolean sensitive);
void et_application_window_tag_area_display_controls (EtApplicationWindow *self, ET_File *ETFile);
GtkWidget * et_application_window_get_log_area (EtApplicationWindow *self);
GtkWidget * et_application_window_get_playlist_dialog (EtApplicationWindow *self);
void et_application_window_show_playlist_dialog (GtkAction *action, gpointer user_data);
GtkWidget * et_application_window_get_load_files_dialog (EtApplicationWindow *self);
void et_application_window_show_load_files_dialog (GtkAction *action, gpointer user_data);
GtkWidget * et_application_window_get_search_dialog (EtApplicationWindow *self);
void et_application_window_show_search_dialog (GtkAction *action, gpointer user_data);
void et_application_window_hide_log_area (EtApplicationWindow *self);
void et_application_window_show_log_area (EtApplicationWindow *self);

G_END_DECLS

#endif /* !ET_APPLICATION_WINDOW_H_ */
