/*
 *  Copyright (C) 2014 Victor A. Santos <victoraur.santos@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef NAUTILUS_EASYTAG_H_
#define NAUTILUS_EASYTAG_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define NAUTILUS_TYPE_EASYTAG (nautilus_easytag_get_type ())
#define NAUTILUS_EASYTAG(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_TYPE_EASYTAG, NautilusEasyTag))
#define NAUTILUS_IS_EASYTAG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_TYPE_EASYTAG))

typedef struct _NautilusEasyTag NautilusEasyTag;
typedef struct _NautilusEasyTagClass NautilusEasyTagClass;

struct _NautilusEasyTag
{
    GObject parent;
};

struct _NautilusEasyTagClass
{
    GObjectClass parent_class;
};

GType nautilus_easytag_get_type (void);
void  nautilus_easytag_register_type (GTypeModule *module);

G_END_DECLS

#endif /* NAUTILUS_EASYTAG_H_ */
