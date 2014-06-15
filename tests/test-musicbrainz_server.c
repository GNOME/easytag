#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <string.h>

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
    printf ("dsfsdfsdf\n");
    if (g_file_get_contents ("./tests/artist-westlife_releases.xml", &response_text,
                             &length, NULL))
    {
        soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                                   response_text, strlen (response_text));
        soup_message_set_status (msg, 200);
    }
}


static void
westlife_release_info_handler (SoupServer *server, SoupMessage *msg, const char *path,
                               GHashTable *query, SoupClientContext *client,
                               gpointer user_data)
{    
    gchar *response_text;
    gsize length;

    if (g_file_get_contents ("./tests/artist-westlife_release_info.xml", &response_text,
                             &length, NULL))
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
    //printf ("sssss\n");
    //printf ("%s\n", value);
    if (value && !g_strcmp0 (value, "artists release-groups"))
    {
       if (g_file_get_contents ("./tests/album-never_gone_info.xml", &response_text,
                                &length, NULL))
        {
            soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                                       response_text, strlen (response_text));
            soup_message_set_status (msg, 200);
        }
    }

    else if (value && !g_strcmp0 (value, "recordings"))
    {
       if (g_file_get_contents ("./tests/album-never_gone_recordings.xml",
                                &response_text, &length, NULL))
        {
            soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
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
    printf ("ddd %s\n", path);
    if (value && !strcmp ((gchar *)value, "artist:Westlife"))
    {
        if (g_file_get_contents ("./tests/artist-westlife.xml", &response_text,
                                 &length, NULL))
        {
            //printf ("sending message %s\n", response_text);
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
        if (g_file_get_contents ("./tests/album-never_gone.xml", &response_text,
                                 &length, NULL))
        {//printf ("sending message %s\n", response_text);

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
    printf ("%s\n", value);
    if (value && !strcmp ((gchar *)value, "recordings:I Still"))
    {
        if (g_file_get_contents ("./tests/recordings-i_still.xml", &response_text,
                                 &length, NULL))
        {printf ("sending message %s\n", response_text);

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

    g_file_get_contents ("./tests/metadata.xml", &response_text, &length, NULL);
    soup_message_set_response (msg, "application/xml", SOUP_MEMORY_TAKE,
                               response_text, strlen (response_text));
    soup_message_set_status (msg, 200);
}

int
main ()
{
    SoupServer *server;

    g_type_init ();

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
                             (SoupServerCallback)never_gone_handler, NULL, NULL);
    soup_server_add_handler (server,
                             "/ws/2/release/182fbd5e-5f3e-413a-9ff1-8a9a81d052c0",
                             (SoupServerCallback)westlife_release_info_handler, NULL, NULL);
    soup_server_add_handler (server, "/ws/2/recording",
                             (SoupServerCallback)recording_handler, NULL,
                             NULL); 
    soup_server_add_handler (server,
                             "/ws/2/recording/ddc5fc5c-8a83-4d20-a711-313797030da6",
                             (SoupServerCallback)i_still_handler, NULL,
                             NULL);
    soup_server_add_handler (server, "/ws/2/discid",
                             (SoupServerCallback)discid_handler, NULL, NULL);

    soup_server_run (server);

    return 0;
}