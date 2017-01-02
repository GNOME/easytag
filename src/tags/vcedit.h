/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2001 Michael Smith <msmith@xiph.org>
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

#ifndef ET_VCEDIT_H_
#define ET_VCEDIT_H_

#include "config.h"

#include <gio/gio.h>

G_BEGIN_DECLS

/* Ogg Vorbis fields names in (most of) ASCII, excluding '=':
 * http://www.xiph.org/vorbis/doc/v-comment.html
 *
 * Field names :
 *
 * TITLE        : Track/Work name
 * ALBUM        : The collection name to which this track belongs
 * TRACKNUMBER  : The track number of this piece if part of a specific larger collection or album
 * ARTIST       : The artist generally considered responsible for the work. In popular music this is usually the performing band or singer. For classical music it would be the composer. For an audio book it would be the author of the original text.
 * ALBUMARTIST  : The compilation artist or overall artist of an album
 * PERFORMER    : The artist(s) who performed the work. In classical music this would be the conductor, orchestra, soloists. In an audio book it would be the actor who did the reading. In popular music this is typically the same as the ARTIST and is omitted.
 * COPYRIGHT    : Copyright attribution, e.g., '2001 Nobody's Band' or '1999 Jack Moffitt'
 * DESCRIPTION  : A short text description of the contents
 * COMMENT      : old (pre-specification) version of DESCRIPTION
 * GENRE        : A short text indication of music genre
 * DATE         : Date the track was recorded
 * LOCATION     : Location where track was recorded
 * CONTACT      : Contact information for the creators or distributors of the track. This could be a URL, an email address, the physical address of the producing label.
 * ISRC         : ISRC number for the track; see the ISRC intro page for more information on ISRC numbers.
 * DISCNUMBER  : if part of a multi-disc album, put the disc number here
 * TRACKTOTAL  :
 * ENCODED-BY  : The person who encoded the Ogg file. May include contact information such as email address and phone number.
 * COMPOSER    : composer of the work. eg, Gustav Mahler
 * PERFORMER   : individual performers singled out for mention; eg, Yoyo Ma (violinist)
 */

/* Vorbis comment field titles that are shown in the UI. */
#define ET_VORBIS_COMMENT_FIELD_ARTIST "ARTIST"
#define ET_VORBIS_COMMENT_FIELD_ALBUM_ARTIST "ALBUMARTIST"
#define ET_VORBIS_COMMENT_FIELD_ALBUM "ALBUM"
#define ET_VORBIS_COMMENT_FIELD_TITLE "TITLE"
#define ET_VORBIS_COMMENT_FIELD_DISC_NUMBER "DISCNUMBER"
#define ET_VORBIS_COMMENT_FIELD_DISC_TOTAL "DISCTOTAL"
#define ET_VORBIS_COMMENT_FIELD_DATE "DATE"
#define ET_VORBIS_COMMENT_FIELD_TRACK_NUMBER "TRACKNUMBER"
#define ET_VORBIS_COMMENT_FIELD_TRACK_TOTAL "TRACKTOTAL"
#define ET_VORBIS_COMMENT_FIELD_GENRE "GENRE"
/* Comment and description are synonymous. */
#define ET_VORBIS_COMMENT_FIELD_COMMENT "COMMENT"
#define ET_VORBIS_COMMENT_FIELD_DESCRIPTION "DESCRIPTION"
#define ET_VORBIS_COMMENT_FIELD_COMPOSER "COMPOSER"
/* FIXME https://bugzilla.gnome.org/show_bug.cgi?id=690299 */
#define ET_VORBIS_COMMENT_FIELD_PERFORMER "PERFORMER"
#define ET_VORBIS_COMMENT_FIELD_COPYRIGHT "COPYRIGHT"
/* Mapped to URL field in the UI. */
#define ET_VORBIS_COMMENT_FIELD_CONTACT "CONTACT"
#define ET_VORBIS_COMMENT_FIELD_ENCODED_BY "ENCODED-BY"
/* Unofficial Ogg cover art fields. */
#define ET_VORBIS_COMMENT_FIELD_COVER_ART "COVERART"
#define ET_VORBIS_COMMENT_FIELD_COVER_ART_TYPE "COVERARTTYPE"
#define ET_VORBIS_COMMENT_FIELD_COVER_ART_MIME "COVERARTMIME"
#define ET_VORBIS_COMMENT_FIELD_COVER_ART_DESCRIPTION "COVERARTDESCRIPTION"
#define ET_VORBIS_COMMENT_FIELD_METADATA_BLOCK_PICTURE "METADATA_BLOCK_PICTURE"

#ifdef ENABLE_OGG

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#ifdef ENABLE_OPUS
#include <opus/opus.h>
#include <opus/opusfile.h>
#endif

#ifdef ENABLE_SPEEX
#include <speex/speex.h>
#include <speex/speex_header.h>
#endif

/* EtOggKind:
 * @ET_OGG_KIND_VORBIS: Vorbis audio
 * @ET_OGG_KIND_SPEEX: Speex audio
 * @ET_OGG_KIND_OPUS: Opus audio
 * @ET_OGG_KIND_UNKNOWN: unknown Ogg stream
 * @ET_OGG_KIND_UNSUPPORTED: invalid or unsupported Ogg stream
 */
typedef enum
{
    ET_OGG_KIND_VORBIS,
    ET_OGG_KIND_SPEEX,
    ET_OGG_KIND_OPUS,
    ET_OGG_KIND_UNKNOWN,
    ET_OGG_KIND_UNSUPPORTED
} EtOggKind;

typedef struct _EtOggState EtOggState;

EtOggState *vcedit_new_state (void);
void vcedit_clear (EtOggState *state);
vorbis_comment * vcedit_comments (EtOggState *state);
#ifdef ENABLE_SPEEX
const SpeexHeader * vcedit_speex_header (EtOggState *state);
#endif /* ENABLE_SPEEX */
int vcedit_open (EtOggState *state, GFile *in, GError **error);
int vcedit_write (EtOggState *state, GFile *file, GError **error);

#endif /* ENABLE_OGG */

G_END_DECLS

#endif /* ET_VCEDIT_H_ */
