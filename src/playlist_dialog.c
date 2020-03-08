/* EasyTAG - tag editor for audio files
 * Copyright (C) 2013-2015  David King <amigadave@amigadave.com>
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

#include "playlist_dialog.h"

#include <glib/gi18n.h>

#include "application_window.h"
#include "browser.h"
#include "charset.h"
#include "easytag.h"
#include "misc.h"
#include "picture.h"
#include "scan.h"
#include "scan_dialog.h"
#include "setting.h"

typedef struct
{
    GtkWidget *name_directory_radio;
    GtkWidget *name_mask_entry;
    GtkWidget *selected_files_check;
    GtkWidget *path_relative_radio;
    GtkWidget *playlist_parent_check;
    GtkWidget *playlist_dos_check;
    GtkWidget *content_filenames_radio;
    GtkWidget *content_extended_radio;
    GtkWidget *content_extended_mask_radio;
    GtkWidget *content_mask_entry;
} EtPlaylistDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtPlaylistDialog, et_playlist_dialog, GTK_TYPE_DIALOG)

/*
 * Function to replace UNIX ForwardSlash with a DOS BackSlash
 */
static void
convert_forwardslash_to_backslash (const gchar *string)
{
    gchar *tmp;

    while ((tmp = strchr (string,'/')) != NULL)
    {
        *tmp = '\\';
    }
}

/*
 * Write a playlist
 *  - 'playlist_name' in file system encoding (not UTF-8)
 */
