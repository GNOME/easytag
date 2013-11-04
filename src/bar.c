/* bar.c - 2000/05/05 */
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "bar.h"
#include "easytag.h"
#include "about.h"
#include "prefs.h"
#include "setting.h"
#include "browser.h"
#include "scan_dialog.h"
#include "cddb.h"
#include "log.h"
#include "misc.h"
#include "charset.h"
#include "ui_manager.h"
#include "gtk2_compat.h"

/***************
 * Declaration *
 ***************/
static GtkWidget *StatusBar = NULL;
static guint StatusBarContext;
static guint timer_cid;
static guint tooltip_cid;
static guint StatusbarTimerId = 0;
static GList *ActionPairsList = NULL;

/**************
 * Prototypes *
 **************/

static void Check_Menu_Item_Toggled_Browse_Hidden_Dir (GtkWidget *checkmenuitem);
static void Check_Menu_Item_Toggled_Browse_Subdir (GtkWidget *checkmenuitem);
static void Init_Menu_Bar (void);
static void Statusbar_Remove_Timer (void);

static void et_statusbar_push_tooltip (const gchar *message);
static void et_statusbar_pop_tooltip (void);
static void et_ui_manager_on_connect_proxy (GtkUIManager *manager,
                                            GtkAction *action,
                                            GtkWidget *proxy,
                                            gpointer user_data);
static void et_ui_manager_on_disconnect_proxy (GtkUIManager *manager,
                                               GtkAction *action,
                                               GtkWidget *proxy,
                                               gpointer user_data);
static void on_menu_item_select (GtkMenuItem *item, gpointer user_data);
static void on_menu_item_deselect (GtkMenuItem *item, gpointer user_data);

/*************
 * Functions o
 *************/

/*
 * Dynamic reimplementation of switch macros
 */
#define QCASE(string,callback) if (quark == g_quark_from_string((string))) { (callback)(); }
#define QCASE_DATA(string,callback,data) if (quark == g_quark_from_string((string))) { (callback)((data)); }

/*
 * Menu bar
 */
static void
Menu_Sort_Action (GtkAction *item, gpointer data)
{
    const gchar *action = gtk_action_get_name(item);
    GQuark quark = g_quark_from_string(action);

    QCASE_DATA(AM_SORT_ASCENDING_FILENAME,         ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_FILENAME);
    QCASE_DATA(AM_SORT_DESCENDING_FILENAME,        ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_FILENAME);
    QCASE_DATA(AM_SORT_ASCENDING_CREATION_DATE,    ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_CREATION_DATE);
    QCASE_DATA(AM_SORT_DESCENDING_CREATION_DATE,   ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_CREATION_DATE);
    QCASE_DATA(AM_SORT_ASCENDING_TRACK_NUMBER,     ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_TRACK_NUMBER);
    QCASE_DATA(AM_SORT_DESCENDING_TRACK_NUMBER,    ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_TRACK_NUMBER);
    QCASE_DATA(AM_SORT_ASCENDING_TITLE,            ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_TITLE);
    QCASE_DATA(AM_SORT_DESCENDING_TITLE,           ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_TITLE);
    QCASE_DATA(AM_SORT_ASCENDING_ARTIST,           ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_ARTIST);
    QCASE_DATA(AM_SORT_DESCENDING_ARTIST,          ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_ARTIST);
    QCASE_DATA(AM_SORT_ASCENDING_ALBUM_ARTIST,     ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_ALBUM_ARTIST);
    QCASE_DATA(AM_SORT_DESCENDING_ALBUM_ARTIST,    ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_ALBUM_ARTIST);
    QCASE_DATA(AM_SORT_ASCENDING_ALBUM,            ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_ALBUM);
    QCASE_DATA(AM_SORT_DESCENDING_ALBUM,           ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_ALBUM);
    QCASE_DATA(AM_SORT_ASCENDING_YEAR,             ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_YEAR);
    QCASE_DATA(AM_SORT_DESCENDING_YEAR,            ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_YEAR);
    QCASE_DATA(AM_SORT_ASCENDING_GENRE,            ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_GENRE);
    QCASE_DATA(AM_SORT_DESCENDING_GENRE,           ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_GENRE);
    QCASE_DATA(AM_SORT_ASCENDING_COMMENT,          ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_COMMENT);
    QCASE_DATA(AM_SORT_DESCENDING_COMMENT,         ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_COMMENT);
    QCASE_DATA(AM_SORT_ASCENDING_COMPOSER,         ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_COMPOSER);
    QCASE_DATA(AM_SORT_DESCENDING_COMPOSER,        ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_COMPOSER);
    QCASE_DATA(AM_SORT_ASCENDING_ORIG_ARTIST,      ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_ORIG_ARTIST);
    QCASE_DATA(AM_SORT_DESCENDING_ORIG_ARTIST,     ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_ORIG_ARTIST);
    QCASE_DATA(AM_SORT_ASCENDING_COPYRIGHT,        ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_COPYRIGHT);
    QCASE_DATA(AM_SORT_DESCENDING_COPYRIGHT,       ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_COPYRIGHT);
    QCASE_DATA(AM_SORT_ASCENDING_URL,              ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_URL);
    QCASE_DATA(AM_SORT_DESCENDING_URL,             ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_URL);
    QCASE_DATA(AM_SORT_ASCENDING_ENCODED_BY,       ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_ENCODED_BY);
    QCASE_DATA(AM_SORT_DESCENDING_ENCODED_BY,      ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_ENCODED_BY);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_TYPE,        ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_FILE_TYPE);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_TYPE,       ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_FILE_TYPE);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_SIZE,        ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_FILE_SIZE);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_SIZE,       ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_FILE_SIZE);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_DURATION,    ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_FILE_DURATION);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_DURATION,   ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_FILE_DURATION);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_BITRATE,     ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_FILE_BITRATE);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_BITRATE,    ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_FILE_BITRATE);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_SAMPLERATE,  ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_ASCENDING_FILE_SAMPLERATE);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_SAMPLERATE, ET_Sort_Displayed_File_List_And_Update_UI, SORTING_BY_DESCENDING_FILE_SAMPLERATE);
    QCASE_DATA(AM_INITIALIZE_TREE,                 Browser_Tree_Rebuild,                      NULL);
    Browser_List_Refresh_Sort ();
}

