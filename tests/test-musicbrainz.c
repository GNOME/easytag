/* test-musicbrainz.c - 2014/06/16 */
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

#include <glib.h>
#include <libsoup/soup.h>
#include <string.h>

#include "mb_search.h"

/****************
 * Declarations *
 ****************/
static gchar name[NAME_MAX_SIZE];
static gchar mbid[NAME_MAX_SIZE];
static SoupServer *server;

/**************
 * Prototypes *
 **************/
static void
mb_search_test (void);
static void
mb_search_in_test (void);
static void
default_handler (SoupServer *server, SoupMessage *msg, const char *path,
                 GHashTable *query, SoupClientContext *client,
                 gpointer user_data);
static void
westlife_handler (SoupServer *server, SoupMessage *msg, const char *path,
                  GHashTable *query, SoupClientContext *client,
                  gpointer user_data);
static void
westlife_release_info_handler (SoupServer *server, SoupMessage *msg,
                               const char *path, GHashTable *query,
                               SoupClientContext *client, gpointer user_data);
static void
i_still_handler (SoupServer *server, SoupMessage *msg, const char *path,
                 GHashTable *query, SoupClientContext *client,
                 gpointer user_data);
static void
never_gone_handler (SoupServer *server, SoupMessage *msg, const char *path,
                    GHashTable *query, SoupClientContext *client,
                    gpointer user_data);
static void
artist_handler (SoupServer *server, SoupMessage *msg, const char *path,
                GHashTable *query, SoupClientContext *client,
                gpointer user_data);
static void
release_handler (SoupServer *server, SoupMessage *msg, const char *path,
                 GHashTable *query, SoupClientContext *client,
                 gpointer user_data);
static void
recording_handler (SoupServer *server, SoupMessage *msg, const char *path,
                   GHashTable *query, SoupClientContext *client,
                   gpointer user_data);
static void 
discid_handler (SoupServer *server, SoupMessage *msg, const char *path,
                GHashTable *query, SoupClientContext *client,
                gpointer user_data);
static void 
discid_release_handler (SoupServer *server, SoupMessage *msg, 
                        const char *path, GHashTable *query,
                        SoupClientContext *client,
                        gpointer user_data);
static gpointer
run_server (gpointer data);

static void
mb_search_test (void)
{
    GNode *mbTreeNode;
    GError *err;

    err = NULL;
    mbTreeNode = g_node_new (NULL);
    if (et_musicbrainz_search ("Westlife", MB_ENTITY_TYPE_ARTIST, mbTreeNode,
                               &err, NULL))
    {
        EtMbEntity *etentity;

        etentity = (EtMbEntity *)(g_node_first_child (mbTreeNode)->data);
        mb5_artist_get_name (etentity->entity, name, NAME_MAX_SIZE);
        g_assert_cmpstr (name, ==, "Westlife");
        free_mb_tree (mbTreeNode);
    }
    else
    {
        free_mb_tree (mbTreeNode);
        return;
    }

    mbTreeNode = g_node_new (NULL);

    if (et_musicbrainz_search ("Never Gone", MB_ENTITY_TYPE_ALBUM, mbTreeNode,
                               &err, NULL))
    {
        EtMbEntity *etentity;

        etentity = (EtMbEntity *)(g_node_first_child (mbTreeNode)->data);
        mb5_release_get_title (etentity->entity, name, NAME_MAX_SIZE);
        g_assert_cmpstr (name, ==, "Never Gone");
        free_mb_tree (mbTreeNode);
    }
    else
    {
        free_mb_tree (mbTreeNode);
        return;
    }

    mbTreeNode = g_node_new (NULL);

    if (et_musicbrainz_search ("I Still", MB_ENTITY_TYPE_TRACK, mbTreeNode,
                               &err, NULL))
    {
        EtMbEntity *etentity;

        etentity = (EtMbEntity *)(g_node_first_child (mbTreeNode)->data);
        mb5_recording_get_title (etentity->entity, name, NAME_MAX_SIZE);
        g_assert_cmpstr (name, ==, "I Still...");
        free_mb_tree (mbTreeNode);
    }
    else
    {
        free_mb_tree (mbTreeNode);
        return;
    }

    mbTreeNode = g_node_new (NULL);

    if (et_musicbrainz_search ("lwHl8fGzJyLXQR33ug60E8jhf4k-",
                               MB_ENTITY_TYPE_DISCID, mbTreeNode,
                               &err, NULL))
    {
        EtMbEntity *etentity;

        etentity = (EtMbEntity *)(g_node_first_child (mbTreeNode)->data);
        mb5_recording_get_title (etentity->entity, name, NAME_MAX_SIZE);
        g_assert_cmpstr (name, ==, "Praat geen poep");
        free_mb_tree (mbTreeNode);
    }
    else
    {
        free_mb_tree (mbTreeNode);
        return;
    }
}