static gboolean
write_playlist (EtPlaylistDialog *self, GFile *file, GError **error)
{
    EtPlaylistDialogPrivate *priv;
    GFile *parent;
    GFileOutputStream *ostream;
    GString *to_write;
    GList *l;
    GList *etfilelist = NULL;
    gchar *basedir;
    gchar *temp;
    EtPlaylistContent playlist_content;

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    priv = et_playlist_dialog_get_instance_private (self);

    ostream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, error);

    if (!ostream)
    {
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    /* 'base directory' where is located the playlist. Used also to write file with a
     * relative path for file located in this directory and sub-directories
     */
    parent = g_file_get_parent (file);
    basedir = g_file_get_path (parent);
    g_object_unref (parent);

    playlist_content = g_settings_get_enum (MainSettings, "playlist-content");

    /* 1) First line of the file (if playlist content is not set to "write only
     * list of files") */
    if (playlist_content != ET_PLAYLIST_CONTENT_FILENAMES)
    {
        gsize bytes_written;

        to_write = g_string_new ("#EXTM3U\r\n");

        if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                                        to_write->str, to_write->len,
                                        &bytes_written, NULL, error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %" G_GSIZE_FORMAT
                     "bytes of data were written", bytes_written,
                     to_write->len);
            g_assert (error == NULL || *error != NULL);
            g_string_free (to_write, TRUE);
            g_object_unref (ostream);
            return FALSE;
        }
        g_string_free (to_write, TRUE);
    }

    if (g_settings_get_boolean (MainSettings, "playlist-selected-only"))
    {
        etfilelist = et_application_window_browser_get_selected_files (ET_APPLICATION_WINDOW (MainWindow));
    }
    else
    {
        etfilelist = ETCore->ETFileList;
    }

    for (l = etfilelist; l != NULL; l = g_list_next (l))
    {
        const ET_File *etfile;
        const gchar *filename;
        gint duration;

        etfile = (ET_File *)l->data;
        filename = ((File_Name *)etfile->FileNameCur->data)->value;
        duration = ((ET_File_Info *)etfile->ETFileInfo)->duration;

        if (g_settings_get_boolean (MainSettings, "playlist-relative"))
        {
            // Keep only files in this directory and sub-dirs
            if ( strncmp(filename,basedir,strlen(basedir))==0 )
            {
                gsize bytes_written;

                /* 2) Write the header. */
                switch (playlist_content)
                {
                    case ET_PLAYLIST_CONTENT_FILENAMES:
                        /* No header written. */
                        break;
                    case ET_PLAYLIST_CONTENT_EXTENDED:
                        /* Header has extended information. */
                        temp = g_path_get_basename (filename);
                        to_write = g_string_new ("#EXTINF:");
                        /* Must be written in system encoding (not UTF-8). */
                        g_string_append_printf (to_write, "%d,%s\r\n", duration,
                                                temp);

                        if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                                                        to_write->str,
                                                        to_write->len,
                                                        &bytes_written, NULL,
                                                        error))
                        {
                            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %"
                                     G_GSIZE_FORMAT "bytes of data were written",
                                     bytes_written, to_write->len);
                            g_assert (error == NULL || *error != NULL);
                            g_string_free (to_write, TRUE);
                            g_object_unref (ostream);
                            return FALSE;
                        }
                        g_string_free (to_write, TRUE);
                        g_free (temp);
                        break;
                    case ET_PLAYLIST_CONTENT_EXTENDED_MASK:
                    {
                        /* Header uses information generated from a mask. */
                        gchar *mask = filename_from_display (gtk_entry_get_text (GTK_ENTRY (priv->content_mask_entry)));
                        /* Special case: do not replace illegal characters and
                         * do not check if there is a directory separator in
                         * the mask. */
                        gchar *filename_generated_utf8 = et_scan_generate_new_filename_from_mask (etfile, mask, TRUE);
                        gchar *filename_generated = filename_from_display (filename_generated_utf8);

                        to_write = g_string_new ("#EXTINF:");
                        /* Must be written in system encoding (not UTF-8). */
                        g_string_append_printf (to_write, "%d,%s\r\n", duration,
                                                filename_generated);

                        if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                                                        to_write->str,
                                                        to_write->len,
                                                        &bytes_written, NULL,
                                                        error))
                        {
                            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %"
                                     G_GSIZE_FORMAT "bytes of data were written",
                                     bytes_written, to_write->len);
                            g_assert (error == NULL || *error != NULL);
                            g_string_free (to_write, TRUE);
                            g_object_unref (ostream);
                            return FALSE;
                        }
                        g_string_free (to_write, TRUE);
                        g_free (mask);
                        g_free (filename_generated_utf8);

                        break;
                    }
                    default:
                        g_assert_not_reached ();
                        break;
                }

                /* 3) Write the file path. */
                if (g_settings_get_boolean (MainSettings,
                                            "playlist-dos-separator"))
                {
                    gchar *filename_conv = g_strdup(filename+strlen(basedir)+1);
                    convert_forwardslash_to_backslash (filename_conv);

                    to_write = g_string_new (filename_conv);
                    /* Must be written in system encoding (not UTF-8)*/
                    to_write = g_string_append (to_write, "\r\n");

                    if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                                                    to_write->str,
                                                    to_write->len,
                                                    &bytes_written, NULL,
                                                    error))
                    {
                        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %"
                                 G_GSIZE_FORMAT "bytes of data were written",
                                 bytes_written, to_write->len);
                        g_assert (error == NULL || *error != NULL);
                        g_string_free (to_write, TRUE);
                        g_object_unref (ostream);
                        return FALSE;
                    }
                    g_string_free (to_write, TRUE);
                    g_free(filename_conv);
                }else
                {
                    to_write = g_string_new (filename+strlen(basedir)+1);
                    /* Must be written in system encoding (not UTF-8)*/
                    to_write = g_string_append (to_write, "\r\n");

                    if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                                                    to_write->str,
                                                    to_write->len,
                                                    &bytes_written, NULL,
                                                    error))
                    {
                        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %"
                                 G_GSIZE_FORMAT "bytes of data were written",
                                 bytes_written, to_write->len);
                        g_assert (error == NULL || *error != NULL);
                        g_string_free (to_write, TRUE);
                        g_object_unref (ostream);
                        return FALSE;
                    }
                    g_string_free (to_write, TRUE);
                }
            }
        }
        else /* !ETSettings:playlist-relative */
        {
            gsize bytes_written;

            /* 2) Write the header. */
            switch (playlist_content)
            {
                case ET_PLAYLIST_CONTENT_FILENAMES:
                    /* No header written. */
                    break;
                case ET_PLAYLIST_CONTENT_EXTENDED:
                    /* Header has extended information. */
                    temp = g_path_get_basename (filename);
                    to_write = g_string_new ("#EXTINF:");
                    /* Must be written in system encoding (not UTF-8). */
                    g_string_append_printf (to_write, "%d,%s\r\n", duration,
                                            temp);
                    g_free (temp);

                    if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                                                    to_write->str, to_write->len,
                                                    &bytes_written, NULL, error))
                    {
                        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %"
                                 G_GSIZE_FORMAT" bytes of data were written",
                                 bytes_written, to_write->len);
                        g_assert (error == NULL || *error != NULL);
                        g_string_free (to_write, TRUE);
                        g_object_unref (ostream);
                        return FALSE;
                    }
                    g_string_free (to_write, TRUE);
                    break;
                case ET_PLAYLIST_CONTENT_EXTENDED_MASK:
                {
                    /* Header uses information generated from a mask. */
                    gchar *mask = filename_from_display (gtk_entry_get_text (GTK_ENTRY (priv->content_mask_entry)));
                    /* Special case: do not replace illegal characters and
                     * do not check if there is a directory separator in
                     * the mask. */
                    gchar *filename_generated_utf8 = et_scan_generate_new_filename_from_mask (etfile, mask, TRUE);
                    gchar *filename_generated = filename_from_display (filename_generated_utf8);

                    to_write = g_string_new ("#EXTINF:");
                    /* Must be written in system encoding (not UTF-8). */
                    g_string_append_printf (to_write, "%d,%s\r\n", duration,
                                            filename_generated);
                    g_free (filename_generated);

                    if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                                                    to_write->str, to_write->len,
                                                    &bytes_written, NULL, error))
                    {
                        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %"
                                 G_GSIZE_FORMAT" bytes of data were written",
                                 bytes_written, to_write->len);
                        g_assert (error == NULL || *error != NULL);
                        g_string_free (to_write, TRUE);
                        g_object_unref (ostream);
                        return FALSE;
                    }
                    g_string_free (to_write, TRUE);
                    g_free (mask);
                    g_free (filename_generated_utf8);
                }
                break;
                default:
                    g_assert_not_reached ();
                    break;
            }

            /* 3) Write the file path. */
            if (g_settings_get_boolean (MainSettings,
                                        "playlist-dos-separator"))
            {
                gchar *filename_conv = g_strdup(filename);
                convert_forwardslash_to_backslash(filename_conv);

                to_write = g_string_new (filename_conv);
                /* Must be written in system encoding (not UTF-8)*/
                to_write = g_string_append (to_write, "\r\n");

                if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                                                to_write->str, to_write->len,
                                                &bytes_written, NULL, error))
                {
                    g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %"
                             G_GSIZE_FORMAT" bytes of data were written",
                             bytes_written, to_write->len);
                    g_assert (error == NULL || *error != NULL);
                    g_string_free (to_write, TRUE);
                    g_object_unref (ostream);
                    return FALSE;
                }
                g_string_free (to_write, TRUE);
                g_free(filename_conv);
            }else
            {
                to_write = g_string_new (filename);
                /* Must be written in system encoding (not UTF-8)*/
                to_write = g_string_append (to_write, "\r\n");

                if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream),
                                                to_write->str, to_write->len,
                                                &bytes_written, NULL, error))
                {
                    g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %"
                             G_GSIZE_FORMAT" bytes of data were written",
                             bytes_written, to_write->len);
                    g_assert (error == NULL || *error != NULL);
                    g_string_free (to_write, TRUE);
                    g_object_unref (ostream);
                    return FALSE;
                }
                g_string_free (to_write, TRUE);
            }
        }
    }

    if (g_settings_get_boolean (MainSettings, "playlist-selected-only"))
    {
        g_list_free (etfilelist);
    }

    g_assert (error == NULL || *error == NULL);
    g_object_unref (ostream);
    g_free(basedir);
    return TRUE;
}

