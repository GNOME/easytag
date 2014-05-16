/* musicbrainz_dialog.c - 2014/05/05 */
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
 
#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "gtk2_compat.h"
#include "easytag.h"
#include "log.h"
#include "musicbrainz_dialog.h"
#include "mbentityview.h"

#define SEARCH_LIMIT_STR "5"
#define SEARCH_LIMIT_INT 5
 
/***************
 * Declaration *
 ***************/

static GtkBuilder *builder;
static GtkWidget *mbDialog;
static GtkWidget *entityView;
static GNode *mb_tree_root;

/**************
 * Prototypes *
 **************/

static void
et_musicbrainz_search_in_entity (gchar *string, enum MB_ENTITY_TYPE child_type,
                                 enum MB_ENTITY_TYPE parent_type,
                                 gchar *parent_mbid, GNode *root);
static void
et_musicbrainz_search (gchar *string, enum MB_ENTITY_TYPE type, GNode *root);

/*************
 * Functions *
 *************/

static void
et_musicbrainz_search_in_entity (gchar *string, enum MB_ENTITY_TYPE child_type,
                                 enum MB_ENTITY_TYPE parent_type,
                                 gchar *parent_mbid, GNode *root)
{
    Mb5Query query;
    Mb5Metadata metadata;
    char error_message[256];
    tQueryResult result;
    //int httpcode;
    char *param_values[1];
    char *param_names[1];

    param_names [0] = "inc";
    query = mb5_query_new ("easytag", NULL, 0);

    if (child_type == MB_ENTITY_TYPE_ALBUM &&
        parent_type == MB_ENTITY_TYPE_ARTIST)
    {
        param_values [0] = "releases";
        metadata = mb5_query_query (query, "artist", parent_mbid, "", 1, param_names,
                                    param_values);
        result = mb5_query_get_lastresult (query);
        //httpcode = mb5_query_get_lasthttpcode (query);
        if (result == eQuery_Success)
        {
            if (metadata)
            {
                int i;
                Mb5ReleaseList list;
                Mb5Artist artist;
                artist = mb5_metadata_get_artist (metadata);
                list = mb5_artist_get_releaselist (artist);

                for (i = 0; i < mb5_release_list_size (list); i++)
                {
                    Mb5Release release;
                    release = mb5_artist_list_item (list, i);
                    if (release)
                    {
                        GNode *node;
                        EtMbEntity *entity;
                        entity = g_malloc (sizeof (EtMbEntity));
                        entity->entity = release;
                        entity->type = MB_ENTITY_TYPE_ALBUM;
                        node = g_node_new (entity);
                        g_node_append (root, node);
                    }
                }
            }

            g_free (param_values [0]);
        }
        else
        {
            goto err;
        }
    }
    else if (child_type == MB_ENTITY_TYPE_TRACK &&
             parent_type == MB_ENTITY_TYPE_ALBUM)
    {
        
    }

    return;
    err:
    mb5_query_get_lasterrormessage (query, error_message,
                                    sizeof(error_message));
    printf ("Error searching MusicBrainz Database: '%s'\n", error_message);
    mb5_query_delete (query);
}

