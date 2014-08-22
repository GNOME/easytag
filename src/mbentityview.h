/* mbentityview.h - 2014/05/05 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2014  Abhinav Jangda <abhijangda@hotmail.com>
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

#ifndef __MB_ENTITY_VIEW_H__
#define __MB_ENTITY_VIEW_H__

#include <gtk/gtk.h>

#define ET_MB_ENTITY_VIEW_TYPE (et_mb_entity_view_get_type ())
#define ET_MB_ENTITY_VIEW (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                                             ET_MB_ENTITY_VIEW_TYPE, \
                                                             EtMbEntityView))

#define ET_MB_ENTITY_VIEW_CLASS (klass) (G_TYPE_CHECK_INSTANCE_CAST ((klass), \
                                                                    ET_MB_ENTITY_VIEW_TYPE, \
                                                                    EtMbEntityView))

#define IS_ET_MB_ENTITY_VIEW (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                                               ET_MB_ENTITY_VIEW_TYPE))

#define IS_ET_MB_ENTITY_VIEW_CLASS (klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                                                     ET_MB_ENTITY_VIEW_TYPE))

/***************
 * Declaration *
 ***************/

/*
 * EtMbEntityView:
 * @vbox: GtkBox, parent class of EtMbEntityView
 *
 * This widget is used to show data recieved from music brainz and helps to
 * navigate in it using breadcrumb widget.
 */
typedef struct
{
    GtkBox vbox;
} EtMbEntityView;

/*
 * EtMbEntityViewClass:
 * @parent: GtkBoxClass, parent class of EtMbEntityViewClass
 *
 * Class of EtMbEntityView.
 */
typedef struct
{
    GtkBoxClass parent;
} EtMbEntityViewClass;

/**************
 * Prototypes *
 **************/

GType
et_mb_entity_view_get_type (void) G_GNUC_CONST;
GtkWidget *
et_mb_entity_view_new (void);
#endif /* __MB_ENTITY_VIEW_H__ */
