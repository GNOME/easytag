/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014-2015  David King <amigadave@amigadave.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "application_window.h"

#include <glib/gi18n.h>

#include "browser.h"
#include "cddb_dialog.h"
#include "charset.h"
#include "easytag.h"
#include "file_area.h"
#include "file_list.h"
#ifdef ENABLE_FLAC
#include "flac_header.h"
#endif
#include "load_files_dialog.h"
#include "log.h"
#include "misc.h"
#ifdef ENABLE_MP4
#include "mp4_header.h"
#endif
#include "mpeg_header.h"
#include "monkeyaudio_header.h"
#include "musepack_header.h"
#ifdef ENABLE_OGG
#include "ogg_header.h"
#endif
#ifdef ENABLE_OPUS
#include "opus_header.h"
#endif
#include "picture.h"
#include "playlist_dialog.h"
#include "preferences_dialog.h"
#include "progress_bar.h"
#include "search_dialog.h"
#include "scan.h"
#include "scan_dialog.h"
#include "setting.h"
#include "status_bar.h"
#include "tag_area.h"
#ifdef ENABLE_WAVPACK
#include "wavpack_header.h"
#endif

typedef struct
{
    GtkWidget *browser;

    GtkWidget *file_area;
    GtkWidget *log_area;
    GtkWidget *tag_area;
    GtkWidget *progress_bar;
    GtkWidget *status_bar;

    GtkWidget *cddb_dialog;
    GtkWidget *load_files_dialog;
    GtkWidget *playlist_dialog;
    GtkWidget *preferences_dialog;
    GtkWidget *scan_dialog;
    GtkWidget *search_dialog;

    GtkWidget *hpaned;
    GtkWidget *vpaned;

    GdkCursor *cursor;

    gboolean is_maximized;
    gint height;
    gint width;
    gint paned_position;
} EtApplicationWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtApplicationWindow, et_application_window, GTK_TYPE_APPLICATION_WINDOW)

/* Used to force to hide the msgbox when deleting file */
static gboolean SF_HideMsgbox_Delete_File;
/* To remember which button was pressed when deleting file */
static gint SF_ButtonPressed_Delete_File;

static gboolean
on_main_window_delete_event (GtkWidget *window,
                             GdkEvent *event,
                             gpointer user_data)
{
    et_application_window_quit (ET_APPLICATION_WINDOW (window));

    /* Handled the event, so stop propagation. */
    return GDK_EVENT_STOP;
}

static void
save_state (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;
    gchar *path;
    GKeyFile *keyfile;
    GError *error = NULL;

    priv = et_application_window_get_instance_private (self);
    keyfile = g_key_file_new ();
    path = g_build_filename (g_get_user_cache_dir (), PACKAGE_TARNAME,
                             "state", NULL);

    /* Try to preserve comments by loading an existing keyfile. */
    if (!g_key_file_load_from_file (keyfile, path, G_KEY_FILE_KEEP_COMMENTS,
                                    &error))
    {
        g_debug ("Error loading window state during saving: %s",
                 error->message);
        g_clear_error (&error);
    }

    g_key_file_set_integer (keyfile, "EtApplicationWindow", "width",
                            priv->width);
    g_key_file_set_integer (keyfile, "EtApplicationWindow", "height",
                            priv->height);
    g_key_file_set_boolean (keyfile, "EtApplicationWindow", "is_maximized",
                            priv->is_maximized);

    if (priv->is_maximized)
    {
        gint width;

        /* Calculate the paned position based on the unmaximized window state,
         * as that is the state at which the window starts. */
        gtk_window_get_size (GTK_WINDOW (self), &width, NULL);
        priv->paned_position -= (width - priv->width);
    }

    g_key_file_set_integer (keyfile, "EtApplicationWindow", "paned_position",
                            priv->paned_position);

    if (!g_key_file_save_to_file (keyfile, path, &error))
    {
        g_warning ("Error saving window state: %s", error->message);
        g_error_free (error);
    }

    g_free (path);
    g_key_file_free (keyfile);
}

static void
restore_state (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;
    GtkWindow *window;
    GKeyFile *keyfile;
    gchar *path;
    GError *error = NULL;

    priv = et_application_window_get_instance_private (self);
    window = GTK_WINDOW (self);

    keyfile = g_key_file_new ();
    path = g_build_filename (g_get_user_cache_dir (), PACKAGE_TARNAME,
                             "state", NULL);

    if (!g_key_file_load_from_file (keyfile, path, G_KEY_FILE_KEEP_COMMENTS,
                                    &error))
    {
        g_debug ("Error loading window state: %s", error->message);
        g_error_free (error);
        g_key_file_free (keyfile);
        g_free (path);
        return;
    }

    g_free (path);

    /* Ignore errors, as the default values are fine. */
    priv->width = g_key_file_get_integer (keyfile, "EtApplicationWindow",
                                          "width", NULL);
    priv->height = g_key_file_get_integer (keyfile, "EtApplicationWindow",
                                           "height", NULL);
    priv->is_maximized = g_key_file_get_boolean (keyfile,
                                                 "EtApplicationWindow",
                                                 "is_maximized", NULL);
    priv->paned_position = g_key_file_get_integer (keyfile,
                                                   "EtApplicationWindow",
                                                   "paned_position", NULL);

    gtk_window_set_default_size (window, priv->width, priv->height);

    /* Only set the unmaximized position, as the maximized position should only
     * be set after the window manager has maximized the window. */
    gtk_paned_set_position (GTK_PANED (priv->hpaned), priv->paned_position);

    if (priv->is_maximized)
    {
        gtk_window_maximize (window);
    }

    g_key_file_free (keyfile);
}

static gboolean
on_configure_event (GtkWidget *window,
                    GdkEvent *event,
                    gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GdkEventConfigure *configure_event;

    self = ET_APPLICATION_WINDOW (window);
    priv = et_application_window_get_instance_private (self);
    configure_event = (GdkEventConfigure *)event;

    if (!priv->is_maximized)
    {
        priv->width = configure_event->width;
        priv->height = configure_event->height;
    }

    return GDK_EVENT_PROPAGATE;
}

static gboolean
on_window_state_event (GtkWidget *window,
                       GdkEvent *event,
                       gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GdkEventWindowState *state_event;

    self = ET_APPLICATION_WINDOW (window);
    priv = et_application_window_get_instance_private (self);
    state_event = (GdkEventWindowState *)event;

    if (state_event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED)
    {
        priv->is_maximized = (state_event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
    }

    return GDK_EVENT_PROPAGATE;
}

static void
on_paned_notify_position (EtApplicationWindow *self,
                          GParamSpec *pspec,
                          gpointer user_data)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    priv->paned_position = gtk_paned_get_position (GTK_PANED (priv->hpaned));
}

File_Tag *
et_application_window_tag_area_create_file_tag (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (ET_APPLICATION_WINDOW (self), NULL);

    priv = et_application_window_get_instance_private (self);

    return et_tag_area_create_file_tag (ET_TAG_AREA (priv->tag_area));
}

static gboolean
et_application_window_tag_area_display_et_file (EtApplicationWindow *self,
                                                const ET_File *ETFile)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (ET_APPLICATION_WINDOW (self), FALSE);

    priv = et_application_window_get_instance_private (self);

    return et_tag_area_display_et_file (ET_TAG_AREA (priv->tag_area), ETFile);
}

/* Clear the entries of tag area. */
void
et_application_window_tag_area_clear (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_tag_area_clear (ET_TAG_AREA (priv->tag_area));
}

static void
et_application_window_show_cddb_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    if (priv->cddb_dialog)
    {
        gtk_widget_show (priv->cddb_dialog);
    }
    else
    {
        priv->cddb_dialog = GTK_WIDGET (et_cddb_dialog_new ());
        gtk_widget_show_all (priv->cddb_dialog);
    }
}

/*
 * Delete the file ETFile
 */
