/* picture.h - 2004/11/21 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef __PICTURE_H__
#define __PICTURE_H__

#include "et_core.h"


/***************
 * Declaration *
 ***************/

/* Defined in et_core.h
typedef struct _Picture Picture;
struct _Picture
{
    gint     type;
    gchar   *description;
    gint     width;         // Original width of the picture
    gint     height;        // Original height of the picture
    gulong   size;          // Picture size in bits
    guchar  *data;
    Picture *next;
};*/

typedef enum // Picture types
{
    // Same values for FLAC, see: http://flac.sourceforge.net/api/group__flac__format.html#ga113
    PICTURE_TYPE_OTHER = 0,
    PICTURE_TYPE_FILE_ICON,
    PICTURE_TYPE_OTHER_FILE_ICON,
    PICTURE_TYPE_FRONT_COVER,
    PICTURE_TYPE_BACK_COVER,
    PICTURE_TYPE_LEAFLET_PAGE,
    PICTURE_TYPE_MEDIA,
    PICTURE_TYPE_LEAD_ARTIST_LEAD_PERFORMER_SOLOIST,
    PICTURE_TYPE_ARTIST_PERFORMER,
    PICTURE_TYPE_CONDUCTOR,
    PICTURE_TYPE_BAND_ORCHESTRA,
    PICTURE_TYPE_COMPOSER,
    PICTURE_TYPE_LYRICIST_TEXT_WRITER,
    PICTURE_TYPE_RECORDING_LOCATION,
    PICTURE_TYPE_DURING_RECORDING,
    PICTURE_TYPE_DURING_PERFORMANCE,
    PICTURE_TYPE_MOVIDE_VIDEO_SCREEN_CAPTURE,
    PICTURE_TYPE_A_BRIGHT_COLOURED_FISH,
    PICTURE_TYPE_ILLUSTRATION,
    PICTURE_TYPE_BAND_ARTIST_LOGOTYPE,
    PICTURE_TYPE_PUBLISHER_STUDIO_LOGOTYPE,
    
    PICTURE_TYPE_UNDEFINED
} Picture_Type;

typedef enum
{
    PICTURE_FORMAT_JPEG,
    PICTURE_FORMAT_PNG,
    PICTURE_FORMAT_UNKNOWN
} Picture_Format;

enum // Columns for PictureEntryView
{
    PICTURE_COLUMN_PIC, // Column 0
    PICTURE_COLUMN_TEXT,
    PICTURE_COLUMN_DATA,
    PICTURE_COLUMN_COUNT
};

enum // Columns for list in properties window
{
    PICTURE_TYPE_COLUMN_TEXT, // Column 0
    PICTURE_TYPE_COLUMN_TYPE_CODE,
    PICTURE_TYPE_COLUMN_COUNT
};

enum
{
    TARGET_URI_LIST
};


/**************
 * Prototypes *
 **************/

void Tag_Area_Picture_Drag_Data (GtkWidget *widget, GdkDragContext *dc, 
                                 gint x, gint y, GtkSelectionData *selection_data,
                                 guint info, guint t, gpointer data);
void Picture_Selection_Changed_cb (GtkTreeSelection *selection, gpointer data);

void Picture_Add_Button_Clicked         (GObject *object);
void Picture_Properties_Button_Clicked  (GObject *object);
void Picture_Save_Button_Clicked        (GObject *object);
void Picture_Clear_Button_Clicked       (GObject *object);

void PictureEntry_Clear  (void);
void PictureEntry_Update (Picture *pic, gboolean select_it);

Picture       *Picture_Allocate         (void);
Picture       *Picture_Copy_One         (const Picture *pic);
Picture       *Picture_Copy             (const Picture *pic);
void           Picture_Free             (Picture *pic);
Picture_Format Picture_Format_From_Data (Picture *pic);
const gchar   *Picture_Mime_Type_String (Picture_Format format);

gboolean Picture_Entry_View_Button_Pressed (GtkTreeView *treeview, GdkEventButton *event, gpointer data);
gboolean Picture_Entry_View_Key_Pressed    (GtkTreeView *treeview, GdkEvent *event, gpointer data);


#endif /* __PICTURE_H__ */
