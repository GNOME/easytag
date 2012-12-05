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

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdio.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#ifdef ENABLE_SPEEX
#include <speex/speex.h>
#include <speex/speex_header.h>
#endif

#define VCEDIT_IS_UNKNOWN   0
#define VCEDIT_IS_SPEEX     1
#define VCEDIT_IS_OGGVORBIS 2

typedef size_t (*vcedit_read_func)(void *, size_t, size_t, void *);
typedef size_t (*vcedit_write_func)(const void *, size_t, size_t, void *);

typedef struct {
    ogg_sync_state      *oy;
    ogg_stream_state    *os;

    int                 oggtype; // Stream is Vorbis or Speex?
    vorbis_comment      *vc;
    vorbis_info         *vi;
#ifdef ENABLE_SPEEX
    SpeexHeader         *si;
#endif

    vcedit_read_func read;
    vcedit_write_func write;

    void        *in;
    long        serial;
    unsigned char   *mainbuf;
    unsigned char   *bookbuf;
    int     mainlen;
    int     booklen;
    char   *lasterror;
    char   *vendor;
    int prevW;
    int extrapage;
    int eosin;
} vcedit_state;

extern vcedit_state    *vcedit_new_state(void);
extern void             vcedit_clear(vcedit_state *state);
extern vorbis_comment  *vcedit_comments(vcedit_state *state);
extern int              vcedit_open(vcedit_state *state, FILE *in);
extern int              vcedit_write(vcedit_state *state, void *out);
extern char            *vcedit_error(vcedit_state *state);

#ifdef __cplusplus
}
#endif 

#endif /* __VCEDIT_H */