static gint
delete_file (ET_File *ETFile, gboolean multiple_files, GError **error)
{
    GtkWidget *msgdialog;
    GtkWidget *msgdialog_check_button = NULL;
    const gchar *cur_filename;
    const gchar *cur_filename_utf8;
    gchar *basename_utf8;
    gint response;
    gint stop_loop;

    g_return_val_if_fail (ETFile != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* Filename of the file to delete. */
    cur_filename      = ((File_Name *)(ETFile->FileNameCur)->data)->value;
    cur_filename_utf8 = ((File_Name *)(ETFile->FileNameCur)->data)->value_utf8;
    basename_utf8 = g_path_get_basename (cur_filename_utf8);

    /*
     * Remove the file
     */
    if (g_settings_get_boolean (MainSettings, "confirm-delete-file")
        && !SF_HideMsgbox_Delete_File)
    {
        if (multiple_files)
        {
            GtkWidget *message_area;
            msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_NONE,
                                               _("Do you really want to delete the file ‘%s’?"),
                                               basename_utf8);
            message_area = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(msgdialog));
            msgdialog_check_button = gtk_check_button_new_with_label(_("Repeat action for the remaining files"));
            gtk_widget_show(GTK_WIDGET(msgdialog_check_button));
            gtk_container_add(GTK_CONTAINER(message_area),msgdialog_check_button);
            gtk_dialog_add_buttons (GTK_DIALOG (msgdialog), _("_Skip"),
                                    GTK_RESPONSE_NO, _("_Cancel"),
                                    GTK_RESPONSE_CANCEL, _("_Delete"),
                                    GTK_RESPONSE_YES, NULL);
            gtk_window_set_title(GTK_WINDOW(msgdialog),_("Delete File"));
            //GTK_TOGGLE_BUTTON(msgbox_check_button)->active = TRUE; // Checked by default
        }else
        {
            msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_NONE,
                                               _("Do you really want to delete the file ‘%s’?"),
                                               basename_utf8);
            gtk_window_set_title(GTK_WINDOW(msgdialog),_("Delete File"));
            gtk_dialog_add_buttons (GTK_DIALOG (msgdialog), _("_Cancel"),
                                    GTK_RESPONSE_NO, _("_Delete"),
                                    GTK_RESPONSE_YES, NULL);
        }
        gtk_dialog_set_default_response (GTK_DIALOG (msgdialog),
                                         GTK_RESPONSE_YES);
        SF_ButtonPressed_Delete_File = response = gtk_dialog_run(GTK_DIALOG(msgdialog));
        if (msgdialog_check_button && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msgdialog_check_button)))
            SF_HideMsgbox_Delete_File = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msgdialog_check_button));
        gtk_widget_destroy(msgdialog);
    }else
    {
        if (SF_HideMsgbox_Delete_File)
            response = SF_ButtonPressed_Delete_File;
        else
            response = GTK_RESPONSE_YES;
    }

    switch (response)
    {
        case GTK_RESPONSE_YES:
        {
            GFile *cur_file = g_file_new_for_path (cur_filename);

            if (g_file_delete (cur_file, NULL, error))
            {
                gchar *msg = g_strdup_printf(_("File ‘%s’ deleted"), basename_utf8);
                et_application_window_status_bar_message (ET_APPLICATION_WINDOW (MainWindow),
                                                          msg, FALSE);
                g_free(msg);
                g_free(basename_utf8);
                g_object_unref (cur_file);
                g_assert (error == NULL || *error == NULL);
                return 1;
            }

            /* Error in deleting file. */
            g_assert (error == NULL || *error != NULL);
            break;
        }
        case GTK_RESPONSE_NO:
            break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_DELETE_EVENT:
            stop_loop = -1;
            g_free(basename_utf8);
            return stop_loop;
            break;
        default:
            g_assert_not_reached ();
            break;
    }

    g_free(basename_utf8);
    return 0;
}

static void
on_open_with (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_show_open_files_with_dialog (ET_BROWSER (priv->browser));
}

static void
on_run_player (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_run_player_for_selection (ET_BROWSER (priv->browser));
}

static void
on_invert_selection (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_application_window_update_et_file_from_ui (self);

    et_browser_invert_selection (ET_BROWSER (priv->browser));
    et_application_window_update_actions (self);
}

static void
on_delete (GSimpleAction *action,
           GVariant *variant,
           gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GList *selfilelist;
    GList *rowreflist = NULL;
    GList *l;
    gint   progress_bar_index;
    gint   saving_answer;
    gint   nb_files_to_delete;
    gint   nb_files_deleted = 0;
    gchar *msg;
    gchar progress_bar_text[30];
    double fraction;
    GtkTreeModel *treemodel;
    GtkTreeRowReference *rowref;
    GtkTreeSelection *selection;
    GError *error = NULL;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_application_window_update_et_file_from_ui (self);

    /* Number of files to save */
    selection = et_application_window_browser_get_selection (self);
    nb_files_to_delete = gtk_tree_selection_count_selected_rows (selection);

    /* Initialize status bar */
    et_application_window_progress_set_fraction (self, 0.0);
    progress_bar_index = 0;
    g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, nb_files_to_delete);
    et_application_window_progress_set_text (self, progress_bar_text);

    /* Set to unsensitive all command buttons (except Quit button) */
    et_application_window_disable_command_actions (self);
    et_application_window_browser_set_sensitive (self, FALSE);
    et_application_window_tag_area_set_sensitive (self, FALSE);
    et_application_window_file_area_set_sensitive (self, FALSE);

    /* Show msgbox (if needed) to ask confirmation */
    SF_HideMsgbox_Delete_File = 0;

    selfilelist = gtk_tree_selection_get_selected_rows (selection, &treemodel);

    for (l = selfilelist; l != NULL; l = g_list_next (l))
    {
        rowref = gtk_tree_row_reference_new (treemodel, l->data);
        rowreflist = g_list_prepend (rowreflist, rowref);
    }

    g_list_free_full (selfilelist, (GDestroyNotify)gtk_tree_path_free);
    rowreflist = g_list_reverse (rowreflist);

    for (l = rowreflist; l != NULL; l = g_list_next (l))
    {
        GtkTreePath *path;
        ET_File *ETFile;

        path = gtk_tree_row_reference_get_path (l->data);
        ETFile = et_browser_get_et_file_from_path (ET_BROWSER (priv->browser),
                                                   path);
        gtk_tree_path_free (path);

        et_application_window_display_et_file (self, ETFile);
        et_application_window_browser_select_file_by_et_file (self, ETFile,
                                                              FALSE);
        fraction = (++progress_bar_index) / (double) nb_files_to_delete;
        et_application_window_progress_set_fraction (self, fraction);
        g_snprintf (progress_bar_text, 30, "%d/%d", progress_bar_index,
                    nb_files_to_delete);
        et_application_window_progress_set_text (self, progress_bar_text);
        /* FIXME: Needed to refresh status bar */
        while (gtk_events_pending ())
        {
            gtk_main_iteration ();
        }

        saving_answer = delete_file (ETFile,
                                     nb_files_to_delete > 1 ? TRUE : FALSE,
                                     &error);

        switch (saving_answer)
        {
            case 1:
                nb_files_deleted += saving_answer;
                /* Remove file in the browser (corresponding line in the
                 * clist). */
                et_browser_remove_file (ET_BROWSER (priv->browser), ETFile);
                /* Remove file from file list. */
                ET_Remove_File_From_File_List (ETFile);
                break;
            case 0:
                /* Distinguish between the file being skipped, and there being
                 * an error during deletion. */
                if (error)
                {
                    Log_Print (LOG_ERROR, _("Cannot delete file ‘%s’"),
                               error->message);
                    g_clear_error (&error);
                }
                break;
            case -1:
                /* Stop deleting files + reinit progress bar. */
                et_application_window_progress_set_fraction (self, 0.0);
                /* To update state of command buttons. */
                et_application_window_update_actions (self);
                et_application_window_browser_set_sensitive (self, TRUE);
                et_application_window_tag_area_set_sensitive (self, TRUE);
                et_application_window_file_area_set_sensitive (self, TRUE);

                return; /*We stop all actions. */
            default:
                g_assert_not_reached ();
                break;
        }
    }

    g_list_free_full (rowreflist, (GDestroyNotify)gtk_tree_row_reference_free);

    if (nb_files_deleted < nb_files_to_delete)
        msg = g_strdup (_("Some files were not deleted"));
    else
        msg = g_strdup (_("All files have been deleted"));

    /* It's important to displayed the new item, as it'll check the changes in et_browser_toggle_display_mode. */
    if (ETCore->ETFileDisplayed)
    {
        et_application_window_display_et_file (self, ETCore->ETFileDisplayed);
    }
    /*else if (ET_Displayed_File_List_Current())
        ET_Display_File_Data_To_UI((ET_File *)ET_Displayed_File_List_Current()->data);*/

    /* Load list... */
    et_browser_load_file_list (ET_BROWSER (priv->browser),
                               ETCore->ETFileDisplayedList, NULL);
    /* Rebuild the list... */
    /*et_browser_toggle_display_mode (ET_BROWSER (priv->browser));*/

    /* To update state of command buttons */
    et_application_window_update_actions (self);
    et_application_window_browser_set_sensitive (self, TRUE);
    et_application_window_tag_area_set_sensitive (self, TRUE);
    et_application_window_file_area_set_sensitive (self, TRUE);

    et_application_window_progress_set_text (self, "");
    et_application_window_progress_set_fraction (self, 0.0);
    et_application_window_status_bar_message (self, msg, TRUE);
    g_free (msg);

    return;
}

static void
on_undo_file_changes (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindow *self;
    GList *etfilelist = NULL;
    GList *l;
    gboolean state = FALSE;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    self = ET_APPLICATION_WINDOW (user_data);

    et_application_window_update_et_file_from_ui (self);

    etfilelist = et_application_window_browser_get_selected_files (self);

    for (l = etfilelist; l != NULL; l = g_list_next (l))
    {
        state |= ET_Undo_File_Data ((ET_File *)l->data);
    }

    g_list_free (etfilelist);

    /* Refresh the whole list (faster than file by file) to show changes. */
    et_application_window_browser_refresh_list (self);

    /* Display the current file */
    et_application_window_display_et_file (self, ETCore->ETFileDisplayed);
    et_application_window_update_actions (self);
}

