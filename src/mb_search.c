/* mb_search.c - 2014/05/05 */
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

#include <glib/gi18n.h>

#include "mb_search.h"
#include "musicbrainz_dialog.h"

static gchar *server = NULL;
static int port = 0;

/*
 * et_mb5_search_error_quark:
 *
 * To get EtMB5SearchError domain.
 *
 * Returns: GQuark for EtMB5SearchError domain
 */
GQuark
et_mb5_search_error_quark (void)
{
    return g_quark_from_static_string ("et-mb5-search-error-quark");
}

void
et_musicbrainz_search_set_server_port (gchar *_server, int _port)
{
    if (server)
    {
        g_free (server);
    }

    server = g_strdup (_server);
    port = _port;
}

/*
 * et_musicbrainz_search_in_entity:
 * @child_type: Type of the children to get.
 * @parent_type: Type of the parent.
 * @parent_mbid: MBID of parent entity.
 * @root: GNode of parent.
 * @error: GError
 *
 * To retrieve children entities of a parent entity.
 *
 * Returns: TRUE if successful, FALSE if not.
 */
gboolean
et_musicbrainz_search_in_entity (enum MB_ENTITY_TYPE child_type,
                                 enum MB_ENTITY_TYPE parent_type,
                                 gchar *parent_mbid, GNode *root,
                                 GError **error, GCancellable *cancellable)
{
    Mb5Query query;
    Mb5Metadata metadata;
    char error_message[256];
    tQueryResult result;
    char *param_values[1];
    char *param_names[1];

    if (g_cancellable_is_cancelled (cancellable))
    {
        g_set_error (error, ET_MB5_SEARCH_ERROR,
                     ET_MB5_SEARCH_ERROR_CANCELLED,
                     _("Operation cancelled by user"));
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    param_names [0] = "inc";
    query = mb5_query_new ("easytag", server, port);

    if (child_type == MB_ENTITY_TYPE_ALBUM &&
        parent_type == MB_ENTITY_TYPE_ARTIST)
    {
        param_values [0] = "releases";
        metadata = mb5_query_query (query, "artist", parent_mbid, "", 1,
                                    param_names, param_values);
        result = mb5_query_get_lastresult (query);

        if (result == eQuery_Success)
        {
            if (metadata)
            {
                int i;
                Mb5ReleaseList list;
                Mb5Artist artist;
                gchar *message;

                if (g_cancellable_is_cancelled (cancellable))
                {
                    g_set_error (error, ET_MB5_SEARCH_ERROR,
                                 ET_MB5_SEARCH_ERROR_CANCELLED,
                                 _("Operation cancelled by user"));
                    mb5_query_delete (query);
                    mb5_metadata_delete (metadata);
                    g_assert (error == NULL || *error != NULL);
                    return FALSE;
                }

                artist = mb5_metadata_get_artist (metadata);
                list = mb5_artist_get_releaselist (artist);
                param_values[0] = "artists release-groups";
                message = g_strdup_printf (_("Found %d Album(s)"),
                                           mb5_release_list_size (list));
                et_show_status_msg_in_idle (message);
                g_free (message);

                for (i = 0; i < mb5_release_list_size (list); i++)
                {
                    Mb5Release release;
                    release = mb5_release_list_item (list, i);

                    if (release)
                    {
                        Mb5Metadata metadata_release;
                        gchar buf [NAME_MAX_SIZE];
                        GNode *node;
                        EtMbEntity *entity;
                        int size;

                        if (g_cancellable_is_cancelled (cancellable))
                        {
                            g_set_error (error, ET_MB5_SEARCH_ERROR,
                                         ET_MB5_SEARCH_ERROR_CANCELLED,
                                         _("Operation cancelled by user"));
                            mb5_query_delete (query);
                            mb5_metadata_delete (metadata);
                            g_assert (error == NULL || *error != NULL);
                            return FALSE;
                        }

                        size = mb5_release_get_title ((Mb5Release)release, buf,
                                                      sizeof (buf));
                        buf [size] = '\0';
                        message = g_strdup_printf (_("Retrieving %s (%d/%d)"),
                                                   buf, i+1,
                                                   mb5_release_list_size (list));
                        et_show_status_msg_in_idle (message);
                        g_free (message);

                        size = mb5_release_get_id ((Mb5Release)release,
                                                   buf,
                                                   sizeof (buf));
                        buf [size] = '\0';
                        metadata_release = mb5_query_query (query, "release",
                                                            buf, "",
                                                            1, param_names,
                                                            param_values);
                        entity = g_malloc (sizeof (EtMbEntity));
                        entity->entity = mb5_release_clone (mb5_metadata_get_release (metadata_release));
                        entity->type = MB_ENTITY_TYPE_ALBUM;
                        entity->is_red_line = FALSE;
                        node = g_node_new (entity);
                        g_node_append (root, node);
                        mb5_metadata_delete (metadata_release);
                    }
                }
            }

            mb5_metadata_delete (metadata);
        }
        else
        {
            goto err;
        }
    }
    else if (child_type == MB_ENTITY_TYPE_TRACK &&
             parent_type == MB_ENTITY_TYPE_ALBUM)
    {
        param_values [0] = "recordings";
        metadata = mb5_query_query (query, "release", parent_mbid, "", 1,
                                    param_names, param_values);
        result = mb5_query_get_lastresult (query);

        if (result == eQuery_Success)
        {
            if (metadata)
            {
                int i;
                Mb5MediumList list;
                Mb5Release release;
                release = mb5_metadata_get_release (metadata);
                list = mb5_release_get_mediumlist (release);
                param_values[0] = "releases artists";

                for (i = 0; i < mb5_medium_list_size (list); i++)
                {
                    Mb5Medium medium;
                    medium = mb5_medium_list_item (list, i);

                    if (medium)
                    {
                        Mb5Metadata metadata_recording;
                        gchar buf [NAME_MAX_SIZE];
                        GNode *node;
                        EtMbEntity *entity;
                        Mb5TrackList track_list;
                        int j;
                        int size;
                        gchar *message;

                        track_list = mb5_medium_get_tracklist (medium);
                        message = g_strdup_printf (_("Found %d Track(s)"),
                                                   mb5_track_list_size (list));
                        et_show_status_msg_in_idle (message);
                        g_free (message);

                        for (j = 0; j < mb5_track_list_size (track_list); j++)
                        {
                            Mb5Recording recording;


                            if (g_cancellable_is_cancelled (cancellable))
                            {
                                g_set_error (error, ET_MB5_SEARCH_ERROR,
                                             ET_MB5_SEARCH_ERROR_CANCELLED,
                                             _("Operation cancelled by user"));
                                mb5_query_delete (query);
                                mb5_metadata_delete (metadata);
                                g_assert (error == NULL || *error != NULL);
                                return FALSE;
                            }

                            recording = mb5_track_get_recording (mb5_track_list_item (track_list, j));
                            size = mb5_recording_get_title (recording, buf,
                                                            sizeof (buf));
                            buf [size] = '\0';
                            message = g_strdup_printf (_("Retrieving %s (%d/%d)"),
                                                       buf, j,
                                                       mb5_track_list_size (track_list));
                            et_show_status_msg_in_idle (message);
                            g_free (message);

                            size = mb5_recording_get_id (recording,
                                                         buf,
                                                         sizeof (buf));
 
                            metadata_recording = mb5_query_query (query,
                                                                  "recording",
                                                                  buf, "", 1,
                                                                  param_names,
                                                                  param_values);
                            entity = g_malloc (sizeof (EtMbEntity));
                            entity->entity = mb5_recording_clone (mb5_metadata_get_recording (metadata_recording));
                            entity->type = MB_ENTITY_TYPE_TRACK;
                            entity->is_red_line = FALSE;
                            node = g_node_new (entity);
                            g_node_append (root, node);
                            mb5_metadata_delete (metadata_recording);
                        }
                    }
                }
            }

            mb5_metadata_delete (metadata);
        }
        else
        {
            goto err;
        }
    }

    mb5_query_delete (query);

    return TRUE;

    err:
    mb5_query_get_lasterrormessage (query, error_message,
                                    sizeof(error_message));

    switch (result)
    {
        case eQuery_ConnectionError:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_CONNECTION, error_message);
            break;

        case eQuery_Timeout:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_TIMEOUT, error_message);
            break;

        case eQuery_AuthenticationError:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_AUTHENTICATION, error_message);
            break;

        case eQuery_FetchError:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_FETCH, error_message);
            break;
 
        case eQuery_RequestError:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_REQUEST, error_message);
            break;
 
        case eQuery_ResourceNotFound:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_RESOURCE_NOT_FOUND,
                         error_message);
            break;

        default:
            break;
    }

    g_assert (error == NULL || *error != NULL);
    return FALSE;
}