static void
mb_search_in_test (void)
{
    GNode *mbTreeNode;
    GError *err;

    err = NULL;
    mbTreeNode = g_node_new (NULL);

    if (et_musicbrainz_search ("Westlife", MB_ENTITY_TYPE_ARTIST, mbTreeNode,
                               &err, NULL))
    {
        EtMbEntity *etentity;
        GNode *westlife_node;

        westlife_node = g_node_first_child (mbTreeNode);
        etentity = (EtMbEntity *)(westlife_node->data);
        mb5_artist_get_name (etentity->entity, name, NAME_MAX_SIZE);
        g_assert_cmpstr (name, ==, "Westlife");

        mb5_artist_get_id (etentity->entity, mbid, NAME_MAX_SIZE);

        if (et_musicbrainz_search_in_entity (MB_ENTITY_TYPE_ALBUM,
                                             MB_ENTITY_TYPE_ARTIST, mbid,
                                             westlife_node, &err, NULL))
        {
            EtMbEntity *etentity;

            etentity = (EtMbEntity *)(g_node_first_child (westlife_node)->data);
            mb5_release_get_title (etentity->entity, name, NAME_MAX_SIZE);
            g_assert_cmpstr (name, ==,
                             "Unbreakable: The Greatest Hits, Volume 1");
        }

        free_mb_tree (mbTreeNode);
    }
    else
    {
        free_mb_tree (mbTreeNode);
        return;
    }

    mbTreeNode = g_node_new (NULL);

    if (et_musicbrainz_search ("Never Gone", MB_ENTITY_TYPE_ALBUM, mbTreeNode,
                               &err, NULL))
    {
        EtMbEntity *etentity;
        GNode *never_gone_node;

        never_gone_node = g_node_first_child (mbTreeNode);
        etentity = (EtMbEntity *)(g_node_first_child (mbTreeNode)->data);
        mb5_release_get_title (etentity->entity, name, NAME_MAX_SIZE);
        g_assert_cmpstr (name, ==, "Never Gone");
        mb5_release_get_id (etentity->entity, mbid, NAME_MAX_SIZE);

        if (et_musicbrainz_search_in_entity (MB_ENTITY_TYPE_TRACK,
                                             MB_ENTITY_TYPE_ALBUM, mbid,
                                             never_gone_node, &err, NULL))
        {
            EtMbEntity *etentity;

            etentity = (EtMbEntity *)(g_node_first_child (never_gone_node)->data);
            mb5_release_get_title (etentity->entity, name, NAME_MAX_SIZE);
            g_assert_cmpstr (name, ==, "I Still...");
        }

        free_mb_tree (mbTreeNode);
    }
    else
    {
        free_mb_tree (mbTreeNode);
        return;
    }
}

static void
default_handler (SoupServer *server, SoupMessage *msg, const char *path,
                 GHashTable *query, SoupClientContext *client,
                 gpointer user_data)
{    
    gchar *response_text;
    gsize length;

    g_file_get_contents ("./tests/error.xml", &response_text, &length, NULL);
    soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                               "error", strlen ("error"));
    soup_message_set_status (msg, 200);
}

static void
westlife_handler (SoupServer *server, SoupMessage *msg, const char *path,
                  GHashTable *query, SoupClientContext *client,
                  gpointer user_data)
{    
    gchar *response_text;
    gsize length;

    if (g_file_get_contents ("./tests/artist-westlife_releases.xml",
                             &response_text, &length, NULL))
    {
        soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                                   response_text, strlen (response_text));
        soup_message_set_status (msg, 200);
    }
}

