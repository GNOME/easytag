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

GtkWidget *
create_main_toolbar (GtkWindow *window)
{
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

    toolbar = gtk_ui_manager_get_widget (UIManager, "/ToolBar");
    gtk_widget_show_all (toolbar);
    gtk_style_context_add_class (gtk_widget_get_style_context (toolbar),
                                 GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

    return toolbar;
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