void
Create_UI (GtkWindow *window, GtkWidget **ppmenubar, GtkWidget **pptoolbar)
{
    GtkWidget *menubar;
    GtkWidget *toolbar;

    /*
     * Structure :
     *  - name
     *  - stock_id
     *  - label
     *  - accelerator
     *  - tooltip
     *  - callback
     */
    GtkActionEntry ActionEntries[] =
    {

        /*
         * Main Menu Actions
         */
        { MENU_FILE,                          NULL,                      _("_File"),                         NULL, NULL,                               NULL},
        { MENU_FILE_SORT_TAG, GTK_STOCK_SORT_ASCENDING, _("Sort List by Tag"),
          NULL, NULL, NULL },
        { MENU_FILE_SORT_PROP, GTK_STOCK_SORT_ASCENDING,
          _("Sort List by Property"), NULL, NULL, NULL },
        { AM_SORT_ASCENDING_FILENAME,         GTK_STOCK_SORT_ASCENDING,  _("Ascending by filename"),         NULL, _("Ascending by filename"),         G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_FILENAME,        GTK_STOCK_SORT_DESCENDING, _("Descending by filename"),        NULL, _("Descending by filename"),        G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_CREATION_DATE,    GTK_STOCK_SORT_ASCENDING,  _("Ascending by creation date"),    NULL, _("Ascending by creation date"),    G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_CREATION_DATE,   GTK_STOCK_SORT_DESCENDING, _("Descending by creation date"),   NULL, _("Descending by creation date"),   G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_TRACK_NUMBER,     GTK_STOCK_SORT_ASCENDING,  _("Ascending by track number"),     NULL, _("Ascending by track number"),     G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_TRACK_NUMBER,    GTK_STOCK_SORT_DESCENDING, _("Descending by track number"),    NULL, _("Descending by track number"),    G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_TITLE,            GTK_STOCK_SORT_ASCENDING,  _("Ascending by title"),            NULL, _("Ascending by title"),            G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_TITLE,           GTK_STOCK_SORT_DESCENDING, _("Descending by title"),           NULL, _("Descending by title"),           G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_ARTIST,           GTK_STOCK_SORT_ASCENDING,  _("Ascending by artist"),           NULL, _("Ascending by artist"),           G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_ARTIST,          GTK_STOCK_SORT_DESCENDING, _("Descending by artist"),          NULL, _("Descending by artist"),          G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_ALBUM_ARTIST,     GTK_STOCK_SORT_ASCENDING,  _("Ascending by album artist"),     NULL, _("Ascending by album artist"),   G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_ALBUM_ARTIST,    GTK_STOCK_SORT_DESCENDING, _("Descending by album artist"),    NULL, _("Descending by album artist"),   G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_ALBUM,            GTK_STOCK_SORT_ASCENDING,  _("Ascending by album"),            NULL, _("Ascending by album"),            G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_ALBUM,           GTK_STOCK_SORT_DESCENDING, _("Descending by album"),           NULL, _("Descending by album"),           G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_YEAR,             GTK_STOCK_SORT_ASCENDING,  _("Ascending by year"),             NULL, _("Ascending by year"),             G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_YEAR,            GTK_STOCK_SORT_DESCENDING, _("Descending by year"),            NULL, _("Descending by year"),            G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_GENRE,            GTK_STOCK_SORT_ASCENDING,  _("Ascending by genre"),            NULL, _("Ascending by genre"),            G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_GENRE,           GTK_STOCK_SORT_DESCENDING, _("Descending by genre"),           NULL, _("Descending by genre"),           G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_COMMENT,          GTK_STOCK_SORT_ASCENDING,  _("Ascending by comment"),          NULL, _("Ascending by comment"),          G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_COMMENT,         GTK_STOCK_SORT_DESCENDING, _("Descending by comment"),         NULL, _("Descending by comment"),         G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_COMPOSER,         GTK_STOCK_SORT_ASCENDING,  _("Ascending by composer"),         NULL, _("Ascending by composer"),         G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_COMPOSER,        GTK_STOCK_SORT_DESCENDING, _("Descending by composer"),        NULL, _("Descending by composer"),        G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_ORIG_ARTIST,      GTK_STOCK_SORT_ASCENDING,  _("Ascending by original artist"),  NULL, _("Ascending by original artist"),  G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_ORIG_ARTIST,     GTK_STOCK_SORT_DESCENDING, _("Descending by original artist"), NULL, _("Descending by original artist"), G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_COPYRIGHT,        GTK_STOCK_SORT_ASCENDING,  _("Ascending by copyright"),        NULL, _("Ascending by copyright"),        G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_COPYRIGHT,       GTK_STOCK_SORT_DESCENDING, _("Descending by copyright"),       NULL, _("Descending by copyright"),       G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_URL,              GTK_STOCK_SORT_ASCENDING,  _("Ascending by URL"),              NULL, _("Ascending by URL"),              G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_URL,             GTK_STOCK_SORT_DESCENDING, _("Descending by URL"),             NULL, _("Descending by URL"),             G_CALLBACK(Menu_Sort_Action) },
	/* Translators: the encoder name is supposed to be the name of a person
         * or organisation, but can sometimes be the name of an application. */
        { AM_SORT_ASCENDING_ENCODED_BY,       GTK_STOCK_SORT_ASCENDING,  _("Ascending by encoder name"),     NULL, _("Ascending by encoder name"),     G_CALLBACK(Menu_Sort_Action) },
	/* Translators: the encoder name is supposed to be the name of a person
         * or organisation, but can sometimes be the name of an application. */
        { AM_SORT_DESCENDING_ENCODED_BY,      GTK_STOCK_SORT_DESCENDING, _("Descending by encoder name"),    NULL, _("Descending by encoder name"),    G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_FILE_TYPE,        GTK_STOCK_SORT_ASCENDING,  _("Ascending by file type"),        NULL, _("Ascending by file type"),        G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_FILE_TYPE,       GTK_STOCK_SORT_DESCENDING, _("Descending by file type"),       NULL, _("Descending by file type"),       G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_FILE_SIZE,        GTK_STOCK_SORT_ASCENDING,  _("Ascending by file size"),        NULL, _("Ascending by file size"),        G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_FILE_SIZE,       GTK_STOCK_SORT_DESCENDING, _("Descending by file size"),       NULL, _("Descending by file size"),       G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_FILE_DURATION,    GTK_STOCK_SORT_ASCENDING,  _("Ascending by duration"),         NULL, _("Ascending by duration"),         G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_FILE_DURATION,   GTK_STOCK_SORT_DESCENDING, _("Descending by duration"),        NULL, _("Descending by duration"),        G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_FILE_BITRATE,     GTK_STOCK_SORT_ASCENDING,  _("Ascending by bitrate"),          NULL, _("Ascending by bitrate"),          G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_FILE_BITRATE,    GTK_STOCK_SORT_DESCENDING, _("Descending by bitrate"),         NULL, _("Descending by bitrate"),         G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_ASCENDING_FILE_SAMPLERATE,  GTK_STOCK_SORT_ASCENDING,  _("Ascending by samplerate"),       NULL, _("Ascending by samplerate"),       G_CALLBACK(Menu_Sort_Action) },
        { AM_SORT_DESCENDING_FILE_SAMPLERATE, GTK_STOCK_SORT_DESCENDING, _("Descending by samplerate"),      NULL, _("Descending by samplerate"),      G_CALLBACK(Menu_Sort_Action) },

        { AM_OPEN_FILE_WITH, GTK_STOCK_OPEN, _("Open Files With…"),
          "<Primary><Shift>O", _("Run a command on the selected files"),
          G_CALLBACK (Browser_Open_Run_Program_List_Window) },
        { AM_SELECT_ALL, GTK_STOCK_SELECT_ALL, NULL, "<Primary>A",
          _("Select all"), G_CALLBACK (et_on_action_select_all) },
        { AM_UNSELECT_ALL, "easytag-unselect-all", _("Unselect All"),
          "<Primary><Shift>A", _("Clear the current selection"),
          G_CALLBACK (et_on_action_unselect_all) },
        { AM_INVERT_SELECTION, "easytag-invert-selection",
          _("Invert File Selection"), "<Primary>I",
          _("Invert file selection"),
          G_CALLBACK (Action_Invert_Files_Selection) },
        { AM_DELETE_FILE,        GTK_STOCK_DELETE,           _("Delete Files"),             NULL,                _("Delete files"),            G_CALLBACK(Action_Delete_Selected_Files) },
        { AM_FIRST, GTK_STOCK_GOTO_FIRST, _("_First File"), "<Primary>Home",
          _("First file"), G_CALLBACK (Action_Select_First_File) },
        { AM_PREV,               GTK_STOCK_GO_BACK,          _("_Previous File"),             "Page_Up",           _("Previous file"),             G_CALLBACK(Action_Select_Prev_File) },
        { AM_NEXT,               GTK_STOCK_GO_FORWARD,       _("_Next File"),                 "Page_Down",         _("Next file"),                 G_CALLBACK(Action_Select_Next_File) },
        { AM_LAST, GTK_STOCK_GOTO_LAST, _("_Last File"), "<Primary>End",
          _("Last file"), G_CALLBACK (Action_Select_Last_File) },
        { AM_SCAN_FILES, GTK_STOCK_APPLY, _("S_can Files"), NULL,
          _("Scan selected files"), G_CALLBACK (Action_Scan_Selected_Files) },
        { AM_REMOVE, GTK_STOCK_CLEAR, _("_Remove Tags"), "<Primary>E",
          _("Remove tags"), G_CALLBACK (Action_Remove_Selected_Tags) },
        { AM_UNDO, GTK_STOCK_UNDO, _("_Undo Last Files Changes"), "<Primary>Z",
          _("Undo last files changes"),
          G_CALLBACK(Action_Undo_Selected_Files) },
        { AM_REDO, GTK_STOCK_REDO, _("R_edo Last Files Changes"),
          "<Primary><Shift>Z", _("Redo last files changes"),
          G_CALLBACK (Action_Redo_Selected_File) },
        { AM_SAVE, GTK_STOCK_SAVE, _("_Save Files"), "<Primary>S",
          _("Save changes to selected files"),
          G_CALLBACK(Action_Save_Selected_Files) },
        { AM_SAVE_FORCED, GTK_STOCK_SAVE, _("_Force Save Files"),
          "<Primary><Shift>S", _("Force saving files"),
          G_CALLBACK (Action_Force_Saving_Selected_Files) },
        { AM_UNDO_HISTORY,       GTK_STOCK_UNDO,             _("Undo Last Changes"),          NULL,                _("Undo last changes"),         G_CALLBACK(Action_Undo_From_History_List) },
        { AM_REDO_HISTORY,       GTK_STOCK_REDO,             _("Redo Last Changes"),          NULL,                _("Redo last changes"),         G_CALLBACK(Action_Redo_From_History_List) },
        { AM_QUIT, GTK_STOCK_QUIT, _("_Quit"), "<Primary>Q", _("Quit"),
          G_CALLBACK (Quit_MainWindow) },

        { MENU_BROWSER,                NULL,                   _("_Browser"),                      NULL,                NULL,                               NULL },
        { AM_LOAD_HOME_DIR, GTK_STOCK_HOME, _("_Home Directory"), "<Alt>Home",
          _("Go to home directory"),
          G_CALLBACK (Browser_Load_Home_Directory) },
        { AM_LOAD_DESKTOP_DIR, "user-desktop", _("Desktop Directory"), NULL,
          _("Go to desktop directory"),
          G_CALLBACK (Browser_Load_Desktop_Directory) },
        { AM_LOAD_DOCUMENTS_DIR, "folder-documents", _("Documents Directory"),
          NULL, _("Go to documents directory"),
          G_CALLBACK (Browser_Load_Documents_Directory) },
        { AM_LOAD_DOWNLOADS_DIR, "folder-download", _("Downloads Directory"),
          NULL, _("Go to downloads directory"),
          G_CALLBACK (Browser_Load_Downloads_Directory) },
        { AM_LOAD_MUSIC_DIR, "folder-music", _("Music Directory"), NULL,
          _("Go to music directory"),
          G_CALLBACK (Browser_Load_Music_Directory) },
        { AM_LOAD_PARENT_DIR, GTK_STOCK_GO_UP, _("_Parent Directory"),
          "<Alt>Up", _("Go to parent directory"),
          G_CALLBACK (et_browser_on_action_parent_directory) },
        { AM_LOAD_DEFAULT_DIR, GTK_STOCK_JUMP_TO, _("_Default Directory"),
          "<Primary>D", _("Go to default directory"),
          G_CALLBACK (Browser_Load_Default_Directory) },
        { AM_SET_PATH_AS_DEFAULT,      GTK_STOCK_DIRECTORY,    _("Set _Current Path as Default"),  NULL,                _("Set current path as default"),   G_CALLBACK(Set_Current_Path_As_Default) },
        { AM_RENAME_DIR,               GTK_STOCK_INDEX,        _("Rename Directory…"),          "F2",                _("Rename directory"),          G_CALLBACK(Browser_Open_Rename_Directory_Window) },
        { AM_RELOAD_DIRECTORY, GTK_STOCK_REFRESH, _("Reload Directory"),
          "<Primary>R", _("Reload directory"),
          G_CALLBACK (Browser_Reload_Directory) },
        { AM_BROWSE_DIRECTORY_WITH, GTK_STOCK_EXECUTE,
          _("Browse Directory With…"), NULL,
          _("Run a command on the directory"),
          G_CALLBACK (Browser_Open_Run_Program_Tree_Window) },
        { AM_COLLAPSE_TREE, NULL, _("_Collapse Tree"), "<Primary><Shift>C",
          _("Collapse directory tree"), G_CALLBACK (Browser_Tree_Collapse) },
        { AM_INITIALIZE_TREE, GTK_STOCK_REFRESH, _("_Reload Tree"),
          "<Primary><Shift>R", _("Reload directory tree"),
          G_CALLBACK (Browser_Tree_Rebuild) },

        { MENU_SCANNER, NULL, _("S_canner Mode"), NULL, NULL, NULL },

        { MENU_MISC,                NULL,                   _("_Miscellaneous"),                             NULL,         NULL,                                 NULL },
        { AM_SEARCH_FILE, GTK_STOCK_FIND, _("_Find…"), "<Primary>F",
          _("Search filenames and tags"),
          G_CALLBACK (Open_Search_File_Window) },
        { AM_CDDB_SEARCH, GTK_STOCK_CDROM, _("CDD_B Search…"), "<Primary>B",
          _("CDDB search"), G_CALLBACK (Open_Cddb_Window) },
        { AM_FILENAME_FROM_TXT, GTK_STOCK_OPEN,
          _("Load Filenames From a Text File…"), "<Primary>T",
          _("Load filenames from a text file"),
          G_CALLBACK (Open_Load_Filename_Window) },
        { AM_WRITE_PLAYLIST, GTK_STOCK_SAVE_AS, _("Generate Playlist…"),
          "<Primary>W", _("Generate a playlist"),
          G_CALLBACK (Open_Write_Playlist_Window) },
        { AM_RUN_AUDIO_PLAYER, GTK_STOCK_MEDIA_PLAY, _("Run Audio Player"),
          "<Primary>M", _("Run audio player"),
          G_CALLBACK (Run_Audio_Player_Using_Selection) },

        { MENU_EDIT, NULL, _("_Edit"), NULL, NULL, NULL },
        { AM_OPEN_OPTIONS_WINDOW, GTK_STOCK_PREFERENCES, _("_Preferences"),
          NULL, _("Preferences"), G_CALLBACK (Open_OptionsWindow) },

        { MENU_VIEW, NULL, _("_View"), NULL, NULL, NULL },
        { MENU_GO, NULL, _("_Go"), NULL, NULL, NULL },

        { MENU_HELP,                NULL,                   _("_Help"),                             NULL,         NULL,                                 NULL },
/* TODO: Link to help.gnome.org, or locally-installed help, on Windows. */
#ifndef G_OS_WIN32
        { AM_OPEN_HELP_CONTENTS, GTK_STOCK_HELP, _("_Contents"), "F1",
          _("Show help"), G_CALLBACK (et_show_help) },
#endif
        { AM_OPEN_ABOUT_WINDOW,     GTK_STOCK_ABOUT,        _("_About"),                            NULL,         _("About"),                           G_CALLBACK(Show_About_Window) },


        /*
         * Following items are on toolbar but not on menu
         */
        { AM_STOP, GTK_STOCK_STOP, _("Stop the current action"), NULL, _("Stop the current action"), G_CALLBACK(Action_Main_Stop_Button_Pressed) },


        /*
         * Popup menu's Actions
         */
        { POPUP_FILE,                   NULL,              _("_File Operations"),          NULL, NULL,                         NULL },
        { POPUP_SUBMENU_SCANNER,        "document-properties",    _("S_canner"),                  NULL, NULL,                         NULL },
        { POPUP_DIR_RUN_AUDIO,          GTK_STOCK_MEDIA_PLAY,   _("Run Audio Player"),          NULL, _("Run audio player"),        G_CALLBACK(Run_Audio_Player_Using_Directory) },
        { AM_ARTIST_RUN_AUDIO_PLAYER,   GTK_STOCK_MEDIA_PLAY,   _("Run Audio Player"),          NULL, _("Run audio player"),        G_CALLBACK(Run_Audio_Player_Using_Browser_Artist_List) },
        { AM_ALBUM_RUN_AUDIO_PLAYER,    GTK_STOCK_MEDIA_PLAY,   _("Run Audio Player"),          NULL, _("Run audio player"),        G_CALLBACK(Run_Audio_Player_Using_Browser_Album_List)  },
        { AM_CDDB_SEARCH_FILE,          GTK_STOCK_CDROM,   _("CDDB Search Files…"),    NULL, _("CDDB search files…"),  G_CALLBACK(Cddb_Popup_Menu_Search_Selected_File) },
        //{ AM_ARTIST_OPEN_FILE_WITH,     GTK_STOCK_OPEN,    _("Open File(s) with…"),     NULL, _("Open File(s) with…"),     G_CALLBACK(Browser_Open_Run_Program_List_Window??? Browser_Open_Run_Program_Tree_Window???) },
        //{ AM_ALBUM_OPEN_FILE_WITH,      GTK_STOCK_OPEN,    _("Open File(s) with…"),     NULL, _("Open File(s) with…"),     G_CALLBACK(Browser_Open_Run_Program_List_Window??? Browser_Open_Run_Program_Tree_Window???) },

        { AM_LOG_CLEAN, GTK_STOCK_CLEAR, _("Clear log"), NULL, _("Clear log"),
	  G_CALLBACK (et_log_area_clear) }

    };

    GtkToggleActionEntry ToggleActionEntries[] =
    {
        //{ AM_BROWSE_SUBDIR,      GTK_STOCK_INDEX,        _("Browse _Subdirectories"),                         NULL, _("Browse _Sub-directories"),                         NULL,                                    FALSE },
        { AM_BROWSE_SUBDIR,      NULL,                   _("Browse _Subdirectories"),                         NULL, _("Browse subdirectories"),                         NULL,                                    BROWSE_SUBDIR },
#ifndef G_OS_WIN32 /* No sense here for Win32, "hidden" means : starts with a
                    * '.'
                    */
        { AM_BROWSER_HIDDEN_DIR, NULL,                   _("Show Hidden Directories"),                         NULL, _("Show hidden directories"),                         G_CALLBACK(Browser_Tree_Rebuild),     BROWSE_HIDDEN_DIR },
#endif /* !G_OS_WIN32 */
        { AM_SCANNER_SHOW, "document-properties", _("_Show Scanner"), NULL,
          _("Show scanner"), G_CALLBACK (et_scan_show),
          OPEN_SCANNER_WINDOW_ON_STARTUP },
    };

    GtkRadioActionEntry view_mode_entries[] =
    {
        { AM_TREE_VIEW_MODE, "audio-x-generic", _("Tree Browser"), NULL,
          _("View by directory tree"), 0 },
        { AM_ARTIST_VIEW_MODE, "easytag-artist-album",
          _("Artist and Album"), NULL,
          _("View by artist and album"), 1 }
    };

    GtkRadioActionEntry scanner_mode_entries[] =
    {
        { AM_SCANNER_FILL_TAG, "document-properties", _("_Fill Tags…"), NULL,
          _("Fill tags"), SCANNER_FILL_TAG },
        { AM_SCANNER_RENAME_FILE, "document-properties",
          _("_Rename Files and Directories…"), NULL,
          _("Rename files and directories"), SCANNER_RENAME_FILE },
        { AM_SCANNER_PROCESS_FIELDS, "document-properties",
          _("_Process Fields…"), NULL, _("Process Fields"),
          SCANNER_PROCESS_FIELDS }
    };

    GError *error = NULL;
    guint num_menu_entries;
    guint num_toggle_entries;
    guint n_view_mode_entries;
    guint n_scanner_mode_entries;
    guint i;

    /* Calculate number of items into the menu */
    num_menu_entries = G_N_ELEMENTS(ActionEntries);
    num_toggle_entries = G_N_ELEMENTS(ToggleActionEntries);
    n_view_mode_entries = G_N_ELEMENTS (view_mode_entries);
    n_scanner_mode_entries = G_N_ELEMENTS (scanner_mode_entries);

    /* Populate quarks list with the entries */
    for(i = 0; i < num_menu_entries; i++)
    {
        Action_Pair* ActionPair = g_malloc0(sizeof(Action_Pair));
        ActionPair->action = ActionEntries[i].name;
        ActionPair->quark  = g_quark_from_string(ActionPair->action);
        ActionPairsList = g_list_prepend (ActionPairsList, ActionPair);
    }

    for(i = 0; i < num_toggle_entries; i++)
    {
        Action_Pair* ActionPair = g_malloc0(sizeof(Action_Pair));
        ActionPair->action = ToggleActionEntries[i].name;
        ActionPair->quark  = g_quark_from_string(ActionPair->action);
        ActionPairsList = g_list_prepend (ActionPairsList, ActionPair);
    }

    ActionPairsList = g_list_reverse (ActionPairsList);

    /* UI Management */
    ActionGroup = gtk_action_group_new("actions");
    gtk_action_group_set_translation_domain (ActionGroup, GETTEXT_PACKAGE);
    gtk_action_group_add_actions(ActionGroup, ActionEntries, num_menu_entries, window);
    gtk_action_group_add_toggle_actions(ActionGroup, ToggleActionEntries, num_toggle_entries, window);
    gtk_action_group_add_radio_actions (ActionGroup, view_mode_entries,
                                        n_view_mode_entries, 0,
                                        Action_Select_Browser_Style, window);
    gtk_action_group_add_radio_actions (ActionGroup, scanner_mode_entries,
                                        n_scanner_mode_entries, 0,
                                        G_CALLBACK (et_on_action_select_scan_mode),
                                        window);

    UIManager = gtk_ui_manager_new();

    g_signal_connect (UIManager, "connect-proxy",
                      G_CALLBACK (et_ui_manager_on_connect_proxy), NULL);
    g_signal_connect (UIManager, "disconnect-proxy",
                      G_CALLBACK (et_ui_manager_on_disconnect_proxy), NULL);

    if (!gtk_ui_manager_add_ui_from_string(UIManager, ui_xml, -1, &error))
    {
        g_error(_("Could not merge UI, error was: %s\n"), error->message);
        g_error_free(error);
    }
    gtk_ui_manager_insert_action_group(UIManager, ActionGroup, 0);
    gtk_window_add_accel_group (window,
                                gtk_ui_manager_get_accel_group (UIManager));

    menubar = gtk_ui_manager_get_widget(UIManager, "/MenuBar");
    Init_Menu_Bar();
    gtk_widget_show_all(menubar);

    toolbar = gtk_ui_manager_get_widget (UIManager, "/ToolBar");
    gtk_widget_show_all(toolbar);
    gtk_style_context_add_class (gtk_widget_get_style_context (toolbar),
                                 GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

    *pptoolbar = toolbar;
    *ppmenubar = menubar;
}


/*
 * Initialize some items of the main menu
 */
static void
Init_Menu_Bar (void)
{
    
    CheckMenuItemBrowseSubdirMainMenu = gtk_ui_manager_get_widget(UIManager, "/MenuBar/BrowserMenu/BrowseSubdir");
    if (CheckMenuItemBrowseSubdirMainMenu)
    {
        // Link to update BROWSE_SUBDIR when changed
        g_signal_connect(G_OBJECT(CheckMenuItemBrowseSubdirMainMenu),"toggled",
            G_CALLBACK(Check_Menu_Item_Toggled_Browse_Subdir),NULL);
    }

    CheckMenuItemBrowseHiddenDirMainMenu = gtk_ui_manager_get_widget (UIManager,
                                                                      "/MenuBar/ViewMenu/BrowseHiddenDir");
    if (CheckMenuItemBrowseHiddenDirMainMenu)
    {
        // Link to update BROWSE_HIDDEN_DIR when changed
        g_signal_connect(G_OBJECT(CheckMenuItemBrowseHiddenDirMainMenu),"toggled",
            G_CALLBACK(Check_Menu_Item_Toggled_Browse_Hidden_Dir),NULL);
    }

    /* If entry not implemented */
    //{GtkWidget *widget = gtk_item_factory_get_widget_by_action(ItemFactory,FILENAME_FROM_TXT);
    //if (widget) gtk_widget_set_sensitive(widget,FALSE);}
}

/*
 * Callback to update state of check button to browse subdir into menu
 */
static void
Check_Menu_Item_Toggled_Browse_Subdir (GtkWidget *checkmenuitem)
{
    BROWSE_SUBDIR = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(checkmenuitem));
    Check_Menu_Item_Update_Browse_Subdir();
}
void Check_Menu_Item_Update_Browse_Subdir (void)
{
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(CheckMenuItemBrowseSubdirMainMenu),BROWSE_SUBDIR);
}