static void
westlife_release_info_handler (SoupServer *server, SoupMessage *msg,
                               const char *path, GHashTable *query,
                               SoupClientContext *client, gpointer user_data)
{    
    gchar *response_text;
    gsize length;

    if (g_file_get_contents ("./tests/artist-westlife_release_info.xml",
                             &response_text, &length, NULL))
    {
        soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                                   response_text, strlen (response_text));
        soup_message_set_status (msg, 200);
    }
}

static void
i_still_handler (SoupServer *server, SoupMessage *msg, const char *path,
                 GHashTable *query, SoupClientContext *client,
                 gpointer user_data)
{    
    gchar *response_text;
    gsize length;

    if (g_file_get_contents ("./tests/album-i_still_info.xml", &response_text,
                             &length, NULL))
    {
        soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                                   response_text, strlen (response_text));
        soup_message_set_status (msg, 200);
    }
}

static void
never_gone_handler (SoupServer *server, SoupMessage *msg, const char *path,
                    GHashTable *query, SoupClientContext *client,
                    gpointer user_data)
{    
    gchar *response_text;
    gsize length;
    gpointer value;

    value = g_hash_table_lookup (query, "inc");

    if (value && !g_strcmp0 (value, "artists release-groups"))
    {
       if (g_file_get_contents ("./tests/album-never_gone_info.xml",
                                &response_text, &length, NULL))
        {
            soup_message_set_response (msg, "application/xml",
                                       SOUP_MEMORY_TAKE,
                                       response_text, strlen (response_text));
            soup_message_set_status (msg, 200);
        }
    }

    else if (value && !g_strcmp0 (value, "recordings"))
    {
       if (g_file_get_contents ("./tests/album-never_gone_recordings.xml",
                                &response_text, &length, NULL))
        {
            soup_message_set_response (msg, "application/xml",
                                       SOUP_MEMORY_TAKE,
                                       response_text, strlen (response_text));
            soup_message_set_status (msg, 200);
        }
    }
}

static void
artist_handler (SoupServer *server, SoupMessage *msg, const char *path,
                GHashTable *query, SoupClientContext *client,
                gpointer user_data)
{
    gchar *response_text;
    gsize length;
    gpointer value;

    value = g_hash_table_lookup (query, "query");

    if (value && !strcmp ((gchar *)value, "artist:Westlife"))
    {
        if (g_file_get_contents ("./tests/artist-westlife.xml",
                                 &response_text, &length, NULL))
        {
            soup_message_set_response (msg, "application/xml",
                                       SOUP_MEMORY_TAKE, response_text,
                                       strlen (response_text));
            soup_message_set_status (msg, 200);
            return;
        }                                        
    }

    g_file_get_contents ("./tests/error.xml", &response_text, &length, NULL);
    soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                               response_text, strlen (response_text));
    soup_message_set_status (msg, 200);
}

static void
release_handler (SoupServer *server, SoupMessage *msg, const char *path,
                 GHashTable *query, SoupClientContext *client,
                 gpointer user_data)
{    
    gchar *response_text;
    gsize length;
    gpointer value;

    value = g_hash_table_lookup (query, "query");

    if (value && !strcmp ((gchar *)value, "release:Never Gone"))
    {
        if (g_file_get_contents ("./tests/album-never_gone.xml",
                                 &response_text, &length, NULL))
        {
            soup_message_set_response (msg, "application/xml",
                                       SOUP_MEMORY_TAKE, response_text,
                                       strlen (response_text));
            soup_message_set_status (msg, 200);
            return;
        }                                        
    }

    g_file_get_contents ("./tests/error.xml", &response_text, &length, NULL);
    soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                               response_text, strlen (response_text));
    soup_message_set_status (msg, 200);
}

