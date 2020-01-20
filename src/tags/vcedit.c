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

#include "config.h" /* For definition of ENABLE_OGG. */

#ifdef ENABLE_OGG
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "vcedit.h"
#include "ogg_header.h"

#define CHUNKSIZE 4096

struct _EtOggState
{
    /*< private >*/
#ifdef ENABLE_SPEEX
    SpeexHeader *si;
#endif
#ifdef ENABLE_OPUS
    OpusHead *oi;
#endif
    vorbis_info *vi;
    ogg_stream_state *os;
    ogg_sync_state *oy;
    vorbis_comment *vc;
    EtOggKind oggtype;
    glong serial;
    guchar *mainbuf;
    guchar *bookbuf;
    gchar *vendor;
    glong mainlen;
    glong booklen;
    glong prevW;
    gboolean extrapage;
    gboolean eosin;
};

EtOggState *
vcedit_new_state (void)
{
    EtOggState *state = g_slice_new0 (EtOggState);
    state->oggtype = ET_OGG_KIND_UNKNOWN;

    return state;
}

vorbis_comment *
vcedit_comments (EtOggState *state)
{
    return state->vc;
}

#ifdef ENABLE_SPEEX
const SpeexHeader *
vcedit_speex_header (EtOggState *state)
{
    return state->si;
}
#endif /* ENABLE_SPEEX */

static void
vcedit_clear_internals (EtOggState *state)
{
    if (state->vc)
    {
        vorbis_comment_clear (state->vc);
        g_slice_free (vorbis_comment, state->vc);
    }

    if (state->os)
    {
        ogg_stream_clear (state->os);
        g_slice_free (ogg_stream_state, state->os);
    }

    if (state->oy)
    {
        ogg_sync_clear (state->oy);
        g_slice_free (ogg_sync_state, state->oy);
    }

    g_free (state->vendor);
    g_free (state->mainbuf);
    g_free (state->bookbuf);

    if (state->vi)
    {
        vorbis_info_clear (state->vi);
        g_slice_free (vorbis_info, state->vi);
    }

#ifdef ENABLE_SPEEX
    if (state->si)
    {
        speex_header_free (state->si);
    }
#endif

#ifdef ENABLE_OPUS
    if (state->oi)
    {
        g_slice_free (OpusHead, state->oi);
    }
#endif /* ENABLE_OPUS */

    memset (state, 0, sizeof (*state));
}

void
vcedit_clear (EtOggState *state)
{
    if (state)
    {
        vcedit_clear_internals (state);
        g_slice_free (EtOggState, state);
    }
}

/* Next two functions pulled straight from libvorbis, apart from one change
 * - we don't want to overwrite the vendor string.
 */
static void
_v_writestring (oggpack_buffer *o,
                const char *s,
                int len)
{
    while (len--)
    {
        oggpack_write (o, *s++, 8);
    }
}

static int
_commentheader_out (EtOggState *state,
                    ogg_packet *op)
{
    vorbis_comment *vc = state->vc;
    const gchar *vendor = state->vendor;
    oggpack_buffer opb;

    oggpack_writeinit (&opb);

    if (state->oggtype == ET_OGG_KIND_VORBIS)
    {
        /* preamble */
        oggpack_write (&opb, 0x03, 8);
        _v_writestring (&opb, "vorbis", 6);
    }
#ifdef ENABLE_OPUS
    else if (state->oggtype == ET_OGG_KIND_OPUS)
    {
        _v_writestring (&opb, "OpusTags", 8);
    }
#endif

    /* vendor */
    oggpack_write (&opb, strlen (vendor), 32);
    _v_writestring (&opb,vendor, strlen (vendor));

    /* comments */
    oggpack_write (&opb, vc->comments, 32);

    if (vc->comments)
    {
        int i;

        for (i=0; i < vc->comments; i++)
        {
            if (vc->user_comments[i])
            {
                oggpack_write (&opb, vc->comment_lengths[i], 32);
                _v_writestring (&opb, vc->user_comments[i],
                                vc->comment_lengths[i]);
            }
            else
            {
                oggpack_write (&opb, 0, 32);
            }
        }
    }

    oggpack_write (&opb, 1, 1);

    op->packet = malloc (oggpack_bytes (&opb));
    memcpy (op->packet, opb.buffer, oggpack_bytes (&opb));

    op->bytes = oggpack_bytes (&opb);
    op->b_o_s = 0;
    op->e_o_s = 0;
    op->granulepos = 0;

    if (state->oggtype == ET_OGG_KIND_VORBIS)
    {
        op->packetno = 1;
    }

    oggpack_writeclear (&opb);
    return 0;
}

