/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "application_window.h"
#include "bar.h"
#include "easytag.h"
#include "preferences_dialog.h"
#include "setting.h"
#include "browser.h"
#include "scan_dialog.h"
#include "cddb_dialog.h"
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

    QCASE_DATA(AM_SORT_ASCENDING_FILENAME,         ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_FILENAME);
    QCASE_DATA(AM_SORT_DESCENDING_FILENAME,        ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_FILENAME);
    QCASE_DATA(AM_SORT_ASCENDING_CREATION_DATE,    ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_CREATION_DATE);
    QCASE_DATA(AM_SORT_DESCENDING_CREATION_DATE,   ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_CREATION_DATE);
    QCASE_DATA(AM_SORT_ASCENDING_TRACK_NUMBER,     ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_TRACK_NUMBER);
    QCASE_DATA(AM_SORT_DESCENDING_TRACK_NUMBER,    ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_TRACK_NUMBER);
    QCASE_DATA(AM_SORT_ASCENDING_TITLE,            ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_TITLE);
    QCASE_DATA(AM_SORT_DESCENDING_TITLE,           ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_TITLE);
    QCASE_DATA(AM_SORT_ASCENDING_ARTIST,           ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_ARTIST);
    QCASE_DATA(AM_SORT_DESCENDING_ARTIST,          ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_ARTIST);
    QCASE_DATA(AM_SORT_ASCENDING_ALBUM_ARTIST,     ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_ALBUM_ARTIST);
    QCASE_DATA(AM_SORT_DESCENDING_ALBUM_ARTIST,    ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_ALBUM_ARTIST);
    QCASE_DATA(AM_SORT_ASCENDING_ALBUM,            ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_ALBUM);
    QCASE_DATA(AM_SORT_DESCENDING_ALBUM,           ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_ALBUM);
    QCASE_DATA(AM_SORT_ASCENDING_YEAR,             ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_YEAR);
    QCASE_DATA(AM_SORT_DESCENDING_YEAR,            ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_YEAR);
    QCASE_DATA(AM_SORT_ASCENDING_GENRE,            ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_GENRE);
    QCASE_DATA(AM_SORT_DESCENDING_GENRE,           ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_GENRE);
    QCASE_DATA(AM_SORT_ASCENDING_COMMENT,          ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_COMMENT);
    QCASE_DATA(AM_SORT_DESCENDING_COMMENT,         ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_COMMENT);
    QCASE_DATA(AM_SORT_ASCENDING_COMPOSER,         ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_COMPOSER);
    QCASE_DATA(AM_SORT_DESCENDING_COMPOSER,        ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_COMPOSER);
    QCASE_DATA(AM_SORT_ASCENDING_ORIG_ARTIST,      ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_ORIG_ARTIST);
    QCASE_DATA(AM_SORT_DESCENDING_ORIG_ARTIST,     ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_ORIG_ARTIST);
    QCASE_DATA(AM_SORT_ASCENDING_COPYRIGHT,        ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_COPYRIGHT);
    QCASE_DATA(AM_SORT_DESCENDING_COPYRIGHT,       ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_COPYRIGHT);
    QCASE_DATA(AM_SORT_ASCENDING_URL,              ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_URL);
    QCASE_DATA(AM_SORT_DESCENDING_URL,             ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_URL);
    QCASE_DATA(AM_SORT_ASCENDING_ENCODED_BY,       ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_ENCODED_BY);
    QCASE_DATA(AM_SORT_DESCENDING_ENCODED_BY,      ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_ENCODED_BY);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_TYPE,        ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_FILE_TYPE);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_TYPE,       ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_FILE_TYPE);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_SIZE,        ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_FILE_SIZE);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_SIZE,       ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_FILE_SIZE);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_DURATION,    ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_FILE_DURATION);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_DURATION,   ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_FILE_DURATION);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_BITRATE,     ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_FILE_BITRATE);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_BITRATE,    ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_FILE_BITRATE);
    QCASE_DATA(AM_SORT_ASCENDING_FILE_SAMPLERATE,  ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_ASCENDING_FILE_SAMPLERATE);
    QCASE_DATA(AM_SORT_DESCENDING_FILE_SAMPLERATE, ET_Sort_Displayed_File_List_And_Update_UI, ET_SORT_MODE_DESCENDING_FILE_SAMPLERATE);
    et_application_window_browser_refresh_sort (ET_APPLICATION_WINDOW (data));
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

        { AM_SCAN_FILES, GTK_STOCK_APPLY, _("S_can Files"), NULL,
          _("Scan selected files"),
          G_CALLBACK (et_application_window_scan_selected_files) },

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
        { AM_ARTIST_RUN_AUDIO_PLAYER, GTK_STOCK_MEDIA_PLAY,
          _("Run Audio Player"), NULL, _("Run audio player"),
          G_CALLBACK (et_application_window_run_player_for_artist_list) },
        { AM_ALBUM_RUN_AUDIO_PLAYER, GTK_STOCK_MEDIA_PLAY,
          _("Run Audio Player"), NULL, _("Run audio player"),
          G_CALLBACK (et_application_window_run_player_for_album_list) },
        { AM_CDDB_SEARCH_FILE, GTK_STOCK_CDROM, _("CDDB Search Files…"), NULL,
          _("CDDB search files…"),
          G_CALLBACK (et_application_window_search_cddb_for_selection) },
        //{ AM_ARTIST_OPEN_FILE_WITH,     GTK_STOCK_OPEN,    _("Open File(s) with…"),     NULL, _("Open File(s) with…"),     G_CALLBACK(Browser_Open_Run_Program_List_Window??? Browser_Open_Run_Program_Tree_Window???) },
        //{ AM_ALBUM_OPEN_FILE_WITH,      GTK_STOCK_OPEN,    _("Open File(s) with…"),     NULL, _("Open File(s) with…"),     G_CALLBACK(Browser_Open_Run_Program_List_Window??? Browser_Open_Run_Program_Tree_Window???) },

        { AM_LOG_CLEAN, GTK_STOCK_CLEAR, _("Clear log"), NULL, _("Clear log"),
	  G_CALLBACK (et_log_area_clear) }

    };

    GError *error = NULL;
    guint num_menu_entries;
    guint i;

    /* Calculate number of items into the menu */
    num_menu_entries = G_N_ELEMENTS(ActionEntries);

    /* Populate quarks list with the entries */
    for(i = 0; i < num_menu_entries; i++)
    {
        Action_Pair* ActionPair = g_malloc0(sizeof(Action_Pair));
        ActionPair->action = ActionEntries[i].name;
        ActionPair->quark  = g_quark_from_string(ActionPair->action);
        ActionPairsList = g_list_prepend (ActionPairsList, ActionPair);
    }

    ActionPairsList = g_list_reverse (ActionPairsList);

    /* UI Management */
    ActionGroup = gtk_action_group_new("actions");
    gtk_action_group_set_translation_domain (ActionGroup, GETTEXT_PACKAGE);
    gtk_action_group_add_actions(ActionGroup, ActionEntries, num_menu_entries, window);

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
    gtk_widget_show_all(menubar);

    toolbar = gtk_ui_manager_get_widget (UIManager, "/ToolBar");
    gtk_widget_show_all(toolbar);
    gtk_style_context_add_class (gtk_widget_get_style_context (toolbar),
                                 GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

    *pptoolbar = toolbar;
    *ppmenubar = menubar;
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