static void
et_musicbrainz_search (gchar *string, enum MB_ENTITY_TYPE type, GNode *root)
{
    /* TODO: to free metadata, first use mb5_<entity>_copy to copy that entity */
    Mb5Query Query;
    Mb5Metadata Metadata;
    char ErrorMessage[256];
    tQueryResult Result;
    //int HTTPCode;
    char *ParamValues[2];
    char *ParamNames[2];

    ParamNames [0] = "query";
    ParamNames [1] = "limit";
    ParamValues [1] = SEARCH_LIMIT_STR;
    Query = mb5_query_new ("easytag", NULL, 0);

    if (type == MB_ENTITY_TYPE_ARTIST)
    {
        ParamValues [0] = g_strconcat ("artist:", string, NULL);
        Metadata = mb5_query_query (Query, "artist", "", "", 2, ParamNames,
                                    ParamValues);
        Result = mb5_query_get_lastresult (Query);
        //HTTPCode = mb5_query_get_lasthttpcode (Query);
        if (Result == eQuery_Success)
        {
            if (Metadata)
            {
                int i;
                Mb5ArtistList ArtistList;
                ArtistList = mb5_metadata_get_artistlist (Metadata);

                for (i = 0; i < mb5_artist_list_size (ArtistList); i++)
                {
                    Mb5Artist Artist;
                    Artist = mb5_artist_list_item (ArtistList, i);
                    if (Artist)
                    {
                        GNode *node;
                        EtMbEntity *entity;
                        entity = g_malloc (sizeof (EtMbEntity));
                        entity->entity = Artist;
                        entity->type = MB_ENTITY_TYPE_ARTIST;
                        node = g_node_new (entity);
                        g_node_append (root, node);
                    }
                }
            }

            g_free (ParamValues [0]);
        }
        else
        {
            goto err;
        }
    }
    else if (type == MB_ENTITY_TYPE_ALBUM)
    {
        ParamValues [0] = g_strconcat ("release:", string, NULL);
        Metadata = mb5_query_query (Query, "release", "", "", 2, ParamNames,
                                    ParamValues);
        Result = mb5_query_get_lastresult (Query);
        //HTTPCode = mb5_query_get_lasthttpcode (Query);
        if (Result == eQuery_Success)
        {
            if (Metadata)
            {
                int i;
                Mb5ReleaseList list;
                list = mb5_metadata_get_releaselist (Metadata);

                for (i = 0; i < mb5_release_list_size (list); i++)
                {
                    Mb5Release release;
                    release = mb5_artist_list_item (list, i);
                    if (release)
                    {
                        GNode *node;
                        EtMbEntity *entity;
                        entity = g_malloc (sizeof (EtMbEntity));
                        entity->entity = release;
                        entity->type = MB_ENTITY_TYPE_ALBUM;
                        node = g_node_new (entity);
                        g_node_append (root, node);
                    }
                }
            }

            g_free (ParamValues [0]);
        }
        else
        {
            goto err;
        }
    }
    else if (type == MB_ENTITY_TYPE_TRACK)
    {
    }

    return;
    err:
    mb5_query_get_lasterrormessage (Query, ErrorMessage,
                                    sizeof(ErrorMessage));
    printf ("Error searching MusicBrainz Database: '%s'\n", ErrorMessage);
    mb5_query_delete (Query);
}

static void
btn_manual_find_clicked (GtkWidget *btn, gpointer user_data)
{
    GtkWidget *cb_manual_search;
    GtkWidget *cb_manual_search_in;
    int type;

    cb_manual_search = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "cbManualSearch"));
    cb_manual_search_in = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "cbManualSearchIn"));
    type = gtk_combo_box_get_active (GTK_COMBO_BOX (cb_manual_search_in));

    if (g_node_first_child (mb_tree_root))
    {
        /* TODO: Clear the tree */
    }

    et_musicbrainz_search (gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cb_manual_search)),
                           type, mb_tree_root);
    et_mb_entity_view_set_tree_root (ET_MB_ENTITY_VIEW (entityView),
                                     mb_tree_root);
}

/*
 * et_open_musicbrainz_dialog:
 *
 * This function will open the musicbrainz dialog.
 */
void
et_open_musicbrainz_dialog ()
{
    GtkWidget *cb_manual_search_in;

    mb_tree_root = g_node_new (NULL);
    entityView = et_mb_entity_view_new ();
    builder = gtk_builder_new ();
    /* TODO: Check the error. */
    gtk_builder_add_from_resource (builder,
                                   "/org/gnome/EasyTAG/musicbrainz_dialog.ui",
                                   NULL);

    mbDialog = GTK_WIDGET (gtk_builder_get_object (builder, "mbDialog"));
    gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (builder, "centralBox")),
                        entityView, TRUE, TRUE, 2);
    /* FIXME: This should not be needed. */
    gtk_box_reorder_child (GTK_BOX (gtk_builder_get_object (builder, "centralBox")),
                           entityView, 0);
    g_signal_connect (gtk_builder_get_object (builder, "btnManualFind"),
                      "clicked", G_CALLBACK (btn_manual_find_clicked),
                      NULL);
    cb_manual_search_in = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "cbManualSearchIn"));

    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb_manual_search_in), "Artist");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb_manual_search_in), "Album");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb_manual_search_in), "Track");

    gtk_widget_show_all (mbDialog);
    gtk_dialog_run (GTK_DIALOG (mbDialog));
    gtk_widget_destroy (mbDialog);
    g_object_unref (G_OBJECT (builder));
}