static gint64
_blocksize (EtOggState *s,
            ogg_packet *p)
{
    glong this = vorbis_packet_blocksize (s->vi, p);
    gint64 ret = (this + s->prevW) / 4;

    if (!s->prevW)
    {
        s->prevW = this;
        return 0;
    }

    s->prevW = this;
    return ret;
}

static gboolean
_fetch_next_packet (EtOggState *s,
                    GInputStream *istream,
                    ogg_packet *p,
                    ogg_page *page,
                    GError **error)
{
    int result;
    char *buffer;
    gssize bytes;

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    result = ogg_stream_packetout (s->os, p);

    if (result > 0)
    {
        return TRUE;
    }
    else
    {
        if (s->eosin)
        {
            g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_EOS,
                         "Page reached end of logical bitstream");
            g_assert (error == NULL || *error != NULL);
            return FALSE;
        }

        while (ogg_sync_pageout (s->oy, page) <= 0)
        {
            buffer = ogg_sync_buffer (s->oy, CHUNKSIZE);
            bytes = g_input_stream_read (istream, buffer, CHUNKSIZE, NULL,
                                         error);
            ogg_sync_wrote (s->oy, bytes);

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

        if (ogg_page_eos (page))
        {
            s->eosin = TRUE;
        }
        else if (ogg_page_serialno (page) != s->serial)
        {
            g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_SN,
                         "Page serial number and state serial number doesn't match");
            s->eosin = TRUE;
            s->extrapage = TRUE;
            g_assert (error == NULL || *error != NULL);
            return FALSE;
        }

        g_assert (error == NULL || *error == NULL);
        ogg_stream_pagein (s->os, page);
        return _fetch_next_packet (s, istream, p, page, error);
    }
}

/*
 * Next functions pulled straight from libvorbis,
 */
static void
_v_readstring (oggpack_buffer *o,
               char *buf,
               int bytes)
{
    while (bytes--)
    {
        *buf++ = oggpack_read (o, 8);
    }
}

/*
 * Next functions pulled straight from libvorbis, apart from one change
 * - disabled the EOP check
 */
static int
_speex_unpack_comment (vorbis_comment *vc,
                       oggpack_buffer *opb)
{
    int i;
    int vendorlen = oggpack_read (opb, 32);

    if (vendorlen < 0) 
    {
        goto err_out;
    }

    vc->vendor = _ogg_calloc (vendorlen + 1, 1);
    _v_readstring (opb, vc->vendor, vendorlen);

    vc->comments = oggpack_read (opb, 32);

    if (vc->comments < 0)
    {
        goto err_out;
    }

    vc->user_comments = _ogg_calloc (vc->comments + 1,
                                     sizeof (*vc->user_comments));
    vc->comment_lengths = _ogg_calloc (vc->comments + 1,
                                       sizeof (*vc->comment_lengths));

    for (i = 0; i < vc->comments; i++)
    {
        int len = oggpack_read (opb, 32);

        if (len < 0)
        {
            goto err_out;
        }

        vc->comment_lengths[i] = len;
        vc->user_comments[i] = _ogg_calloc (len + 1, 1);
        _v_readstring (opb, vc->user_comments[i], len);
    }

    /*if(oggpack_read(opb,1)!=1)goto err_out; EOP check */ 
    return(0);

err_out:
    vorbis_comment_clear(vc);
    return(1);
}

