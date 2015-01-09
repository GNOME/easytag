/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
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

#ifndef ET_TAG_AREA_H_
#define ET_TAG_AREA_H_

#include <gtk/gtk.h>

#include "et_core.h"

G_BEGIN_DECLS

#define ET_TYPE_TAG_AREA (et_tag_area_get_type ())
#define ET_TAG_AREA(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_TAG_AREA, EtTagArea))

typedef struct _EtTagArea EtTagArea;
typedef struct _EtTagAreaClass EtTagAreaClass;

struct _EtTagArea
{
    /*< private >*/
    GtkBin parent_instance;
};

struct _EtTagAreaClass
{
    /*< private >*/
    GtkBinClass parent_class;
};

GType et_tag_area_get_type (void);
GtkWidget * et_tag_area_new (void);
void et_tag_area_update_controls (EtTagArea *self, const ET_File *ETFile);
void et_tag_area_clear (EtTagArea *self);
void et_tag_area_title_grab_focus (EtTagArea *self);
File_Tag * et_tag_area_create_file_tag (EtTagArea *self);
gboolean et_tag_area_display_et_file (EtTagArea *self, const ET_File *ETFile);
gboolean et_tag_area_select_all_if_focused (EtTagArea *self, GtkWidget *focused);
gboolean et_tag_area_unselect_all_if_focused (EtTagArea *self, GtkWidget *focused);

void on_entry_populate_popup (GtkEntry *entry, GtkWidget *menu, EtTagArea *self);

G_END_DECLS

#endif /* ET_TAG_AREA_H_ */