static void
on_redo_file_changes (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindow *self;
    GList *etfilelist;
    GList *l;
    gboolean state = FALSE;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    self = ET_APPLICATION_WINDOW (user_data);

    et_application_window_update_et_file_from_ui (self);

    etfilelist = et_application_window_browser_get_selected_files (self);

    for (l = etfilelist; l != NULL; l = g_list_next (l))
    {
        state |= ET_Redo_File_Data ((ET_File *)l->data);
    }

    g_list_free (etfilelist);

    /* Refresh the whole list (faster than file by file) to show changes. */
    et_application_window_browser_refresh_list (ET_APPLICATION_WINDOW (user_data));

    /* Display the current file */
    et_application_window_display_et_file (self, ETCore->ETFileDisplayed);
    et_application_window_update_actions (self);
}

static void
on_save (GSimpleAction *action,
         GVariant *variant,
         gpointer user_data)
{
    Action_Save_Selected_Files ();
}

static void
on_save_force (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    Action_Force_Saving_Selected_Files ();
}

static void
on_find (GSimpleAction *action,
         GVariant *variant,
         gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (priv->search_dialog)
    {
        gtk_widget_show (priv->search_dialog);
    }
    else
    {
        priv->search_dialog = GTK_WIDGET (et_search_dialog_new (GTK_WINDOW (self)));
        gtk_widget_show_all (priv->search_dialog);
    }
}

static void
on_select_all (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GtkWidget *focused;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    /* Use the currently-focused widget and "select all" as appropriate.
     * https://bugzilla.gnome.org/show_bug.cgi?id=697515 */
    focused = gtk_window_get_focus (GTK_WINDOW (user_data));

    if (GTK_IS_EDITABLE (focused))
    {
        gtk_editable_select_region (GTK_EDITABLE (focused), 0, -1);
    }
    else if (!et_tag_area_select_all_if_focused (ET_TAG_AREA (priv->tag_area),
                                                 focused))
    /* Assume that other widgets should select all in the file view. */
    {
        et_application_window_update_et_file_from_ui (self);

        et_browser_select_all (ET_BROWSER (priv->browser));
        et_application_window_update_actions (self);
    }
}

static void
on_unselect_all (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GtkWidget *focused;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    focused = gtk_window_get_focus (GTK_WINDOW (user_data));

    if (GTK_IS_EDITABLE (focused))
    {
        GtkEditable *editable;
        gint pos;

        editable = GTK_EDITABLE (focused);
        pos = gtk_editable_get_position (editable);
        gtk_editable_select_region (editable, 0, 0);
        gtk_editable_set_position (editable, pos);
    }
    else if (!et_tag_area_unselect_all_if_focused (ET_TAG_AREA (priv->tag_area),
                                                   focused))
    /* Assume that other widgets should unselect all in the file view. */
    {
        et_application_window_update_et_file_from_ui (self);

        et_browser_unselect_all (ET_BROWSER (priv->browser));

        ETCore->ETFileDisplayed = NULL;
    }
}

static void
on_undo_last_changes (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindow *self;
    ET_File *ETFile;

    self = ET_APPLICATION_WINDOW (user_data);

    g_return_if_fail (ETCore->ETFileList != NULL);

    et_application_window_update_et_file_from_ui (self);

    ETFile = ET_Undo_History_File_Data ();

    if (ETFile)
    {
        et_application_window_display_et_file (self, ETFile);
        et_application_window_browser_select_file_by_et_file (self, ETFile,
                                                              TRUE);
        et_application_window_browser_refresh_file_in_list (self, ETFile);
    }

    et_application_window_update_actions (self);
}

static void
on_redo_last_changes (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindow *self;
    ET_File *ETFile;

    self = ET_APPLICATION_WINDOW (user_data);

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    et_application_window_update_et_file_from_ui (self);

    ETFile = ET_Redo_History_File_Data ();

    if (ETFile)
    {
        et_application_window_display_et_file (self, ETFile);
        et_application_window_browser_select_file_by_et_file (self, ETFile,
                                                              TRUE);
        et_application_window_browser_refresh_file_in_list (self, ETFile);
    }

    et_application_window_update_actions (self);
}

static void
on_remove_tags (GSimpleAction *action,
                GVariant *variant,
                gpointer user_data)
{
    EtApplicationWindow *self;
    GList *etfilelist;
    GList *l;
    File_Tag *FileTag;
    gint progress_bar_index;
    gint selectcount;
    double fraction;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    self = ET_APPLICATION_WINDOW (user_data);

    et_application_window_update_et_file_from_ui (self);

    /* Initialize status bar */
    et_application_window_progress_set_fraction (self, 0.0);
    etfilelist = et_application_window_browser_get_selected_files (self);
    selectcount = g_list_length (etfilelist);
    progress_bar_index = 0;

    for (l = etfilelist; l != NULL; l = g_list_next (l))
    {
        ET_File *etfile = (ET_File *)l->data;
        FileTag = et_file_tag_new ();
        ET_Manage_Changes_Of_File_Data (etfile, NULL, FileTag);

        fraction = (++progress_bar_index) / (double) selectcount;
        et_application_window_progress_set_fraction (self, fraction);
        /* Needed to refresh status bar */
        while (gtk_events_pending ())
        {
            gtk_main_iteration ();
        }
    }

    g_list_free (etfilelist);

    /* Refresh the whole list (faster than file by file) to show changes. */
    et_application_window_browser_refresh_list (self);

    /* Display the current file */
    et_application_window_display_et_file (self, ETCore->ETFileDisplayed);
    et_application_window_update_actions (self);

    et_application_window_progress_set_fraction (self, 0.0);
    et_application_window_status_bar_message (self,
                                              _("All tags have been removed"),
                                              TRUE);
}

static void
on_preferences (GSimpleAction *action,
                GVariant *variant,
                gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (priv->preferences_dialog)
    {
        gtk_widget_show (priv->preferences_dialog);
    }
    else
    {
        priv->preferences_dialog = GTK_WIDGET (et_preferences_dialog_new (GTK_WINDOW (self)));
        gtk_widget_show_all (priv->preferences_dialog);
    }
}

static void
on_action_toggle (GSimpleAction *action,
                  GVariant *variant,
                  gpointer user_data)
{
    GVariant *state;

    /* Toggle the current state. */
    state = g_action_get_state (G_ACTION (action));
    g_action_change_state (G_ACTION (action),
                           g_variant_new_boolean (!g_variant_get_boolean (state)));
    g_variant_unref (state);
}

static void
on_action_radio (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    /* Set the action state to the just-activated state. */
    g_action_change_state (G_ACTION (action), variant);
}

static void
on_scanner_change (GSimpleAction *action,
                   GVariant *variant,
                   gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    gboolean active;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);
    active = g_variant_get_boolean (variant);

    if (!active)
    {
        if (priv->scan_dialog)
        {
            gtk_widget_hide (priv->scan_dialog);
        }
        else
        {
            return;
        }
    }
    else
    {
        if (priv->scan_dialog)
        {
            gtk_widget_show (priv->scan_dialog);
        }
        else
        {
            priv->scan_dialog = GTK_WIDGET (et_scan_dialog_new (GTK_WINDOW (self)));
            gtk_widget_show (priv->scan_dialog);
        }
    }

    g_simple_action_set_state (action, variant);
}

static void
on_file_artist_view_change (GSimpleAction *action,
                            GVariant *variant,
                            gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    const gchar *state;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);
    state = g_variant_get_string (variant, NULL);

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    et_application_window_update_et_file_from_ui (self);

    if (strcmp (state, "file") == 0)
    {
        et_browser_set_display_mode (ET_BROWSER (priv->browser),
                                     ET_BROWSER_MODE_FILE);
    }
    else if (strcmp (state, "artist") == 0)
    {
        et_browser_set_display_mode (ET_BROWSER (priv->browser),
                                     ET_BROWSER_MODE_ARTIST);
    }
    else
    {
        g_assert_not_reached ();
    }

    g_simple_action_set_state (action, variant);

    et_application_window_update_actions (ET_APPLICATION_WINDOW (user_data));
}

static void
on_collapse_tree (GSimpleAction *action,
                  GVariant *variant,
                  gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_collapse (ET_BROWSER (priv->browser));
}

static void
on_reload_tree (GSimpleAction *action,
                GVariant *variant,
                gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_reload (ET_BROWSER (priv->browser));
}

static void
on_reload_directory (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_reload_directory (ET_BROWSER (priv->browser));
}

static void
on_set_default_path (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_set_current_path_default (ET_BROWSER (priv->browser));
}

static void
on_rename_directory (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_show_rename_directory_dialog (ET_BROWSER (priv->browser));
}

static void
on_browse_directory (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_show_open_directory_with_dialog (ET_BROWSER (priv->browser));
}

static void
on_show_cddb (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);

    et_application_window_show_cddb_dialog (self);
}

static void
on_show_load_filenames (GSimpleAction *action,
                        GVariant *variant,
                        gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (priv->load_files_dialog)
    {
        gtk_widget_show (priv->load_files_dialog);
    }
    else
    {
        priv->load_files_dialog = GTK_WIDGET (et_load_files_dialog_new (GTK_WINDOW (self)));
        gtk_widget_show_all (priv->load_files_dialog);
    }
}