/*
 * Callback to update state of check button to show hiddendirectories into menu
 */
static void
Check_Menu_Item_Toggled_Browse_Hidden_Dir (GtkWidget *checkmenuitem)
{
    BROWSE_HIDDEN_DIR = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(checkmenuitem));
    Check_Menu_Item_Update_Browse_Hidden_Dir();

    // Reload directory, in case we have changed BROWSE_HIDDEN_DIR
    //Browser_Tree_Rebuild(NULL); // Commented, as already done in GtkToggleActionEntry for AM_BROWSER_HIDDEN_DIR
}

void
Check_Menu_Item_Update_Browse_Hidden_Dir (void)
{
#ifndef G_OS_WIN32
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(CheckMenuItemBrowseHiddenDirMainMenu),BROWSE_HIDDEN_DIR);
#endif /* !G_OS_WIN32 */
}









/*
 * Status bar functions
 */
GtkWidget *Create_Status_Bar (void)
{
    StatusBar = gtk_statusbar_new();
    /* Specify a size to avoid statubar resizing if the message is too long */
    gtk_widget_set_size_request(StatusBar, 200, -1);
    /* Create serie */
    StatusBarContext = gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar),"Messages");
    timer_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (StatusBar),
                                              "timer");
    tooltip_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (StatusBar),
                                                "tooltip");

    Statusbar_Message (_("Ready to start"), TRUE);

    gtk_widget_show(StatusBar);
    return StatusBar;
}

