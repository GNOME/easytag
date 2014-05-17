
#ifndef __MB_SEARCH_H__
#define __MB_SEARCH_H__

#include <gtk/gtk.h>
#include <musicbrainz5/mb5_c.h>

#define SEARCH_LIMIT_STR "5"
#define SEARCH_LIMIT_INT 5

enum MB_ENTITY_TYPE
{
    MB_ENTITY_TYPE_ARTIST = 0,
    MB_ENTITY_TYPE_ALBUM,
    MB_ENTITY_TYPE_TRACK,
    MB_ENTITY_TYPE_COUNT,
};

typedef struct
{
    Mb5Entity entity;
    enum MB_ENTITY_TYPE type;    
} EtMbEntity;

/**************
 * Prototypes *
 **************/

void
et_musicbrainz_search_in_entity (gchar *string, enum MB_ENTITY_TYPE child_type,
                                 enum MB_ENTITY_TYPE parent_type,
                                 gchar *parent_mbid, GNode *root);
void
et_musicbrainz_search (gchar *string, enum MB_ENTITY_TYPE type, GNode *root);
void
free_mb_tree (GNode *node);
#endif /* __MB_SEARCH_H__ */