static void
write_button_clicked (EtPlaylistDialog *self)
{
    EtPlaylistDialogPrivate *priv;
    gchar *playlist_name = NULL;
    gchar *playlist_path_utf8;      // Path
    gchar *playlist_basename_utf8;  // Filename
    gchar *playlist_name_utf8;      // Path + filename
    gchar *temp;
    GtkWidget *msgdialog;

    priv = et_playlist_dialog_get_instance_private (self);

    /* Check if playlist name was filled. */
    if (g_settings_get_boolean (MainSettings, "playlist-use-mask")
        && *(gtk_entry_get_text (GTK_ENTRY (priv->name_mask_entry))) == '\0')
    {
        /* TODO: Can this happen? */
        g_settings_set_boolean (MainSettings, "playlist-use-mask", FALSE);
    }

    // Path of the playlist file (may be truncated later if PLAYLIST_CREATE_IN_PARENT_DIR is TRUE)
    temp = g_file_get_path (et_application_window_get_current_path (ET_APPLICATION_WINDOW (MainWindow)));
    playlist_path_utf8 = g_filename_display_name (temp);
    g_free (temp);

    /* Build the playlist filename. */
    if (g_settings_get_boolean (MainSettings, "playlist-use-mask"))
    {
        EtConvertSpaces convert_mode;

        if (!ETCore->ETFileList)
            return;

        playlist_name = g_settings_get_string (MainSettings,
                                               "playlist-filename-mask");

        /* Generate filename from tag of the current selected file (FIXME). */
        temp = filename_from_display (playlist_name);
        g_free (playlist_name);
        playlist_basename_utf8 = et_scan_generate_new_filename_from_mask (ETCore->ETFileDisplayed,
                                                                          temp,
                                                                          FALSE);
        g_free (temp);

        /* Replace Characters (with scanner). */
        convert_mode = g_settings_get_enum (MainSettings,
                                            "rename-convert-spaces");

        switch (convert_mode)
        {
            case ET_CONVERT_SPACES_SPACES:
                Scan_Convert_Underscore_Into_Space (playlist_basename_utf8);
                Scan_Convert_P20_Into_Space (playlist_basename_utf8);
                break;
            case ET_CONVERT_SPACES_UNDERSCORES:
                Scan_Convert_Space_Into_Underscore (playlist_basename_utf8);
                break;
            case ET_CONVERT_SPACES_REMOVE:
                Scan_Remove_Spaces (playlist_basename_utf8);
                break;
            /* FIXME: Check that this is intended. */
            case ET_CONVERT_SPACES_NO_CHANGE:
            default:
                g_assert_not_reached ();
                break;
        }
    }else // PLAYLIST_USE_DIR_NAME
    {

        if ( strcmp(playlist_path_utf8,G_DIR_SEPARATOR_S)==0 )
        {
            playlist_basename_utf8 = g_strdup("playlist");
        }else
        {
            gchar *tmp_string = g_strdup(playlist_path_utf8);
            // Remove last '/'
            if (tmp_string[strlen(tmp_string)-1]==G_DIR_SEPARATOR)
                tmp_string[strlen(tmp_string)-1] = '\0';
            // Get directory name
            temp = g_path_get_basename(tmp_string);
            playlist_basename_utf8 = g_strdup(temp);
            g_free(tmp_string);
            g_free(temp);
        }

    }

    /* Must be placed after "Build the playlist filename", as we can truncate
     * the path! */
    if (g_settings_get_boolean (MainSettings, "playlist-parent-directory"))
    {
        if ( (strcmp(playlist_path_utf8,G_DIR_SEPARATOR_S) != 0) )
        {
            gchar *tmp;
            // Remove last '/'
            if (playlist_path_utf8[strlen(playlist_path_utf8)-1]==G_DIR_SEPARATOR)
                playlist_path_utf8[strlen(playlist_path_utf8)-1] = '\0';
            // Get parent directory
            if ( (tmp=strrchr(playlist_path_utf8,G_DIR_SEPARATOR)) != NULL )
                *(tmp + 1) = '\0';
        }
    }

    // Generate path + filename of playlist
    if (playlist_path_utf8[strlen(playlist_path_utf8)-1]==G_DIR_SEPARATOR)
        playlist_name_utf8 = g_strconcat(playlist_path_utf8,playlist_basename_utf8,".m3u",NULL);
    else
        playlist_name_utf8 = g_strconcat(playlist_path_utf8,G_DIR_SEPARATOR_S,playlist_basename_utf8,".m3u",NULL);

    g_free(playlist_path_utf8);
    g_free(playlist_basename_utf8);

    playlist_name = filename_from_display(playlist_name_utf8);

    {
        GFile *file = g_file_new_for_path (playlist_name);
        GError *error = NULL;

        if (!write_playlist (self, file, &error))
        {
            // Writing fails...
            msgdialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_CLOSE,
                                               _("Cannot write playlist file ‘%s’"),
                                               playlist_name_utf8);
            gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
                                                      "%s", error->message);
            gtk_window_set_title(GTK_WINDOW(msgdialog),_("Playlist File Error"));

            gtk_dialog_run(GTK_DIALOG(msgdialog));
            gtk_widget_destroy(msgdialog);
            g_error_free (error);
        }else
        {
            gchar *msg;
            msg = g_strdup_printf (_("Wrote playlist file ‘%s’"),
                                   playlist_name_utf8);
            et_application_window_status_bar_message (ET_APPLICATION_WINDOW (MainWindow),
                                                      msg, TRUE);
            g_free (msg);
        }
        g_object_unref (file);
    }
    g_free(playlist_name_utf8);
    g_free(playlist_name);
}