static gboolean
Statusbar_Stop_Timer (void)
{
    gtk_statusbar_pop (GTK_STATUSBAR (StatusBar), timer_cid);
    return G_SOURCE_REMOVE;
}

static void
et_statusbar_reset_timer (void)
{
    StatusbarTimerId = 0;
}

static void
Statusbar_Start_Timer (void)
{
    Statusbar_Remove_Timer ();
    StatusbarTimerId = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, 4,
                                                   (GSourceFunc)Statusbar_Stop_Timer,
                                                   NULL,
                                                   (GDestroyNotify)et_statusbar_reset_timer);
    g_source_set_name_by_id (StatusbarTimerId, "Statusbar stop timer");
}

static void
Statusbar_Remove_Timer (void)
{
    if (StatusbarTimerId)
    {
        Statusbar_Stop_Timer ();
        g_source_remove(StatusbarTimerId);
        et_statusbar_reset_timer ();
    }
}

/*
 * Send a message to the status bar
 *  - with_timer: if TRUE, the message will be displayed during 4s
 *                if FALSE, the message will be displayed up to the next posted message
 */
void
Statusbar_Message (const gchar *message, gboolean with_timer)
{
    gchar *msg_temp;

    g_return_if_fail (StatusBar != NULL);

    msg_temp = Try_To_Validate_Utf8_String(message);
    
    /* Push the given message */
    if (with_timer)
    {
        Statusbar_Start_Timer ();
        gtk_statusbar_push (GTK_STATUSBAR (StatusBar), timer_cid, msg_temp);
    }
    else
    {
        gtk_statusbar_pop (GTK_STATUSBAR (StatusBar), StatusBarContext);
        gtk_statusbar_push (GTK_STATUSBAR (StatusBar), StatusBarContext,
                            msg_temp);
    }

    g_free(msg_temp);
}

