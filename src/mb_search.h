/* mb_search.h - 2014/05/05 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2014  Abhinav Jangda <abhijangda@hotmail.com>
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

#ifndef __MB_SEARCH_H__
#define __MB_SEARCH_H__

#include <gtk/gtk.h>
#include <musicbrainz5/mb5_c.h>

/****************
 * Declarations *
 ****************/

#define NAME_MAX_SIZE 256
#define SEARCH_LIMIT_STR "5"
#define SEARCH_LIMIT_INT 5

GCancellable *mb5_search_cancellable;

/*
 * Error domain and codes for errors while reading/writing Opus files
 */
GQuark et_mb5_search_error_quark (void);

#define ET_MB5_SEARCH_ERROR et_mb5_search_error_quark ()

/*
 * EtMB5SearchError:
 * @ET_MB5_SEARCH_ERROR_CONNECTION: Connection Error
 * @ET_MB5_SEARCH_ERROR_TIMEOUT: Timeout reached while communicating
 * @ET_MB5_SEARCH_ERROR_AUTHENTICATION: Server Authentication not matched
 * @ET_MB5_SEARCH_ERROR_FETCH: Cannot Fetch Data
 * @ET_MB5_SEARCH_ERROR_REQUEST: Request to MusicBrainz Server cannot be made
 * @ET_MB5_SEARCH_ERROR_RESOURCE_NOT_FOUND: Resource user is trying to search is not found
 * @ET_MB5_SEARCH_ERROR_CANCELLED: Operation was cancelled
 * @ET_MB5_SEARCH_ERROR_DISCID: Cannot find DiscID
 *
 * Errors while searching MusicBrainz Server.
 */
typedef enum
{
    ET_MB5_SEARCH_ERROR_CONNECTION,
    ET_MB5_SEARCH_ERROR_TIMEOUT,
    ET_MB5_SEARCH_ERROR_AUTHENTICATION,
    ET_MB5_SEARCH_ERROR_FETCH,
    ET_MB5_SEARCH_ERROR_REQUEST,
    ET_MB5_SEARCH_ERROR_RESOURCE_NOT_FOUND,
    ET_MB5_SEARCH_ERROR_CANCELLED,
    ET_MB5_SEARCH_ERROR_DISCID,
} EtMB5SearchError;

enum MB_ENTITY_TYPE
{
    MB_ENTITY_TYPE_ARTIST = 0,
    MB_ENTITY_TYPE_ALBUM,
    MB_ENTITY_TYPE_TRACK,
    MB_ENTITY_TYPE_COUNT,
    MB_ENTITY_TYPE_DISCID,
};

typedef struct
{
    Mb5Entity entity;
    enum MB_ENTITY_TYPE type;
    gboolean is_red_line;
} EtMbEntity;

/**************
 * Prototypes *
 **************/

void
et_musicbrainz_search_set_server_port (gchar *server, int port);
gboolean
et_musicbrainz_search_in_entity (enum MB_ENTITY_TYPE child_type,
                                 enum MB_ENTITY_TYPE parent_type,
                                 gchar *parent_mbid, GNode *root,
                                 GError **error, GCancellable *cancellable);
gboolean
et_musicbrainz_search (gchar *string, enum MB_ENTITY_TYPE type, GNode *root,
                       GError **error, GCancellable *cancellable);
void
free_mb_tree (GNode *node);
#endif /* __MB_SEARCH_H__ */