static void
on_show_playlist (GSimpleAction *action,
                  GVariant *variant,
                  gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (priv->playlist_dialog)
    {
        gtk_widget_show (priv->playlist_dialog);
    }
    else
    {
        priv->playlist_dialog = GTK_WIDGET (et_playlist_dialog_new (GTK_WINDOW (self)));
        gtk_widget_show_all (priv->playlist_dialog);
    }
}

static void
on_go_home (GSimpleAction *action,
            GVariant *variant,
            gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_home (ET_BROWSER (priv->browser));
}

static void
on_go_desktop (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_desktop (ET_BROWSER (priv->browser));
}

static void
on_go_documents (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_documents (ET_BROWSER (priv->browser));
}

static void
on_go_downloads (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_downloads (ET_BROWSER (priv->browser));
}

static void
on_go_music (GSimpleAction *action,
             GVariant *variant,
             gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_music (ET_BROWSER (priv->browser));
}

static void
on_go_parent (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_parent (ET_BROWSER (priv->browser));
}

static void
on_go_default (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_load_default_dir (ET_BROWSER (priv->browser));
}

static void
update_ui_for_et_file (EtApplicationWindow *self,
                       ET_File *et_file)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    if (et_file)
    {
        /* To avoid the last line still selected. */
        et_browser_unselect_all (ET_BROWSER (priv->browser));
        et_application_window_browser_select_file_by_et_file (self, et_file,
                                                              TRUE);
        et_application_window_display_et_file (self, et_file);
    }

    et_application_window_update_actions (self);
    et_application_window_scan_dialog_update_previews (self);

    if (!g_settings_get_boolean (MainSettings, "tag-preserve-focus"))
    {
        et_tag_area_title_grab_focus (ET_TAG_AREA (priv->tag_area));
    }
}

static void
on_go_first (GSimpleAction *action,
             GVariant *variant,
             gpointer user_data)
{
    EtApplicationWindow *self;
    GList *etfilelist;

    self = ET_APPLICATION_WINDOW (user_data);

    g_return_if_fail (ETCore->ETFileDisplayedList);

    et_application_window_update_et_file_from_ui (self);

    /* Go to the first item of the list */
    etfilelist = ET_Displayed_File_List_First ();

    update_ui_for_et_file (self, etfilelist ? (ET_File *)etfilelist->data
                                            : NULL);
}

static void
on_go_previous (GSimpleAction *action,
                GVariant *variant,
                gpointer user_data)
{
    EtApplicationWindow *self;
    GList *etfilelist;

    self = ET_APPLICATION_WINDOW (user_data);

    g_return_if_fail (ETCore->ETFileDisplayedList
                      && ETCore->ETFileDisplayedList->prev);

    et_application_window_update_et_file_from_ui (self);

    /* Go to the prev item of the list */
    etfilelist = ET_Displayed_File_List_Previous ();

    update_ui_for_et_file (self, etfilelist ? (ET_File *)etfilelist->data
                                            : NULL);
}

static void
on_go_next (GSimpleAction *action,
            GVariant *variant,
            gpointer user_data)
{
    EtApplicationWindow *self;
    GList *etfilelist;

    self = ET_APPLICATION_WINDOW (user_data);

    g_return_if_fail (ETCore->ETFileDisplayedList
                      && ETCore->ETFileDisplayedList->next);

    et_application_window_update_et_file_from_ui (self);

    /* Go to the next item of the list */
    etfilelist = ET_Displayed_File_List_Next ();

    update_ui_for_et_file (self, etfilelist ? (ET_File *)etfilelist->data
                                            : NULL);
}

static void
on_go_last (GSimpleAction *action,
            GVariant *variant,
            gpointer user_data)
{
    EtApplicationWindow *self;
    GList *etfilelist;

    self = ET_APPLICATION_WINDOW (user_data);

    g_return_if_fail (ETCore->ETFileDisplayedList
                      && ETCore->ETFileDisplayedList->next);

    et_application_window_update_et_file_from_ui (self);

    /* Go to the last item of the list */
    etfilelist = ET_Displayed_File_List_Last ();

    update_ui_for_et_file (self, etfilelist ? (ET_File *)etfilelist->data
                                            : NULL);
}

static void
on_show_cddb_selection (GSimpleAction *action,
                        GVariant *variant,
                        gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_application_window_show_cddb_dialog (self);
    et_cddb_dialog_search_from_selection (ET_CDDB_DIALOG (priv->cddb_dialog));
}

static void
on_clear_log (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_log_area_clear (ET_LOG_AREA (priv->log_area));
}

static void
on_run_player_album (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_run_player_for_album_list (ET_BROWSER (priv->browser));
}

static void
on_run_player_artist (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_run_player_for_artist_list (ET_BROWSER (priv->browser));
}

static gboolean
run_audio_player_using_directory (GError **error)
{
    GList *l;
    GList *file_list = NULL;
    gboolean res;

    for (l = ETCore->ETFileList; l != NULL; l = g_list_next (l))
    {
        ET_File *etfile = (ET_File *)l->data;
        const gchar *path = ((File_Name *)etfile->FileNameCur->data)->value;
        file_list = g_list_prepend (file_list, g_file_new_for_path (path));
    }

    file_list = g_list_reverse (file_list);

    res = et_run_audio_player (file_list, error);

    g_list_free_full (file_list, g_object_unref);

    return res;
}

static void
on_run_player_directory (GSimpleAction *action,
                         GVariant *variant,
                         gpointer user_data)
{
    GError *error = NULL;

    if (!run_audio_player_using_directory (&error))
    {
        Log_Print (LOG_ERROR, _("Failed to launch program ‘%s’"),
                   error->message);
        g_error_free (error);
    }
}

static void
on_stop (GSimpleAction *action,
         GVariant *variant,
         gpointer user_data)
{
    Action_Main_Stop_Button_Pressed ();
}

static const GActionEntry actions[] =
{
    /* File menu. */
    { "open-with", on_open_with },
    { "run-player", on_run_player },
    { "invert-selection", on_invert_selection },
    { "delete", on_delete },
    { "undo-file-changes", on_undo_file_changes },
    { "redo-file-changes", on_redo_file_changes },
    { "save", on_save },
    { "save-force", on_save_force },
    /* Edit menu. */
    { "find", on_find },
    { "select-all", on_select_all },
    { "unselect-all", on_unselect_all },
    { "undo-last-changes", on_undo_last_changes },
    { "redo-last-changes", on_redo_last_changes },
    { "remove-tags", on_remove_tags },
    { "preferences", on_preferences },
    /* View menu. */
    { "scanner", on_action_toggle, NULL, "false", on_scanner_change },
    /* { "scan-mode", on_action_radio, NULL, "false", on_scan_mode_change },
     * Created from GSetting. */
    /* { "sort-mode", on_action_radio, "s", "'ascending-filename'",
     * on_sort_mode_change }, Created from GSetting */
    { "file-artist-view", on_action_radio, "s", "'file'",
      on_file_artist_view_change },
    { "collapse-tree", on_collapse_tree },
    { "reload-tree", on_reload_tree },
    { "reload-directory", on_reload_directory },
    /* { "browse-show-hidden", on_action_toggle, NULL, "true",
      on_browse_show_hidden_change }, Created from GSetting. */
    /* Browser menu. */
    { "set-default-path", on_set_default_path },
    { "rename-directory", on_rename_directory },
    { "browse-directory", on_browse_directory },
    /* { "browse-subdir", on_browse_subdir }, Created from GSetting. */
    /* Miscellaneous menu. */
    { "show-cddb", on_show_cddb },
    { "show-load-filenames", on_show_load_filenames },
    { "show-playlist", on_show_playlist },
    /* Go menu. */
    { "go-home", on_go_home },
    { "go-desktop", on_go_desktop },
    { "go-documents", on_go_documents },
    { "go-downloads", on_go_downloads },
    { "go-music", on_go_music },
    { "go-parent", on_go_parent },
    { "go-default", on_go_default },
    { "go-first", on_go_first },
    { "go-previous", on_go_previous },
    { "go-next", on_go_next },
    { "go-last", on_go_last },
    /* Popup menus. */
    { "show-cddb-selection", on_show_cddb_selection },
    { "clear-log", on_clear_log },
    { "run-player-album", on_run_player_album },
    { "run-player-artist", on_run_player_artist },
    { "run-player-directory", on_run_player_directory },
    /* Toolbar. */
    { "stop", on_stop }
};

static void
et_application_window_dispose (GObject *object)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (object);
    priv = et_application_window_get_instance_private (self);

    save_state (self);

    if (ETCore)
    {
        ET_Core_Free ();
    }

    if (priv->cddb_dialog)
    {
        gtk_widget_destroy (priv->cddb_dialog);
        priv->cddb_dialog = NULL;
    }

    if (priv->load_files_dialog)
    {
        gtk_widget_destroy (priv->load_files_dialog);
        priv->load_files_dialog = NULL;
    }

    if (priv->playlist_dialog)
    {
        gtk_widget_destroy (priv->playlist_dialog);
        priv->playlist_dialog = NULL;
    }

    if (priv->preferences_dialog)
    {
        gtk_widget_destroy (priv->preferences_dialog);
        priv->preferences_dialog = NULL;
    }

    if (priv->scan_dialog)
    {
        gtk_widget_destroy (priv->scan_dialog);
        priv->scan_dialog = NULL;
    }

    if (priv->search_dialog)
    {
        gtk_widget_destroy (priv->search_dialog);
        priv->search_dialog = NULL;
    }

    if (priv->cursor)
    {
        g_object_unref (priv->cursor);
        priv->cursor = NULL;
    }

    G_OBJECT_CLASS (et_application_window_parent_class)->dispose (object);
}

