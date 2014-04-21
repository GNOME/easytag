/* This program is licensed under the GNU Library General Public License, version 2,
 * a copy of which is included with this program (with filename LICENSE.LGPL).
 *
 * (c) 2000-2001 Michael Smith <msmith@xiph.org>
 *
 * VCEdit header.
 *
 * last modified: $ID:$
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