/*
 * on_response:
 * @dialog: the dialog which emitted the response signal
 * @response_id: the response ID
 * @user_data: user data set when the signal was connected
 *
 * Signal handler for the write playlist dialog.
 */
static void
on_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
    switch (response_id)
    {
        case GTK_RESPONSE_OK:
            write_button_clicked (ET_PLAYLIST_DIALOG (dialog));
            break;
        case GTK_RESPONSE_CLOSE:
            gtk_widget_hide (GTK_WIDGET (dialog));
            break;
        case GTK_RESPONSE_DELETE_EVENT:
            break;
        default:
            g_assert_not_reached ();
    }
}

/*
 * entry_check_content_mask:
 * @entry: the entry for which to check the mask
 * @user_data: user data set when the signal was connected
 *
 * Display an icon in the entry if the current text contains an invalid mask.
 */
static void
entry_check_content_mask (GtkEntry *entry, gpointer user_data)
{
    gchar *tmp  = NULL;
    gchar *mask = NULL;

    g_return_if_fail (entry != NULL);

    mask = g_strdup (gtk_entry_get_text (entry));

    if (et_str_empty (mask))
        goto Bad_Mask;

    while (mask)
    {
        if ( (tmp=strrchr(mask,'%'))==NULL )
        {
            /* There is no more code. */
            /* No code in mask is accepted. */
            goto Good_Mask;
        }
        if (strlen(tmp)>1 && (tmp[1]=='t' || tmp[1]=='a' || tmp[1]=='b' || tmp[1]=='y' ||
                              tmp[1]=='g' || tmp[1]=='n' || tmp[1]=='l' || tmp[1]=='c' || tmp[1]=='i'))
        {
            /* The code is valid. */
            /* No separator is accepted. */
            *(mask+strlen(mask)-strlen(tmp)) = '\0';
        }else
        {
            goto Bad_Mask;
        }
    }

    Bad_Mask:
        g_free(mask);
        gtk_entry_set_icon_from_icon_name (entry, GTK_ENTRY_ICON_SECONDARY,
                                           "emblem-unreadable");
        gtk_entry_set_icon_tooltip_text (entry, GTK_ENTRY_ICON_SECONDARY,
                                         _("Invalid scanner mask"));
        return;

    Good_Mask:
        g_free(mask);
        gtk_entry_set_icon_from_icon_name (entry, GTK_ENTRY_ICON_SECONDARY,
                                           NULL);
        return;
}