static void
et_application_window_init (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;
    GAction *action;
    GtkWindow *window;
    GtkWidget *main_vbox;
    GtkWidget *hbox;
    GtkWidget *grid;

    priv = et_application_window_get_instance_private (self);

    g_action_map_add_action_entries (G_ACTION_MAP (self), actions,
                                     G_N_ELEMENTS (actions), self);

    action = g_settings_create_action (MainSettings, "browse-show-hidden");
    g_action_map_add_action (G_ACTION_MAP (self), action);
    g_object_unref (action);
    action = g_settings_create_action (MainSettings, "browse-subdir");
    g_action_map_add_action (G_ACTION_MAP (self), action);
    g_object_unref (action);
    action = g_settings_create_action (MainSettings, "scan-mode");
    g_action_map_add_action (G_ACTION_MAP (self), action);
    g_object_unref (action);
    action = g_settings_create_action (MainSettings, "sort-mode");
    g_action_map_add_action (G_ACTION_MAP (self), action);
    g_object_unref (action);

    window = GTK_WINDOW (self);

    gtk_window_set_icon_name (window, "org.gnome.EasyTAG");
    gtk_window_set_title (window, _(PACKAGE_NAME));

    g_signal_connect (self, "configure-event",
                      G_CALLBACK (on_configure_event), NULL);
    g_signal_connect (self, "delete-event",
                      G_CALLBACK (on_main_window_delete_event), NULL);
    g_signal_connect (self, "window-state-event",
                      G_CALLBACK (on_window_state_event), NULL);

    /* Mainvbox for Menu bar + Tool bar + "Browser Area & FileArea & TagArea" + Log Area + "Status bar & Progress bar" */
    main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (self), main_vbox);

    /* Menu bar and tool bar. */
    {
        GtkBuilder *builder;
        GtkWidget *toolbar;

        builder = gtk_builder_new_from_resource ("/org/gnome/EasyTAG/toolbar.ui");

        toolbar = GTK_WIDGET (gtk_builder_get_object (builder, "main_toolbar"));
        gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);

        g_object_unref (builder);
    }

    /* The two panes: BrowserArea on the left, FileArea+TagArea on the right */
    priv->hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

    /* Browser (Tree + File list + Entry) */
    priv->browser = GTK_WIDGET (et_browser_new ());
    gtk_paned_pack1 (GTK_PANED (priv->hpaned), priv->browser, TRUE, TRUE);

    /* Vertical box for FileArea + TagArea */
    grid = gtk_grid_new ();
    gtk_orientable_set_orientation (GTK_ORIENTABLE (grid),
                                    GTK_ORIENTATION_VERTICAL);
    gtk_paned_pack2 (GTK_PANED (priv->hpaned), grid, FALSE, FALSE);

    /* File */
    priv->file_area = et_file_area_new ();
    gtk_container_add (GTK_CONTAINER (grid), priv->file_area);

    /* Tag */
    priv->tag_area = et_tag_area_new ();
    gtk_container_add (GTK_CONTAINER (grid), priv->tag_area);

    /* Update the stored paned position whenever it is changed (after configure
     * and windows state events have been processed). */
    g_signal_connect_swapped (priv->hpaned, "notify::position",
                              G_CALLBACK (on_paned_notify_position), self);
    restore_state (self);

    /* Vertical pane for Browser Area + FileArea + TagArea */
    priv->vpaned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start (GTK_BOX (main_vbox), priv->vpaned, TRUE, TRUE, 0);
    gtk_paned_pack1 (GTK_PANED (priv->vpaned), priv->hpaned, TRUE,
                     FALSE);


    /* Log */
    priv->log_area = et_log_area_new ();
    gtk_paned_pack2 (GTK_PANED (priv->vpaned), priv->log_area, FALSE, FALSE);

    /* Horizontal box for Status bar + Progress bar */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    /* Status bar */
    priv->status_bar = et_status_bar_new ();
    gtk_box_pack_start (GTK_BOX (hbox), priv->status_bar, TRUE, TRUE, 0);

    /* Progress bar */
    priv->progress_bar = et_progress_bar_new ();
    gtk_box_pack_end (GTK_BOX (hbox), priv->progress_bar, FALSE, FALSE, 0);

    gtk_widget_show_all (GTK_WIDGET (main_vbox));

    /* Bind the setting after the log area has been shown, to avoid
     * force-enabling the visibility on startup. */
    g_settings_bind (MainSettings, "log-show", priv->log_area, "visible",
                     G_SETTINGS_BIND_DEFAULT);
}

static void
et_application_window_class_init (EtApplicationWindowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = et_application_window_dispose;
}

/*
 * et_application_window_new:
 *
 * Create a new EtApplicationWindow instance.
 *
 * Returns: a new #EtApplicationWindow
 */
EtApplicationWindow *
et_application_window_new (GtkApplication *application)
{
    g_return_val_if_fail (GTK_IS_APPLICATION (application), NULL);

    return g_object_new (ET_TYPE_APPLICATION_WINDOW, "application",
                         application, NULL);
}

void
et_application_window_scan_dialog_update_previews (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (self != NULL);

    priv = et_application_window_get_instance_private (self);

    if (priv->scan_dialog)
    {
        et_scan_dialog_update_previews (ET_SCAN_DIALOG (priv->scan_dialog));
    }
}

void
et_application_window_progress_set_fraction (EtApplicationWindow *self,
                                             gdouble fraction)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar),
                                   fraction);
}

void
et_application_window_progress_set_text (EtApplicationWindow *self,
                                         const gchar *text)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
}

void
et_application_window_status_bar_message (EtApplicationWindow *self,
                                          const gchar *message,
                                          gboolean with_timer)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_status_bar_message (ET_STATUS_BAR (priv->status_bar), message,
                           with_timer);
}

GtkWidget *
et_application_window_get_log_area (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->log_area;
}

void
et_application_window_show_preferences_dialog_scanner (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    if (!priv->preferences_dialog)
    {
        priv->preferences_dialog = GTK_WIDGET (et_preferences_dialog_new (GTK_WINDOW (self)));
    }

    et_preferences_dialog_show_scanner (ET_PREFERENCES_DIALOG (priv->preferences_dialog));
}

void
et_application_window_browser_toggle_display_mode (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;
    GVariant *variant;

    priv = et_application_window_get_instance_private (self);

    variant = g_action_group_get_action_state (G_ACTION_GROUP (self),
                                               "file-artist-view");

    if (strcmp (g_variant_get_string (variant, NULL), "file") == 0)
    {
        et_browser_set_display_mode (ET_BROWSER (priv->browser),
                                     ET_BROWSER_MODE_FILE);
    }
    else if (strcmp (g_variant_get_string (variant, NULL), "artist") == 0)
    {
        et_browser_set_display_mode (ET_BROWSER (priv->browser),
                                     ET_BROWSER_MODE_ARTIST);
    }
    else
    {
        g_assert_not_reached ();
    }

    g_variant_unref (variant);
}

void
et_application_window_browser_set_sensitive (EtApplicationWindow *self,
                                             gboolean sensitive)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->browser != NULL);

    et_browser_set_sensitive (ET_BROWSER (priv->browser), sensitive);
}

void
et_application_window_browser_clear (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->browser != NULL);

    et_browser_clear (ET_BROWSER (priv->browser));
}

void
et_application_window_browser_clear_album_model (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->browser != NULL);

    et_browser_clear_album_model (ET_BROWSER (priv->browser));
}

void
et_application_window_browser_clear_artist_model (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->browser != NULL);

    et_browser_clear_artist_model (ET_BROWSER (priv->browser));
}

void
et_application_window_select_dir (EtApplicationWindow *self,
                                  GFile *file)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    et_browser_select_dir (ET_BROWSER (priv->browser), file);
}

/*
 * Select a file in the "main list" using the ETFile address of each item.
 */
void
et_application_window_select_file_by_et_file (EtApplicationWindow *self,
                                              ET_File *ETFile)
{
    if (!ETCore->ETFileDisplayedList)
        return;

    et_application_window_update_et_file_from_ui (self);

    /* Display the item */
    et_application_window_browser_select_file_by_et_file (self, ETFile, TRUE);
    /* Just to update 'ETFileDisplayedList'. */
    ET_Displayed_File_List_By_Etfile (ETFile);
    et_application_window_display_et_file (self, ETFile);

    et_application_window_update_actions (self);
    et_application_window_scan_dialog_update_previews (self);
}

/*
 * Save displayed filename into list if it had been changed. Generates also an history list for undo/redo.
 *  - ETFile : the current etfile that we want to save,
 *  - FileName : where is 'temporary' saved the new filename.
 *
 * Note : it builds new filename (with path) from strings encoded into file system
 *        encoding, not UTF-8 (it preserves file system encoding of parent directories).
 */
