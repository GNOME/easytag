/* bar.h - 2000/05/05 */
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


#ifndef __BAR_H__
#define __BAR_H__

/***************
 * Declaration *
 ***************/
GtkWidget      *MenuBar;
GtkWidget      *ProgressBar;
GtkUIManager   *UIManager;
GtkActionGroup *ActionGroup;
guint           StatusBarContext;

GtkWidget *CheckMenuItemBrowseSubdirMainMenu;
GtkWidget *CheckMenuItemBrowseHiddenDirMainMenu;

#define MENU_FILE       "FileMenu"
#define MENU_BROWSER    "BrowserMenu"
#define MENU_SCANNER    "ScannerMenu"
#define MENU_MISC       "MiscMenu"
#define MENU_SETTINGS   "SettingsMenu"
#define MENU_HELP       "HelpMenu"

#define MENU_FILE_SORT_TAG      "SortTagMenu"
#define MENU_FILE_SORT_PROP     "SortPropMenu"
#define MENU_SORT_TAG_PATH      "FileMenu/SortTagMenu"
#define MENU_SORT_PROP_PATH     "FileMenu/SortPropMenu"

#define POPUP_FILE              "FilePopup"
#define POPUP_DIR               "DirPopup"
#define POPUP_SUBMENU_SCANNER   "ScannerSubpopup"
#define POPUP_DIR_RUN_AUDIO     "DirPopupRunAudio"
#define POPUP_LOG               "LogPopup"

#define AM_PREV                     "PreviousFile"
#define AM_NEXT                     "NextFile"
#define AM_FIRST                    "FirstFile"
#define AM_LAST                     "LastFile"
#define AM_SCAN                     "ScanFile"
#define AM_REMOVE                   "RemoveTag"
#define AM_UNDO                     "UndoFile"
#define AM_REDO                     "RedoFile"
#define AM_UNDO_HISTORY             "Undo"
#define AM_REDO_HISTORY             "Redo"
#define AM_SAVE                     "SaveFile"
#define AM_SAVE_FORCED              "SaveFileForced"
#define AM_SELECT_ALL_FILES         "SelAll"
#define AM_UNSELECT_ALL_FILES       "UnselAll"
#define AM_INVERT_SELECTION         "SelInv"
#define AM_DELETE_FILE              "DeleteFile"
#define AM_LOAD_HOME_DIR            "GoToHome"
#define AM_LOAD_DESKTOP_DIR         "GoToDesktop"
#define AM_LOAD_DOCUMENTS_DIR       "GoToDocument"
#define AM_LOAD_DOWNLOADS_DIR       "GoToDownload"
#define AM_LOAD_MUSIC_DIR           "GoToMusic"
#define AM_LOAD_DEFAULT_DIR         "GoToDefaultPath"
#define AM_SET_PATH_AS_DEFAULT      "SetDefaultPath"
#define AM_RENAME_DIR               "RenameDir"
#define AM_BROWSE_SUBDIR            "BrowseSubdir"
#define AM_BROWSER_HIDDEN_DIR       "BrowseHiddenDir"
#define AM_COLLAPSE_TREE            "CollapseTree"
#define AM_INITIALIZE_TREE          "RefreshTree"
#define AM_RELOAD_DIRECTORY         "ReloadDir"
#define AM_TREE_OR_ARTISTALBUM_VIEW "ViewMode"
#define AM_BROWSE_DIRECTORY_WITH    "BrowseDir"
#define AM_OPEN_FILE_WITH           "OpenFile"
#define AM_OPEN_OPTIONS_WINDOW      "Preferences"
#define AM_SCANNER_FILL_TAG         "FillTag"
#define AM_SCANNER_RENAME_FILE      "RenameFile"
#define AM_SCANNER_PROCESS_FIELDS   "ProcessFields"
#define AM_SEARCH_FILE              "SearchFile"
#define AM_CDDB_SEARCH_FILE         "CDDBSearchFile"
#define AM_CDDB_SEARCH              "CDDBSearch"
#define AM_FILENAME_FROM_TXT        "LoadFilenames"
#define AM_WRITE_PLAYLIST           "WritePlaylist"
#define AM_RUN_AUDIO_PLAYER         "RunAudio"
#define AM_OPEN_ABOUT_WINDOW        "About"
#define AM_QUIT                     "Quit"

#define AM_ARTIST_RUN_AUDIO_PLAYER  "ArtistRunAudio"
#define AM_ARTIST_OPEN_FILE_WITH    "ArtistOpenFile"
#define AM_ALBUM_RUN_AUDIO_PLAYER   "AlbumRunAudio"
#define AM_ALBUM_OPEN_FILE_WITH     "AlbumOpenFile"

