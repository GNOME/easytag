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

#ifndef ET_GIO_WRAPPER_H_
#define ET_GIO_WRAPPER_H_

#include "config.h"

#ifdef ENABLE_MP4

#include <tiostream.h>
#include <gio/gio.h>

class GIO_InputStream : public TagLib::IOStream
{
public:
    GIO_InputStream (GFile *file_);
    virtual ~GIO_InputStream ();
    virtual TagLib::FileName name () const;
    virtual TagLib::ByteVector readBlock (size_t length);
    virtual void writeBlock (TagLib::ByteVector const &data);
    virtual void insert (TagLib::ByteVector const &data, TagLib::offset_t start = 0, size_t replace = 0);
    virtual void removeBlock (TagLib::offset_t start = 0, size_t length = 0);
    virtual bool readOnly () const;
    virtual bool isOpen () const;
    virtual void seek (TagLib::offset_t offset, TagLib::IOStream::Position p = TagLib::IOStream::Beginning);
    virtual void clear ();
    virtual TagLib::offset_t tell () const;
    virtual TagLib::offset_t length ();
    virtual void truncate (TagLib::offset_t length);

    virtual const GError *getError() const;

private:
    GIO_InputStream (const GIO_InputStream &other);
    GFile *file;
    GFileInputStream *stream;
    char *filename;
    GError *error;
};

class GIO_IOStream : public TagLib::IOStream
{
public:
    GIO_IOStream (GFile *file_);
    virtual ~GIO_IOStream ();
    virtual TagLib::FileName name () const;
    virtual TagLib::ByteVector readBlock (size_t length);
    virtual void writeBlock (TagLib::ByteVector const &data);
    virtual void insert (TagLib::ByteVector const &data, TagLib::offset_t start = 0, size_t replace = 0);
    virtual void removeBlock (TagLib::offset_t start = 0, size_t len = 0);
    virtual bool readOnly () const;
    virtual bool isOpen () const;
    virtual void seek (TagLib::offset_t offset, TagLib::IOStream::Position p = TagLib::IOStream::Beginning);
    virtual void clear ();
    virtual TagLib::offset_t tell () const;
    virtual TagLib::offset_t length ();
    virtual void truncate (TagLib::offset_t length);

    virtual const GError *getError() const;

private:
    GIO_IOStream (const GIO_IOStream &other);
    GFile *file;
    GFileIOStream *stream;
    char *filename;
    GError *error;
};

#endif /* ENABLE_MP4 */

#endif /* ET_GIO_WRAPPER_H_ */