static void
et_application_window_update_file_name_from_ui (EtApplicationWindow *self,
                                                const ET_File *ETFile,
                                                File_Name *FileName)
{
    gchar *filename_new = NULL;
    gchar *dirname = NULL;
    gchar *filename;
    const gchar *filename_utf8;
    gchar *extension;

    g_return_if_fail (ETFile != NULL && FileName != NULL);

    filename_utf8 = et_application_window_file_area_get_filename (self);
    filename = filename_from_display (filename_utf8);

    if (!filename)
    {
        /* If conversion fails... */
        GtkWidget *msgdialog;
        gchar *filename_escaped_utf8 = g_strescape(filename_utf8, NULL);
        msgdialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE,
                                            _("Could not convert filename ‘%s’ to system filename encoding"),
                                            filename_escaped_utf8);
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
                                                  _("Try setting the environment variable G_FILENAME_ENCODING."));
        gtk_window_set_title (GTK_WINDOW (msgdialog),
                              _("Filename translation"));

        gtk_dialog_run (GTK_DIALOG (msgdialog));
        gtk_widget_destroy (msgdialog);
        g_free (filename_escaped_utf8);
        return;
    }

    /* Get the current path to the file. */
    dirname = g_path_get_dirname (((File_Name *)ETFile->FileNameNew->data)->value);

    /* Convert filename extension (lower or upper). */
    extension = ET_File_Format_File_Extension (ETFile);

    // Check length of filename (limit ~255 characters)
    //ET_File_Name_Check_Length(ETFile,filename);

    /* Filename (in file system encoding!). */
    if (!et_str_empty (filename))
    {
        /* Regenerate the new filename (without path). */
        filename_new = g_strconcat (filename, extension, NULL);
    }
    else
    {
        /* Keep the 'last' filename (if a 'blank' filename was entered in the
         * fileentry for example). */
        filename_new = g_path_get_basename (((File_Name *)ETFile->FileNameNew->data)->value);
    }

    g_free (filename);
    g_free (extension);

    et_file_name_set_from_components (FileName, filename_new, dirname,
                                      g_settings_get_boolean (MainSettings,
                                                              "rename-replace-illegal-chars"));

    g_free (filename_new);
    g_free (dirname);
    return;
}

void
et_application_window_update_et_file_from_ui (EtApplicationWindow *self)
{
    const ET_File_Description *description;
    ET_File *et_file;
    File_Name *FileName;
    File_Tag  *FileTag;
    const gchar *cur_filename_utf8;

    if (!ETCore->ETFileDisplayed)
    {
        return;
    }

    et_file = ETCore->ETFileDisplayed;

    g_return_if_fail (et_file != NULL && et_file->FileNameCur != NULL
                      && et_file->FileNameCur->data != NULL);

    /* Save the current displayed data */
    cur_filename_utf8 = ((File_Name *)((GList *)et_file->FileNameCur)->data)->value_utf8;
    description = et_file->ETFileDescription;

    /* Save filename and generate undo for filename. */
    FileName = et_file_name_new ();
    et_application_window_update_file_name_from_ui (self, et_file, FileName);

    switch (description->TagType)
    {
#ifdef ENABLE_MP3
        case ID3_TAG:
#endif
#ifdef ENABLE_OGG
        case OGG_TAG:
#endif
#ifdef ENABLE_FLAC
        case FLAC_TAG:
#endif
#ifdef ENABLE_MP4
        case MP4_TAG:
#endif
#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
#endif
#ifdef ENABLE_OPUS
        case OPUS_TAG:
#endif
        case APE_TAG:
            FileTag = et_application_window_tag_area_create_file_tag (self);
            et_file_tag_copy_other_into (FileTag, et_file->FileTag->data);
            break;
#ifndef ENABLE_MP3
        case ID3_TAG:
#endif
#ifndef ENABLE_OGG
        case OGG_TAG:
#endif
#ifndef ENABLE_FLAC
        case FLAC_TAG:
#endif
#ifndef ENABLE_MP4
        case MP4_TAG:
#endif
#ifndef ENABLE_WAVPACK
        case WAVPACK_TAG:
#endif
#ifndef ENABLE_OPUS
        case OPUS_TAG:
#endif
        case UNKNOWN_TAG:
        default:
            FileTag = et_file_tag_new ();
            Log_Print (LOG_ERROR,
                       "FileTag: Undefined tag type %d for file %s.",
                       (gint)description->TagType, cur_filename_utf8);
            break;
    }

    /*
     * Generate undo for the file and the main undo list.
     * If no changes detected, FileName and FileTag item are deleted.
     */
    ET_Manage_Changes_Of_File_Data (et_file, FileName, FileTag);

    /* Refresh file into browser list */
    et_application_window_browser_refresh_file_in_list (self, et_file);
}

static void
et_file_header_fields_free (EtFileHeaderFields *fields)
{
    g_free (fields->version);
    g_free (fields->bitrate);
    g_free (fields->samplerate);
    g_free (fields->mode);
    g_free (fields->size);
    g_free (fields->duration);
    g_slice_free (EtFileHeaderFields, fields);
}

static void
et_application_window_display_file_name (EtApplicationWindow *self,
                                         const ET_File *ETFile)
{
    const gchar *new_filename_utf8;
    gchar *dirname_utf8;
    gchar *text;

    g_return_if_fail (ETFile != NULL);

    new_filename_utf8 = ((File_Name *)((GList *)ETFile->FileNameNew)->data)->value_utf8;

    /*
     * Set the path to the file into BrowserEntry (dirbrowser)
     */
    dirname_utf8 = g_path_get_dirname (new_filename_utf8);
    et_application_window_browser_entry_set_text (self, dirname_utf8);

    // And refresh the number of files in this directory
    text = g_strdup_printf (ngettext ("One file", "%u files",
                                      et_file_list_get_n_files_in_path (ETCore->ETFileList,
                                                                        dirname_utf8)),
                            et_file_list_get_n_files_in_path (ETCore->ETFileList,
                                                              dirname_utf8));
    et_application_window_browser_label_set_text (self, text);
    g_free(dirname_utf8);
    g_free(text);
}

/*
 * "Default" way to display File Info to the user interface.
 */
static EtFileHeaderFields *
et_header_fields_new_from_unknown (const ET_File *ETFile)
{
    EtFileHeaderFields *fields;
    ET_File_Info *info;
    gchar *time  = NULL;
    gchar *time1 = NULL;
    gchar *size  = NULL;
    gchar *size1 = NULL;

    info = ETFile->ETFileInfo;
    fields = g_slice_new (EtFileHeaderFields);

    fields->description = _("File");

    /* MPEG, Layer versions */
    fields->version = g_strdup_printf ("%d, Layer %" G_GSIZE_FORMAT,
                                       info->version, info->layer);

    /* Bitrate */
    fields->bitrate = g_strdup_printf (_("%d kb/s"), info->bitrate);

    /* Samplerate */
    fields->samplerate = g_strdup_printf (_("%d Hz"), info->samplerate);

    /* Mode */
    fields->mode = g_strdup_printf ("%d", info->mode);

    /* Size */
    size = g_format_size (info->size);
    size1 = g_format_size (ETCore->ETFileDisplayedList_TotalSize);
    fields->size = g_strdup_printf ("%s (%s)", size, size1);
    g_free (size);
    g_free (size1);

    /* Duration */
    time = Convert_Duration (info->duration);
    time1 = Convert_Duration (ETCore->ETFileDisplayedList_TotalDuration);
    fields->duration = g_strdup_printf ("%s (%s)", time, time1);
    g_free (time);
    g_free (time1);

    return fields;
}

/*
 * Display information of the file (Position + Header + Tag) to the user interface.
 * Before doing it, it saves data of the file currently displayed
 */
