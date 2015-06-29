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

#include "config.h"

#ifdef ENABLE_MP4

#include "gio_wrapper.h"

GIO_InputStream::GIO_InputStream (GFile * file_) :
    file ((GFile *)g_object_ref (gpointer (file_))),
    filename (g_file_get_uri (file)),
    error (NULL)
{
    stream = g_file_read (file, NULL, &error);
}

GIO_InputStream::~GIO_InputStream ()
{
    clear ();

    g_clear_object (&stream);
    g_free (filename);
    g_object_unref (file);
}

TagLib::FileName
GIO_InputStream::name () const
{
    return TagLib::FileName (filename);
}

TagLib::ByteVector
GIO_InputStream::readBlock (TagLib::ulong len)
{
    if (error)
    {
        return TagLib::ByteVector::null;
    }

    TagLib::ByteVector rv (len, 0);
    gsize bytes;
    g_input_stream_read_all (G_INPUT_STREAM (stream), (void *)rv.data (),
                             len, &bytes, NULL, &error);

    return rv.resize (bytes);
}

void
GIO_InputStream::writeBlock (TagLib::ByteVector const &data)
{
    g_warning ("%s", "Trying to write to read-only file!");
}

void
GIO_InputStream::insert (TagLib::ByteVector const &data,
                         TagLib::ulong start,
                         TagLib::ulong replace)
{
    g_warning ("%s", "Trying to write to read-only file!");
}

void
GIO_InputStream::removeBlock (TagLib::ulong start, TagLib::ulong len)
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
    if (error)
    {
        return;
    }

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

    g_seekable_seek (G_SEEKABLE (stream), offset, type, NULL, &error);
}

void
GIO_InputStream::clear ()
{
    if (error)
    {
        g_error_free(error);
        error = NULL;
    }
}

long int
GIO_InputStream::tell () const
{
    return g_seekable_tell (G_SEEKABLE (stream));
}

long int
GIO_InputStream::length ()
{
    if (error)
    {
        return -1;
    }

    long int rv = -1;
    GFileInfo *info = g_file_input_stream_query_info (stream,
                                                      G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                                      NULL, &error);
    if (info)
    {
        rv = g_file_info_get_size (info);
        g_object_unref (info);
    }

    return rv;
}

void
GIO_InputStream::truncate (long int len)
{
    g_warning ("%s", "Trying to truncate read-only file");
}

GIO_IOStream::GIO_IOStream (GFile *file_) :
    file ((GFile *)g_object_ref (gpointer (file_))),
    filename (g_file_get_uri (file_)),
    error (NULL)
{
    stream = g_file_open_readwrite (file, NULL, &error);
}

const GError *
GIO_InputStream::getError () const
{
    return error;
}

GIO_IOStream::~GIO_IOStream ()
{
    clear ();

    if (stream)
    {
        g_object_unref (stream);
    }

    g_free (filename);
    g_object_unref (G_OBJECT (file));
}

TagLib::FileName
GIO_IOStream::name () const
{
    return TagLib::FileName (filename);
}

TagLib::ByteVector
GIO_IOStream::readBlock (TagLib::ulong len)
{
    if (error)
    {
        return TagLib::ByteVector::null;
    }

    gsize bytes = 0;
    TagLib::ByteVector rv (len, 0);
    GInputStream *istream = g_io_stream_get_input_stream (G_IO_STREAM (stream));
    g_input_stream_read_all (istream,
                             (void *)rv.data (), len,
                             &bytes,
                             NULL, &error);

    return rv.resize(bytes);
}

void
GIO_IOStream::writeBlock (TagLib::ByteVector const &data)
{
    if (error)
    {
        return;
    }

    gsize bytes_written;
    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (stream));

    if (!g_output_stream_write_all (ostream, data.data (), data.size (),
                                    &bytes_written, NULL, &error))
    {
        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %u bytes of data were "
                 "written", bytes_written, data.size ());
    }
}

void
GIO_IOStream::insert (TagLib::ByteVector const &data,
                      TagLib::ulong start,
                      TagLib::ulong replace)
{
    if (error)
    {
        return;
    }

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
    GFile *tmp = g_file_new_tmp ("easytag-XXXXXX", &tstr, &error);

    if (tmp == NULL)
    {
        return;
    }

    char buffer[4096];
    gsize r;

    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (tstr));
    GInputStream *istream = g_io_stream_get_input_stream (G_IO_STREAM (stream));

    seek (0);

    while (g_input_stream_read_all (istream, buffer,
                                    MIN (G_N_ELEMENTS (buffer), start),
                                    &r, NULL, &error) && r > 0)
    {
        gsize w;
        g_output_stream_write_all (ostream, buffer, r, &w, NULL, &error);

        if (w != r)
        {
            g_warning ("%s", "Unable to write all bytes");
        }

	if (error)
	{
            g_object_unref (tstr);
            g_object_unref (tmp);
            return;
	}

        start -= r;
    }

    if (error)
    {
        g_object_unref (tstr);
        g_object_unref (tmp);
        return;
    }

    g_output_stream_write_all (ostream, data.data (), data.size (), NULL,
                               NULL, &error);
    seek (replace, TagLib::IOStream::Current);

    if (error)
    {
        g_object_unref (tstr);
        g_object_unref (tmp);
        return;
    }

    while (g_input_stream_read_all (istream, buffer, sizeof (buffer), &r,
                                    NULL, &error) && r > 0)
    {
        gsize bytes_written;

        if (!g_output_stream_write_all (ostream, buffer, r, &bytes_written,
                                        NULL, &error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %" G_GSIZE_FORMAT
                     " bytes of data were written", bytes_written, r);
            g_object_unref (tstr);
            g_object_unref (tmp);
            return;
        }
    }

    g_object_unref (tstr);
    g_object_unref (stream);
    stream = NULL;

    g_file_move (tmp, file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);

    if (error)
    {
            g_object_unref (tmp);
	    return;
    }

    stream = g_file_open_readwrite (file, NULL, &error);

    g_object_unref (tmp);
}

void
GIO_IOStream::removeBlock (TagLib::ulong start, TagLib::ulong len)
{
    if (start + len >= (TagLib::ulong)length ())
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
        gsize bytes_written;

        seek (start);

        if (!g_output_stream_write_all (ostream, buffer, r, &bytes_written,
                                        NULL, &error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %" G_GSIZE_FORMAT
                     " bytes of data were written", bytes_written, r);
            return;
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
    if (error)
    {
        return;
    }

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

    g_seekable_seek (G_SEEKABLE (stream), offset, type, NULL, &error);
}

void
GIO_IOStream::clear ()
{
    g_clear_error (&error);
}

long int
GIO_IOStream::tell () const
{
    return g_seekable_tell (G_SEEKABLE (stream));
}

long int
GIO_IOStream::length ()
{
    long rv = -1;

    if (error)
    {
	return rv;
    }

    GFileInfo *info = g_file_io_stream_query_info (stream,
                                                   G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                                   NULL, &error);

    if (info)
    {
        rv = g_file_info_get_size (info);
        g_object_unref (info);
    }

    return rv;
}

void
GIO_IOStream::truncate (long int len)
{
    if (error)
    {
        return;
    }

    g_seekable_truncate (G_SEEKABLE (stream), len, NULL, &error);
}

const GError *GIO_IOStream::getError () const
{
    return error;
}

#endif /* ENABLE_MP4 */
