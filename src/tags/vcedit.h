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

#ifndef __VCEDIT_H
#define __VCEDIT_H

#include "config.h"

#ifdef ENABLE_OGG

G_BEGIN_DECLS

#include <stdio.h>
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

#define VCEDIT_IS_UNKNOWN   0
#define VCEDIT_IS_SPEEX     1
#define VCEDIT_IS_OGGVORBIS 2
#define VCEDIT_IS_OPUS      3

typedef struct {
    ogg_sync_state      *oy;
    ogg_stream_state    *os;

    int                 oggtype; // Stream is Vorbis or Speex?
    vorbis_comment      *vc;
    vorbis_info         *vi;
#ifdef ENABLE_SPEEX
    SpeexHeader         *si;
#endif
#ifdef ENABLE_OPUS
    OpusHead            *oi;
#endif
    GFileInputStream    *in;
    long        serial;
    unsigned char   *mainbuf;
    unsigned char   *bookbuf;
    int     mainlen;
    int     booklen;
    char   *vendor;
    int prevW;
    int extrapage;
    int eosin;
} vcedit_state;

extern vcedit_state    *vcedit_new_state(void);
extern void             vcedit_clear(vcedit_state *state);
extern vorbis_comment  *vcedit_comments(vcedit_state *state);
extern int              vcedit_open(vcedit_state *state, GFile *in, GError **error);
extern int              vcedit_write(vcedit_state *state, GFile *file, GError **error);

G_END_DECLS

#endif /* ENABLE_OGG */

#endif /* __VCEDIT_H */
