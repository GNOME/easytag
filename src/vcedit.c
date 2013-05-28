/* This program is licensed under the GNU Library General Public License, version 2,
 * a copy of which is included with this program (LICENCE.LGPL).
 *
 * (c) 2000-2001 Michael Smith <msmith@xiph.org>
 *
 *
 * Comment editing backend, suitable for use by nice frontend interfaces.
 *
 * last modified: $Id: vcedit.c,v 1.23 2003/09/03 07:58:05 calc Exp $
 */

#include "config.h" /* For definition of ENABLE_OGG. */

#ifdef ENABLE_OGG
#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "vcedit.h"
#include "ogg_header.h"

#define CHUNKSIZE 4096

vcedit_state *vcedit_new_state(void)
{
    vcedit_state *state = malloc(sizeof(vcedit_state));
    memset(state, 0, sizeof(vcedit_state));
    state->oggtype = VCEDIT_IS_UNKNOWN;

    return state;
}

vorbis_comment *vcedit_comments(vcedit_state *state)
{
    return state->vc;
}

static void vcedit_clear_internals(vcedit_state *state)
{
    if(state->vc)
    {
        vorbis_comment_clear(state->vc);
        free(state->vc);
    }
    if(state->os)
    {
        ogg_stream_clear(state->os);
        free(state->os);
    }
    if(state->oy)
    {
        ogg_sync_clear(state->oy);
        free(state->oy);
    }
    if(state->vendor)
        free(state->vendor);
    if(state->mainbuf)
        free(state->mainbuf);
    if(state->bookbuf)
        free(state->bookbuf);
    if(state->vi) {
        vorbis_info_clear(state->vi);
        free(state->vi);
    }
    g_object_unref (state->in);
    memset(state, 0, sizeof(*state));
}

void vcedit_clear(vcedit_state *state)
{
    if(state)
    {
        vcedit_clear_internals(state);
        free(state);
    }
}

/* Next two functions pulled straight from libvorbis, apart from one change
 * - we don't want to overwrite the vendor string.
 */
static void _v_writestring(oggpack_buffer *o,char *s, int len)
{
    while(len--)
    {
        oggpack_write(o,*s++,8);
    }
}

static int _commentheader_out(vcedit_state *state, ogg_packet *op)
{
    vorbis_comment *vc = state->vc;
    char           *vendor = state->vendor;
    oggpack_buffer opb;

    oggpack_writeinit(&opb);

    if (state->oggtype == VCEDIT_IS_OGGVORBIS)
    {
        /* preamble */
        oggpack_write(&opb,0x03,8);
        _v_writestring(&opb,"vorbis", 6);
    }

    /* vendor */
    oggpack_write(&opb,strlen(vendor),32);
    _v_writestring(&opb,vendor, strlen(vendor));

    /* comments */
    oggpack_write(&opb,vc->comments,32);
    if(vc->comments){
        int i;
        for(i=0;i<vc->comments;i++){
            if(vc->user_comments[i]){
                oggpack_write(&opb,vc->comment_lengths[i],32);
                _v_writestring(&opb,vc->user_comments[i],
                               vc->comment_lengths[i]);
            }else{
                oggpack_write(&opb,0,32);
            }
        }
    }
    oggpack_write(&opb,1,1);

    op->packet = _ogg_malloc(oggpack_bytes(&opb));
    memcpy(op->packet, opb.buffer, oggpack_bytes(&opb));

    op->bytes=oggpack_bytes(&opb);
    op->b_o_s=0;
    op->e_o_s=0;
    op->granulepos=0;
    if (state->oggtype == VCEDIT_IS_OGGVORBIS)
    {
        op->packetno = 1;
    }

    oggpack_writeclear(&opb);
    return 0;
}

static int _blocksize(vcedit_state *s, ogg_packet *p)
{
    int this = vorbis_packet_blocksize(s->vi, p);
    int ret = (this + s->prevW)/4;

    if(!s->prevW)
    {
        s->prevW = this;
        return 0;
    }

    s->prevW = this;
    return ret;
}

