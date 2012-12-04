/* crc32.c - the crc check algorithm for cksfv modified by oliver for easytag

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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "crc32.h"

/*
 * Calculate the CRC-32 value of audio data (doesn't read the ID3v2 and ID3v1 tags).
 * Return 0 if OK
 */
int crc32_file_with_ID3_tag(char *filename, unsigned long *main_val)
{
    char          buf[BUFFERSIZE], *p;
    int           nr;
    unsigned long crc = ~0, crc32_total = ~0;
    FILE         *fd;
    unsigned char tmp_id3[4];
    int           id3v2size = 0;
    struct        stat statbuf;
    unsigned long size = 0;
    int           has_id3v1 = 0;


    if (!filename)
        return 1;

    stat(filename,&statbuf);
    size = statbuf.st_size;

    if ( (fd = fopen(filename,"r"))==NULL )
        return 1;

    // Check if there is an ID3v1 tag...
    fseek(fd, -128, SEEK_END);
    if (fread(tmp_id3, 1, 3, fd) != 3)
        return 1;
    if (tmp_id3[0] == 'T' && tmp_id3[1] == 'A' && tmp_id3[2] == 'G')
        has_id3v1 = 1;


    // Check if there is an ID3v2 tag...
    fseek(fd, 0L, SEEK_SET);
    if (fread(tmp_id3, 1, 4, fd) != 4)
        return 1;
    // Calculate ID3v2 length
    if (tmp_id3[0] == 'I' && tmp_id3[1] == 'D' && tmp_id3[2] == '3' && tmp_id3[3] < 0xFF)
    {
        // id3v2 tag skipeer $49 44 33 yy yy xx zz zz zz zz [zz size]
        fseek(fd, 2, SEEK_CUR); // Size is 6-9 position
        if (fread(tmp_id3, 1, 4, fd) != 4)
            return 1;
        id3v2size = 10 + ( (long)(tmp_id3[3])        | ((long)(tmp_id3[2]) << 7)
                        | ((long)(tmp_id3[1]) << 14) | ((long)(tmp_id3[0]) << 21) );

        fseek(fd, id3v2size, SEEK_SET);
        size = size - id3v2size;
    }else
    {
        fseek(fd, 0L, SEEK_SET);
    }

    while ((nr = fread(buf, sizeof(char), sizeof(buf), fd)) > 0)
    {
        if (has_id3v1 && nr <= 128) // We are reading end of ID3v1 tag
            break;
        if ( has_id3v1 && ((size=size-nr) < 128) ) // ID3v1 tag is in the current buf
            nr = nr - 128 + size;

        for (p = buf; nr--; ++p)
        {
            crc = (crc >> 8) ^ crctable[(crc ^ *p) & 0xff];
            crc32_total = (crc >> 8) ^ crctable[(crc32_total ^ *p) & 0xff];
        }
    }

    fclose(fd);

    if (nr < 0)
        return 1;

    *main_val = ~crc;

    return 0;
}
