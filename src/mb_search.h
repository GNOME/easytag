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
et_musicbrainz_search_in_entity (enum MB_ENTITY_TYPE child_type,
                                 enum MB_ENTITY_TYPE parent_type,
                                 gchar *parent_mbid, GNode *root);
void
et_musicbrainz_search (gchar *string, enum MB_ENTITY_TYPE type, GNode *root);
void
free_mb_tree (GNode *node);
#endif /* __MB_SEARCH_H__ */