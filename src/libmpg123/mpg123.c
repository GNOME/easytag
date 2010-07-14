/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999,2000  Håvard Kvålen
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Note : removed code not used in EasyTAG
 */

#include "mpg123.h"


double mpg123_compute_tpf(struct frame *fr)
{
	const int bs[4] = {0, 384, 1152, 1152};
	double tpf;

	tpf = bs[fr->lay];
	tpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
	return tpf;
}


static guint32 convert_to_header(guint8 * buf)
{

	return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}


#define DET_BUF_SIZE 1024

#if 0 /* Not used at the present time */
static gboolean mpg123_detect_by_content(gchar *filename)
{
	FILE *file;
	guchar tmp[4];
	guint32 head;
	struct frame fr;
	guchar buf[DET_BUF_SIZE];
	gint in_buf, i;

	if((file = fopen(filename, "rb")) == NULL)
		return FALSE;
	if (fread(tmp, 1, 4, file) != 4)
		goto done;
	head = convert_to_header(tmp);
	while(!mpg123_head_check(head))
	{
		/*
		 * The mpeg-stream can start anywhere in the file,
		 * so we check the entire file
		 */
		/* Optimize this */
		in_buf = fread(buf, 1, DET_BUF_SIZE, file);
		if(in_buf == 0)
			goto done;

		for (i = 0; i < in_buf; i++)
		{
			head <<= 8;
			head |= buf[i];
			if(mpg123_head_check(head))
			{
				fseek(file, i+1-in_buf, SEEK_CUR);
				break;
			}
		}
	}
	if (mpg123_decode_header(&fr, head))
	{
		/*
		 * We found something which looks like a MPEG-header.
		 * We check the next frame too, to be sure
		 */
		
		if (fseek(file, fr.framesize, SEEK_CUR) != 0)
			goto done;
		if (fread(tmp, 1, 4, file) != 4)
			goto done;
		head = convert_to_header(tmp);
		if (mpg123_head_check(head) && mpg123_decode_header(&fr, head))
		{
			fclose(file);
			return TRUE;
		}
	}

 done:
	fclose(file);
	return FALSE;
}
#endif

//static guint get_song_time(FILE * file)
guint mpg123_get_song_time(FILE * file)
{
	guint32 head;
	guchar tmp[4], *buf;
	struct frame frm;
	XHEADDATA xing_header;
	double tpf, bpf;
	guint32 len;
	long id3v2size = 0;			

	if (!file)
		return -1;

	fseek(file, 0, SEEK_SET);
	if (fread(tmp, 1, 4, file) != 4)
		return 0;

    // Skip data of the ID3v2.x tag (patch from Artur Polaczynski)
	if (tmp[0] == 'I' && tmp[1] == 'D' && tmp[2] == '3' && tmp[3] < 0xFF)
	{
		// id3v2 tag skipeer $49 44 33 yy yy xx zz zz zz zz [zz size]
		fseek(file, 2, SEEK_CUR); // Size is 6-9 position
		if (fread(tmp, 1, 4, file) != 4)
			return 0;
		id3v2size = 10 + ( (long)(tmp[3]) | ((long)(tmp[2]) << 7) | ((long)(tmp[1]) << 14) | ((long)(tmp[0]) << 21) );
		fseek(file, id3v2size, SEEK_SET);
		if (fread(tmp, 1, 4, file) != 4) // Read mpeg header
			return 0;
	}

	head = convert_to_header(tmp);
	while (!mpg123_head_check(head))
	{
		head <<= 8;
		if (fread(tmp, 1, 1, file) != 1)
			return 0;
		head |= tmp[0];
	}
	if (mpg123_decode_header(&frm, head))
	{
		buf = g_malloc(frm.framesize + 4);
		fseek(file, -4, SEEK_CUR);
		fread(buf, 1, frm.framesize + 4, file);
		xing_header.toc = NULL;
		tpf = mpg123_compute_tpf(&frm);
		if (mpg123_get_xing_header(&xing_header, buf))
		{
			g_free(buf);
			return ((guint) (tpf * xing_header.frames * 1000));
		}
		g_free(buf);
		bpf = mpg123_compute_bpf(&frm);
		fseek(file, 0, SEEK_END);
		len = ftell(file) - id3v2size;
		fseek(file, -128, SEEK_END);
		fread(tmp, 1, 3, file);
		if (!strncmp((gchar *)tmp, "TAG", 3))
			len -= 128;
		return ((guint) ((guint)(len / bpf) * tpf * 1000));
	}
	return 0;
}