static void
recording_handler (SoupServer *server, SoupMessage *msg, const char *path,
                   GHashTable *query, SoupClientContext *client,
                   gpointer user_data)
{    
    gchar *response_text;
    gsize length;
    gpointer value;

    value = g_hash_table_lookup (query, "query");

    if (value && !strcmp ((gchar *)value, "recordings:I Still"))
    {
        if (g_file_get_contents ("./tests/recordings-i_still.xml",
                                 &response_text,
                                 &length, NULL))
        {
            soup_message_set_response (msg, "application/xml",
                                       SOUP_MEMORY_TAKE, response_text,
                                       strlen (response_text));
            soup_message_set_status (msg, 200);
            return;
        }
    }

    g_file_get_contents ("./tests/error.xml", &response_text, &length, NULL);
    soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                               response_text, strlen (response_text));
    soup_message_set_status (msg, 200);
}

static void 
discid_handler (SoupServer *server, SoupMessage *msg, const char *path,
                GHashTable *query, SoupClientContext *client,
                gpointer user_data)
{    
    gchar *response_text;
    gsize length;

    g_file_get_contents ("./tests/discid.xml", &response_text, &length,
                         NULL);
    soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                               response_text, strlen (response_text));
    soup_message_set_status (msg, 200);
}

static void 
discid_release_handler (SoupServer *server, SoupMessage *msg, 
                        const char *path, GHashTable *query,
                        SoupClientContext *client,
                        gpointer user_data)
{    
    gchar *response_text;
    gsize length;

    g_file_get_contents ("./tests/discid-release_info.xml", &response_text, 
                         &length, NULL);
    soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                               response_text, strlen (response_text));
    soup_message_set_status (msg, 200);
}

static gpointer
run_server (gpointer data)
{
    soup_server_run (server);
    return NULL;
}

int
main (int argc, char** argv)
{
    GThread *thread;

    g_type_init ();

    /* Initializing test functions */
    g_test_init (&argc, &argv, NULL);
    et_musicbrainz_search_set_server_port ("localhost", 8088);
    g_test_add_func ("/musicbrainz/mb_search", mb_search_test);
    g_test_add_func ("/musicbrainz/mb_search_in", mb_search_in_test);

    /* Creating a local MusicBrainz Server */
    server = soup_server_new (SOUP_SERVER_PORT, 8088, NULL);    
    soup_server_add_handler (server, "/", (SoupServerCallback)default_handler,
                             NULL, NULL);
    soup_server_add_handler (server, "/ws/2/artist",
                             (SoupServerCallback)artist_handler, NULL, NULL);
    soup_server_add_handler (server,
                             "/ws/2/artist/5f000e69-3cfd-4871-8f1b-faa7f0d4bcbc",
                             (SoupServerCallback)westlife_handler, NULL, NULL);
    soup_server_add_handler (server, "/ws/2/release",
                             (SoupServerCallback)release_handler, NULL, NULL);
    soup_server_add_handler (server,
                             "/ws/2/release/fd77296e-86f0-436e-8ea2-b657151f2167",
                             (SoupServerCallback)never_gone_handler, NULL,
                             NULL);
    soup_server_add_handler (server,
                             "/ws/2/release/30ac9626-4dac-457c-88be-11d0b7d2f5a4",
                             (SoupServerCallback)discid_release_handler, NULL,
                             NULL);
    soup_server_add_handler (server,
                             "/ws/2/release/182fbd5e-5f3e-413a-9ff1-8a9a81d052c0",
                             (SoupServerCallback)westlife_release_info_handler,
                              NULL, NULL);
    soup_server_add_handler (server, "/ws/2/recording",
                             (SoupServerCallback)recording_handler, NULL,
                             NULL); 
    soup_server_add_handler (server,
                             "/ws/2/recording/ddc5fc5c-8a83-4d20-a711-313797030da6",
                             (SoupServerCallback)i_still_handler, NULL,
                             NULL);
    soup_server_add_handler (server, "/ws/2/discid",
                             (SoupServerCallback)discid_handler, NULL, NULL);

    /* Running Server in a new thread */
    thread = g_thread_new ("Server", run_server, NULL);

    g_test_run ();

    soup_server_quit (server);
    g_object_unref (server);
    g_thread_join (thread);
    g_thread_unref (thread);

    return 0;
}