void
et_application_window_display_et_file (EtApplicationWindow *self,
                                       ET_File *ETFile)
{
    const ET_File_Description *description;
    const gchar *cur_filename_utf8;
    gchar *msg;
    EtFileHeaderFields *fields;

    g_return_if_fail (ETFile != NULL &&
                      ((GList *)ETFile->FileNameCur)->data != NULL);
                      /* For the case where ETFile is an "empty" structure. */

    cur_filename_utf8 = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value_utf8;
    description = ETFile->ETFileDescription;

    /* Save the current displayed file */
    ETCore->ETFileDisplayed = ETFile;

    /* Display position in list + show/hide icon if file writable/read_only (cur_filename) */
    et_application_window_file_area_set_file_fields (self, ETFile);

    /* Display filename (and his path) (value in FileNameNew) */
    et_application_window_display_file_name (self, ETFile);

    /* Display tag data */
    et_application_window_tag_area_display_et_file (self, ETFile);

    /* Display controls in tag area */
    et_application_window_tag_area_display_controls (self, ETFile);

    /* Display file data, header data and file type */
    switch (description->FileType)
    {
#if defined ENABLE_MP3 && defined ENABLE_ID3LIB
        case MP3_FILE:
        case MP2_FILE:
            fields = et_mpeg_header_display_file_info_to_ui (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_mpeg_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_OGG
        case OGG_FILE:
            fields = et_ogg_header_display_file_info_to_ui (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_ogg_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_SPEEX
        case SPEEX_FILE:
            fields = et_ogg_header_display_file_info_to_ui (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_ogg_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_FLAC
        case FLAC_FILE:
            fields = et_flac_header_display_file_info_to_ui (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_flac_file_header_fields_free (fields);
            break;
#endif
        case MPC_FILE:
            fields = et_mpc_header_display_file_info_to_ui (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_mpc_file_header_fields_free (fields);
            break;
        case MAC_FILE:
            fields = et_mac_header_display_file_info_to_ui (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_mac_file_header_fields_free (fields);
            break;
#ifdef ENABLE_MP4
        case MP4_FILE:
            fields = et_mp4_header_display_file_info_to_ui (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_mp4_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_WAVPACK
        case WAVPACK_FILE:
            fields = et_wavpack_header_display_file_info_to_ui (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_wavpack_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_OPUS
        case OPUS_FILE:
            fields = et_opus_header_display_file_info_to_ui (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_opus_file_header_fields_free (fields);
            break;
#endif
        case OFR_FILE:
#if !defined ENABLE_MP3 && !defined ENABLE_ID3LIB
        case MP3_FILE:
        case MP2_FILE:
#endif
#ifndef ENABLE_OGG
        case OGG_FILE:
#endif
#ifndef ENABLE_SPEEX
        case SPEEX_FILE:
#endif
#ifndef ENABLE_FLAC
        case FLAC_FILE:
#endif
#ifndef ENABLE_MP4
        case MP4_FILE:
#endif
#ifndef ENABLE_WAVPACK
        case WAVPACK_FILE:
#endif
#ifndef ENABLE_OPUS
        case OPUS_FILE:
#endif
        case UNKNOWN_FILE:
        default:
            /* Default displaying. */
            fields = et_header_fields_new_from_unknown (ETFile);
            et_application_window_file_area_set_header_fields (self, fields);
            et_file_header_fields_free (fields);
            Log_Print (LOG_ERROR,
                       "ETFileInfo: Undefined file type %d for file %s.",
                       (gint)description->FileType, cur_filename_utf8);
            break;
    }

    msg = g_strdup_printf (_("File: ‘%s’"), cur_filename_utf8);
    et_application_window_status_bar_message (self, msg, FALSE);
    g_free (msg);
}

GFile *
et_application_window_get_current_path (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (ET_APPLICATION_WINDOW (self), NULL);

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_current_path (ET_BROWSER (priv->browser));
}

GtkWidget *
et_application_window_get_scan_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->scan_dialog;
}

void
et_application_window_apply_changes (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    if (priv->scan_dialog)
    {
        et_scan_dialog_apply_changes (ET_SCAN_DIALOG (priv->scan_dialog));
    }

    if (priv->search_dialog)
    {
        et_search_dialog_apply_changes (ET_SEARCH_DIALOG (priv->search_dialog));
    }
}

/*
 * Disable (FALSE) / Enable (TRUE) all user widgets in the tag area
 */
void
et_application_window_tag_area_set_sensitive (EtApplicationWindow *self,
                                              gboolean sensitive)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->tag_area != NULL);

    /* TAG Area (entries + buttons). */
    gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (priv->tag_area)),
                              sensitive);
}

void
et_application_window_file_area_clear (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_file_area_clear (ET_FILE_AREA (priv->file_area));
}

const gchar *
et_application_window_file_area_get_filename (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (ET_APPLICATION_WINDOW (self), NULL);

    priv = et_application_window_get_instance_private (self);

    return et_file_area_get_filename (ET_FILE_AREA (priv->file_area));
}

void
et_application_window_file_area_set_file_fields (EtApplicationWindow *self,
                                                 const ET_File *ETFile)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_file_area_set_file_fields (ET_FILE_AREA (priv->file_area), ETFile);
}

void
et_application_window_file_area_set_header_fields (EtApplicationWindow *self,
                                                   EtFileHeaderFields *fields)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_file_area_set_header_fields (ET_FILE_AREA (priv->file_area), fields);
}

/*
 * Disable (FALSE) / Enable (TRUE) all user widgets in the file area
 */
void
et_application_window_file_area_set_sensitive (EtApplicationWindow *self,
                                               gboolean sensitive)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->file_area != NULL);

    /* File Area. */
    gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (priv->file_area)),
                              sensitive);
}

static void
set_action_state (EtApplicationWindow *self,
                  const gchar *action_name,
                  gboolean enabled)
{
    GSimpleAction *action;

    action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self),
                                                          action_name));

    if (action == NULL)
    {
        g_error ("Unable to find action '%s' in application window",
                 action_name);
    }

    g_simple_action_set_enabled (action, enabled);
}

/* et_application_window_disable_command_actions:
 * Disable buttons when saving files (do not disable Quit button).
 */
void
et_application_window_disable_command_actions (EtApplicationWindow *self)
{
    GtkDialog *dialog;

    dialog = GTK_DIALOG (et_application_window_get_scan_dialog (self));

    /* Scanner Window */
    if (dialog)
    {
        gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_APPLY, FALSE);
    }

    /* "File" menu commands */
    set_action_state (self, "open-with", FALSE);
    set_action_state (self, "invert-selection", FALSE);
    set_action_state (self, "delete", FALSE);
    set_action_state (self, "go-first", FALSE);
    set_action_state (self, "go-previous", FALSE);
    set_action_state (self, "go-next", FALSE);
    set_action_state (self, "go-last", FALSE);
    set_action_state (self, "remove-tags", FALSE);
    set_action_state (self, "undo-file-changes", FALSE);
    set_action_state (self, "redo-file-changes", FALSE);
    set_action_state (self, "save", FALSE);
    set_action_state (self, "save-force", FALSE);
    set_action_state (self, "undo-last-changes", FALSE);
    set_action_state (self, "redo-last-changes", FALSE);

    /* FIXME: "Scanner" menu commands */
    /*set_action_state (self, "scan-mode", FALSE);*/
}


/* et_application_window_update_actions:
 * Set to sensitive/unsensitive the state of each button into
 * the commands area and menu items in function of state of the "main list".
 */
void
et_application_window_update_actions (EtApplicationWindow *self)
{
    GtkDialog *dialog;

    dialog = GTK_DIALOG (et_application_window_get_scan_dialog (self));

    if (!ETCore->ETFileDisplayedList)
    {
        /* No file found */

        /* File and Tag frames */
        et_application_window_file_area_set_sensitive (self, FALSE);
        et_application_window_tag_area_set_sensitive (self, FALSE);

        /* Tool bar buttons (the others are covered by the menu) */
        set_action_state (self, "stop", FALSE);

        /* Scanner Window */
        if (dialog)
        {
            gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_APPLY,
                                               FALSE);
        }

        /* Menu commands */
        set_action_state (self, "open-with", FALSE);
        set_action_state (self, "invert-selection", FALSE);
        set_action_state (self, "delete", FALSE);
        /* FIXME: set_action_state (self, "sort-mode", FALSE); */
        set_action_state (self, "go-previous", FALSE);
        set_action_state (self, "go-next", FALSE);
        set_action_state (self, "go-first", FALSE);
        set_action_state (self, "go-last", FALSE);
        set_action_state (self, "remove-tags", FALSE);
        set_action_state (self, "undo-file-changes", FALSE);
        set_action_state (self, "redo-file-changes", FALSE);
        set_action_state (self, "save", FALSE);
        set_action_state (self, "save-force", FALSE);
        set_action_state (self, "undo-last-changes", FALSE);
        set_action_state (self, "redo-last-changes", FALSE);
        set_action_state (self, "find", FALSE);
        set_action_state (self, "show-load-filenames", FALSE);
        set_action_state (self, "show-playlist", FALSE);
        set_action_state (self, "run-player", FALSE);
        /* FIXME set_action_state (self, "scan-mode", FALSE);*/
        set_action_state (self, "file-artist-view", FALSE);

        return;
    }else
    {
        gboolean has_undo = FALSE;
        gboolean has_redo = FALSE;
        //gboolean has_to_save = FALSE;

        /* File and Tag frames */
        et_application_window_file_area_set_sensitive (self, TRUE);
        et_application_window_tag_area_set_sensitive (self, TRUE);

        /* Tool bar buttons */
        set_action_state (self, "stop", FALSE);

        /* Scanner Window */
        if (dialog)
        {
            gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_APPLY,
                                               TRUE);
        }

        /* Commands into menu */
        set_action_state (self, "open-with", TRUE);
        set_action_state (self, "invert-selection", TRUE);
        set_action_state (self, "delete", TRUE);
        /* FIXME set_action_state (self, "sort-mode", TRUE); */
        set_action_state (self, "remove-tags", TRUE);
        set_action_state (self, "find", TRUE);
        set_action_state (self, "show-load-filenames", TRUE);
        set_action_state (self, "show-playlist", TRUE);
        set_action_state (self, "run-player", TRUE);
        /* FIXME set_action_state (self, "scan-mode", TRUE); */
        set_action_state (self, "file-artist-view", TRUE);

        /* Check if one of the selected files has undo or redo data */
        {
            GList *etfilelist;
            GList *l;

            etfilelist = et_application_window_browser_get_selected_files (self);

            for (l = etfilelist; l != NULL; l = g_list_next (l))
            {
                const ET_File *etfile = (ET_File *)l->data;

                has_undo |= ET_File_Data_Has_Undo_Data (etfile);
                has_redo |= ET_File_Data_Has_Redo_Data (etfile);
                /* has_to_save |= et_file_check_saved (etfile); */
                if ((has_undo && has_redo /*&& has_to_save*/) || !l->next) // Useless to check the other files
                    break;
            }

            g_list_free (etfilelist);
        }

        /* Enable undo commands if there are undo data */
        if (has_undo)
        {
            set_action_state (self, "undo-file-changes", TRUE);
        }
        else
        {
            set_action_state (self, "undo-file-changes", FALSE);
        }

        /* Enable redo commands if there are redo data */
        if (has_redo)
        {
            set_action_state (self, "redo-file-changes", TRUE);
        }
        else
        {
            set_action_state (self, "redo-file-changes", FALSE);
        }

        /* Enable save file command if file has been changed */
        // Desactivated because problem with only one file in the list, as we can't change the selected file => can't mark file as changed
        /*if (has_to_save)
            ui_widget_set_sensitive(MENU_FILE, AM_SAVE, FALSE);
        else*/
            set_action_state (self, "save", TRUE);
        
        set_action_state (self, "save-force", TRUE);

        /* Enable undo command if there are data into main undo list (history list) */
        if (et_history_list_has_undo (ETCore->ETHistoryFileList))
        {
            set_action_state (self, "undo-last-changes", TRUE);
        }
        else
        {
            set_action_state (self, "undo-last-changes", FALSE);
        }

        /* Enable redo commands if there are data into main redo list (history list) */
        if (et_history_list_has_redo (ETCore->ETHistoryFileList))
        {
            set_action_state (self, "redo-last-changes", TRUE);
        }
        else
        {
            set_action_state (self, "redo-last-changes", FALSE);
        }

        {
            GVariant *variant;
            const gchar *state;

            variant = g_action_group_get_action_state (G_ACTION_GROUP (self),
                                                     "file-artist-view");

            state = g_variant_get_string (variant, NULL);

            if (strcmp (state, "artist") == 0)
            {
                set_action_state (self, "collapse-tree", FALSE);
                set_action_state (self, "reload-tree", FALSE);
            }
            else if (strcmp (state, "file") == 0)
            {
                set_action_state (self, "collapse-tree", TRUE);
                set_action_state (self, "reload-tree", TRUE);
            }
            else
            {
                g_assert_not_reached ();
            }

            g_variant_unref (variant);
        }
    }

    if (!ETCore->ETFileDisplayedList->prev)    /* Is it the 1st item ? */
    {
        set_action_state (self, "go-previous", FALSE);
        set_action_state (self, "go-first", FALSE);
    }
    else
    {
        set_action_state (self, "go-previous", TRUE);
        set_action_state (self, "go-first", TRUE);
    }

    if (!ETCore->ETFileDisplayedList->next)    /* Is it the last item ? */
    {
        set_action_state (self, "go-next", FALSE);
        set_action_state (self, "go-last", FALSE);
    }
    else
    {
        set_action_state (self, "go-next", TRUE);
        set_action_state (self, "go-last", TRUE);
    }
}