static gboolean
_fetch_next_packet (vcedit_state *s, ogg_packet *p, ogg_page *page,
                    GError **error)
{
    int result;
    char *buffer;
    gssize bytes;

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    result = ogg_stream_packetout(s->os, p);

    if(result > 0)
        return TRUE;
    else
    {
        if(s->eosin)
        {
            g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_EOS,
                         "Page reached end of logical bitstream");
            g_assert (error == NULL || *error != NULL);
            return FALSE;
        }
        while(ogg_sync_pageout(s->oy, page) <= 0)
        {
            buffer = ogg_sync_buffer(s->oy, CHUNKSIZE);
            bytes = g_input_stream_read (G_INPUT_STREAM (s->in), buffer,
                                         CHUNKSIZE, NULL, error);
            ogg_sync_wrote(s->oy, bytes);
            if(bytes == 0)
            {
                g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_EOF,
                             "Reached end of file");
                g_assert (error == NULL || *error != NULL);
                return FALSE;
            }
            else if (bytes == -1)
            {
                g_assert (error == NULL || *error != NULL);
                return FALSE;
            }
        }
        if(ogg_page_eos(page))
            s->eosin = 1;
        else if(ogg_page_serialno(page) != s->serial)
        {
            g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_SN,
                         "Page serial number and state serial number doesn't match");
            s->eosin = 1;
            s->extrapage = 1;
            g_assert (error == NULL || *error != NULL);
            return FALSE;
        }
        g_assert (error == NULL || *error == NULL);
        ogg_stream_pagein(s->os, page);
        return _fetch_next_packet(s, p, page, error);
    }
}

/*
 * Next functions pulled straight from libvorbis,
 */
static void _v_readstring(oggpack_buffer *o,char *buf,int bytes){
  while(bytes--){
    *buf++=oggpack_read(o,8);
  }
}

/*
 * Next functions pulled straight from libvorbis, apart from one change
 * - disabled the EOP check
 */
static int _speex_unpack_comment(vorbis_comment *vc,oggpack_buffer *opb)
{
    int i;
    int vendorlen=oggpack_read(opb,32);
    if (vendorlen<0) 
        goto err_out;

    vc->vendor=_ogg_calloc(vendorlen+1,1);
    _v_readstring(opb,vc->vendor,vendorlen);

    vc->comments=oggpack_read(opb,32);
    if (vc->comments<0)
        goto err_out;
    vc->user_comments=_ogg_calloc(vc->comments+1,sizeof(*vc->user_comments));
    vc->comment_lengths=_ogg_calloc(vc->comments+1, sizeof(*vc->comment_lengths));
    for (i=0;i<vc->comments;i++){
        int len=oggpack_read(opb,32);
        if (len<0)
            goto err_out;
        vc->comment_lengths[i]=len;
        vc->user_comments[i]=_ogg_calloc(len+1,1);
        _v_readstring(opb,vc->user_comments[i],len);
    }
    //if(oggpack_read(opb,1)!=1)goto err_out; /* EOP check */
    return(0);

err_out:
    vorbis_comment_clear(vc);
    return(1);
}