/*
 * et_musicbrainz_search:
 * @string: String to search in MusicBrainz database.
 * @type: Type of entity to search.
 * @root: Root of the mbTree.
 * @error: GError.
 *
 * To search for an entity in MusicBrainz Database.
 *
 * Returns: TRUE if successfull, FALSE if not.
 */
gboolean
et_musicbrainz_search (gchar *string, enum MB_ENTITY_TYPE type, GNode *root,
                       GError **error, GCancellable *cancellable)
{
    Mb5Query query;
    Mb5Metadata metadata;
    char error_message[256];
    tQueryResult result;
    char *param_values[2];
    char *param_names[2];

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    param_names [0] = "query";
    param_names [1] = "limit";
    param_values [1] = SEARCH_LIMIT_STR;
    query = mb5_query_new ("easytag", server, port);

    if (g_cancellable_is_cancelled (cancellable))
    {
        g_set_error (error, ET_MB5_SEARCH_ERROR,
                     ET_MB5_SEARCH_ERROR_CANCELLED,
                     _("Operation cancelled by user"));
        mb5_query_delete (query);
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    if (type == MB_ENTITY_TYPE_ARTIST)
    {
        param_values [0] = g_strconcat ("artist:", string, NULL);
        metadata = mb5_query_query (query, "artist", "", "", 2, param_names,
                                    param_values);
        g_free (param_values [0]);
        result = mb5_query_get_lastresult (query);

        if (result == eQuery_Success)
        {
            if (metadata)
            {
                int i;
                Mb5ArtistList list;
                list = mb5_metadata_get_artistlist (metadata);
 
                for (i = 0; i < mb5_artist_list_size (list); i++)
                {
                    Mb5Artist artist;

                    if (g_cancellable_is_cancelled (cancellable))
                    {
                        g_set_error (error, ET_MB5_SEARCH_ERROR,
                                     ET_MB5_SEARCH_ERROR_CANCELLED,
                                     _("Operation cancelled by user"));
                        mb5_query_delete (query);
                        mb5_metadata_delete (metadata);
                        g_assert (error == NULL || *error != NULL);
                        return FALSE;
                    }

                    artist = mb5_artist_list_item (list, i);

                    if (artist)
                    {
                        GNode *node;
                        EtMbEntity *entity;
                        entity = g_malloc (sizeof (EtMbEntity));
                        entity->entity = mb5_artist_clone (artist);
                        entity->type = MB_ENTITY_TYPE_ARTIST;
                        entity->is_red_line = FALSE;
                        node = g_node_new (entity);
                        g_node_append (root, node);
                    }
                }
            }

            mb5_metadata_delete (metadata);
        }
        else
        {
            goto err;
        }
    }

    else if (type == MB_ENTITY_TYPE_ALBUM)
    {
        param_values [0] = g_strconcat ("release:", string, NULL);
        metadata = mb5_query_query (query, "release", "", "", 2, param_names,
                                    param_values);
        result = mb5_query_get_lastresult (query);
        g_free (param_values [0]);

        if (result == eQuery_Success)
        {
            if (metadata)
            {
                int i;
                Mb5ReleaseList list;
                gchar *message;

                list = mb5_metadata_get_releaselist (metadata);
                param_names [0] = "inc";
                param_values [0] = "artists release-groups";
                message = g_strdup_printf (_("Found %d Album(s)"),
                                           mb5_release_list_size (list));
                et_show_status_msg_in_idle (message);
                g_free (message);

                for (i = 0; i < mb5_release_list_size (list); i++)
                {
                    Mb5Release release;

                    if (g_cancellable_is_cancelled (cancellable))
                    {
                        g_set_error (error, ET_MB5_SEARCH_ERROR,
                                     ET_MB5_SEARCH_ERROR_CANCELLED,
                                     _("Operation cancelled by user"));
                        mb5_query_delete (query);
                        mb5_metadata_delete (metadata);
                        g_assert (error == NULL || *error != NULL);
                        return FALSE;
                    }

                    release = mb5_release_list_item (list, i);

                    if (release)
                    {
                        Mb5Metadata metadata_release;
                        gchar buf [NAME_MAX_SIZE];
                        GNode *node;
                        EtMbEntity *entity;
                        int size;

                        size = mb5_release_get_title ((Mb5Release)release,
                                                      buf, sizeof (buf));
                        buf [size] = '\0';
                        message = g_strdup_printf (_("Retrieving %s (%d/%d)"),
                                                   buf, i,
                                                   mb5_release_list_size (list));
                        et_show_status_msg_in_idle (message);
                        g_free (message);

                        mb5_release_get_id ((Mb5Release)release,
                                            buf,
                                            sizeof (buf));
                        metadata_release = mb5_query_query (query, "release",
                                                            buf, "",
                                                            1, param_names,
                                                            param_values);
                        entity = g_malloc (sizeof (EtMbEntity));
                        entity->entity = mb5_release_clone (mb5_metadata_get_release (metadata_release));
                        entity->type = MB_ENTITY_TYPE_ALBUM;
                        entity->is_red_line = FALSE;
                        node = g_node_new (entity);
                        g_node_append (root, node);
                        mb5_metadata_delete (metadata_release);
                    }
                }
            }

            mb5_metadata_delete (metadata);
        }
        else
        {
            goto err;
        }
    }

    else if (type == MB_ENTITY_TYPE_TRACK)
    {
        param_values [0] = g_strconcat ("recordings:", string, NULL);
        metadata = mb5_query_query (query, "recording", "", "", 2,
                                    param_names, param_values);
        result = mb5_query_get_lastresult (query);
        g_free (param_values [0]);

        if (result == eQuery_Success)
        {
            if (metadata)
            {
                int i;
                Mb5RecordingList list;
                gchar *message;

                list = mb5_metadata_get_recordinglist (metadata);
                param_names [0] = "inc";
                param_values[0] = "releases artists";
                message = g_strdup_printf (_("Found %d Track(s)"),
                                           mb5_recording_list_size (list));
                et_show_status_msg_in_idle (message);
                g_free (message);

                for (i = 0; i < mb5_recording_list_size (list); i++)
                {
                    Mb5Recording recording;
                    Mb5Metadata metadata_recording;
                    gchar buf [NAME_MAX_SIZE];
                    GNode *node;
                    EtMbEntity *entity;
                    int size;

                    if (g_cancellable_is_cancelled (cancellable))
                    {
                        g_set_error (error, ET_MB5_SEARCH_ERROR,
                                     ET_MB5_SEARCH_ERROR_CANCELLED,
                                     _("Operation cancelled by user"));
                        mb5_query_delete (query);
                        mb5_metadata_delete (metadata);
                        g_assert (error == NULL || *error != NULL);
                        return FALSE;
                    }

                    recording = mb5_recording_list_item (list, i);
                    size = mb5_recording_get_title (recording, buf, sizeof (buf));
                    buf [size] = '\0';
                    message = g_strdup_printf (_("Retrieving %s (%d/%d)"),
                                               buf, i,
                                               mb5_track_list_size (list));
                    et_show_status_msg_in_idle (message);
                    g_free (message);

                    mb5_recording_get_id (recording,
                                          buf,
                                          sizeof (buf));
                    metadata_recording = mb5_query_query (query, "recording",
                                                          buf, "",
                                                          1, param_names,
                                                          param_values);
                    entity = g_malloc (sizeof (EtMbEntity));
                    entity->entity = mb5_recording_clone (mb5_metadata_get_recording (metadata_recording));
                    entity->type = MB_ENTITY_TYPE_TRACK;
                    entity->is_red_line = FALSE;
                    node = g_node_new (entity);
                    g_node_append (root, node);
                    mb5_metadata_delete (metadata_recording);
                }
            }

            mb5_metadata_delete (metadata);
        }
        else
        {
            goto err;
        }
    }

    else if (type == MB_ENTITY_TYPE_DISCID)
    {
        param_names[0] = "toc";
        param_values [0] = "";
        metadata = mb5_query_query (query, "discid", string, "", 1, param_names,
                                    param_values);
        result = mb5_query_get_lastresult (query);

        if (result == eQuery_Success)
        {
            if (metadata)
            {
                int i;
                Mb5ReleaseList list;
                gchar *message;
                Mb5Disc disc;

                disc = mb5_metadata_get_disc (metadata);
                list = mb5_disc_get_releaselist (disc);
                param_names [0] = "inc";
                param_values [0] = "artists release-groups";
                message = g_strdup_printf (_("Found %d Album(s)"),
                                           mb5_release_list_size (list));
                et_show_status_msg_in_idle (message);
                g_free (message);

                for (i = 0; i < mb5_release_list_size (list); i++)
                {
                    Mb5Release release;

                    if (g_cancellable_is_cancelled (cancellable))
                    {
                        g_set_error (error, ET_MB5_SEARCH_ERROR,
                                     ET_MB5_SEARCH_ERROR_CANCELLED,
                                     _("Operation cancelled by user"));
                        mb5_query_delete (query);
                        mb5_metadata_delete (metadata);
                        g_assert (error == NULL || *error != NULL);
                        return FALSE;
                    }

                    release = mb5_release_list_item (list, i);

                    if (release)
                    {
                        Mb5Metadata metadata_release;
                        gchar buf [NAME_MAX_SIZE];
                        GNode *node;
                        EtMbEntity *entity;
                        int size;

                        size = mb5_release_get_title ((Mb5Release)release, buf,
                                                      sizeof (buf));
                        buf [size] = '\0';
                        message = g_strdup_printf (_("Retrieving %s (%d/%d)"),
                                                   buf, i,
                                                   mb5_release_list_size (list));
                        et_show_status_msg_in_idle (message);
                        g_free (message);

                        mb5_release_get_id ((Mb5Release)release,
                                            buf,
                                            sizeof (buf));
                        metadata_release = mb5_query_query (query, "release",
                                                            buf, "",
                                                            1, param_names,
                                                            param_values);
                        entity = g_malloc (sizeof (EtMbEntity));
                        entity->entity = mb5_release_clone (mb5_metadata_get_release (metadata_release));
                        entity->type = MB_ENTITY_TYPE_ALBUM;
                        entity->is_red_line = FALSE;
                        node = g_node_new (entity);
                        g_node_append (root, node);
                        mb5_metadata_delete (metadata_release);
                    }
                }
            }

            mb5_metadata_delete (metadata);
        }
        else
        {
            goto err;
        }
    }

    mb5_query_delete (query);

    return TRUE;

    err:
    mb5_query_get_lasterrormessage (query, error_message,
                                    sizeof(error_message));

    switch (result)
    {
        case eQuery_ConnectionError:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_CONNECTION, error_message);
            break;

        case eQuery_Timeout:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_TIMEOUT, error_message);
            break;

        case eQuery_AuthenticationError:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_AUTHENTICATION, error_message);
            break;

        case eQuery_FetchError:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_FETCH, error_message);
            break;
 
        case eQuery_RequestError:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_REQUEST, error_message);
            break;
 
        case eQuery_ResourceNotFound:
            g_set_error (error, ET_MB5_SEARCH_ERROR,
                         ET_MB5_SEARCH_ERROR_RESOURCE_NOT_FOUND,
                         error_message);
            break;

        default:
            break;
    }

    g_assert (error == NULL || *error != NULL);
    return FALSE;
}

/*
 * free_mb_tree:
 * @node: Root of the tree to start freeing with.
 *
 * To free the memory occupied by the tree.
 */
void
free_mb_tree (GNode *node)
{
    EtMbEntity *et_entity;
    GNode *child;

    et_entity = (EtMbEntity *)node->data;

    if (et_entity)
    {
        if (et_entity->type == MB_ENTITY_TYPE_ARTIST)
        {
            mb5_artist_delete ((Mb5Artist)et_entity->entity);
        }

        else if (et_entity->type == MB_ENTITY_TYPE_ALBUM)
        {
            mb5_release_delete ((Mb5Release)et_entity->entity);
        }

        else if (et_entity->type == MB_ENTITY_TYPE_TRACK)
        {
            mb5_recording_delete ((Mb5Recording)et_entity->entity);
        }
    }

    g_free (et_entity);
    g_node_unlink (node);
    child = g_node_first_child (node);

    while (child)
    {
        free_mb_tree (child);
        child = g_node_next_sibling (child);
    }

    g_node_destroy (node);
}