#define AM_LOG_CLEAN                "CleanLog"

#define AM_STOP                     "Stop"
#define AM_VIEWMODE_TOGGLE          "ViewModeToggle"

#define AM_SORT_ASCENDING_FILENAME          "SortFilenameAsc"
#define AM_SORT_DESCENDING_FILENAME         "SortFilenameDesc"
#define AM_SORT_ASCENDING_CREATION_DATE     "SortDateAsc"
#define AM_SORT_DESCENDING_CREATION_DATE    "SortDateDesc"
#define AM_SORT_ASCENDING_TRACK_NUMBER      "SortTrackNumAsc"
#define AM_SORT_DESCENDING_TRACK_NUMBER     "SortTrackNumDesc"
#define AM_SORT_ASCENDING_TITLE             "SortTitleAsc"
#define AM_SORT_DESCENDING_TITLE            "SortTitleDesc"
#define AM_SORT_ASCENDING_ARTIST            "SortArtistAsc"
#define AM_SORT_DESCENDING_ARTIST           "SortArtistDesc"
#define AM_SORT_ASCENDING_ALBUM_ARTIST      "SortAlbumArtistAsc"
#define AM_SORT_DESCENDING_ALBUM_ARTIST     "SortAlbumArtistDesc"
#define AM_SORT_ASCENDING_ALBUM             "SortAlbumAsc"
#define AM_SORT_DESCENDING_ALBUM            "SortAlbumDesc"
#define AM_SORT_ASCENDING_YEAR              "SortYearAsc"
#define AM_SORT_DESCENDING_YEAR             "SortYearDesc"
#define AM_SORT_ASCENDING_GENRE             "SortGenreAsc"
#define AM_SORT_DESCENDING_GENRE            "SortGenreDesc"
#define AM_SORT_ASCENDING_COMMENT           "SortCommentAsc"
#define AM_SORT_DESCENDING_COMMENT          "SortCommentDesc"
#define AM_SORT_ASCENDING_COMPOSER          "SortComposerAsc"
#define AM_SORT_DESCENDING_COMPOSER         "SortComposerDesc"
#define AM_SORT_ASCENDING_ORIG_ARTIST       "SortOrigArtistAsc"
#define AM_SORT_DESCENDING_ORIG_ARTIST      "SortOrigArtistDesc"
#define AM_SORT_ASCENDING_COPYRIGHT         "SortCopyrightAsc"
#define AM_SORT_DESCENDING_COPYRIGHT        "SortCopyrightDesc"
#define AM_SORT_ASCENDING_URL               "SortUrlAsc"
#define AM_SORT_DESCENDING_URL              "SortUrlDesc"
#define AM_SORT_ASCENDING_ENCODED_BY        "SortEncodedByAsc"
#define AM_SORT_DESCENDING_ENCODED_BY       "SortEncodedByDesc"
#define AM_SORT_ASCENDING_FILE_TYPE         "SortTypeAsc"
#define AM_SORT_DESCENDING_FILE_TYPE        "SortTypeDesc"
#define AM_SORT_ASCENDING_FILE_SIZE         "SortSizeAsc"
#define AM_SORT_DESCENDING_FILE_SIZE        "SortSizeDesc"
#define AM_SORT_ASCENDING_FILE_DURATION     "SortDurationAsc"
#define AM_SORT_DESCENDING_FILE_DURATION    "SortDurationDesc"
#define AM_SORT_ASCENDING_FILE_BITRATE      "SortBitrateAsc"
#define AM_SORT_DESCENDING_FILE_BITRATE     "SortBitrateDesc"
#define AM_SORT_ASCENDING_FILE_SAMPLERATE   "SortSamplerateAsc"
#define AM_SORT_DESCENDING_FILE_SAMPLERATE  "SortSamplerateDesc"

typedef struct _Action_Pair Action_Pair;
struct _Action_Pair {
    const gchar *action;
    GQuark quark;
};

/**************
 * Prototypes *
 **************/

void       Create_UI           (GtkWidget **menubar, GtkWidget **toolbar);
GtkWidget *Create_Status_Bar   (void);
void       Statusbar_Message   (gchar *message, gint with_timer);
GtkWidget *Create_Progress_Bar (void);

void Check_Menu_Item_Update_Browse_Subdir  (void);
void Check_Menu_Item_Update_Browse_Hidden_Dir  (void);

#endif /* __BAR_H__ */
