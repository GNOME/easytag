/* cddb.h - 2002/09/15 */
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


#ifndef __CDDB_H__
#define __CDDB_H__


/****************
 * Declarations *
 ****************/

GtkWidget *CddbWindow;
GtkWidget *CddbWindowHPaned;

GtkWidget *CddbSearchInAllFields;
GtkWidget *CddbSearchInArtistField;
GtkWidget *CddbSearchInTitleField;
GtkWidget *CddbSearchInTrackNameField;
GtkWidget *CddbSearchInOtherField;

GtkWidget *CddbSeparatorH;
GtkWidget *CddbSeparatorV;
GtkWidget *CddbShowCategoriesButton;

GtkWidget *CddbSearchInAllCategories;
GtkWidget *CddbSearchInBluesCategory;
GtkWidget *CddbSearchInClassicalCategory;
GtkWidget *CddbSearchInCountryCategory;
GtkWidget *CddbSearchInFolkCategory;
GtkWidget *CddbSearchInJazzCategory;
GtkWidget *CddbSearchInMiscCategory;
GtkWidget *CddbSearchInNewageCategory;
GtkWidget *CddbSearchInReggaeCategory;
GtkWidget *CddbSearchInRockCategory;
GtkWidget *CddbSearchInSoundtrackCategory;

GtkWidget *CddbSetToAllFields;
GtkWidget *CddbSetToTitle;
GtkWidget *CddbSetToArtist;
GtkWidget *CddbSetToAlbum;
GtkWidget *CddbSetToYear;
GtkWidget *CddbSetToTrack;
GtkWidget *CddbSetToTrackTotal;
GtkWidget *CddbSetToGenre;
GtkWidget *CddbSetToFileName;

GtkWidget *CddbRunScanner;
GtkWidget *CddbUseDLM2; // '2' as also used in the prefs.c
GtkWidget *CddbUseLocalAccess;



/*
 * Structure used for each item of the album list. Aslo attached to each row of the album list
 */
typedef struct _CddbAlbum CddbAlbum;
struct _CddbAlbum
{
    gchar *server_name;     /* Remote access : Param of server name used for the connection     - Local access : NULL */
    gint   server_port;     /* Remote access : Param of server port used for the connection     - Local access : 0 */
    gchar *server_cgi_path; /* Remote access : Param of server cgi path used for the connection - Local access : discid file path */

    GdkPixbuf *bitmap;      /* Pixmap corresponding to the server  */

    gchar *artist_album;    /* CDDB artist+album (allocated) */
    gchar *category;        /* CDDB genre (allocated) */
    gchar *id;              /* example : 8d0de30c (allocated) */
    GList *track_list;      /* List of tracks name of the album (list of CddbTrackAlbum items) */
    gboolean other_version; /* To TRUE if this album is an other version of the previous one */

    // We fill these data when loading the track list
    gchar *artist;    /* (allocated) */
    gchar *album;     /* (allocated) */
    gchar *genre;     /* (allocated) */
    gchar *year;      /* (allocated) */
    guint  duration;
};


/*
 * Structure used for each item of the track_list of the CddbAlbum structure.
 */
typedef struct _CddbTrackAlbum CddbTrackAlbum;
struct _CddbTrackAlbum
{
    guint  track_number;
    gchar *track_name;    /* (allocated) */
    guint  duration;
    CddbAlbum *cddbalbum; /* Pointer to the parent CddbAlbum structure (to access quickly to album properties) */
};


typedef struct _CddbTrackFrameOffset CddbTrackFrameOffset;
struct _CddbTrackFrameOffset
{
    gulong  offset;
};



/**************
 * Prototypes *
 **************/

void Init_CddbWindow  (void);
void Open_Cddb_Window (void);
void Cddb_Popup_Menu_Search_Selected_File (void);
void Cddb_Window_Apply_Changes (void);



#endif /* __CDDB_H__ */
