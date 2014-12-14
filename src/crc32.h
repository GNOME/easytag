/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 David King <amigadave@amigadave.com>
 * Copyright (C) 2000 Bryan Call <bc@fodder.org>
 * Copyright (C) 2003 Oliver Schinagl <oliver@are-b.org>
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
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ET_CRC32_H_
#define ET_CRC32_H_

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

gboolean crc32_file_with_ID3_tag (GFile *file, guint32 *crc32, GError **err);

G_END_DECLS

#endif /* ET_CRC32_H_ */