/*
 * et_statusbar_push_tooltip:
 * @message: a tooltip to display in the status bar
 *
 * Display a tooltip in the status bar of the main window. Call
 * et_statusbar_pop_tooltip() to stop displaying the tooltip message.
 */
static void
et_statusbar_push_tooltip (const gchar *message)
{
    g_return_if_fail (StatusBar != NULL && message != NULL);

    gtk_statusbar_push (GTK_STATUSBAR (StatusBar), tooltip_cid, message);
}

/*
 * et_statusbar_pop_tooltip:
 *
 * Pop a tooltip message from the status bar. et_statusbar_push_tooltip() must
 * have been called first.
 */
static void
et_statusbar_pop_tooltip (void)
{
    g_return_if_fail (StatusBar != NULL);

    gtk_statusbar_pop (GTK_STATUSBAR (StatusBar), tooltip_cid);
}




/*
 * Progress bar
 */
GtkWidget *Create_Progress_Bar (void)
{
    ProgressBar = et_progress_bar_new ();

    gtk_widget_show(ProgressBar);
    return ProgressBar;
}

/*
 * et_ui_manager_on_connect_proxy:
 * @manager: the UI manager which generated the signal
 * @action: the action which was connected to @proxy
 * @proxy: the widget which was connected to @action
 * @user_data: user data set when the signal was connected
 *
 * Connect handlers for selection and deselection of menu items, in order to
 * set tooltips for the menu items as status bar messages.
 */
