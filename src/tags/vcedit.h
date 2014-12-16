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

#ifdef ENABLE_OGG

#include <gio/gio.h>

G_BEGIN_DECLS

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

typedef enum
{
    ET_OGG_KIND_VORBIS,
    ET_OGG_KIND_SPEEX,
    ET_OGG_KIND_OPUS,
    ET_OGG_KIND_UNKNOWN
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

G_END_DECLS

#endif /* ENABLE_OGG */

#endif /* ET_VCEDIT_H_ */
