/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2007  Jerome Couderc <easytag@gmail.com>
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

#ifndef ET_LOG_AREA_H_
#define ET_LOG_AREA_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ET_TYPE_LOG_AREA (et_log_area_get_type ())
#define ET_LOG_AREA(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_LOG_AREA, EtLogArea))

typedef struct _EtLogArea EtLogArea;
typedef struct _EtLogAreaClass EtLogAreaClass;

struct _EtLogArea
{
    /*< private >*/
    GtkBin parent_instance;
};

struct _EtLogAreaClass
{
    /*< private >*/
    GtkBinClass parent_class;
};

/*
 * Types of errors
 */
typedef enum
{                  
    LOG_UNKNOWN,
    LOG_OK,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} EtLogAreaKind;


/**************
 * Prototypes *
 **************/

GType et_log_area_get_type (void);
GtkWidget * et_log_area_new (void);
void et_log_area_clear (EtLogArea *self);
void Log_Print (EtLogAreaKind error_type,
                const gchar * const format, ...) G_GNUC_PRINTF (2, 3);

G_END_DECLS

#endif /* ET_LOG_AREA_H_ */