static void
et_ui_manager_on_connect_proxy (GtkUIManager *manager, GtkAction *action,
                                GtkWidget *proxy, gpointer user_data)
{
    if (GTK_IS_MENU_ITEM (proxy))
    {
        guint id;

        id = g_signal_connect (proxy, "select",
                               G_CALLBACK (on_menu_item_select), action);
        g_object_set_data (G_OBJECT (proxy), "select-id",
                           GUINT_TO_POINTER (id));
        id = g_signal_connect (proxy, "deselect",
                               G_CALLBACK (on_menu_item_deselect), NULL);
        g_object_set_data (G_OBJECT (proxy), "deselect-id",
                           GUINT_TO_POINTER (id));
    }
}

/*
 * et_ui_manager_on_disconnect_proxy:
 * @manager: the UI manager which generated the signal
 * @action: the action which was connected to @proxy
 * @proxy: the widget which was connected to @action
 * @user_data: user data set when the signal was connected
 *
 * Disconnect handlers for selecting and deselecting menu items.
 */
static void
et_ui_manager_on_disconnect_proxy (GtkUIManager *manager, GtkAction *action,
                                   GtkWidget *proxy, gpointer user_data)
{
    if (GTK_IS_MENU_ITEM (proxy))
    {
        guint id;

        id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (proxy),
                                                  "select-id"));
        g_signal_handler_disconnect (proxy, id);
        id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (proxy),
                                                  "deselect-id"));
        g_signal_handler_disconnect (proxy, id);
    }
}

/*
 * on_menu_item_select:
 * @item: the menu item which was selected
 * @user_data: the #GtkAction corresponding to @item
 *
 * Set the current status bar message to the tooltip of the menu @item which
 * was selected.
 */
static void
on_menu_item_select (GtkMenuItem *item, gpointer user_data)
{
    GtkAction *action;
    const gchar *message;

    g_return_if_fail (user_data != NULL);

    action = GTK_ACTION (user_data);
    message = gtk_action_get_tooltip (action);

    if (message)
    {
        et_statusbar_push_tooltip (message);
    }
}

/*
 * on_menu_item_deselect:
 * @item: the menu item which was deselected
 * @user_data: user data set when the signal was connected
 *
 * Clear the current tooltip status bar message when the menu item is
 * deselected.
 */
static void
on_menu_item_deselect (GtkMenuItem *item, gpointer user_data)
{
    et_statusbar_pop_tooltip ();
}
