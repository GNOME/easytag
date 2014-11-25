/* EasyTAG - tag editor for audio files
 * Copyright (C) 2015  David King <amigadave@amigadave.com>
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
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ET_MUSICBRAINZ_H_
#define ET_MUSICBRAINZ_H_

#include "config.h"

#ifdef ENABLE_MUSICBRAINZ

#include <gio/gio.h>

G_BEGIN_DECLS

#define ET_TYPE_MUSICBRAINZ (et_musicbrainz_get_type ())
#define ET_MUSICBRAINZ(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_MUSICBRAINZ, EtMusicbrainz))

typedef struct _EtMusicbrainz EtMusicbrainz;
typedef struct _EtMusicbrainzClass EtMusicbrainzClass;

/*
 * EtMusicbrainzEntity:
 * @ET_MUSICBRAINZ_ENTITY_RELEASE_GROUP:
 * @ET_MUSICBRAINZ_ENTITY_ARTIST:
 *
 * Entity type to query against the MusicBrainz web service.
 */
typedef enum
{
    ET_MUSICBRAINZ_ENTITY_RELEASE_GROUP,
    ET_MUSICBRAINZ_ENTITY_ARTIST
} EtMusicbrainzEntity;

typedef struct _EtMusicbrainzQuery EtMusicbrainzQuery;
typedef struct _EtMusicbrainzResult EtMusicbrainzResult;

struct _EtMusicbrainz
{
    /*< private >*/
    GObject parent_instance;
};

struct _EtMusicbrainzClass
{
    /*< private >*/
    GObjectClass parent_class;
};

GType et_musicbrainz_get_type (void);
EtMusicbrainz *et_musicbrainz_new (void);

void et_musicbrainz_search_async (EtMusicbrainz *self, EtMusicbrainzQuery *query, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
EtMusicbrainzResult * et_musicbrainz_search_finish (EtMusicbrainz *self, GAsyncResult *result, GError **error);

EtMusicbrainzQuery * et_musicbrainz_query_new (EtMusicbrainzEntity entity, const gchar *search_string);
EtMusicbrainzQuery * et_musicbrainz_query_ref (EtMusicbrainzQuery *query);
void et_musicbrainz_query_unref (EtMusicbrainzQuery *query);

EtMusicbrainzResult * et_musicbrainz_result_ref (EtMusicbrainzResult *result);
void et_musicbrainz_result_unref (EtMusicbrainzResult *result);
GList * et_musicbrainz_result_get_results (EtMusicbrainzResult *result);

G_END_DECLS

#endif /* ENABLE_MUSICBRAINZ */

#endif /* !ET_MUSICBRAINZ_H_ */
