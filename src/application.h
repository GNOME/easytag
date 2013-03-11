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

#ifndef ET_APPLICATION_H_
#define ET_APPLICATION_H_

#include <gio/gio.h>

G_BEGIN_DECLS

#define ET_TYPE_APPLICATION (et_application_get_type ())

typedef struct _EtApplication EtApplication;
typedef struct _EtApplicationClass EtApplicationClass;
typedef struct _EtApplicationPrivate EtApplicationPrivate;

struct _EtApplication
{
    /*< private >*/
    GApplication parent_instance;
    EtApplicationPrivate *priv;
};

struct _EtApplicationClass
{
    /*< private >*/
    GApplicationClass parent_class;
};

GType et_application_get_type (void);
EtApplication *et_application_new (void);

G_END_DECLS

#endif /* !ET_APPLICATION_H_ */