/* vcedit_supported_stream:
 * @state: current reader state
 * @page: first page of the Ogg stream
 * @error: a #GError to set on failure to find a supported stream
 *
 * Checks the type of the Ogg stream given by @page to see if it is supported.
 *
 * Returns: #ET_OGG_KIND_UNSUPPORTED and sets @error on error, other values
 *          from #EtOggKind otherwise
 */
static EtOggKind
vcedit_supported_stream (EtOggState *state,
                         ogg_page *page,
                         GError **error)
{
    ogg_stream_state stream_state;
    ogg_packet header;
    EtOggKind result;

    ogg_stream_init (&stream_state, ogg_page_serialno (page));

    if (!ogg_page_bos (page))
    {
        /* FIXME: Translatable string. */
        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_BOS,
                     "Beginning of stream packet not found");
        result = ET_OGG_KIND_UNSUPPORTED;
        goto err;
    }

    if (ogg_stream_pagein (&stream_state, page) < 0)
    {
        /* FIXME: Translatable string. */
        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_PAGE,
                     "Error reading first page of Ogg bitstream");
        result = ET_OGG_KIND_UNSUPPORTED;
        goto err;
    }

    if (ogg_stream_packetout (&stream_state, &header) != 1)
    {
        /* FIXME: Translatable string. */
        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_HEADER,
                     "Error reading initial header packet");
        result = ET_OGG_KIND_UNSUPPORTED;
        goto err;
    }

    if (vorbis_synthesis_idheader (&header) > 0)
    {
        result = ET_OGG_KIND_VORBIS;
    }
    else
    {
        result = ET_OGG_KIND_UNKNOWN;

#ifdef ENABLE_SPEEX
        {
            SpeexHeader *speex;

            /* Done after "Ogg test" to avoid displaying an error message in
             * speex_packet_to_header() when the file is not Speex. */
            if ((speex = speex_packet_to_header ((char*)(&header)->packet,
                                                 (&header)->bytes)))
            {
                result = ET_OGG_KIND_SPEEX;
                speex_header_free (speex);
            }
        }
#endif

#ifdef ENABLE_OPUS
        if (result == ET_OGG_KIND_UNKNOWN)
        {
            /* TODO: Check for other return values, such as OP_ENOTFORMAT. */
            if (opus_head_parse (NULL, (unsigned char*)(&header)->packet,
                                 (&header)->bytes) == 0)
            {
                result = ET_OGG_KIND_OPUS;
            }
        }
#endif
    }

err:
    ogg_stream_clear (&stream_state);

    return result;
}