gboolean
vcedit_open(vcedit_state *state, GFile *file, GError **error)
{
    char *buffer;
    gssize bytes;
    int i;
    int chunks = 0;
    int headerpackets = 0;
    oggpack_buffer opb;
    ogg_packet *header;
    ogg_packet  header_main;
    ogg_packet  header_comments;
    ogg_packet  header_codebooks;
    ogg_page    og;
    GFileInputStream *istream;

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    istream = g_file_read (file, NULL, error);
    if (!istream)
    {
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    state->in = istream;

    state->oy = malloc(sizeof(ogg_sync_state));
    ogg_sync_init(state->oy);

    while(1)
    {
        buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);
        bytes = g_input_stream_read (G_INPUT_STREAM (state->in), buffer,
                                     CHUNKSIZE, NULL, error);
        if (bytes == -1)
        {
            goto err;
        }

        ogg_sync_wrote(state->oy, bytes);

        if(ogg_sync_pageout(state->oy, &og) == 1)
            break;

        if(chunks++ >= 10) /* Bail if we don't find data in the first 40 kB */
        {
            if(bytes<CHUNKSIZE)
                g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_TRUNC,
                             "Input truncated or empty");
            else
                g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_NOTOGG,
                             "Input is not an OGG bitstream");
            goto err;
        }
    }

    state->serial = ogg_page_serialno(&og);

    state->os = malloc(sizeof(ogg_stream_state));
    ogg_stream_init(state->os, state->serial);

    state->vi = malloc(sizeof(vorbis_info));
    vorbis_info_init(state->vi);

    state->vc = malloc(sizeof(vorbis_comment));
    vorbis_comment_init(state->vc);

    if(ogg_stream_pagein(state->os, &og) < 0)
    {
        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_PAGE,
                     "Error reading first page of OGG bitstream");
        goto err;
    }

    if(ogg_stream_packetout(state->os, &header_main) != 1)
    {
        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_HEADER,
                     "Error reading initial header packet");
        goto err;
    }

    // We save the main header first, (it seems speex_packet_to_header munge's it??)
    state->mainlen = header_main.bytes;
    state->mainbuf = malloc(state->mainlen);
    memcpy(state->mainbuf, header_main.packet, header_main.bytes);

    state->oggtype = VCEDIT_IS_UNKNOWN;
    if(vorbis_synthesis_headerin(state->vi, state->vc, &header_main) == 0)
    {
        state->oggtype = VCEDIT_IS_OGGVORBIS;
    }
#ifdef ENABLE_SPEEX
    else
    {
        // Done after "Ogg test" to avoid to display an error message in function
        // speex_packet_to_header when the file is not Speex.
        state->si = malloc(sizeof(SpeexHeader));
        if((state->si = speex_packet_to_header((char*)(&header_main)->packet,(&header_main)->bytes)))
            state->oggtype = VCEDIT_IS_SPEEX;
    }
#endif

    if (state->oggtype == VCEDIT_IS_UNKNOWN)
    {
        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_INVALID,
                     "OGG bitstream contains neither Speex nor Vorbis data");
        goto err;
    }

    switch (state->oggtype)
    {
        case VCEDIT_IS_OGGVORBIS:
            header = &header_comments;
            headerpackets = 3;
            break;
#ifdef ENABLE_SPEEX
        case VCEDIT_IS_SPEEX:
            header = &header_comments;
            headerpackets = 2 + state->si->extra_headers;
            break;
#endif
    }
    i = 1;
    while(i<headerpackets)
    {
        while(i<headerpackets)
        {
            int result = ogg_sync_pageout(state->oy, &og);
            if(result == 0) break; /* Too little data so far */
            else if(result == 1)
            {
                ogg_stream_pagein(state->os, &og);
                while(i< headerpackets)
                {
                    result = ogg_stream_packetout(state->os, header);
                    if(result == 0) break;
                    if(result == -1)
                    {
                        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_CORRUPT,
                                     "Corrupt secondary header");
                        goto err;
                    }
                    switch (state->oggtype)
                    {
                        case VCEDIT_IS_OGGVORBIS:
                            vorbis_synthesis_headerin(state->vi, state->vc, header);
                            switch (i) 
                            {
                                // 0 packet was the vorbis header
                                case 1:
                                    header = &header_codebooks;
                                    break;
                                case 2:
                                    state->booklen = header->bytes;
                                    state->bookbuf = malloc(state->booklen);
                                    memcpy(state->bookbuf, header->packet, 
                                           header->bytes);
                                    break;
                            }
                            break;
                        case VCEDIT_IS_SPEEX:
                            switch (i) 
                            {
                                // 0 packet was the speex header
                                case 1:
                                    oggpack_readinit(&opb,header->packet,header->bytes);
                                    _speex_unpack_comment(state->vc,&opb);
                                    break;
                                default: /* FIXME. */
                                    g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_EXTRA,
                                                 "Need to save extra headers");
                                    goto err;
                                    break;
                            }
                    }
                    i++;
                }
            }
        }

        buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);
        bytes = g_input_stream_read (G_INPUT_STREAM (state->in), buffer,
                                     CHUNKSIZE, NULL, error);
        if (bytes == -1)
        {
            goto err;
        }

        if(bytes == 0 && i < 2)
        {
            g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_VORBIS,
                         "EOF before end of Vorbis headers");
            goto err;
        }
        ogg_sync_wrote(state->oy, bytes);
    }

    /* Copy the vendor tag */
    state->vendor = malloc(strlen(state->vc->vendor) +1);
    strcpy(state->vendor, state->vc->vendor);

    /* Headers are done! */
    g_assert (error == NULL || *error == NULL);

    return TRUE;