void
et_application_window_set_busy_cursor (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    if (!priv->cursor)
    {
        priv->cursor = gdk_cursor_new_for_display (gdk_window_get_display (gtk_widget_get_window (GTK_WIDGET (self))),
                                                   GDK_WATCH);

    }

    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self)),
                           priv->cursor);
}

void
et_application_window_set_normal_cursor (EtApplicationWindow *self)
{
    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    /* Back to standard cursor */
    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self)), NULL);
}

/*
 * Display controls according the kind of tag... (Hide some controls if not available for a tag type)
 */
void
et_application_window_tag_area_display_controls (EtApplicationWindow *self,
                                                 const ET_File *ETFile)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));
    g_return_if_fail (ETFile != NULL && ETFile->ETFileDescription != NULL);

    priv = et_application_window_get_instance_private (self);

    et_tag_area_update_controls (ET_TAG_AREA (priv->tag_area), ETFile);
}

void
et_application_window_browser_entry_set_text (EtApplicationWindow *self,
                                              const gchar *text)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    et_browser_entry_set_text (ET_BROWSER (priv->browser), text);
}

void
et_application_window_browser_label_set_text (EtApplicationWindow *self,
                                              const gchar *text)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    et_browser_label_set_text (ET_BROWSER (priv->browser), text);
}

ET_File *
et_application_window_browser_get_et_file_from_path (EtApplicationWindow *self,
                                                     GtkTreePath *path)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_et_file_from_path (ET_BROWSER (priv->browser), path);
}

ET_File *
et_application_window_browser_get_et_file_from_iter (EtApplicationWindow *self,
                                                     GtkTreeIter *iter)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_et_file_from_iter (ET_BROWSER (priv->browser), iter);
}

GList *
et_application_window_browser_get_selected_files (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_selected_files (ET_BROWSER (priv->browser));
}

GtkTreeSelection *
et_application_window_browser_get_selection (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_selection (ET_BROWSER (priv->browser));
}

GtkTreeViewColumn *
et_application_window_browser_get_column_for_column_id (EtApplicationWindow *self,
                                                        gint column_id)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_column_for_column_id (ET_BROWSER (priv->browser),
                                                column_id);
}

GtkSortType
et_application_window_browser_get_sort_order_for_column_id (EtApplicationWindow *self,
                                                            gint column_id)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_sort_order_for_column_id (ET_BROWSER (priv->browser),
                                                    column_id);
}

void
et_application_window_browser_select_file_by_iter_string (EtApplicationWindow *self,
                                                          const gchar *iter_string,
                                                          gboolean select)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    et_browser_select_file_by_iter_string (ET_BROWSER (priv->browser),
                                           iter_string, select);
}

void
et_application_window_browser_select_file_by_et_file (EtApplicationWindow *self,
                                                      const ET_File *file,
                                                      gboolean select)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_select_file_by_et_file (ET_BROWSER (priv->browser), file,
                                              select);
}

GtkTreePath *
et_application_window_browser_select_file_by_et_file2 (EtApplicationWindow *self,
                                                       const ET_File *file,
                                                       gboolean select,
                                                       GtkTreePath *start_path)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_select_file_by_et_file2 (ET_BROWSER (priv->browser),
                                               file, select, start_path);
}

ET_File *
et_application_window_browser_select_file_by_dlm (EtApplicationWindow *self,
                                                  const gchar *string,
                                                  gboolean select)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_select_file_by_dlm (ET_BROWSER (priv->browser), string,
                                          select);
}

void
et_application_window_browser_unselect_all (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    et_application_window_update_et_file_from_ui (self);

    et_browser_unselect_all (ET_BROWSER (priv->browser));
    ETCore->ETFileDisplayed = NULL;
}

void
et_application_window_browser_refresh_list (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_browser_refresh_list (ET_BROWSER (priv->browser));
}

void
et_application_window_browser_refresh_file_in_list (EtApplicationWindow *self,
                                                    const ET_File *file)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_browser_refresh_file_in_list (ET_BROWSER (priv->browser), file);
}

static void
quit_confirmed (EtApplicationWindow *self)
{
    /* Save the configuration when exiting. */
    et_application_window_apply_changes (self);
    
    /* Quit EasyTAG. */
    Log_Print (LOG_OK, _("Normal exit"));

    gtk_widget_destroy (GTK_WIDGET (self));
}

static void
save_and_quit (EtApplicationWindow *self)
{
    /* Save modified tags */
    if (Save_All_Files_With_Answer (FALSE) == -1)
        return;

    quit_confirmed (self);
}

void
et_application_window_quit (EtApplicationWindow *self)
{
    GtkWidget *msgbox;
    gint response;

    /* If you change the displayed data and quit immediately */
    et_application_window_update_et_file_from_ui (self);

    /* Check if all files have been saved before exit */
    if (g_settings_get_boolean (MainSettings, "confirm-when-unsaved-files")
        && et_file_list_check_all_saved (ETCore->ETFileList) != TRUE)
    {
        /* Some files haven't been saved */
        msgbox = gtk_message_dialog_new (GTK_WINDOW (self),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_NONE,
                                         "%s",
                                         _("Some files have been modified but not saved"));
        gtk_dialog_add_buttons (GTK_DIALOG (msgbox), _("_Discard"),
                                GTK_RESPONSE_NO, _("_Cancel"),
                                GTK_RESPONSE_CANCEL, _("_Save"),
                                GTK_RESPONSE_YES, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (msgbox),
                                         GTK_RESPONSE_YES);
        gtk_window_set_title (GTK_WINDOW (msgbox), _("Quit"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgbox),
                                                  "%s",
                                                  _("Do you want to save them before quitting?"));
        response = gtk_dialog_run (GTK_DIALOG (msgbox));
        gtk_widget_destroy (msgbox);

        switch (response)
        {
            case GTK_RESPONSE_YES:
                save_and_quit (self);
                break;
            case GTK_RESPONSE_NO:
                quit_confirmed (self);
                break;
            case GTK_RESPONSE_CANCEL:
            case GTK_RESPONSE_DELETE_EVENT:
                return;
                break;
            default:
                g_assert_not_reached ();
                break;
        }

    }
    else
    {
        quit_confirmed (self);
    }
}
