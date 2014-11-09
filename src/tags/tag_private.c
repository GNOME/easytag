/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
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

#include "tag_private.h"

/*
 * guint32_from_be_bytes:
 * @str: a pointer to the first of 4 bytes from which to read an integer
 *
 * Reads from the given @str, in big-endian byte order, and gives a 32-bit
 * integer.
 *
 * Returns: a 32-bit integer corresponding to @str
 */
guint32
guint32_from_be_bytes (guchar *str)
{
    gsize i;
    guint32 result = 0;

    for (i = 0; i < 4; i++)
    {
        result = (result << 8) + str[i];
    }

    return result;
}

/*
 * guint32_from_le_bytes:
 * @str: a pointer to the first of 4 bytes from which to read an integer
 *
 * Reads from the given @str, in little-endian byte order, and gives a 32-bit
 * integer.
 *
 * Returns: a 32-bit integer corresponding to @str
 */
guint32
guint32_from_le_bytes (guchar *str)
{
    gsize i;
    guint32 result = 0;

    for (i = 4; i > 0; i--)
    {
        result = (result << 8) + str[i - 1];
    }

    return result;
}

/*
 * guint64_from_le_bytes:
 * @str: a pointer to the first of 8 bytes from which to read an integer
 *
 * Reads from the given @str, in little-endian byte order, and gives a 64-bit
 * integer.
 *
 * Returns: a 64-bit integer corresponding to @str
 */
guint64
guint64_from_le_bytes (guchar *str)
{
    gsize i;
    guint64 result = 0;

    for (i = 8; i > 0; i--)
    {
        result = (result << 8) + str[i - 1];
    }

    return result;
}
