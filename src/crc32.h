/* crc32.h - the crc check algorithm for cksfv modified by oliver for easytag
   
   Copyright (C) 2000 Bryan Call <bc@fodder.org>
   Copyright (C) 2003 Oliver Schinagl <oliver@are-b.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */


#ifndef __CRC32_H__
#define __CRC32_H__


gboolean crc32_file_with_ID3_tag (gchar *filename, guint32 *crc32);


#endif /* __CRC32_H__ */
