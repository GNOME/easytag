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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ET_STATUS_BAR_H_
#define ET_STATUS_BAR_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ET_TYPE_STATUS_BAR (et_status_bar_get_type ())
#define ET_STATUS_BAR(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_STATUS_BAR, EtStatusBar))

typedef struct _EtStatusBar EtStatusBar;
typedef struct _EtStatusBarClass EtStatusBarClass;

struct _EtStatusBar
{
    /*< private >*/
    GtkStatusbar parent_instance;
};

struct _EtStatusBarClass
{
    /*< private >*/
    GtkStatusbarClass parent_class;
};

GType et_status_bar_get_type (void);
GtkWidget * et_status_bar_new (void);
void et_status_bar_message (EtStatusBar *self, const gchar *message, gboolean with_timer);

G_END_DECLS

#endif /* ET_STATUS_BAR_H_ */