gboolean
vcedit_open (EtOggState *state,
             GFile *file,
             GError **error)
{
    char *buffer;
    gssize bytes;
    guint i;
    guint chunks = 0;
    guint headerpackets = 0;
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

    state->oy = g_slice_new (ogg_sync_state);
    ogg_sync_init (state->oy);

    while(1)
    {
        buffer = ogg_sync_buffer (state->oy, CHUNKSIZE);
        bytes = g_input_stream_read (G_INPUT_STREAM (istream), buffer,
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
                             _("Input truncated or empty"));
            else
                g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_NOTOGG,
                             "Input is not an Ogg bitstream");
            goto err;
        }
    }

    state->serial = ogg_page_serialno(&og);

    state->os = g_slice_new (ogg_stream_state);
    ogg_stream_init (state->os, state->serial);

    state->vi = g_slice_new (vorbis_info);
    vorbis_info_init (state->vi);

    state->vc = g_slice_new (vorbis_comment);
    vorbis_comment_init (state->vc);

    if (ogg_stream_pagein (state->os, &og) < 0)
    {
        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_PAGE,
                     "Error reading first page of Ogg bitstream");
        goto err;
    }

    if (ogg_stream_packetout (state->os, &header_main) != 1)
    {
        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_HEADER,
                     "Error reading initial header packet");
        goto err;
    }

    /* FIXME: This is bogus, as speex_packet_to_header() only reads data from
     * the header packet. */
    /* Save the main header first, it seems speex_packet_to_header() munges
     * it. */
    state->mainlen = header_main.bytes;
    state->mainbuf = g_memdup (header_main.packet, header_main.bytes);

    state->oggtype = vcedit_supported_stream (state, &og, error);

    switch (state->oggtype)
    {
        case ET_OGG_KIND_VORBIS:
            /* TODO: Check for success. */
            vorbis_synthesis_headerin (state->vi, state->vc, &header_main);
            break;
#ifdef ENABLE_SPEEX
        case ET_OGG_KIND_SPEEX:
            /* TODO: Check for success. */
            state->si = speex_packet_to_header ((char*)(&header_main)->packet,
                                                (&header_main)->bytes);
            break;
#endif
#ifdef ENABLE_OPUS
        case ET_OGG_KIND_OPUS:
            state->oi = g_slice_new (OpusHead);

            /* TODO: Check for success. */
            if (opus_head_parse (state->oi, (unsigned char*)(&header_main)->packet,
                                 (&header_main)->bytes)!= 0) {
                g_set_error(error, ET_OGG_ERROR, ET_OGG_ERROR_INVALID,
                            "Failed to parse opus header");
                goto err;
            }
            break;
#endif
#ifndef ENABLE_SPEEX
        case ET_OGG_KIND_SPEEX:
#endif
#ifndef ENABLE_OPUS
        case ET_OGG_KIND_OPUS:
#endif
        case ET_OGG_KIND_UNKNOWN:
        case ET_OGG_KIND_UNSUPPORTED:
            /* TODO: Translatable string. */
            g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_INVALID,
                         "Ogg bitstream contains unknown or unsupported data");
            goto err;
            break;
        default:
            g_assert_not_reached ();
            break;
    }

    switch (state->oggtype)
    {
        case ET_OGG_KIND_VORBIS:
            header = &header_comments;
            headerpackets = 3;
            break;
#ifdef ENABLE_SPEEX
        case ET_OGG_KIND_SPEEX:
            header = &header_comments;
            headerpackets = 2 + state->si->extra_headers;
            break;
#endif
#ifdef ENABLE_OPUS
        case ET_OGG_KIND_OPUS:
            header = &header_comments;
            headerpackets = 2;
#endif
            break;
#ifndef ENABLE_SPEEX
        case ET_OGG_KIND_SPEEX:
#endif
#ifndef ENABLE_OPUS
        case ET_OGG_KIND_OPUS:
#endif
        case ET_OGG_KIND_UNKNOWN:
        case ET_OGG_KIND_UNSUPPORTED:
        default:
            g_assert_not_reached ();
            break;
    }

    i = 1;

    while (i < headerpackets)
    {
        while (i < headerpackets)
        {
            int result = ogg_sync_pageout (state->oy, &og);

            if (result == 0)
            {
                break; /* Too little data so far */
            }
            else if (result == 1)
            {
                ogg_stream_pagein (state->os, &og);

                while (i < headerpackets)
                {
                    result = ogg_stream_packetout(state->os, header);
                    if (result == 0)
                    {
                        break;
                    }

                    if (result == -1)
                    {
                        g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_CORRUPT,
                                     "Corrupt secondary header");
                        goto err;
                    }
                    switch (state->oggtype)
                    {
                        case ET_OGG_KIND_VORBIS:
                            vorbis_synthesis_headerin (state->vi, state->vc,
                                                       header);
                            switch (i) 
                            {
                                /* 0 packet was the Vorbis header. */
                                case 1:
                                    header = &header_codebooks;
                                    break;
                                case 2:
                                    state->booklen = header->bytes;
                                    state->bookbuf = g_memdup (header->packet,
                                                               header->bytes);
                                    break;
                                default:
                                    g_assert_not_reached ();
                                    break;
                            }
                            break;
                        case ET_OGG_KIND_SPEEX:
                            switch (i) 
                            {
                                /* 0 packet was the Speex header. */
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
                            break;
#ifdef ENABLE_OPUS
                        case ET_OGG_KIND_OPUS:
                            switch (opus_tags_parse ((OpusTags *)state->vc,
                                                     header->packet,
                                                     header->bytes))
                            {
                                case 0:
                                    break;

                                case OP_ENOTFORMAT:
                                    g_set_error (error, ET_OGG_ERROR,
                                                 ET_OGG_ERROR_HEADER,
                                                 "Ogg Opus tags do not start with \"OpusTags\"");
                                    goto err;
                                    break;

                                case OP_EFAULT:
                                    g_set_error (error, ET_OGG_ERROR,
                                                 ET_OGG_ERROR_HEADER,
                                                 "Not enough memory to store Ogg Opus tags");
                                    goto err;
                                    break;

                                case OP_EBADHEADER:
                                    g_set_error (error, ET_OGG_ERROR,
                                                 ET_OGG_ERROR_HEADER,
                                                 "Ogg Opus tags do not follow the specification");
                                    goto err;
                                    break;
                                default:
                                    g_assert_not_reached ();
                                    break;
                            }
                            break;
#endif
#ifndef ENABLE_OPUS
                        case ET_OGG_KIND_OPUS:
#endif
                        case ET_OGG_KIND_UNKNOWN:
                        case ET_OGG_KIND_UNSUPPORTED:
                        default:
                            g_assert_not_reached ();
                            break;
                    }

                    i++;
                }
            }
        }

        buffer = ogg_sync_buffer (state->oy, CHUNKSIZE);
        bytes = g_input_stream_read (G_INPUT_STREAM (istream), buffer,
                                     CHUNKSIZE, NULL, error);

        if (bytes == -1)
        {
            goto err;
        }

        if (bytes == 0 && i < 2)
        {
            g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_VORBIS,
                         "EOF before end of Vorbis headers");
            goto err;
        }
        ogg_sync_wrote (state->oy, bytes);
    }

    /* Copy the vendor tag */
    state->vendor = g_strdup (state->vc->vendor);

    /* Headers are done! */
    g_assert (error == NULL || *error == NULL);
    /* TODO: Handle error during stream close. */
    g_object_unref (istream);

    return TRUE;

