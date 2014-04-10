/*
 *  EasyTAG - Tag editor for audio files
 *  Copyright (C) 2014  Santtu Lakkala <inz@inz.fi>
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

#include "gio_wrapper.h"

GIO_InputStream::GIO_InputStream (GFile * file_) :
    file ((GFile *)g_object_ref (gpointer (file_))),
    filename (g_file_get_uri (file))
{
    stream = g_file_read (file, NULL, NULL);
}

GIO_InputStream::~GIO_InputStream ()
{
    g_input_stream_close (G_INPUT_STREAM (stream), NULL, NULL);
    g_free (filename);
    g_object_unref (G_OBJECT (file));
}

TagLib::FileName
GIO_InputStream::name () const
{
    return TagLib::FileName (filename);
}

TagLib::ByteVector
GIO_InputStream::readBlock (ulong length)
{
    TagLib::ByteVector rv (length, 0);
    rv = rv.mid (0, g_input_stream_read (G_INPUT_STREAM (stream),
                                         (void *)rv.data (), length, NULL,
                                         NULL));

    return rv;
}

void
GIO_InputStream::writeBlock (TagLib::ByteVector const &data)
{
    g_warning ("%s", "Trying to write to read-only file!");
}

void
GIO_InputStream::insert (TagLib::ByteVector const &data, ulong start, ulong replace)
{
    g_warning ("%s", "Trying to write to read-only file!");
}

void
GIO_InputStream::removeBlock (ulong start, ulong length)
{
    g_warning ("%s", "Trying to write to read-only file!");
}

bool
GIO_InputStream::readOnly () const
{
    return true;
}

bool
GIO_InputStream::isOpen () const
{
    return !!stream;
}

void
GIO_InputStream::seek (long int offset, TagLib::IOStream::Position p)
{
    GSeekType type;

    switch (p)
    {
        case TagLib::IOStream::Beginning:
            type = G_SEEK_SET;
            break;
        case TagLib::IOStream::Current:
            type = G_SEEK_CUR;
            break;
        case TagLib::IOStream::End:
            type = G_SEEK_END;
            break;
        default:
            g_warning ("Unknown seek");
            return;
    }

    g_seekable_seek (G_SEEKABLE (stream), offset, type, NULL, NULL);
}

void
GIO_InputStream::clear ()
{
}

long int
GIO_InputStream::tell () const
{
    return g_seekable_tell (G_SEEKABLE (stream));
}

long int
GIO_InputStream::length ()
{
    long rv;
    GFileInfo *info = g_file_input_stream_query_info (stream,
                                                      G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                                      NULL, NULL);
    rv = g_file_info_get_size (info);
    g_object_unref (info);

    return rv;
}

void
GIO_InputStream::truncate (long int length)
{
    g_warning ("%s", "Trying to truncate read-only file");
}

GIO_IOStream::GIO_IOStream (GFile *file_) :
    file ((GFile *)g_object_ref (gpointer (file_))),
    filename (g_file_get_uri (file_))
{
    stream = g_file_open_readwrite (file, NULL, NULL);
}

GIO_IOStream::~GIO_IOStream ()
{
    g_io_stream_close (G_IO_STREAM (stream), NULL, NULL);
    g_free (filename);
    g_object_unref (G_OBJECT (file));
}

TagLib::FileName
GIO_IOStream::name () const
{
    return TagLib::FileName (filename);
}

TagLib::ByteVector
GIO_IOStream::readBlock (ulong length)
{
    TagLib::ByteVector rv (length, 0);
    GInputStream *istream = g_io_stream_get_input_stream (G_IO_STREAM (stream));
    rv = rv.mid (0, g_input_stream_read (istream, (void *)rv.data (), length,
                                         NULL, NULL));

    return rv;
}

void
GIO_IOStream::writeBlock (TagLib::ByteVector const &data)
{
    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (stream));
    g_output_stream_write (ostream, data.data (), data.size (), NULL, NULL);
}

void
GIO_IOStream::insert (TagLib::ByteVector const &data,
                      ulong start,
                      ulong replace)
{
    if (data.size () == replace)
    {
        seek (start, TagLib::IOStream::Beginning);
        writeBlock (data);
        return;
    }
    else if (data.size () < replace)
    {
        removeBlock (start, replace - data.size ());
        seek (start);
        writeBlock (data);
        return;
    }

    GFileIOStream *tstr;
    GFile *tmp = g_file_new_tmp ("easytag-XXXXXX", &tstr, NULL);
    char *buffer[4096];
    gsize r;
    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (tstr));
    GInputStream *istream = g_io_stream_get_input_stream (G_IO_STREAM (stream));

    seek (0);

    while (g_input_stream_read_all (istream, buffer,
                                    MIN (sizeof (buffer), start), &r, NULL,
                                    NULL) && r > 0)
    {
        gsize w;
        g_output_stream_write_all (ostream, buffer, r, &w, NULL, NULL);

        if (w != r)
        {
            g_warning ("%s", "Unable to write all bytes");
        }

        start -= r;
        g_warning ("Wrote %lu bytes of %lu: %.*s", r, start, (int)r, buffer);
    }

    g_output_stream_write (ostream, data.data (), data.size (), NULL, NULL);
    seek (replace, TagLib::IOStream::Current);

    while (g_input_stream_read_all (istream, buffer, sizeof (buffer), &r,
                                    NULL, NULL) && r > 0)
    {
        gsize w;
        g_output_stream_write_all (ostream, buffer, r, &w, NULL, NULL);

        if (w != r)
        {
            g_warning ("%s", "Unable to write all bytes");
        }
    }

    g_io_stream_close (G_IO_STREAM (tstr), NULL, NULL);
    g_io_stream_close (G_IO_STREAM (stream), NULL, NULL);
    g_file_move (tmp, file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
    g_object_unref (gpointer(tmp));
    stream = g_file_open_readwrite (file, NULL, NULL);
}

void
GIO_IOStream::removeBlock (ulong start, ulong len)
{
    if (start + len >= (ulong)length ())
    {
        truncate (start);
        return;
    }

    char *buffer[4096];
    gsize r;
    GInputStream *istream = g_io_stream_get_input_stream (G_IO_STREAM (stream));
    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (stream));
    seek (start + len);

    while (g_input_stream_read_all (istream, buffer, sizeof (buffer), &r, NULL,
                                    NULL) && r > 0)
    {
        gsize w;
        seek (start);
        g_output_stream_write_all (ostream, buffer, r, &w, NULL, NULL);

        if (w != r)
        {
            g_warning ("%s", "Unable to write all bytes");
        }

        start += r;
        seek (start + len);
    }

    truncate (start);
}

bool
GIO_IOStream::readOnly () const
{
    return !stream;
}

bool
GIO_IOStream::isOpen () const
{
    return !!stream;
}

void
GIO_IOStream::seek (long int offset, TagLib::IOStream::Position p)
{
    GSeekType type;

    switch (p)
    {
        case TagLib::IOStream::Beginning:
            type = G_SEEK_SET;
            break;
        case TagLib::IOStream::Current:
            type = G_SEEK_CUR;
            break;
        case TagLib::IOStream::End:
            type = G_SEEK_END;
            break;
        default:
            g_warning ("%s", "Unknown seek");
            return;
    }

    g_seekable_seek (G_SEEKABLE (stream), offset, type, NULL, NULL);
}

void
GIO_IOStream::clear ()
{
}

long int
GIO_IOStream::tell () const
{
    return g_seekable_tell (G_SEEKABLE (stream));
}

long int
GIO_IOStream::length ()
{
    long rv;
    GFileInfo *info = g_file_io_stream_query_info (stream,
                                                   G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                                   NULL, NULL);
    rv = g_file_info_get_size (info);
    g_object_unref (info);

    return rv;
}

void
GIO_IOStream::truncate (long int length)
{
    g_seekable_truncate (G_SEEKABLE (stream), length, NULL, NULL);
}