err:
    g_assert (error == NULL || *error != NULL);
    vcedit_clear_internals(state);
    return FALSE;
}

gboolean
vcedit_write(vcedit_state *state, GFile *file, GError **error)
{
    ogg_stream_state streamout;
    ogg_packet header_main;
    ogg_packet header_comments;
    ogg_packet header_codebooks;

    ogg_page ogout, ogin;
    ogg_packet op;
    ogg_int64_t granpos = 0;
    int result;
    char *buffer;
    int bytes;
    int needflush=0, needout=0;
    GFileOutputStream *ostream;

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    ostream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE,
                              NULL, error);

    if (!ostream)
    {
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    state->eosin = 0;
    state->extrapage = 0;

    header_main.bytes = state->mainlen;
    header_main.packet = state->mainbuf;
    header_main.b_o_s = 1;
    header_main.e_o_s = 0;
    header_main.granulepos = 0;

    header_codebooks.bytes = state->booklen;
    header_codebooks.packet = state->bookbuf;
    header_codebooks.b_o_s = 0;
    header_codebooks.e_o_s = 0;
    header_codebooks.granulepos = 0;

    ogg_stream_init(&streamout, state->serial);

    _commentheader_out(state, &header_comments);

    ogg_stream_packetin(&streamout, &header_main);
    ogg_stream_packetin(&streamout, &header_comments);

    if (state->oggtype != VCEDIT_IS_SPEEX)
        ogg_stream_packetin(&streamout, &header_codebooks);

    while((result = ogg_stream_flush(&streamout, &ogout)))
    {
        if (g_output_stream_write (G_OUTPUT_STREAM (ostream), ogout.header,
                                   ogout.header_len, NULL, error)
            != (gssize) ogout.header_len)
        {
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }

        if (g_output_stream_write (G_OUTPUT_STREAM (ostream), ogout.body,
                                   ogout.body_len, NULL, error)
            != (gssize) ogout.body_len)
        {
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }
    }

    while (_fetch_next_packet (state, &op, &ogin, error))
    {
        if(needflush)
        {
            if (g_output_stream_write (G_OUTPUT_STREAM (ostream), ogout.header,
                                       ogout.header_len, NULL, error)
                != (gssize) ogout.header_len)
            {
                g_assert (error == NULL || *error != NULL);
                goto cleanup;
            }

            if (g_output_stream_write (G_OUTPUT_STREAM (ostream), ogout.body,
                                       ogout.body_len, NULL, error)
                != (gssize) ogout.body_len)
            {
                g_assert (error == NULL || *error != NULL);
                goto cleanup;
            }
        }
        else if(needout)
        {
            if(ogg_stream_pageout(&streamout, &ogout))
            {
                if (g_output_stream_write (G_OUTPUT_STREAM (ostream),
                                           ogout.header, ogout.header_len,
                                           NULL, error)
                    != (gssize) ogout.header_len)
                {
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }

                if (g_output_stream_write (G_OUTPUT_STREAM (ostream),
                                           ogout.body, ogout.body_len, NULL,
                                           error)
                    != (gssize) ogout.body_len)
                {
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }
            }
        }

        needflush=needout=0;

        if (state->oggtype == VCEDIT_IS_OGGVORBIS)
        {
            int size;
            size = _blocksize(state, &op);
            granpos += size;

            if(op.granulepos == -1)
            {
                op.granulepos = granpos;
                ogg_stream_packetin(&streamout, &op);
            }
            else /* granulepos is set, validly. Use it, and force a flush to 
                account for shortened blocks (vcut) when appropriate */ 
            {
                if(granpos > op.granulepos)
                {
                    granpos = op.granulepos;
                    ogg_stream_packetin(&streamout, &op);
                    needflush=1;
                }
                else 
                {
                    ogg_stream_packetin(&streamout, &op);
                    needout=1;
                }
            }
        }
        /* Don't know about granulepos for speex, will just assume the original 
           was appropriate. Not sure about the flushing?? */
        else if (state->oggtype == VCEDIT_IS_SPEEX)
        {
            ogg_stream_packetin(&streamout, &op);
            needout=1;
        }
    }

    if (error == NULL || *error != NULL)
    {
        goto cleanup;
    }

    streamout.e_o_s = 1;
    while(ogg_stream_flush(&streamout, &ogout))
    {
        if (g_output_stream_write (G_OUTPUT_STREAM (ostream), ogout.header,
                                   ogout.header_len, NULL, error)
            != (gssize) ogout.header_len)
        {
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }

        if (g_output_stream_write (G_OUTPUT_STREAM (ostream), ogout.body,
                                   ogout.body_len, NULL, error)
            != (gssize) ogout.body_len)
        {
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }
    }

    if (state->extrapage)
    {
        if (g_output_stream_write (G_OUTPUT_STREAM (ostream), ogout.header,
                                   ogout.header_len, NULL, error)
            != (gssize) ogout.header_len)
        {
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }

        if (g_output_stream_write (G_OUTPUT_STREAM (ostream), ogout.body,
                                   ogout.body_len, NULL, error)
            != (gssize) ogout.body_len)
        {
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }
    }

    state->eosin=0; /* clear it, because not all paths to here do */
    while(!state->eosin) /* We reached eos, not eof */
    {
        /* We copy the rest of the stream (other logical streams)
         * through, a page at a time. */
        while(1)
        {
            result = ogg_sync_pageout(state->oy, &ogout);
            if(result==0)
                break;
            if(result<0)
            {
                g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_FAILED,
                             "Corrupt or missing data, continuing…");
                goto cleanup;
            }
            else
            {
                /* Don't bother going through the rest, we can just
                 * write the page out now */
                if (g_output_stream_write (G_OUTPUT_STREAM (ostream),
                                           ogout.header, ogout.header_len,
                                           NULL, error)
                    != (gssize) ogout.header_len)
                {
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }

                if (g_output_stream_write (G_OUTPUT_STREAM (ostream),
                                           ogout.body, ogout.body_len, NULL,
                                           error)
                    != (gssize) ogout.body_len)
                {
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }
            }
        }
        buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);

        bytes = g_input_stream_read (G_INPUT_STREAM (state->in), buffer,
                                     CHUNKSIZE, NULL, error);

        if (bytes == -1)
        {
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }

        ogg_sync_wrote(state->oy, bytes);
        if(bytes == 0)
        {
            state->eosin = 1;
            break;
        }
    }


cleanup:
    ogg_stream_clear(&streamout);
    ogg_packet_clear(&header_comments);

    free(state->mainbuf);
    free(state->bookbuf);
    state->mainbuf = state->bookbuf = NULL;

    if (!state->eosin)
    {
        if (!error)
        {
            g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_OUTPUT,
                         "Error writing stream to output. Output stream may be corrupted or truncated");
        }
    }

    g_object_unref (ostream);

    if (error == NULL || *error != NULL)
        return FALSE;

    g_assert (error == NULL || *error == NULL);
    return 0;
}

#endif /* ENABLE_OGG */
