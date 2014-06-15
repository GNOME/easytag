#include <glib.h>

#include "mb_search.h"

static gchar name[NAME_MAX_SIZE];
static gchar mbid[NAME_MAX_SIZE];
void
mb_search_test ()
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
}

void
mb_search_in_test ()
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

int
main(int argc, char** argv)
{
    g_test_init(&argc, &argv, NULL);
    et_musicbrainz_search_set_server_port ("localhost", 8088);
    g_test_add_func("/musicbrainz/mb_search", mb_search_test);
    g_test_add_func("/musicbrainz/mb_search_in", mb_search_in_test);

    return g_test_run();
}