static void
create_playlist_dialog (EtPlaylistDialog *self)
{
    EtPlaylistDialogPrivate *priv;

    priv = et_playlist_dialog_get_instance_private (self);

    /* Playlist name */
    g_settings_bind (MainSettings, "playlist-filename-mask",
                     priv->name_mask_entry, "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "playlist-use-mask",
                     priv->name_directory_radio, "active",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_INVERT_BOOLEAN);

    /* Playlist options */
    g_settings_bind (MainSettings, "playlist-selected-only",
                     priv->selected_files_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (MainSettings, "playlist-relative",
                     priv->path_relative_radio, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Create playlist in parent directory. */
    g_settings_bind (MainSettings, "playlist-parent-directory",
                     priv->playlist_parent_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* DOS Separator. */
    g_settings_bind (MainSettings, "playlist-dos-separator",
                     priv->playlist_dos_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Playlist content */
    g_settings_bind (MainSettings, "playlist-default-mask",
                     priv->content_mask_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* Mask status icon. Signal connection to check if mask is correct in the
     * mask entry. */
    g_signal_connect (priv->content_mask_entry, "changed",
                      G_CALLBACK (entry_check_content_mask), NULL);

    g_settings_bind_with_mapping (MainSettings, "playlist-content",
                                  priv->content_filenames_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->content_filenames_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "playlist-content",
                                  priv->content_extended_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->content_extended_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "playlist-content",
                                  priv->content_extended_mask_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->content_extended_mask_radio, NULL);

    /* To initialize the mask status icon and visibility. */
    g_signal_emit_by_name (priv->name_mask_entry, "changed");
    g_signal_emit_by_name (priv->content_mask_entry, "changed");
}

static void
et_playlist_dialog_init (EtPlaylistDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
    create_playlist_dialog (self);
}

static void
et_playlist_dialog_class_init (EtPlaylistDialogClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/EasyTAG/playlist_dialog.ui");
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  name_directory_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  name_mask_entry);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  selected_files_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  path_relative_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  playlist_parent_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  playlist_dos_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  content_filenames_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  content_extended_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  content_extended_mask_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPlaylistDialog,
                                                  content_mask_entry);
    gtk_widget_class_bind_template_callback (widget_class,
                                             entry_check_content_mask);
    gtk_widget_class_bind_template_callback (widget_class, on_response);
}

/*
 * et_playlist_dialog_new:
 *
 * Create a new EtPlaylistDialog instance.
 *
 * Returns: a new #EtPlaylistDialog
 */
EtPlaylistDialog *
et_playlist_dialog_new (GtkWindow *parent)
{
    GtkSettings *settings;
    gboolean use_header_bar = FALSE;

    g_return_val_if_fail (GTK_WINDOW (parent), NULL);

    settings = gtk_settings_get_default ();

    if (settings)
    {
        g_object_get (settings, "gtk-dialogs-use-header", &use_header_bar,
                      NULL);
    }

    return g_object_new (ET_TYPE_PLAYLIST_DIALOG, "transient-for", parent,
                         "use-header-bar", use_header_bar, NULL);
}