err:
    g_assert (error == NULL || *error != NULL);
    g_object_unref (istream);
    vcedit_clear_internals (state);
    return FALSE;
}

gboolean
vcedit_write (EtOggState *state,
              GFile *file,
              GError **error)
{
    ogg_stream_state streamout;
    ogg_packet header_main;
    ogg_packet header_comments;
    ogg_packet header_codebooks;

    ogg_page ogout, ogin;
    ogg_packet op;
    ogg_int64_t granpos = 0;
    gint result;
    gchar *buffer;
    glong bytes;
    gboolean needflush = FALSE;
    gboolean needout = FALSE;
    GFileInputStream *istream;
    GOutputStream *ostream;
    gchar *buf;
    gsize size;
    GFileInfo *fileinfo;

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    istream = g_file_read (file, NULL, error);

    if (!istream)
    {
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    fileinfo = g_file_input_stream_query_info (istream,
                                               G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                               NULL, error);

    if (!fileinfo)
    {
        g_assert (error == NULL || *error != NULL);
        g_object_unref (istream);
        return FALSE;
    }

    buf = g_malloc (g_file_info_get_size (fileinfo));
    ostream = g_memory_output_stream_new (buf,
                                          g_file_info_get_size (fileinfo),
                                          g_realloc, g_free);
    g_object_unref (fileinfo);

    state->eosin = FALSE;
    state->extrapage = FALSE;

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

    ogg_stream_init (&streamout, state->serial);

    _commentheader_out (state, &header_comments);

    ogg_stream_packetin (&streamout, &header_main);
    ogg_stream_packetin (&streamout, &header_comments);

    if (state->oggtype == ET_OGG_KIND_VORBIS)
    {
        ogg_stream_packetin (&streamout, &header_codebooks);
    }

    while ((result = ogg_stream_flush (&streamout, &ogout)))
    {
        gsize bytes_written;

        if (!g_output_stream_write_all (ostream, ogout.header,
                                        ogout.header_len, &bytes_written, NULL,
                                        error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes of data "
                     "were written", bytes_written, ogout.header_len);
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }

        if (!g_output_stream_write_all (ostream, ogout.body, ogout.body_len,
                                        &bytes_written, NULL, error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes of data "
                     "were written", bytes_written, ogout.body_len);
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }
    }

    while (_fetch_next_packet (state, G_INPUT_STREAM (istream), &op, &ogin,
                               error))
    {
        if (needflush)
        {
            if (ogg_stream_flush (&streamout, &ogout))
            {
                gsize bytes_written;

                if (!g_output_stream_write_all (ostream, ogout.header,
                                                ogout.header_len, &bytes_written,
                                                NULL, error))
                {
                    g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes of "
                             "data were written", bytes_written, ogout.header_len);
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }

                if (!g_output_stream_write_all (ostream, ogout.body,
                                                ogout.body_len, &bytes_written,
                                                NULL, error))
                {
                    g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes of "
                             "data were written", bytes_written, ogout.body_len);
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }
            }
        }
        else if (needout)
        {
            if(ogg_stream_pageout (&streamout, &ogout))
            {
                gsize bytes_written;

                if (!g_output_stream_write_all (ostream, ogout.header,
                                                ogout.header_len,
                                                &bytes_written, NULL, error))
                {
                    g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes "
                             "of data were written", bytes_written,
                             ogout.header_len);
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }

                if (!g_output_stream_write_all (ostream, ogout.body,
                                                ogout.body_len, &bytes_written,
                                                NULL, error))
                {
                    g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes "
                             "of data were written", bytes_written,
                             ogout.body_len);
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }
            }
        }

        needflush = needout = FALSE;

        if (state->oggtype == ET_OGG_KIND_VORBIS ||
            state->oggtype == ET_OGG_KIND_OPUS)
        {
            if (state->oggtype == ET_OGG_KIND_VORBIS)
            {
                granpos += _blocksize (state, &op);
            }
#ifdef ENABLE_OPUS
            else
            {
                granpos += opus_packet_get_samples_per_frame (op.packet,
                                                              48000);
            }
#endif
            if(op.granulepos == -1)
            {
                op.granulepos = granpos;
                ogg_stream_packetin (&streamout, &op);
            }
            else /* granulepos is set, validly. Use it, and force a flush to 
                account for shortened blocks (vcut) when appropriate */ 
            {
                if (granpos > op.granulepos)
                {
                    granpos = op.granulepos;
                    ogg_stream_packetin (&streamout, &op);
                    needflush = TRUE;
                }
                else 
                {
                    ogg_stream_packetin (&streamout, &op);
                    needout = TRUE;
                }
            }
        }
        /* Don't know about granulepos for speex, will just assume the original 
           was appropriate. Not sure about the flushing?? */
        else if (state->oggtype == ET_OGG_KIND_SPEEX)
        {
            ogg_stream_packetin (&streamout, &op);
            needout = TRUE;
        }
    }

    if (error != NULL)
    {
        if (g_error_matches (*error, ET_OGG_ERROR, ET_OGG_ERROR_EOF)
            || g_error_matches (*error, ET_OGG_ERROR, ET_OGG_ERROR_EOS))
        {
            /* While nominally errors, these are expected and can be safely
             * ignored. */
            g_clear_error (error);
        }
        else
        {
            goto cleanup;
        }
    }

    streamout.e_o_s = 1;

    while (ogg_stream_flush (&streamout, &ogout))
    {
        gsize bytes_written;

        if (!g_output_stream_write_all (ostream, ogout.header,
                                        ogout.header_len, &bytes_written, NULL,
                                        error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes of data "
                     "were written", bytes_written, ogout.header_len);
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }

        if (!g_output_stream_write_all (ostream, ogout.body, ogout.body_len,
                                        &bytes_written, NULL, error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes of data "
                     "were written", bytes_written, ogout.body_len);
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }
    }

    if (state->extrapage)
    {
        gsize bytes_written;

        if (!g_output_stream_write_all (ostream, ogout.header,
                                        ogout.header_len, &bytes_written, NULL,
                                        error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes of data "
                     "were written", bytes_written, ogout.header_len);
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }

        if (!g_output_stream_write_all (ostream, ogout.body, ogout.body_len,
                                        &bytes_written, NULL, error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %ld bytes of data "
                     "were written", bytes_written, ogout.body_len);
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }
    }

    state->eosin = FALSE; /* clear it, because not all paths to here do */

    while (!state->eosin) /* We reached eos, not eof */
    {
        /* We copy the rest of the stream (other logical streams)
         * through, a page at a time. */
        while (1)
        {
            result = ogg_sync_pageout (state->oy, &ogout);

            if (result == 0)
            {
                break;
            }

            if (result < 0)
            {
                g_debug ("%s", "Corrupt or missing data, continuing");
            }
            else
            {
                gsize bytes_written;

                /* Don't bother going through the rest, we can just
                 * write the page out now */
                if (!g_output_stream_write_all (ostream, ogout.header,
                                                ogout.header_len,
                                                &bytes_written, NULL, error))
                {
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }

                if (!g_output_stream_write_all (ostream, ogout.body,
                                                ogout.body_len, &bytes_written,
                                                NULL, error))
                {
                    g_assert (error == NULL || *error != NULL);
                    goto cleanup;
                }
            }
        }

        buffer = ogg_sync_buffer (state->oy, CHUNKSIZE);

        bytes = g_input_stream_read (G_INPUT_STREAM (istream), buffer,
                                     CHUNKSIZE, NULL, error);

        if (bytes == -1)
        {
            g_assert (error == NULL || *error != NULL);
            goto cleanup;
        }

        ogg_sync_wrote (state->oy, bytes);

        if (bytes == 0)
        {
            state->eosin = TRUE;
            break;
        }
    }

cleanup:
    ogg_stream_clear (&streamout);
    ogg_packet_clear (&header_comments);

    if (!g_input_stream_close (G_INPUT_STREAM (istream), NULL, error))
    {
        /* Ignore the _close() failure, and try the write anyway. */
        g_warning ("Error closing Ogg file for reading: %s",
                   (*error)->message);
        g_clear_error (error);
    }

    g_object_unref (istream);
    g_free (state->mainbuf);
    g_free (state->bookbuf);
    state->mainbuf = state->bookbuf = NULL;

    if (!state->eosin)
    {
        if (!error)
        {
            g_set_error (error, ET_OGG_ERROR, ET_OGG_ERROR_OUTPUT,
                         "Error writing stream to output. Output stream may be corrupted or truncated");
        }
    }

    if (error == NULL || *error != NULL)
    {
        g_object_unref (ostream);
        return FALSE;
    }

    g_assert (error == NULL || *error == NULL);

    if (!g_output_stream_close (ostream, NULL, error))
    {
        g_object_unref (ostream);
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    g_assert (error == NULL || *error == NULL);

    buf = g_memory_output_stream_steal_data (G_MEMORY_OUTPUT_STREAM (ostream));
    size = g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (ostream));

    /* Write the in-memory data back out to the original file. */
    if (!g_file_replace_contents (file, buf, size, NULL, FALSE,
                                  G_FILE_CREATE_NONE, NULL, NULL, error))
    {
        g_object_unref (ostream);
        g_free (buf);

        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    g_free (buf);
    g_object_unref (ostream);

    return TRUE;
}

#endif /* ENABLE_OGG */
