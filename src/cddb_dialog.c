/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014,2015  David King <amigadave@amigadave.com>
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

#include "config.h"

#include "cddb_dialog.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <libsoup/soup.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "win32/win32dep.h"
#include <errno.h>

#include "application_window.h"
#include "easytag.h"
#include "enums.h"
#include "et_core.h"
#include "browser.h"
#include "scan_dialog.h"
#include "log.h"
#include "misc.h"
#include "setting.h"
#include "id3_tag.h"
#include "setting.h"
#include "charset.h"

typedef struct
{
    GtkWidget *album_list_view;
    GtkWidget *track_list_view;

    GList *album_list;

    GtkListStore *album_list_model;
    GtkListStore *track_list_model;

    GtkWidget *search_entry;

    GtkWidget *apply_button;
    GtkWidget *automatic_search_button;
    GtkWidget *manual_search_button;
    GtkWidget *stop_search_button;

    GtkWidget *status_bar;
    guint status_bar_context;

    gboolean stop_searching;

    GtkWidget *artist_check;
    GtkWidget *album_check;
    GtkWidget *track_check;
    GtkWidget *other_check;

    GtkWidget *blues_check;
    GtkWidget *classical_check;
    GtkWidget *country_check;
    GtkWidget *folk_check;
    GtkWidget *jazz_check;
    GtkWidget *misc_check;
    GtkWidget *newage_check;
    GtkWidget *reggae_check;
    GtkWidget *rock_check;
    GtkWidget *soundtrack_check;

    GtkWidget *filename_check;
    GtkWidget *title_check;
    GtkWidget *fill_artist_check;
    GtkWidget *fill_album_check;
    GtkWidget *year_check;
    GtkWidget *fill_track_check;
    GtkWidget *track_total_check;
    GtkWidget *genre_check;

    GtkWidget *scanner_check;
    GtkWidget *dlm_check;

    SoupSession *session;
} EtCDDBDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtCDDBDialog, et_cddb_dialog, GTK_TYPE_DIALOG)

/*
 * Structure used for each item of the album list. Aslo attached to each row of
 * the album list
 */
typedef struct
{
    gchar *server_name; /* Remote access: server name. Local access : NULL */
    guint server_port; /* Remote access: server port. Local access: 0 */
    gchar *server_cgi_path; /* Remote access: server CGI path.
                             * Local access: discid file path */

    GdkPixbuf *bitmap; /* Pixmap logo for the server. */

    gchar *artist_album; /* CDDB artist+album (allocated) */
    gchar *category; /* CDDB genre (allocated) */
    gchar *id; /* example : 8d0de30c (allocated) */
    GList *track_list; /* List of CddbTrackAlbum items. */
    gboolean other_version; /* TRUE if this album is another version of the
                             * previous one. */

    /* Filled when loading the track list. */
    gchar *artist; /* (allocated) */
    gchar *album; /* (allocated) */
    gchar *genre; /* (allocated) */
    gchar *year; /* (allocated) */
    guint duration;
} CddbAlbum;


/*
 * Structure used for each item of the track_list of the CddbAlbum structure.
 */
typedef struct
{
    guint track_number;
    gchar *track_name; /* (allocated) */
    guint duration;
    CddbAlbum *cddbalbum; /* Pointer to the parent CddbAlbum structure (to
                           * quickly access album properties). */
} CddbTrackAlbum;


typedef struct
{
    gulong offset;
} CddbTrackFrameOffset;

enum
{
    CDDB_ALBUM_LIST_PIXBUF,
    CDDB_ALBUM_LIST_ALBUM,
    CDDB_ALBUM_LIST_CATEGORY,
    CDDB_ALBUM_LIST_DATA,
    CDDB_ALBUM_LIST_FONT_STYLE,
    CDDB_ALBUM_LIST_FONT_WEIGHT,
    CDDB_ALBUM_LIST_FOREGROUND_COLOR,
    CDDB_ALBUM_LIST_COUNT
};

enum
{
    CDDB_TRACK_LIST_NUMBER,
    CDDB_TRACK_LIST_NAME,
    CDDB_TRACK_LIST_TIME,
    CDDB_TRACK_LIST_DATA,
    CDDB_TRACK_LIST_ETFILE,
    CDDB_TRACK_LIST_COUNT
};

enum
{
    SORT_LIST_NUMBER,
    SORT_LIST_NAME
};


#define CDDB_GENRE_MAX ( sizeof(cddb_genre_vs_id3_genre)/sizeof(cddb_genre_vs_id3_genre[0]) - 1 )
static const gchar *cddb_genre_vs_id3_genre [][2] =
{
    /* Cddb Genre - ID3 Genre */
    {"Blues",       "Blues"},
    {"Classical",   "Classical"},
    {"Country",     "Country"},
    {"Data",        "Other"},
    {"Folk",        "Folk"},
    {"Jazz",        "Jazz"},
    {"NewAge",      "New Age"},
    {"Reggae",      "Reggae"},
    {"Rock",        "Rock"},
    {"Soundtrack",  "Soundtrack"},
    {"Misc",        "Other"}
};

static const guint MAX_STRING_LEN = 1024;


/**************
 * Prototypes *
 **************/
static gboolean Cddb_Free_Track_Album_List (GList *track_list);

static GdkPixbuf *Cddb_Get_Pixbuf_From_Server_Name (const gchar *server_name);

static const gchar *Cddb_Get_Id3_Genre_From_Cddb_Genre (const gchar *cddb_genre);

static gint Cddb_Track_List_Sort_Func (GtkTreeModel *model, GtkTreeIter *a,
                                       GtkTreeIter *b, gpointer data);

static void Cddb_Get_Album_Tracks_List_CB (EtCDDBDialog *self, GtkTreeSelection *selection);


/*
 * The window to connect to the cd data base.
 * Protocol information: http://ftp.freedb.org/pub/freedb/latest/CDDBPROTO
 */

static void
update_apply_button_sensitivity (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;

    priv = et_cddb_dialog_get_instance_private (self);

    /* If any field is set, enable the apply button. */
    if (priv->apply_button
        && gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->track_list_model),
                                           NULL) > 0
        && (g_settings_get_flags (MainSettings, "cddb-set-fields") != 0))
    {
        gtk_widget_set_sensitive (GTK_WIDGET (priv->apply_button), TRUE);
    }
    else
    {
        gtk_widget_set_sensitive (GTK_WIDGET (priv->apply_button), FALSE);
    }
}

static void
update_search_button_sensitivity (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;

    priv = et_cddb_dialog_get_instance_private (self);

    if (priv->manual_search_button
        && *(gtk_entry_get_text (GTK_ENTRY (priv->search_entry)))
        && (g_settings_get_flags (MainSettings, "cddb-search-fields") != 0)
        && (g_settings_get_flags (MainSettings, "cddb-search-categories") != 0))
    {
        gtk_widget_set_sensitive (GTK_WIDGET (priv->manual_search_button), TRUE);
    }
    else
    {
        gtk_widget_set_sensitive (GTK_WIDGET (priv->manual_search_button), FALSE);
    }
}

/*
 * Show collected infos of the album in the status bar
 */
static void
show_album_info (EtCDDBDialog *self, GtkTreeSelection *selection)
{
    EtCDDBDialogPrivate *priv;
    CddbAlbum *cddbalbum = NULL;
    gchar *msg, *duration_str;
    GtkTreeIter row;
    priv = et_cddb_dialog_get_instance_private (self);

    if (gtk_tree_selection_get_selected(selection, NULL, &row))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(priv->album_list_model), &row, CDDB_ALBUM_LIST_DATA, &cddbalbum, -1);
    }
    if (!cddbalbum)
        return;

    duration_str = Convert_Duration((gulong)cddbalbum->duration);
    msg = g_strdup_printf(_("Album: ‘%s’, "
                            "artist: ‘%s’, "
                            "length: ‘%s’, "
                            "year: ‘%s’, "
                            "genre: ‘%s’, "
                            "disc ID: ‘%s’"),
                            cddbalbum->album ? cddbalbum->album : "",
                            cddbalbum->artist ? cddbalbum->artist : "",
                            duration_str,
                            cddbalbum->year ? cddbalbum->year : "",
                            cddbalbum->genre ? cddbalbum->genre : "",
                            cddbalbum->id ? cddbalbum->id : "");
    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar), priv->status_bar_context, msg);
    g_free(msg);
    g_free(duration_str);
}

/*
 * Select the corresponding file into the main file list
 */
static void
Cddb_Track_List_Row_Selected (EtCDDBDialog *self, GtkTreeSelection *selection)
{
    EtCDDBDialogPrivate *priv;
    GList       *selectedRows;
    GList *l;

    priv = et_cddb_dialog_get_instance_private (self);

    // Exit if we don't have to select files in the main list
    if (!g_settings_get_boolean (MainSettings, "cddb-follow-file"))
        return;

    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    // We might be called with no rows selected
    if (!selectedRows)
    {
        return;
    }

    /* Unselect files in the main list before re-selecting them... */
    et_application_window_browser_unselect_all (ET_APPLICATION_WINDOW (MainWindow));

    for (l = selectedRows; l != NULL; l = g_list_next (l))
    {
        GtkTreeIter currentFile;
        gchar *text_path;

        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->track_list_model),
                                     &currentFile, (GtkTreePath*)l->data))
        {
            if (g_settings_get_boolean (MainSettings, "cddb-dlm-enabled"))
            {
                ET_File *etfile;

                gtk_tree_model_get (GTK_TREE_MODEL (priv->track_list_model),
                                    &currentFile, CDDB_TRACK_LIST_NAME,
                                    &text_path, -1);
                etfile = et_application_window_browser_select_file_by_dlm (ET_APPLICATION_WINDOW (MainWindow),
                                                                            text_path,
                                                                            TRUE);
                gtk_list_store_set (priv->track_list_model, &currentFile,
                                    CDDB_TRACK_LIST_ETFILE, etfile, -1);
            }
            else
            {
                text_path = gtk_tree_model_get_string_from_iter (GTK_TREE_MODEL (priv->track_list_model),
                                                                 &currentFile);
                et_application_window_browser_select_file_by_iter_string (ET_APPLICATION_WINDOW (MainWindow),
                                                                          text_path,
                                                                          TRUE);
            }

            g_free (text_path);
        }
    }

    g_list_free_full (selectedRows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * Invert the selection of every row in the track list
 */
static void
Cddb_Track_List_Invert_Selection (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gboolean valid;

    priv = et_cddb_dialog_get_instance_private (self);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->track_list_view));

    if (selection)
    {
        /* Must block the select signal to avoid selecting all files (one by one) in the main list */
        g_signal_handlers_block_by_func (selection,
                                         G_CALLBACK (Cddb_Track_List_Row_Selected),
                                         NULL);

        valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->track_list_model), &iter);
        while (valid)
        {
            if (gtk_tree_selection_iter_is_selected(selection, &iter))
            {
                gtk_tree_selection_unselect_iter(selection, &iter);
            } else
            {
                gtk_tree_selection_select_iter(selection, &iter);
            }
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->track_list_model), &iter);
        }
        g_signal_handlers_unblock_by_func (selection,
                                           G_CALLBACK (Cddb_Track_List_Row_Selected),
                                           NULL);
        g_signal_emit_by_name(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->track_list_view))), "changed");
    }
}

/*
 * Set the row apperance depending if we have cached info or not
 * Bold/Red = Info are already loaded, but not displayed
 * Italic/Light Red = Duplicate CDDB entry
 */
static void
Cddb_Album_List_Set_Row_Appearance (EtCDDBDialog *self, GtkTreeIter *row)
{
    EtCDDBDialogPrivate *priv;
    CddbAlbum *cddbalbum = NULL;

    priv = et_cddb_dialog_get_instance_private (self);

    gtk_tree_model_get(GTK_TREE_MODEL(priv->album_list_model), row,
                       CDDB_ALBUM_LIST_DATA, &cddbalbum, -1);

    if (cddbalbum->track_list != NULL)
    {
        if (g_settings_get_boolean (MainSettings, "file-changed-bold"))
        {
            gtk_list_store_set(priv->album_list_model, row,
                               CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                               CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_BOLD,
                               CDDB_ALBUM_LIST_FOREGROUND_COLOR, NULL,-1);
        } else
        {
            if (cddbalbum->other_version == TRUE)
            {
                const GdkRGBA LIGHT_RED = { 1.0, 0.5, 0.5, 1.0 };
                gtk_list_store_set(priv->album_list_model, row,
                                   CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                                   CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                                   CDDB_ALBUM_LIST_FOREGROUND_COLOR, &LIGHT_RED, -1);
            } else
            {
                gtk_list_store_set(priv->album_list_model, row,
                                   CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                                   CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                                   CDDB_ALBUM_LIST_FOREGROUND_COLOR, &RED, -1);
            }
        }
    }
    else
    {
        if (cddbalbum->other_version == TRUE)
        {
            if (g_settings_get_boolean (MainSettings, "file-changed-bold"))
            {
                gtk_list_store_set(priv->album_list_model, row,
                                   CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_ITALIC,
                                   CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                                   CDDB_ALBUM_LIST_FOREGROUND_COLOR, NULL,-1);
            } else
            {
                const GdkRGBA GREY = { 0.664, 0.664, 0.664, 1.0 };
                gtk_list_store_set(priv->album_list_model, row,
                                   CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                                   CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                                   CDDB_ALBUM_LIST_FOREGROUND_COLOR, &GREY, -1);
            }
        } else
        {
            gtk_list_store_set(priv->album_list_model, row,
                               CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                               CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                               CDDB_ALBUM_LIST_FOREGROUND_COLOR, NULL, -1);
        }
    }
}

/*
 * Clear the album model, blocking the tree view selection changed handlers
 * during the process, to prevent the handlers being called on removed rows.
 */
static void
cddb_album_model_clear (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    GtkTreeSelection *selection;

    priv = et_cddb_dialog_get_instance_private (self);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->album_list_view));

    g_signal_handlers_block_by_func (selection,
                                     G_CALLBACK (Cddb_Get_Album_Tracks_List_CB),
                                     self);
    g_signal_handlers_block_by_func (selection, G_CALLBACK (show_album_info),
                                     self);

    gtk_list_store_clear (priv->album_list_model);

    g_signal_handlers_unblock_by_func (selection, G_CALLBACK (show_album_info),
                                       self);
    g_signal_handlers_unblock_by_func (selection,
                                       G_CALLBACK (Cddb_Get_Album_Tracks_List_CB),
                                       self);
}

/*
 * Clear the album model, blocking the tree view selection changed handlers
 * during the process, to prevent the handlers being called on removed rows.
 */
static void
cddb_track_model_clear (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    GtkTreeSelection *selection;

    priv = et_cddb_dialog_get_instance_private (self);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->track_list_view));

    g_signal_handlers_block_by_func (selection,
                                     G_CALLBACK (Cddb_Track_List_Row_Selected),
                                     self);

    gtk_list_store_clear (priv->track_list_model);

    g_signal_handlers_unblock_by_func (selection,
                                       G_CALLBACK (Cddb_Track_List_Row_Selected),
                                       self);
}

/*
 * Load the CddbTrackList into the corresponding List
 */
static void
Cddb_Load_Track_Album_List (EtCDDBDialog *self, GList *track_list)
{
    EtCDDBDialogPrivate *priv;

    priv = et_cddb_dialog_get_instance_private (self);

    if (track_list && priv->track_list_view)
    {
        GList *l;

        /* Must block the select signal of the target to avoid looping. */
        cddb_track_model_clear (self);

        for (l = g_list_first (track_list); l != NULL; l = g_list_next (l))
        {
            gchar *row_text;
            CddbTrackAlbum *cddbtrackalbum = l->data;

            row_text = Convert_Duration ((gulong)cddbtrackalbum->duration);

            /* Load the row in the list. */
            gtk_list_store_insert_with_values (priv->track_list_model, NULL,
                                               G_MAXINT,
                                               CDDB_TRACK_LIST_NUMBER,
                                               cddbtrackalbum->track_number,
                                               CDDB_TRACK_LIST_NAME,
                                               cddbtrackalbum->track_name,
                                               CDDB_TRACK_LIST_TIME,
                                               row_text,
                                               CDDB_TRACK_LIST_DATA,
                                               cddbtrackalbum,
                                               -1);

            g_free (row_text);
        }

        update_apply_button_sensitivity (self);
    }
}

static void
cddb_track_frame_offset_free (CddbTrackFrameOffset *offset)
{
    g_slice_free (CddbTrackFrameOffset, offset);
}

/*
 * check_message_successful:
 * @message: the message on which to check for success
 *
 * Check the result of @message to see whether it was a success. Soup handles
 * redirects, so that the result code is that of the final request.
 *
 * Returns: %TRUE if the request was successful, %FALSE otherwise
 */
static gboolean
check_message_successful (SoupMessage *message)
{
    return message->status_code == SOUP_STATUS_OK;
}

/*
 * read_cddb_result_line:
 * @dstream: a #GDataInputStream corresponding to a CDDB result, with the line
 *           ending mode set appropriately
 * @cancellable: a #GCancellable for the operation
 * @result: a location to store the resulting line
 *
 * Read a single line from a CDDB result.
 *
 * Returns: %TRUE on success, %FALSE otherwise. @result is always set, and
 *          should be freed when finished with
 */
static gboolean
read_cddb_result_line (GDataInputStream *dstream,
                       GCancellable *cancellable,
                       gchar **result)
{
    gchar *buffer;
    gsize bytes_read;
    GError *error = NULL;

    if ((buffer = g_data_input_stream_read_line_utf8 (dstream, &bytes_read,
                                                      cancellable, &error))
        == NULL)
    {
        /* The error is not set if there are no more lines to read. */
        if (error)
        {
            g_debug ("Error reading line from CDDB result: %s",
                     error->message);
            g_error_free (error);
        }

        *result = NULL;
        return FALSE;
    }

    if (bytes_read > MAX_STRING_LEN)
    {
        /* TODO: Although the maximum line length is not specified, this is
         * probably bogus. */
        g_warning ("Overly long line in CDDB result");
    }

    *result = buffer;

    return TRUE;
}

/*
 * read_cddb_header_line:
 * @dstream: a #GDataInputStream corresponding to a CDDB header, with the line
 *           ending mode set appropriately
 * @cancellable: a #GCancellable for the operation
 * @result: a location to store the resulting line
 *
 * Read a single line from a CDDB header.
 *
 * Returns: %TRUE on success, %FALSE otherwise. @result is always set, and
 *          should be freed when finished with
 */
static gboolean
read_cddb_header_line (GDataInputStream *dstream,
                       GCancellable *cancellable,
                       gchar **result)
{
    gchar *buffer;
    gsize bytes_read;
    GError *error = NULL;

    if ((buffer = g_data_input_stream_read_line_utf8 (dstream, &bytes_read,
                                                      cancellable, &error))
        == NULL)
    {
        /* The error is not set if there are no more lines to read. */
        if (error)
        {
            g_debug ("Error reading line from CDDB header: %s",
                     error->message);
            g_error_free (error);
        }

        return FALSE;
    }

    if (bytes_read > MAX_STRING_LEN)
    {
        /* TODO: Although the maximum line length is not specified, this is
         * probably bogus. */
        g_warning ("Overly long line in CDDB header");
    }

    *result = buffer;

    if (result == NULL || (strncmp (*result, "200", 3) != 0
                           && strncmp (*result, "210", 3) != 0
                           && strncmp (*result, "211", 3) != 0))
    {
        return FALSE;
    }

    return TRUE;
}

/*
 * log_message_from_request_error:
 * @message: the message which failed to be sent
 * @error: the error associated with the failed message
 *
 * Create an appropriate message for @message based on the status code and
 * contents of @error.
 *
 * Returns: a newly-allocated string
 */
static gchar *
log_message_from_request_error (SoupMessage *message,
                                GError *error)
{
    SoupURI *uri;
    gchar *msg;

    uri = soup_message_get_uri (message);

    switch (message->status_code)
    {
        case SOUP_STATUS_CANT_RESOLVE:
        case SOUP_STATUS_CANT_RESOLVE_PROXY:
            msg = g_strdup_printf (_("Cannot resolve host: ‘%s’: %s"),
                                   soup_uri_get_host (uri), error->message);
            break;
        case SOUP_STATUS_CANT_CONNECT:
        case SOUP_STATUS_CANT_CONNECT_PROXY:
        case SOUP_STATUS_TOO_MANY_REDIRECTS:
        default:
            msg = g_strdup_printf (_("Cannot connect to host: ‘%s’: %s"),
                                   soup_uri_get_host (uri), error->message);
    }

    return msg;
}

/*
 * Look up a specific album in freedb, and save to a CddbAlbum structure
 */
static gboolean
Cddb_Get_Album_Tracks_List (EtCDDBDialog *self, GtkTreeSelection* selection)
{
    EtCDDBDialogPrivate *priv;
    CddbAlbum *cddbalbum = NULL;
    GList     *TrackOffsetList = NULL;
    gchar *uri;
    gchar *cddb_out = NULL;
    gchar *msg, *copy, *valid;
    gchar     *cddb_server_name;
    guint cddb_server_port;
    gchar     *cddb_server_cgi_path;
    gboolean   read_track_offset = FALSE;
    GtkTreeIter row;
    SoupMessage *message;
    GCancellable *cancellable;
    GError *error = NULL;
    GInputStream *istream;
    GDataInputStream *dstream;

    priv = et_cddb_dialog_get_instance_private (self);

    cddb_track_model_clear (self);
    update_apply_button_sensitivity (self);

    if (gtk_tree_selection_get_selected(selection, NULL, &row))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(priv->album_list_model), &row, CDDB_ALBUM_LIST_DATA, &cddbalbum, -1);
    }
    if (!cddbalbum)
        return FALSE;

    // We have already the track list
    if (cddbalbum->track_list != NULL)
    {
        Cddb_Load_Track_Album_List (self, cddbalbum->track_list);
        return TRUE;
    }

    // Parameters of the server used
    cddb_server_name     = cddbalbum->server_name;
    cddb_server_port     = cddbalbum->server_port;
    cddb_server_cgi_path = cddbalbum->server_cgi_path;


    /* Connection to the server. */
    if (strstr (cddb_server_name, "gnudb") != NULL)
    {
        /* For gnudb. */
        uri = g_strdup_printf ("http://%s:%u/gnudb/%s/%s", cddb_server_name,
                               cddb_server_port, cddbalbum->category,
                               cddbalbum->id);
    }
    else
    {
        /* CDDB Request (ex: GET /~cddb/cddb.cgi?cmd=cddb+read+jazz+0200a401&hello=noname+localhost+EasyTAG+0.31&proto=1 HTTP/1.1\r\nHost: freedb.freedb.org:80\r\nConnection: close). */
        uri = g_strdup_printf ("http://%s:%u%s?cmd=cddb+read+%s+%s&hello=noname+localhost+%s+%s&proto=6",
                               cddb_server_name, cddb_server_port,
                               cddb_server_cgi_path, cddbalbum->category,
                               cddbalbum->id, PACKAGE_NAME, PACKAGE_VERSION);
    }

    /* Send the request. */
    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,_("Sending request…"));
    while (gtk_events_pending()) gtk_main_iteration();

    /* Write result in a file. */
    message = soup_message_new (SOUP_METHOD_GET, uri);

    if (!message)
    {
        g_critical ("Invalid CDDB request URI: %s", uri);
        g_free (uri);
        return FALSE;
    }

    g_free (uri);

    cancellable = g_cancellable_new ();
    istream = soup_session_send (priv->session, message, cancellable,
                                 &error);

    if (!check_message_successful (message))
    {
        msg = log_message_from_request_error (message, error);
        gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                            priv->status_bar_context, msg);
        Log_Print (LOG_ERROR, "%s", msg);
        g_free (msg);
        g_error_free (error);
        g_object_unref (cancellable);
        g_object_unref (message);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->stop_search_button),FALSE);
        return FALSE;
    }

    dstream = g_data_input_stream_new (istream);
    g_object_unref (istream);
    g_data_input_stream_set_newline_type (dstream,
                                          G_DATA_STREAM_NEWLINE_TYPE_ANY);

    /* Parse server answer: Check CDDB Header (freedb only). */
    if (strstr (cddb_server_name, "gnudb") == NULL)
    {
        /* For freedb. */
        if (!read_cddb_header_line (dstream, cancellable, &cddb_out))
        {
            msg = g_strdup_printf (_("The server returned a bad response ‘%s’"),
                                   cddb_out);
            gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                                priv->status_bar_context, msg);
            Log_Print (LOG_ERROR, "%s", msg);

            g_object_unref (dstream);
            g_object_unref (cancellable);
            g_object_unref (message);
            g_free (msg);
            g_free (cddb_out);

            return FALSE;
        }
    }

    g_free (cddb_out);

    while (!priv->stop_searching
           && read_cddb_result_line (dstream, cancellable, &cddb_out))
    {
        if (!cddb_out) // Empty line?
            continue;

        if (strstr (cddb_out, "Track frame offsets") != NULL) /* We read the Track frame offset. */
        {
            read_track_offset = TRUE; // The next reads are for the tracks offset
            g_free (cddb_out);
            continue;
        }
        else if (read_track_offset) /* We are reading a track offset? (generates TrackOffsetList). */
        {
            if ( strtoul(cddb_out+1,NULL,10)>0 )
            {
                CddbTrackFrameOffset *cddbtrackframeoffset = g_slice_new (CddbTrackFrameOffset);
                cddbtrackframeoffset->offset = strtoul(cddb_out+1,NULL,10);
                TrackOffsetList = g_list_append(TrackOffsetList,cddbtrackframeoffset);
            }else
            {
                read_track_offset = FALSE; // No more track offset
            }

            g_free (cddb_out);
            continue;

        }
        else if (strstr (cddb_out, "Disc length: ") != NULL) /* Length of album (in second). */
        {
            cddbalbum->duration = atoi(strchr(cddb_out,':')+1);

            if (TrackOffsetList) // As it must be the last item, do nothing if no previous data
            {
                CddbTrackFrameOffset *cddbtrackframeoffset = g_slice_new (CddbTrackFrameOffset);
                cddbtrackframeoffset->offset = cddbalbum->duration * 75; // It's the last offset
                TrackOffsetList = g_list_append(TrackOffsetList,cddbtrackframeoffset);
            }

            g_free (cddb_out);
            continue;

        }
        else if (strncmp (cddb_out, "DTITLE=", 7) == 0) /* "Artist / Album" names. */
        {
            // Note : disc title too long take severals lines. For example :
            // DTITLE=Marilyn Manson / The Nobodies (2005 Against All Gods Mix - Korea Tour L
            // DTITLE=imited Edition)
            if (!cddbalbum->album)
            {
                // It is the first time we find DTITLE...

                gchar *alb_ptr = strstr(cddb_out," / ");
                // Album
                if (alb_ptr && alb_ptr[3])
                {
                    cddbalbum->album = Try_To_Validate_Utf8_String(alb_ptr+3);
                    *alb_ptr = 0;
                }

                // Artist
                cddbalbum->artist = Try_To_Validate_Utf8_String(cddb_out+7); // '7' to skip 'DTITLE='
            }else
            {
                // It is at least the second time we find DTITLE
                // So we suppose that only the album was truncated

                // Album
                valid = Try_To_Validate_Utf8_String(cddb_out+7); // '7' to skip 'DTITLE='
                copy = cddbalbum->album; // To free...
                cddbalbum->album = g_strconcat(cddbalbum->album,valid,NULL);
                g_free(copy);
            }

            g_free (cddb_out);
            continue;

        }else if ( strncmp(cddb_out,"DYEAR=",6)==0 ) // Year
        {
            valid = Try_To_Validate_Utf8_String(cddb_out+6); // '6' to skip 'DYEAR='
            if (!et_str_empty (valid))
                cddbalbum->year = valid;

            g_free (cddb_out);
            continue;

        }else if ( strncmp(cddb_out,"DGENRE=",7)==0 ) // Genre
        {
            valid = Try_To_Validate_Utf8_String(cddb_out+7); // '7' to skip 'DGENRE='
            if (!et_str_empty (valid))
                cddbalbum->genre = valid;

            g_free (cddb_out);
            continue;

        }else if ( strncmp(cddb_out,"TTITLE",6)==0 ) // Track title (for exemple : TTITLE10=xxxx)
        {
            CddbTrackAlbum *cddbtrackalbum_last = NULL;

            CddbTrackAlbum *cddbtrackalbum = g_slice_new0 (CddbTrackAlbum);
            cddbtrackalbum->cddbalbum = cddbalbum; // To find the CddbAlbum father quickly

            // Here is a fix when TTITLExx doesn't contain an "=", we skip the line
            if ((copy = strchr (cddb_out, '=')) != NULL)
            {
                cddbtrackalbum->track_name = Try_To_Validate_Utf8_String(copy+1);
            }else
            {
                g_free (cddb_out);
                continue;
            }

            *strchr (cddb_out, '=') = 0;
            cddbtrackalbum->track_number = atoi(cddb_out+6)+1;

            // Note : titles too long take severals lines. For example :
            // TTITLE15=Bob Marley vs. Funkstar De Luxe Remix - Sun Is Shining (Radio De Lu
            // TTITLE15=xe Edit)
            // So to check it, we compare current track number with the previous one...
            if (cddbalbum->track_list)
                cddbtrackalbum_last = g_list_last(cddbalbum->track_list)->data;
            if (cddbtrackalbum_last && cddbtrackalbum_last->track_number == cddbtrackalbum->track_number)
            {
                gchar *track_name = g_strconcat(cddbtrackalbum_last->track_name,cddbtrackalbum->track_name,NULL);
                g_free(cddbtrackalbum_last->track_name);

                cddbtrackalbum_last->track_name = Try_To_Validate_Utf8_String(track_name);

                /* Frees useless allocated data previously. */
                g_free(cddbtrackalbum->track_name);
                g_slice_free (CddbTrackAlbum, cddbtrackalbum);
            }else
            {
                if (TrackOffsetList && TrackOffsetList->next)
                {
                    cddbtrackalbum->duration = (((CddbTrackFrameOffset *)TrackOffsetList->next->data)->offset
                                                 - ((CddbTrackFrameOffset *)TrackOffsetList->data)->offset)
                                                / 75; /* Calculate time in seconds. */
                    TrackOffsetList = g_list_next (TrackOffsetList);
                }
                cddbalbum->track_list = g_list_append(cddbalbum->track_list,cddbtrackalbum);
            }

            g_free (cddb_out);
            continue;

        }else if ( strncmp(cddb_out,"EXTD=",5)==0 ) // Extended album data
        {
            gchar *genre_ptr = strstr(cddb_out,"ID3G:");
            gchar *year_ptr  = strstr(cddb_out,"YEAR:");
            // May contains severals EXTD field it too long
            // EXTD=Techno
            // EXTD= YEAR: 1997 ID3G:  18
            // EXTD= ID3G:  17
            if (year_ptr && cddbalbum->year)
                cddbalbum->year = g_strdup_printf("%d",atoi(year_ptr+5));
            if (genre_ptr && cddbalbum->genre)
                cddbalbum->genre = g_strdup(Id3tag_Genre_To_String(atoi(genre_ptr+5)));

            g_free (cddb_out);
            continue;
        }

        g_free(cddb_out);
    }

    /* Remote access. */
    /* Close connection */
    g_object_unref (dstream);
    g_object_unref (cancellable);
    g_object_unref (message);

    /* Set color of the selected row (without reloading the whole list) */
    Cddb_Album_List_Set_Row_Appearance (self, &row);

    /* Load the track list of the album */
    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,_("Loading album track list…"));
    while (gtk_events_pending()) gtk_main_iteration();
    Cddb_Load_Track_Album_List (self, cddbalbum->track_list);

    show_album_info (self, gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->album_list_view)));

    g_list_free_full (g_list_first (TrackOffsetList),
                      (GDestroyNotify)cddb_track_frame_offset_free);
    return TRUE;
}

/*
 * Callback when selecting a row in the Album List.
 * We get the list of tracks of the selected album
 */
static void
Cddb_Get_Album_Tracks_List_CB (EtCDDBDialog *self, GtkTreeSelection *selection)
{
    gint i;
    gint i_max = 5;

    /* As may be not opened the first time (The server returned a wrong answer!)
     * me try to reconnect severals times */
    for (i = 1; i <= i_max; i++)
    {
        if (Cddb_Get_Album_Tracks_List (self, selection) == TRUE)
        {
            break;
        }
    }
}

/*
 * Load the priv->album_list into the corresponding List
 */
static void
Cddb_Load_Album_List (EtCDDBDialog *self, gboolean only_red_lines)
{
    EtCDDBDialogPrivate *priv;
    GtkTreeIter iter;
    GList *l;

    GtkTreeSelection *selection;
    GList            *selectedRows = NULL;
    GtkTreeIter       currentIter;
    CddbAlbum        *cddbalbumSelected = NULL;

    priv = et_cddb_dialog_get_instance_private (self);

    // Memorize the current selected item
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->album_list_view));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);
    if (selectedRows)
    {
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->album_list_model), &currentIter, (GtkTreePath*)selectedRows->data))
            gtk_tree_model_get(GTK_TREE_MODEL(priv->album_list_model), &currentIter,
                               CDDB_ALBUM_LIST_DATA, &cddbalbumSelected, -1);
    }

    /* Remove lines. */
    cddb_album_model_clear (self);

    // Reload list following parameter 'only_red_lines'
    for (l = g_list_first (priv->album_list); l != NULL; l = g_list_next (l))
    {
        CddbAlbum *cddbalbum = l->data;

        if ( (only_red_lines && cddbalbum->track_list) || !only_red_lines)
        {
            /* Load the row in the list. */
            gtk_list_store_insert_with_values (priv->album_list_model, &iter,
                                               G_MAXINT,
                                               CDDB_ALBUM_LIST_PIXBUF,
                                               cddbalbum->bitmap,
                                               CDDB_ALBUM_LIST_ALBUM,
                                               cddbalbum->artist_album,
                                               CDDB_ALBUM_LIST_CATEGORY,
                                               cddbalbum->category,
                                               CDDB_ALBUM_LIST_DATA,
                                               cddbalbum, -1);

            Cddb_Album_List_Set_Row_Appearance (self, &iter);

            // Select this item if it is the saved one...
            if (cddbalbum == cddbalbumSelected)
                gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->album_list_view)), &iter);
        }
    }
}

/*
 * Free priv->album_list
 */
static gboolean
Cddb_Free_Album_List (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    GList *l;

    priv = et_cddb_dialog_get_instance_private (self);

    g_return_val_if_fail (priv->album_list != NULL, FALSE);

    priv->album_list = g_list_first (priv->album_list);

    for (l = priv->album_list; l != NULL; l = g_list_next (l))
    {
        CddbAlbum *cddbalbum = l->data;

        if (cddbalbum)
        {
            g_free(cddbalbum->server_name);
            g_free (cddbalbum->server_cgi_path);
            g_object_unref(cddbalbum->bitmap);

            g_free(cddbalbum->artist_album);
            g_free(cddbalbum->category);
            g_free(cddbalbum->id);
            if (cddbalbum->track_list)
            {
                Cddb_Free_Track_Album_List(cddbalbum->track_list);
                cddbalbum->track_list = NULL;
            }
            g_free(cddbalbum->artist);
            g_free(cddbalbum->album);
            g_free(cddbalbum->genre);
            g_free(cddbalbum->year);

            g_slice_free (CddbAlbum, cddbalbum);
        }
    }

    g_list_free (priv->album_list);
    priv->album_list = NULL;

    return TRUE;
}

/*
 * Fields          : artist, title, track, rest
 * CDDB Categories : blues, classical, country, data, folk, jazz, misc, newage, reggae, rock, soundtrack
 */
static gchar *
Cddb_Generate_Request_String_With_Fields_And_Categories_Options (EtCDDBDialog *self)
{
    GString *string;
    guint search_fields;
    guint search_categories;

    /* Init. */
    string = g_string_sized_new (256);

    /* Fields. */
    /* FIXME: Fetch cddb-search-fields "all-set" mask. */
#if 0
    if (search_all_fields)
    {
        g_string_append (string, "&allfields=YES");
    }
    else
    {
        g_string_append (string, "&allfields=NO");
    }
#endif

    search_fields = g_settings_get_flags (MainSettings, "cddb-search-fields");

    if (search_fields & ET_CDDB_SEARCH_FIELD_ARTIST)
    {
        g_string_append (string, "&fields=artist");
    }
    if (search_fields & ET_CDDB_SEARCH_FIELD_TITLE)
    {
        g_string_append (string, "&fields=title");
    }
    if (search_fields & ET_CDDB_SEARCH_FIELD_TRACK)
    {
        g_string_append (string, "&fields=track");
    }
    if (search_fields & ET_CDDB_SEARCH_FIELD_OTHER)
    {
        g_string_append (string, "&fields=rest");
    }

    /* Categories (warning: there is one other CDDB category that is not used
     * here ("data")) */
    search_categories = g_settings_get_flags (MainSettings,
                                              "cddb-search-categories");
    g_string_append (string, "&allcats=NO");

    if (search_categories & ET_CDDB_SEARCH_CATEGORY_BLUES)
    {
        g_string_append (string, "&cats=blues");
    }
    if (search_categories & ET_CDDB_SEARCH_CATEGORY_CLASSICAL)
    {
        g_string_append (string, "&cats=classical");
    }
    if (search_categories & ET_CDDB_SEARCH_CATEGORY_COUNTRY)
    {
        g_string_append (string, "&cats=country");
    }
    if (search_categories & ET_CDDB_SEARCH_CATEGORY_FOLK)
    {
        g_string_append (string, "&cats=folk");
    }
    if (search_categories & ET_CDDB_SEARCH_CATEGORY_JAZZ)
    {
        g_string_append (string, "&cats=jazz");
    }
    if (search_categories & ET_CDDB_SEARCH_CATEGORY_MISC)
    {
        g_string_append (string, "&cats=misc");
    }
    if (search_categories & ET_CDDB_SEARCH_CATEGORY_NEWAGE)
    {
        g_string_append (string, "&cats=newage");
    }
    if (search_categories & ET_CDDB_SEARCH_CATEGORY_REGGAE)
    {
        g_string_append (string, "&cats=reggae");
    }
    if (search_categories & ET_CDDB_SEARCH_CATEGORY_ROCK)
    {
        g_string_append (string, "&cats=rock");
    }
    if (search_categories & ET_CDDB_SEARCH_CATEGORY_SOUNDTRACK)
    {
        g_string_append (string, "&cats=soundtrack");
    }

    return g_string_free (string, FALSE);
}

/*
 * Site FREEDB.ORG - Manual Search
 * Send request (using the HTML search page in freedb.org site) to the CD database
 * to get the list of albums matching to a string.
 */
static gboolean
Cddb_Search_Album_List_From_String_Freedb (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    gchar *string = NULL;
    gchar *tmp, *tmp1;
    gchar *cddb_in;         // For the request to send
    gchar *cddb_out = NULL; // Answer received
    gchar *cddb_out_tmp;
    gchar *msg;
    gchar *cddb_server_name;
    guint cddb_server_port;
    gchar *cddb_server_cgi_path;

    gchar *ptr_cat, *cat_str, *id_str, *art_alb_str;
    gchar *art_alb_tmp = NULL;
    gboolean use_art_alb = FALSE;
    gchar *end_str;
    gchar *html_end_str;
    gchar  buffer[MAX_STRING_LEN+1];
    gboolean web_search_disabled = FALSE;

    SoupMessage *message;
    GCancellable *cancellable;
    GError *error = NULL;
    GInputStream *istream;
    GDataInputStream *dstream;

    priv = et_cddb_dialog_get_instance_private (self);

    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,"");

    /* Get words to search... */
    string = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->search_entry)));
    if (et_str_empty (string))
    {
        return FALSE;
    }

    /* Format the string of words */
    g_strstrip (string);
    /* Remove the duplicated spaces */
    while ((tmp=strstr(string,"  "))!=NULL) // Search 2 spaces
    {
        tmp1 = tmp + 1;
        while (*tmp1)
            *(tmp++) = *(tmp1++);
        *tmp = '\0';
    }

    /* Convert spaces to '+' */
    while ( (tmp=strchr(string,' '))!=NULL )
        *tmp = '+';

    cddb_server_name = g_settings_get_string (MainSettings,
                                              "cddb-manual-search-hostname");
    cddb_server_port = g_settings_get_uint (MainSettings,
                                            "cddb-manual-search-port");
    cddb_server_cgi_path = g_settings_get_string (MainSettings,
                                                  "cddb-manual-search-path");

    /* Build request */
    cddb_in = g_strdup_printf ("http://%s:%u/freedb_search.php?words=%s%s&grouping=none",
                               cddb_server_name,
                               cddb_server_port,
                               string,
                               (tmp = Cddb_Generate_Request_String_With_Fields_And_Categories_Options (self)));

    message = soup_message_new (SOUP_METHOD_GET, cddb_in);

    g_free(string);
    g_free(tmp);
    g_free (cddb_in);
    //g_print("Request Cddb_Search_Album_List_From_String_Freedb : '%s'\n", cddb_in);

    /* Send the request. */
    cancellable = g_cancellable_new ();
    istream = soup_session_send (priv->session, message, cancellable, &error);
    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,_("Sending request…"));
    while (gtk_events_pending()) gtk_main_iteration();

    if (!check_message_successful (message))
    {
        msg = log_message_from_request_error (message, error);
        gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                            priv->status_bar_context, msg);
        Log_Print (LOG_ERROR, "%s", msg);
        g_error_free (error);
        g_object_unref (message);
        g_free(cddb_server_name);
        g_free(cddb_server_cgi_path);
        return FALSE;
    }

    /* Delete previous album list. */
    cddb_album_model_clear (self);
    cddb_track_model_clear (self);

    if (priv->album_list)
    {
        Cddb_Free_Album_List (self);
    }
    gtk_widget_set_sensitive (GTK_WIDGET (priv->stop_search_button), TRUE);


    /*
     * Read the answer
     */
    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,_("Receiving data…"));
    while (gtk_events_pending())
        gtk_main_iteration();

    /* Parse server answer : Check returned code in the first line. */
    dstream = g_data_input_stream_new (istream);
    g_object_unref (istream);
    g_data_input_stream_set_newline_type (dstream,
                                          G_DATA_STREAM_NEWLINE_TYPE_ANY);

    // Read other lines, and get list of matching albums
    // Composition of a line :
    //  - freedb.org
    // <a href="http://www.freedb.org/freedb_search_fmt.php?cat=rock&id=8c0f0a0b">Bob Dylan / MTV Unplugged</a><br>
    cat_str      = g_strdup("http://www.freedb.org/freedb_search_fmt.php?cat=");
    id_str       = g_strdup("&id=");
    art_alb_str  = g_strdup("\">");
    end_str      = g_strdup("</a>"); //"</a><br>");
    html_end_str = g_strdup("</body>"); // To avoid the cddb lookups to hang
    while (!priv->stop_searching && read_cddb_result_line (dstream,
                                                           cancellable,
                                                           &cddb_out))
    {
        cddb_out_tmp = cddb_out;
        //g_print("%s\n",cddb_out); // To print received data

        // If the web search is disabled! (ex : http://www.freedb.org/modules.php?name=News&file=article&sid=246)
        // The following string is displayed in the search page
        if (cddb_out != NULL && strstr(cddb_out_tmp,"Sorry, The web-based search is currently down.") != NULL)
        {
            web_search_disabled = TRUE;
            break;
        }

        // We may have severals album in the same line (other version of the same album?)
        // Note : we test that the 'end' delimiter exists to avoid crashes
        while ( cddb_out != NULL && (ptr_cat=strstr(cddb_out_tmp,cat_str)) != NULL && strstr(cddb_out_tmp,end_str) != NULL )
        {
            gchar *ptr_font, *ptr_font1;
            gchar *ptr_id, *ptr_art_alb, *ptr_end;
            gchar *copy;
            CddbAlbum *cddbalbum;

            cddbalbum = g_slice_new0 (CddbAlbum);


            // Parameters of the server used
            cddbalbum->server_name     = g_strdup(cddb_server_name);
            cddbalbum->server_port     = cddb_server_port;
            cddbalbum->server_cgi_path = g_strdup(cddb_server_cgi_path);
            cddbalbum->bitmap          = Cddb_Get_Pixbuf_From_Server_Name(cddbalbum->server_name);

            // Get album category
            cddb_out_tmp = ptr_cat + strlen(cat_str);
            strncpy(buffer,cddb_out_tmp,MAX_STRING_LEN);
            if ( (ptr_id=strstr(buffer,id_str)) != NULL )
                *ptr_id = 0;
            cddbalbum->category = Try_To_Validate_Utf8_String(buffer);


            // Get album ID
            //cddb_out_tmp = strstr(cddb_out_tmp,id_str) + strlen(id_str);
            cddb_out_tmp = ptr_cat + strlen(cat_str) + 2;
            strncpy(buffer,cddb_out_tmp,MAX_STRING_LEN);
            if ( (ptr_art_alb=strstr(buffer,art_alb_str)) != NULL )
                *ptr_art_alb = 0;
            cddbalbum->id = Try_To_Validate_Utf8_String(buffer);


            // Get album and artist names.
            // Note : some names can be like this "<font size=-1>2</font>" (for other version of the same album)
            cddb_out_tmp = strstr(cddb_out_tmp,art_alb_str) + strlen(art_alb_str);
            strncpy(buffer,cddb_out_tmp,MAX_STRING_LEN);
            if ( (ptr_end=strstr(buffer,end_str)) != NULL )
                *ptr_end = 0;
            if ( (ptr_font=strstr(buffer,"</font>")) != NULL )
            {
                copy = NULL;
                *ptr_font = 0;
                if ( (ptr_font1=strstr(buffer,">")) != NULL )
                {
                    copy = g_strdup_printf("%s -> %s",ptr_font1+1,art_alb_tmp);
                    cddbalbum->other_version = TRUE;
                }else
                {
                    copy = g_strdup(buffer);
                }

            }else
            {
                copy = g_strdup(buffer);
                art_alb_tmp = cddbalbum->artist_album;
                use_art_alb = TRUE;
            }

            cddbalbum->artist_album = Try_To_Validate_Utf8_String(copy);
            g_free(copy);

            if (use_art_alb)
            {
                art_alb_tmp = cddbalbum->artist_album;
                use_art_alb = FALSE;
            }


            // New position the search the next string
            cddb_out_tmp = strstr(cddb_out_tmp,end_str) + strlen(end_str);

            priv->album_list = g_list_append(priv->album_list,cddbalbum);
        }

        // To avoid the cddb lookups to hang (Patch from Paul Giordano)
        /* It appears that on some systems that cddb lookups continue to attempt
         * to get data from the socket even though the other system has completed
         * sending. Here we see if the actual end of data is in the last block read.
         * In the case of the html scan, the </body> tag is used because there's
         * no crlf followint the </html> tag.
         */
        if (strstr(cddb_out_tmp,html_end_str)!=NULL)
        {
            g_free(cddb_out);
            break;
        }
        g_free(cddb_out);
    }
    g_free(cat_str); g_free(id_str); g_free(art_alb_str); g_free(end_str); g_free(html_end_str);
    g_free(cddb_server_name);
    g_free(cddb_server_cgi_path);

    gtk_widget_set_sensitive(GTK_WIDGET(priv->stop_search_button),FALSE);

    /* Close connection. */
    g_object_unref (dstream);
    g_object_unref (cancellable);
    g_object_unref (message);

    if (web_search_disabled)
        msg = g_strdup_printf(_("Sorry, the web-based search is currently not available"));
    else
    {
        msg = g_strdup_printf (ngettext ("Found one matching album",
                                         "Found %u matching albums",
                                         g_list_length (priv->album_list)),
                               g_list_length (priv->album_list));
    }

    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
    g_free(msg);

    /* Load the albums found in the list. */
    Cddb_Load_Album_List (self, FALSE);

    return TRUE;
}

/*
 * Site GNUDB.ORG - Manual Search
 * Send request (using the HTML search page in freedb.org site) to the CD database
 * to get the list of albums matching to a string.
 */
static gboolean
Cddb_Search_Album_List_From_String_Gnudb (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    gchar *string = NULL;
    gchar *tmp, *tmp1;
    gchar *cddb_out = NULL; // Answer received
    gchar *cddb_out_tmp;
    gchar *msg;
    gchar *cddb_server_name;
    guint cddb_server_port;
    gchar *cddb_server_cgi_path;

    gchar *ptr_cat, *cat_str, *art_alb_str;
    gchar *end_str;
    gchar *ptr_sraf, *sraf_str, *sraf_end_str;
    gchar *html_end_str;
    gchar  buffer[MAX_STRING_LEN+1];
    gint   num_albums = 0;
    gint   total_num_albums = 0;

    gint   next_page_cpt = 0;
    gboolean next_page_found;

    priv = et_cddb_dialog_get_instance_private (self);

    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,"");

    /* Get words to search... */
    string = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->search_entry)));
    if (et_str_empty (string))
    {
        return FALSE;
    }

    /* Format the string of words */
    g_strstrip (string);
    /* Remove the duplicated spaces */
    while ((tmp=strstr(string,"  "))!=NULL) // Search 2 spaces
    {
        tmp1 = tmp + 1;
        while (*tmp1)
            *(tmp++) = *(tmp1++);
        *tmp = '\0';
    }

    /* Convert spaces to '+' */
    while ( (tmp=strchr(string,' '))!=NULL )
        *tmp = '+';

    /* Delete previous album list. */
    cddb_album_model_clear (self);
    cddb_track_model_clear (self);

    if (priv->album_list)
    {
        Cddb_Free_Album_List (self);
    }
    gtk_widget_set_sensitive(GTK_WIDGET(priv->stop_search_button),TRUE);


    // Do a loop to load all the pages of results
    do
    {
        gchar *uri;
        SoupMessage *message;
        GCancellable *cancellable;
        GError *error = NULL;
        GInputStream *istream;
        GDataInputStream *dstream;
        gchar *next_page;

        cddb_server_name = g_settings_get_string (MainSettings,
                                                  "cddb-manual-search-hostname");
        cddb_server_port = g_settings_get_uint (MainSettings,
                                                "cddb-manual-search-port");
        cddb_server_cgi_path = g_settings_get_string (MainSettings,
                                                      "cddb-manual-search-path");

        /* Build request */
        uri = g_strdup_printf ("http://%s:%u/search/%s?page=%d",
                               cddb_server_name, cddb_server_port, string,
                               next_page_cpt);
        next_page_found = FALSE;

        message = soup_message_new (SOUP_METHOD_GET, uri);

        if (!message)
        {
            g_critical ("Invalid CDDB_request URI: %s", uri);
            g_free (uri);
            return FALSE;
        }

        g_free (uri);

        /* Send the request. */
        gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                            priv->status_bar_context, _("Sending request…"));
        while (gtk_events_pending()) gtk_main_iteration();

        cancellable = g_cancellable_new ();
        istream = soup_session_send (priv->session, message, cancellable,
                                     &error);

        if (!check_message_successful (message))
        {
            msg = log_message_from_request_error (message, error);
            gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                                priv->status_bar_context, msg);
            Log_Print (LOG_ERROR, "%s", msg);
            g_free (msg);
            g_object_unref (cancellable);
            g_object_unref (message);
            g_error_free (error);
            g_free (string);
            g_free (cddb_server_name);
            g_free (cddb_server_cgi_path);
            gtk_widget_set_sensitive (GTK_WIDGET (priv->stop_search_button),
                                      FALSE);
            return FALSE;
        }


        /*
         * Read the answer
         */
        if (total_num_albums != 0)
            msg = g_strdup_printf(_("Receiving data of page %d (album %d/%d)…"),next_page_cpt,num_albums,total_num_albums);
        else
            msg = g_strdup_printf(_("Receiving data of page %d…"),next_page_cpt);

        gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
        g_free(msg);
        while (gtk_events_pending())
            gtk_main_iteration();

        /* Parse server answer : Check returned code in the first line. */
        dstream = g_data_input_stream_new (istream);
        g_object_unref (istream);
        g_data_input_stream_set_newline_type (dstream,
                                              G_DATA_STREAM_NEWLINE_TYPE_ANY);

        if (!read_cddb_result_line (dstream, cancellable, &cddb_out)
            || !cddb_out)
        {
            msg = g_strdup (_("The server returned a bad response"));
            gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                                priv->status_bar_context, msg);
            Log_Print (LOG_ERROR, "%s", msg);
            g_object_unref (dstream);
            g_object_unref (cancellable);
            g_object_unref (message);
            g_free(msg);
            g_free(cddb_out);
            g_free(string);
            g_free(cddb_server_name);
            g_free(cddb_server_cgi_path);
            gtk_widget_set_sensitive(GTK_WIDGET(priv->stop_search_button),FALSE);
            return FALSE;
        }
        g_free(cddb_out);

        // The next page if exists will contains this url :
        next_page = g_strdup_printf("?page=%d",++next_page_cpt);

        // Read other lines, and get list of matching albums
        // Composition of a line :
        //  - gnudb.org
        // <a href="http://www.gnudb.org/cd/ro21123813"><b>Indochine / Le Birthday Album</b></a><br>
        cat_str      = g_strdup("http://www.gnudb.org/cd/");
        art_alb_str  = g_strdup("\"><b>");
        end_str      = g_strdup("</b></a>"); //"</a><br>");
        html_end_str = g_strdup("</body>"); // To avoid the cddb lookups to hang
        // Composition of a line displaying the number of albums
        // <h2>Search Results, 3486 albums found:</h2>
        sraf_str     = g_strdup("<h2>Search Results, ");
        sraf_end_str = g_strdup(" albums found:</h2>");

        while (!priv->stop_searching && read_cddb_result_line (dstream,
                                                               cancellable,
                                                               &cddb_out))
        {
            cddb_out_tmp = cddb_out;
            //g_print("%s\n",cddb_out); // To print received data

            // Line that displays the number of total albums return by the search
            if ( cddb_out != NULL
            && total_num_albums == 0 // Do it only the first time
            && (ptr_sraf=strstr(cddb_out_tmp,sraf_end_str)) != NULL
            && strstr(cddb_out_tmp,sraf_str) != NULL )
            {
                // Get total number of albums
                ptr_sraf = 0;
                total_num_albums = atoi(cddb_out_tmp + strlen(sraf_str));
            }

            // For GNUDB.ORG : one album per line
            if ( cddb_out != NULL
            && (ptr_cat=strstr(cddb_out_tmp,cat_str)) != NULL
            && strstr(cddb_out_tmp,end_str) != NULL )
            {
                gchar *ptr_art_alb, *ptr_end;
                gchar *valid;
                CddbAlbum *cddbalbum;

                cddbalbum = g_slice_new0 (CddbAlbum);

                // Parameters of the server used
                cddbalbum->server_name     = g_strdup(cddb_server_name);
                cddbalbum->server_port     = cddb_server_port;
                cddbalbum->server_cgi_path = g_strdup(cddb_server_cgi_path);
                cddbalbum->bitmap          = Cddb_Get_Pixbuf_From_Server_Name(cddbalbum->server_name);

                num_albums++;

                // Get album category
                cddb_out_tmp = ptr_cat + strlen(cat_str);
                strncpy(buffer,cddb_out_tmp,MAX_STRING_LEN);
                *(buffer+2) = 0;

                // Check only the 2 first characters to set the right category
                if ( strncmp(buffer,"blues",2)==0 )
                    valid = g_strdup("blues");
                else if ( strncmp(buffer,"classical",2)==0 )
                    valid = g_strdup("classical");
                else if ( strncmp(buffer,"country",2)==0 )
                    valid = g_strdup("country");
                else if ( strncmp(buffer,"data",2)==0 )
                    valid = g_strdup("data");
                else if ( strncmp(buffer,"folk",2)==0 )
                    valid = g_strdup("folk");
                else if ( strncmp(buffer,"jazz",2)==0 )
                    valid = g_strdup("jazz");
                else if ( strncmp(buffer,"misc",2)==0 )
                    valid = g_strdup("misc");
                else if ( strncmp(buffer,"newage",2)==0 )
                    valid = g_strdup("newage");
                else if ( strncmp(buffer,"reggae",2)==0 )
                    valid = g_strdup("reggae");
                else if ( strncmp(buffer,"rock",2)==0 )
                    valid = g_strdup("rock");
                else //if ( strncmp(buffer,"soundtrack",2)==0 )
                    valid = g_strdup("soundtrack");

                cddbalbum->category = valid; //Not useful -> Try_To_Validate_Utf8_String(valid);


                // Get album ID
                cddb_out_tmp = ptr_cat + strlen(cat_str) + 2;
                strncpy(buffer,cddb_out_tmp,MAX_STRING_LEN);
                if ( (ptr_art_alb=strstr(buffer,art_alb_str)) != NULL )
                    *ptr_art_alb = 0;
                cddbalbum->id = Try_To_Validate_Utf8_String(buffer);


                // Get album and artist names.
                cddb_out_tmp = strstr(cddb_out_tmp,art_alb_str) + strlen(art_alb_str);
                strncpy(buffer,cddb_out_tmp,MAX_STRING_LEN);
                if ( (ptr_end=strstr(buffer,end_str)) != NULL )
                    *ptr_end = 0;
                cddbalbum->artist_album = Try_To_Validate_Utf8_String(buffer);

                priv->album_list = g_list_append(priv->album_list,cddbalbum);
            }

            // To avoid the cddb lookups to hang (Patch from Paul Giordano)
            /* It appears that on some systems that cddb lookups continue to attempt
             * to get data from the socket even though the other system has completed
             * sending. Here we see if the actual end of data is in the last block read.
             * In the case of the html scan, the </body> tag is used because there's
             * no crlf followint the </html> tag.
             */
            /***if (strstr(cddb_out_tmp,html_end_str)!=NULL)
                break;***/


            // Check if the link to the next results exists to loop again with the next link
            if (cddb_out != NULL && next_page != NULL
            && (strstr(cddb_out_tmp,next_page) != NULL || next_page_cpt < 2) ) // BUG : "next_page_cpt < 2" to fix a bug in gnudb : the page 0 doesn't contain link to the page=1, so we force it...
            {
                next_page_found = TRUE;

                if ( !(next_page_cpt < 2) ) // Don't display message in this case as it will be displayed each line of page 0 and 1
                {
                    msg = g_strdup_printf(_("More results to load…"));
                    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
                    g_free(msg);

                    while (gtk_events_pending())
                        gtk_main_iteration();
                }
            }

            g_free(cddb_out);
        }

        g_free(cat_str); g_free(art_alb_str); g_free(end_str); g_free(html_end_str);
        g_free(sraf_str);g_free(sraf_end_str);
        g_free(cddb_server_name);
        g_free(cddb_server_cgi_path);
        g_free (next_page);

        /* Close connection. */
        g_object_unref (dstream);
        g_object_unref (cancellable);
        g_object_unref (message);
    } while (next_page_found);
    g_free(string);


    gtk_widget_set_sensitive(GTK_WIDGET(priv->stop_search_button),FALSE);

    msg = g_strdup_printf(ngettext("Found one matching album","Found %d matching albums",num_albums),num_albums);
    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
    g_free(msg);

    /* Load the albums found in the list. */
    Cddb_Load_Album_List (self, FALSE);

    return TRUE;
}

/*
 * Select the function to use according the server adress for the manual search
 *      - freedb.freedb.org
 *      - gnudb.gnudb.org
 */
static gboolean
Cddb_Search_Album_List_From_String (EtCDDBDialog *self)
{
    gchar *hostname = g_settings_get_string (MainSettings,
                                             "cddb-manual-search-hostname");

    if (strstr (hostname, "gnudb") != NULL)
    {
        /* Use gnudb. */
        g_free (hostname);
        return Cddb_Search_Album_List_From_String_Gnudb (self);
    }
    else
    {
        /* Use freedb. */
        g_free (hostname);
        return Cddb_Search_Album_List_From_String_Freedb (self);
    }
}

/*
 * set_et_file_from_cddb_album:
 * @etfile: an ET_File on which to set values
 * @cddbtrackalbum: a CddbTrackAlbum from which to take values
 * @set_fields: flags value for the fields to set
 * @list_length: length of the CDDB album data (in tracks)
 *
 * Set the values obtained from CDDB and stored in @cddbtrackalbum to the
 * given @etfile.
 */
static void
set_et_file_from_cddb_album (ET_File * etfile,
                             CddbTrackAlbum *cddbtrackalbum,
                             guint set_fields,
                             guint list_length)
{
    File_Tag *FileTag = NULL;
    File_Name *FileName = NULL;

    g_return_if_fail (etfile != NULL);
    g_return_if_fail (cddbtrackalbum != NULL);

    if (set_fields != 0)
    {
        /* Allocation of a new FileTag. */
        FileTag = et_file_tag_new ();
        et_file_tag_copy_into (FileTag, etfile->FileTag->data);

        if (set_fields & ET_CDDB_SET_FIELD_TITLE)
        {
            et_file_tag_set_title (FileTag,
                                   cddbtrackalbum->track_name);
        }

        if ((set_fields & ET_CDDB_SET_FIELD_ARTIST)
            && cddbtrackalbum->cddbalbum->artist)
        {
            et_file_tag_set_artist (FileTag,
                                    cddbtrackalbum->cddbalbum->artist);
        }

        if ((set_fields & ET_CDDB_SET_FIELD_ALBUM)
            && cddbtrackalbum->cddbalbum->album)
        {
            et_file_tag_set_album (FileTag,
                                   cddbtrackalbum->cddbalbum->album);
        }

        if ((set_fields & ET_CDDB_SET_FIELD_YEAR)
            && cddbtrackalbum->cddbalbum->year)
        {
            et_file_tag_set_year (FileTag,
                                  cddbtrackalbum->cddbalbum->year);
        }

        if (set_fields & ET_CDDB_SET_FIELD_TRACK)
        {
            gchar *track_number;

            track_number = et_track_number_to_string (cddbtrackalbum->track_number);

            et_file_tag_set_track_number (FileTag, track_number);

            g_free (track_number);
        }

        if (set_fields & ET_CDDB_SET_FIELD_TRACK_TOTAL)
        {
            gchar *track_total;

            track_total = et_track_number_to_string (list_length);

            et_file_tag_set_track_total (FileTag, track_total);

            g_free (track_total);
        }

        if ((set_fields & ET_CDDB_SET_FIELD_GENRE)
            && (cddbtrackalbum->cddbalbum->genre
                || cddbtrackalbum->cddbalbum->category))
        {
            if (!et_str_empty (cddbtrackalbum->cddbalbum->genre))
            {
                et_file_tag_set_genre (FileTag,
                                       Cddb_Get_Id3_Genre_From_Cddb_Genre (cddbtrackalbum->cddbalbum->genre));
            }
            else
            {
                et_file_tag_set_genre (FileTag,
                                       Cddb_Get_Id3_Genre_From_Cddb_Genre (cddbtrackalbum->cddbalbum->category));
            }
        }
    }

    /* Filename field. */
    if (set_fields & ET_CDDB_SET_FIELD_FILENAME)
    {
        gchar *track_number;
        gchar *filename_generated_utf8;
        gchar *filename_new_utf8;

        /* Allocation of a new FileName. */
        FileName = et_file_name_new ();

        /* Build the filename with the path. */
        track_number = et_track_number_to_string (cddbtrackalbum->track_number);

        filename_generated_utf8 = g_strconcat (track_number, " - ",
                                               cddbtrackalbum->track_name,
                                               NULL);
        et_filename_prepare (filename_generated_utf8,
                             g_settings_get_boolean (MainSettings,
                                                     "rename-replace-illegal-chars"));
        filename_new_utf8 = et_file_generate_name (etfile,
                                                   filename_generated_utf8);

        ET_Set_Filename_File_Name_Item(FileName,filename_new_utf8,NULL);

        g_free (track_number);
        g_free(filename_generated_utf8);
        g_free(filename_new_utf8);
    }

    ET_Manage_Changes_Of_File_Data (etfile, FileName, FileTag);

    /* Then run current scanner if requested. */
    if (g_settings_get_boolean (MainSettings, "cddb-run-scanner"))
    {
        EtScanDialog *dialog;

        dialog = ET_SCAN_DIALOG (et_application_window_get_scan_dialog (ET_APPLICATION_WINDOW (MainWindow)));

        if (dialog)
        {
            Scan_Select_Mode_And_Run_Scanner (dialog, etfile);
        }
    }
}
/*
 * Set CDDB data (from tracks list) into tags of the main file list
 */
static gboolean
Cddb_Set_Track_Infos_To_File_List (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    guint row;
    guint list_length;
    guint rows_to_loop = 0;
    guint selectedcount;
    guint file_selectedcount;
    guint counter = 0;
    GList *file_iterlist = NULL;
    GList *file_selectedrows;
    GList *selectedrows = NULL;
    gboolean CddbTrackList_Line_Selected;
    CddbTrackAlbum *cddbtrackalbum = NULL;
    GtkTreeSelection *selection = NULL;
    GtkTreeSelection *file_selection = NULL;
    GtkListStore *fileListModel;
    GtkTreePath *currentPath = NULL;
    GtkTreeIter  currentIter;
    GtkTreeIter *fileIter;
    gpointer iterptr;

    g_return_val_if_fail (ETCore->ETFileDisplayedList != NULL, FALSE);

    priv = et_cddb_dialog_get_instance_private (self);

    et_application_window_update_et_file_from_ui (ET_APPLICATION_WINDOW (MainWindow));

    /* FIXME: Hack! */
    file_selection = et_application_window_browser_get_selection (ET_APPLICATION_WINDOW (MainWindow));
    fileListModel = GTK_LIST_STORE (gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (file_selection)));
    list_length = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->track_list_model), NULL);

    // Take the selected files in the cddb track list, else the full list
    // Note : Just used to calculate "cddb_track_list_length" because
    // "GPOINTER_TO_INT(cddb_track_list->data)" doesn't return the number of the
    // line when "cddb_track_list = g_list_first(GTK_CLIST(CddbTrackCList)->row_list)"
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->track_list_view));
    selectedcount = gtk_tree_selection_count_selected_rows(selection);

    /* Check if at least one line was selected. No line selected is equal to all lines selected. */
    if (selectedcount > 0)
    {
        /* Loop through selected rows only */
        CddbTrackList_Line_Selected = TRUE;
        rows_to_loop = selectedcount;
        selectedrows = gtk_tree_selection_get_selected_rows(selection, NULL);
    } else
    {
        /* Loop through all rows */
        CddbTrackList_Line_Selected = FALSE;
        rows_to_loop = list_length;
    }

    file_selectedcount = gtk_tree_selection_count_selected_rows (file_selection);

    if (file_selectedcount > 0)
    {
        GList *l;

        /* Rows are selected in the file list, apply tags to them only */
        file_selectedrows = gtk_tree_selection_get_selected_rows(file_selection, NULL);

        for (l = file_selectedrows; l != NULL; l = g_list_next (l))
        {
            counter++;
            iterptr = g_malloc0(sizeof(GtkTreeIter));
            if (gtk_tree_model_get_iter (GTK_TREE_MODEL (fileListModel),
                                         (GtkTreeIter *)iterptr,
                                         (GtkTreePath *)l->data))
            {
                file_iterlist = g_list_prepend (file_iterlist, iterptr);
            }

            if (counter == rows_to_loop) break;
        }

        /* Free the useless bit */
        g_list_free_full (file_selectedrows,
                          (GDestroyNotify)gtk_tree_path_free);

    } else /* No rows selected, use the first x items in the list */
    {
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(fileListModel), &currentIter);

        do
        {
            counter++;
            iterptr = g_memdup(&currentIter, sizeof(GtkTreeIter));
            file_iterlist = g_list_prepend (file_iterlist, iterptr);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(fileListModel), &currentIter));

        file_selectedcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(fileListModel), NULL);
    }

    if (file_selectedcount != rows_to_loop)
    {
        GtkWidget *msgdialog;
        gint response;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(self),
                                           GTK_DIALOG_MODAL  | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_QUESTION,
                                           GTK_BUTTONS_NONE,
                                           "%s",
                                           _("The number of CDDB results does not match the number of selected files"));
        gtk_dialog_add_buttons (GTK_DIALOG (msgdialog), _("_Cancel"),
                                GTK_RESPONSE_CANCEL, _("_Apply"),
                                GTK_RESPONSE_APPLY, NULL);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),"%s","Do you want to continue?");
        gtk_window_set_title (GTK_WINDOW (msgdialog),
                              _("Write Tag from CDDB"));
        response = gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);

        if (response != GTK_RESPONSE_APPLY)
        {
            g_list_free_full (file_iterlist, (GDestroyNotify)g_free);
            //gdk_window_raise(CddbWindow->window);
            return FALSE;
        }
    }

    file_iterlist = g_list_reverse (file_iterlist);
    //ET_Debug_Print_File_List (NULL, __FILE__, __LINE__, __FUNCTION__);

    for (row=0; row < rows_to_loop; row++)
    {
        if (CddbTrackList_Line_Selected == FALSE)
        {
            if(row == 0)
                currentPath = gtk_tree_path_new_first();
            else
                gtk_tree_path_next(currentPath);
        } else /* (e.g.: if CddbTrackList_Line_Selected == TRUE) */
        {
            if(row == 0)
            {
                currentPath = (GtkTreePath *)selectedrows->data;
            } else
            {
                selectedrows = g_list_next(selectedrows);
                currentPath = (GtkTreePath *)selectedrows->data;
            }
        }

        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->track_list_model),
                                     &currentIter, currentPath))
        {
            gtk_tree_model_get (GTK_TREE_MODEL (priv->track_list_model),
                                &currentIter, CDDB_TRACK_LIST_DATA,
                                &cddbtrackalbum, -1);
        }
        else
        {
            g_warning ("Iter not found matching path in CDDB track list model");
        }

        /* Set values in the ETFile. */
        if (g_settings_get_boolean (MainSettings, "cddb-dlm-enabled"))
        {
            ET_File *etfile = NULL;
            guint set_fields;

            gtk_tree_model_get (GTK_TREE_MODEL (priv->track_list_model),
                                &currentIter, CDDB_TRACK_LIST_ETFILE, &etfile,
                                -1);

            /* If the row in the model does not already have an ET_File
             * associated with it, take one from the browser selection. */
            if (!etfile)
            {
                fileIter = (GtkTreeIter*) file_iterlist->data;
                etfile = et_application_window_browser_get_et_file_from_iter (ET_APPLICATION_WINDOW (MainWindow),
                                                                              fileIter);
            }

            /* Tag fields. */
            set_fields = g_settings_get_flags (MainSettings, "cddb-set-fields");

            set_et_file_from_cddb_album (etfile, cddbtrackalbum, set_fields,
                                         list_length);
        }
        else if (cddbtrackalbum)
        {
            ET_File *etfile;
            guint set_fields;

            fileIter = (GtkTreeIter*) file_iterlist->data;
            etfile = et_application_window_browser_get_et_file_from_iter (ET_APPLICATION_WINDOW (MainWindow),
                                                                          fileIter);

            /* Tag fields. */
            set_fields = g_settings_get_flags (MainSettings, "cddb-set-fields");

            set_et_file_from_cddb_album (etfile, cddbtrackalbum, set_fields,
                                         list_length);
        }

        if(!file_iterlist->next) break;
        file_iterlist = file_iterlist->next;
    }

    g_list_free_full (g_list_first (file_iterlist), (GDestroyNotify)g_free);
    g_list_free_full (g_list_first (selectedrows),
                      (GDestroyNotify)gtk_tree_path_free);

    et_application_window_browser_refresh_list (ET_APPLICATION_WINDOW (MainWindow));
    et_application_window_display_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                           ETCore->ETFileDisplayed);

    return TRUE;
}

static void
stop_search (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;

    priv = et_cddb_dialog_get_instance_private (self);

    priv->stop_searching = TRUE;
}

/*
 * Unselect all rows in the track list
 */
static void
track_list_unselect_all (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    GtkTreeSelection *selection;

    priv = et_cddb_dialog_get_instance_private (self);

    g_return_if_fail (priv->track_list_view != NULL);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->track_list_view));
    if (selection)
    {
        gtk_tree_selection_unselect_all (selection);
    }
}

/*
 * Select all rows in the track list
 */
static void
track_list_select_all (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    GtkTreeSelection *selection;

    priv = et_cddb_dialog_get_instance_private (self);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->track_list_view));

    if (selection)
    {
        gtk_tree_selection_select_all (selection);
    }
}

static gboolean
on_track_list_button_press_event (EtCDDBDialog *self,
                                  GdkEventButton *event,
                                  GtkTreeView *track_list_view)
{
    if (event->type == GDK_2BUTTON_PRESS
        && event->button == GDK_BUTTON_PRIMARY)
    {
        GdkWindow *bin_window;

        bin_window = gtk_tree_view_get_bin_window (track_list_view);

        if (bin_window != event->window)
        {
            /* Ignore the event if it is on the header (which is not the bin
             * window. */
            return GDK_EVENT_PROPAGATE;
        }

        /* Double left mouse click */
        track_list_select_all (self);

        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

static void
et_cddb_dialog_on_response (EtCDDBDialog *self,
                            gint response_id,
                            gpointer user_data)
{
    switch (response_id)
    {
        case GTK_RESPONSE_CLOSE:
            gtk_widget_hide (GTK_WIDGET (self));
            break;
        case GTK_RESPONSE_DELETE_EVENT:
            break;
        default:
            g_assert_not_reached ();
            break;
    }
}

static void
Cddb_Destroy_Window (EtCDDBDialog *self)
{
    et_cddb_dialog_on_response (self, GTK_RESPONSE_CLOSE, NULL);
}

static void
init_search_field_check (GtkWidget *widget)
{
    g_object_set_data (G_OBJECT (widget), "flags-type",
                       GSIZE_TO_POINTER (ET_TYPE_CDDB_SEARCH_FIELD));
    g_object_set_data (G_OBJECT (widget), "flags-key",
                       (gpointer) "cddb-search-fields");
    g_settings_bind_with_mapping (MainSettings, "cddb-search-fields", widget,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_flags_toggle_get,
                                  et_settings_flags_toggle_set, widget, NULL);
}
static void
init_search_category_check (GtkWidget *widget)
{
    g_object_set_data (G_OBJECT (widget), "flags-type",
                       GSIZE_TO_POINTER (ET_TYPE_CDDB_SEARCH_CATEGORY));
    g_object_set_data (G_OBJECT (widget), "flags-key",
                       (gpointer) "cddb-search-categories");
    g_settings_bind_with_mapping (MainSettings, "cddb-search-categories",
                                  widget, "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_flags_toggle_get,
                                  et_settings_flags_toggle_set, widget, NULL);
}

static void
init_set_field_check (GtkWidget *widget)
{
    g_object_set_data (G_OBJECT (widget), "flags-type",
                       GSIZE_TO_POINTER (ET_TYPE_CDDB_SET_FIELD));
    g_object_set_data (G_OBJECT (widget), "flags-key",
                       (gpointer) "cddb-set-fields");
    g_settings_bind_with_mapping (MainSettings, "cddb-set-fields", widget,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_flags_toggle_get,
                                  et_settings_flags_toggle_set, widget, NULL);
}

static void
create_cddb_dialog (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;
    GtkTreePath *path;

    priv = et_cddb_dialog_get_instance_private (self);

    /* Button to generate CddbId and request string from the selected files. */
    gtk_widget_grab_default (priv->automatic_search_button);

    /* Set content of the clipboard if available. */
    gtk_editable_paste_clipboard (GTK_EDITABLE (priv->search_entry));

    /* Button to run the search. */
    gtk_widget_grab_default (priv->manual_search_button);

    /* Search options. */
    init_search_field_check (priv->artist_check);
    init_search_field_check (priv->album_check);
    init_search_field_check (priv->track_check);
    init_search_field_check (priv->other_check);

    init_search_category_check (priv->blues_check);
    init_search_category_check (priv->classical_check);
    init_search_category_check (priv->country_check);
    init_search_category_check (priv->folk_check);
    init_search_category_check (priv->jazz_check);
    init_search_category_check (priv->misc_check);
    init_search_category_check (priv->newage_check);
    init_search_category_check (priv->reggae_check);
    init_search_category_check (priv->rock_check);
    init_search_category_check (priv->soundtrack_check);

    /* Result of search. */
    /* List of albums. */
    path = gtk_tree_path_new_first ();
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->album_list_view), path, NULL,
                              FALSE);
    gtk_tree_path_free (path);

    /* List of tracks. */
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->track_list_model),
                                     SORT_LIST_NUMBER,
                                     Cddb_Track_List_Sort_Func,
                                     GINT_TO_POINTER (SORT_LIST_NUMBER), NULL);
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->track_list_model),
                                     SORT_LIST_NAME, Cddb_Track_List_Sort_Func,
                                     GINT_TO_POINTER (SORT_LIST_NAME), NULL);

    /* Apply results to fields.  */
    init_set_field_check (priv->filename_check);
    init_set_field_check (priv->title_check);
    init_set_field_check (priv->fill_artist_check);
    init_set_field_check (priv->fill_album_check);
    init_set_field_check (priv->year_check);
    init_set_field_check (priv->fill_track_check);
    init_set_field_check (priv->track_total_check);
    init_set_field_check (priv->genre_check);

    /* Check box to run the scanner. */
    g_settings_bind (MainSettings, "cddb-run-scanner",
                     priv->scanner_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Check box to use DLM (also used in the preferences window). */
    g_settings_bind (MainSettings, "cddb-dlm-enabled", priv->dlm_check,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Status bar. */
    priv->status_bar_context = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->status_bar),
                                                             "Messages");
    gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                        priv->status_bar_context, _("Ready to search"));

    g_signal_emit_by_name (priv->search_entry, "changed");
    priv->stop_searching = FALSE;

    /* The User-Agent header is not used by the CDDB protocol over HTTP, but it
     * is still good practice to set it appropriately. */
    /* FIXME: Enable a SoupLogger with g_parse_debug_string(). */
    priv->session = soup_session_new_with_options (SOUP_SESSION_USER_AGENT,
                                                   PACKAGE_NAME " " PACKAGE_VERSION,
                                                   NULL);
}

/*
 * Sort the track list
 */
static gint
Cddb_Track_List_Sort_Func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
                           gpointer data)
{
    gint sortcol = GPOINTER_TO_INT(data);
    gchar *text1, *text1cp;
    gchar *text2, *text2cp;
    gint num1;
    gint num2;
    gint ret = 0;

    switch (sortcol)
    {
        case SORT_LIST_NUMBER:
            gtk_tree_model_get(model, a, CDDB_TRACK_LIST_NUMBER, &num1, -1);
            gtk_tree_model_get(model, b, CDDB_TRACK_LIST_NUMBER, &num2, -1);
            if (num1 < num2)
                return -1;
            else if(num1 > num2)
                return 1;
            else
                return 0;
            break;

        case SORT_LIST_NAME:
            gtk_tree_model_get(model, a, CDDB_TRACK_LIST_NAME, &text1, -1);
            gtk_tree_model_get(model, b, CDDB_TRACK_LIST_NAME, &text2, -1);
            text1cp = g_utf8_collate_key_for_filename(text1, -1);
            text2cp = g_utf8_collate_key_for_filename(text2, -1);
            // Must be the same rules as "ET_Comp_Func_Sort_File_By_Ascending_Filename" to be
            // able to sort in the same order files in cddb and in the file list.
            ret = g_settings_get_boolean (MainSettings,
                                          "sort-case-sensitive") ? strcmp (text1cp, text2cp)
                                                                 : strcasecmp (text1cp, text2cp);

            g_free(text1);
            g_free(text2);
            g_free(text1cp);
            g_free(text2cp);
            break;
        default:
            g_assert_not_reached ();
            break;
    }

    return ret;
}

static gboolean
Cddb_Free_Track_Album_List (GList *track_list)
{
    GList *l;

    g_return_val_if_fail (track_list != NULL, FALSE);

    track_list = g_list_first (track_list);

    for (l = track_list; l != NULL; l = g_list_next (l))
    {
        CddbTrackAlbum *cddbtrackalbum = l->data;

        if (cddbtrackalbum)
        {
            g_free(cddbtrackalbum->track_name);
            g_slice_free (CddbTrackAlbum, cddbtrackalbum);
        }
    }

    g_list_free (track_list);

    return TRUE;
}

/*
 * Send cddb query using the CddbId generated from the selected files to get the
 * list of albums matching with this cddbid.
 */
gboolean
et_cddb_dialog_search_from_selection (EtCDDBDialog *self)
{
    EtCDDBDialogPrivate *priv;

    gchar *cddb_out = NULL;       /* Answer received */
    gchar *msg;
    gchar *cddb_server_name;
    guint cddb_server_port;
    gchar *cddb_server_cgi_path;
    gint   server_try = 0;
    GString *query_string;
    gchar *cddb_discid;

    guint total_frames = 150;   /* First offset is (almost) always 150 */
    guint disc_length  = 2;     /* and 2s elapsed before first track */

    GtkTreeSelection *file_selection = NULL;
    guint n_files = 0;
    guint total_id;
    guint num_tracks;

    GList *filelist = NULL;
    GList *l;

    priv = et_cddb_dialog_get_instance_private (self);

    /* Number of selected files. */
    /* FIXME: Hack! */
    file_selection = et_application_window_browser_get_selection (ET_APPLICATION_WINDOW (MainWindow));
    n_files = gtk_tree_selection_count_selected_rows (file_selection);

    /* Either take the selected files, or use all files if no files are
     * selected. */
    if (n_files > 0)
    {
        filelist = et_application_window_browser_get_selected_files (ET_APPLICATION_WINDOW (MainWindow));
    }
    else /* No rows selected, use the whole list */
    {
        GtkTreeIter iter;
        GtkTreeModel *model;

        model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (file_selection));

        if (gtk_tree_model_get_iter_first (model, &iter))
        {
            do
            {
                ET_File *etfile;

                etfile = et_application_window_browser_get_et_file_from_iter (ET_APPLICATION_WINDOW (MainWindow), &iter);
                filelist = g_list_prepend (filelist, etfile);
                n_files++;
            } while (gtk_tree_model_iter_next (model, &iter));

            filelist = g_list_reverse (filelist);
        }
    }

    if (n_files == 0)
    {
        msg = g_strdup_printf(_("No file selected"));
        gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
        g_free(msg);
        return TRUE;
    }
    else if (n_files > 99)
    {
        // The CD redbook standard defines the maximum number of tracks as 99, any
        // queries with more than 99 tracks will never return a result.
        msg = g_strdup_printf(_("More than 99 files selected. Cannot send request"));
        gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
        g_free(msg);
        return FALSE;
    }else
    {
        msg = g_strdup_printf (ngettext ("One file selected",
                                         "%u files selected",
                                         n_files),
                               n_files);
        gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
        g_free(msg);
    }

    // Generate query string and compute discid from the list 'file_iterlist'
    total_id = 0;
    num_tracks = n_files;
    query_string = g_string_new ("");

    /* FIXME: Split this out to a separate function. */
    for (l = filelist; l != NULL; l = g_list_next (l))
    {
        const ET_File *etfile;
        gulong secs = 0;

        etfile = (const ET_File *)l->data;

        if (query_string->len > 0)
        {
            g_string_append_printf (query_string, "+%u", total_frames);
        }
        else
        {
            g_string_append_printf (query_string, "%u", total_frames);
        }

        secs = etfile->ETFileInfo->duration;
        total_frames += secs * 75;
        disc_length  += secs;

        while (secs > 0)
        {
            total_id = total_id + (secs % 10);
            secs = secs / 10;
        }
    }

    /* No need to free the contained ET_File *. */
    g_list_free (filelist);

    /* Compute CddbId. */
    cddb_discid = g_strdup_printf ("%08x", (guint)(((total_id % 0xFF) << 24) |
                                           (disc_length << 8) | num_tracks));


    /* Delete previous album list. */
    cddb_album_model_clear (self);
    cddb_track_model_clear (self);

    if (priv->album_list)
    {
        Cddb_Free_Album_List (self);
    }
    gtk_widget_set_sensitive(GTK_WIDGET(priv->stop_search_button),TRUE);


    {
        /*
         * Remote cddb acces
         *
         * Request the two servers
         *   - 1) www.freedb.org
         *   - 2) MusicBrainz Gateway : freedb.musicbrainz.org (in Easytag < 2.1.1, it was: www.mb.inhouse.co.uk)
         */
        while (server_try < 2)
        {
            gchar *uri;
            SoupMessage *message;
            GError *error = NULL;
            GCancellable *cancellable;
            GInputStream *istream;
            GDataInputStream *dstream;

            server_try++;

            if (server_try == 1)
            {
                /* 1st try. */
                cddb_server_name = g_settings_get_string (MainSettings,
                                                          "cddb-automatic-search-hostname");
                cddb_server_port = g_settings_get_uint (MainSettings,
                                                        "cddb-automatic-search-port");
                cddb_server_cgi_path = g_settings_get_string (MainSettings,
                                                              "cddb-automatic-search-path");
 
            }
            else
            {
                /* 2nd try. */
                cddb_server_name = g_settings_get_string (MainSettings,
                                                          "cddb-automatic-search-hostname2");
                cddb_server_port = g_settings_get_uint (MainSettings,
                                                        "cddb-automatic-search-port");
                cddb_server_cgi_path = g_settings_get_string (MainSettings,
                                                              "cddb-automatic-search-path2");
            }

            /* Check values. */
            if (et_str_empty (cddb_server_name))
            {
                g_free (cddb_server_name);
                g_free (cddb_server_cgi_path);
                continue;
            }

            // CDDB Request (ex: GET /~cddb/cddb.cgi?cmd=cddb+query+0800ac01+1++150+172&hello=noname+localhost+EasyTAG+0.31&proto=1 HTTP/1.1\r\nHost: freedb.freedb.org:80\r\nConnection: close)
            // proto=1 => ISO-8859-1 - proto=6 => UTF-8
            uri = g_strdup_printf ("http://%s:%u%s?cmd=cddb+query+%s+%u+%s+%u&hello=noname+localhost+%s+%s&proto=6",
                                   cddb_server_name, cddb_server_port,
                                   cddb_server_cgi_path, cddb_discid,
                                   num_tracks, query_string->str, disc_length,
                                   PACKAGE_NAME, PACKAGE_VERSION);
            message = soup_message_new (SOUP_METHOD_GET, uri);

            if (!message)
            {
                g_critical ("Invalid CDDB request URI: %s", uri);
                g_free (uri);
                return FALSE;
            }

            g_free (uri);

            msg = g_strdup_printf (_("Sending request (disc ID: %s, #tracks: %u, Disc length: %u)…"),
                                   cddb_discid, num_tracks, disc_length);
            gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
            g_free(msg);

            while (gtk_events_pending())
                gtk_main_iteration();

            cancellable = g_cancellable_new ();
            istream = soup_session_send (priv->session, message, cancellable,
                                         &error);

            if (!check_message_successful (message))
            {
                msg = log_message_from_request_error (message, error);
                gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                                    priv->status_bar_context, msg);
                Log_Print (LOG_ERROR, "%s", msg);
                g_free (msg);
                g_error_free (error);
                g_free (cddb_server_name);
                g_free (cddb_server_cgi_path);
                g_string_free (query_string, TRUE);
                g_free (cddb_discid);
                g_object_unref (cancellable);
                g_object_unref (message);
                gtk_widget_set_sensitive (GTK_WIDGET (priv->stop_search_button),
                                          FALSE);
                return FALSE;
            }

            /* Read the answer. */
            dstream = g_data_input_stream_new (istream);
            g_object_unref (istream);
            g_data_input_stream_set_newline_type (dstream,
                                                  G_DATA_STREAM_NEWLINE_TYPE_ANY);

            /*
             * Format :
             * For Freedb, Gnudb, the lines to read are like :
             *      211 Found inexact matches, list follows (until terminating `.')
             *      rock 8f0dc00b Archive / Noise
             *      rock 7b0dd80b Archive / Noise
             *      .
             * For MusicBrainz Cddb Gateway (see http://wiki.musicbrainz.org/CddbGateway), the lines to read are like :
             *      200 jazz 7e0a100a Pink Floyd / Dark Side of the Moon
             */
            while (!priv->stop_searching
                   && read_cddb_result_line (dstream, cancellable, &cddb_out))
            {
                const gchar *cddb_out_tmp = cddb_out;

                if (!cddb_out_tmp)
                {
                    break;
                }

                /* Compatibility for the MusicBrainz CddbGateway. */
                if (strlen (cddb_out_tmp) > 3
                    && (strncmp (cddb_out_tmp, "200", 3) == 0
                        || strncmp (cddb_out_tmp, "210", 3) == 0
                        || strncmp (cddb_out_tmp, "211", 3) == 0))
                {
                    cddb_out_tmp += 4;
                }

                // Reading of lines with albums (skiping return code lines :
                // "211 Found inexact matches, list follows (until terminating `.')" )
                if (strstr (cddb_out_tmp, "/") != NULL)
                {
                    gchar* ptr;
                    CddbAlbum *cddbalbum;

                    cddbalbum = g_slice_new0 (CddbAlbum);

                    // Parameters of the server used
                    cddbalbum->server_name     = g_strdup(cddb_server_name);
                    cddbalbum->server_port     = cddb_server_port;
                    cddbalbum->server_cgi_path = g_strdup(cddb_server_cgi_path);
                    cddbalbum->bitmap          = Cddb_Get_Pixbuf_From_Server_Name(cddbalbum->server_name);

                    // Get album category
                    if ( (ptr = strstr(cddb_out_tmp, " ")) != NULL )
                    {
                        *ptr = 0;
                        cddbalbum->category = Try_To_Validate_Utf8_String(cddb_out_tmp);
                        *ptr = ' ';
                        cddb_out_tmp = ptr + 1;
                    }

                    // Get album ID
                    if ( (ptr = strstr(cddb_out_tmp, " ")) != NULL )
                    {
                        *ptr = 0;
                        cddbalbum->id = Try_To_Validate_Utf8_String(cddb_out_tmp);
                        *ptr = ' ';
                        cddb_out_tmp = ptr + 1;
                    }

                    // Get album and artist names.
                    cddbalbum->artist_album = Try_To_Validate_Utf8_String(cddb_out_tmp);

                    priv->album_list = g_list_append(priv->album_list,cddbalbum);
                }

                /* There is no need to explicitly check for the terminating
                 * '.' */
                g_free(cddb_out);
            }

            g_free (cddb_out);
            g_free(cddb_server_name);
            g_free(cddb_server_cgi_path);
            g_object_unref (dstream);
            g_object_unref (cancellable);
            g_object_unref (message);
        }
    }

    g_string_free (query_string, TRUE);

    msg = g_strdup_printf (ngettext ("DiscID ‘%s’ gave one matching album",
                                     "DiscID ‘%s’ gave %u matching albums",
                                     g_list_length (priv->album_list)),
                           cddb_discid, g_list_length (priv->album_list));
    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
    g_free(msg);

    g_free(cddb_discid);

    gtk_widget_set_sensitive (GTK_WIDGET (priv->stop_search_button), FALSE);

    /* Load the albums found in the list. */
    Cddb_Load_Album_List (self, FALSE);

    return TRUE;
}

/*
 * Returns the corresponding ID3 genre (the name, not the value)
 */
static const gchar *
Cddb_Get_Id3_Genre_From_Cddb_Genre (const gchar *cddb_genre)
{
    gsize i;

    g_return_val_if_fail (cddb_genre != NULL, "");

    for (i = 0; i <= CDDB_GENRE_MAX; i++)
    {
        if (g_ascii_strcasecmp (cddb_genre, cddb_genre_vs_id3_genre[i][0]) == 0)
        {
            return cddb_genre_vs_id3_genre[i][1];
        }
    }

    return cddb_genre;
}

/*
 * Returns the pixmap to display following the server name
 */
static GdkPixbuf *
Cddb_Get_Pixbuf_From_Server_Name (const gchar *server_name)
{
    g_return_val_if_fail (server_name != NULL, NULL);

    if (strstr (server_name, "freedb.org"))
        return gdk_pixbuf_new_from_resource ("/org/gnome/EasyTAG/images/freedb.png",
                                             NULL);
    else if (strstr(server_name,"gnudb.org"))
        return gdk_pixbuf_new_from_resource ("/org/gnome/EasyTAG/images/gnudb.png",
                                             NULL);
    else if (strstr(server_name,"musicbrainz.org"))
        return gdk_pixbuf_new_from_resource ("/org/gnome/EasyTAG/images/musicbrainz.png",
                                             NULL);
    else
        return NULL;
}

static void
et_cddb_dialog_finalize (GObject *object)
{
    EtCDDBDialog *self;
    EtCDDBDialogPrivate *priv;

    self = ET_CDDB_DIALOG (object);
    priv = et_cddb_dialog_get_instance_private (self);

    if (priv->album_list)
    {
        Cddb_Free_Album_List (self);
    }

    g_object_unref (priv->session);

    G_OBJECT_CLASS (et_cddb_dialog_parent_class)->finalize (object);
}

static void
et_cddb_dialog_init (EtCDDBDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
    create_cddb_dialog (self);
}

static void
et_cddb_dialog_class_init (EtCDDBDialogClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    G_OBJECT_CLASS (klass)->finalize = et_cddb_dialog_finalize;

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/EasyTAG/cddb_dialog.ui");
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  album_list_model);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  album_list_view);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  track_list_model);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  track_list_view);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  search_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  apply_button);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  automatic_search_button);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  manual_search_button);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  stop_search_button);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  status_bar);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  artist_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  album_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  track_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  other_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  blues_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  classical_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  country_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  folk_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  jazz_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  misc_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  newage_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  reggae_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  rock_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  soundtrack_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  filename_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  title_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  fill_artist_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  fill_album_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  year_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  fill_track_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  track_total_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  genre_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  dlm_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtCDDBDialog,
                                                  scanner_check);
    gtk_widget_class_bind_template_callback (widget_class,
                                             et_cddb_dialog_on_response);
    gtk_widget_class_bind_template_callback (widget_class,
                                             et_cddb_dialog_search_from_selection);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_track_list_button_press_event);
    gtk_widget_class_bind_template_callback (widget_class, show_album_info);
    gtk_widget_class_bind_template_callback (widget_class, stop_search);
    gtk_widget_class_bind_template_callback (widget_class,
                                             track_list_select_all);
    gtk_widget_class_bind_template_callback (widget_class,
                                             track_list_unselect_all);
    gtk_widget_class_bind_template_callback (widget_class,
                                             update_apply_button_sensitivity);
    gtk_widget_class_bind_template_callback (widget_class,
                                             update_search_button_sensitivity);
    gtk_widget_class_bind_template_callback (widget_class,
                                             Cddb_Destroy_Window);
    gtk_widget_class_bind_template_callback (widget_class,
                                             Cddb_Get_Album_Tracks_List_CB);
    gtk_widget_class_bind_template_callback (widget_class,
                                             Cddb_Set_Track_Infos_To_File_List);
    gtk_widget_class_bind_template_callback (widget_class,
                                             Cddb_Search_Album_List_From_String);
    gtk_widget_class_bind_template_callback (widget_class,
                                             Cddb_Track_List_Invert_Selection);
    gtk_widget_class_bind_template_callback (widget_class,
                                             Cddb_Track_List_Row_Selected);
}

/*
 * et_cddb_dialog_new:
 *
 * Create a new EtCDDBDialog instance.
 *
 * Returns: a new #EtCDDBDialog
 */
EtCDDBDialog *
et_cddb_dialog_new (void)
{
    GtkSettings *settings;
    gboolean use_header_bar = FALSE;

    settings = gtk_settings_get_default ();

    if (settings)
    {
        g_object_get (settings, "gtk-dialogs-use-header", &use_header_bar,
                      NULL);
    }

    return g_object_new (ET_TYPE_CDDB_DIALOG, "use-header-bar", use_header_bar,
                         NULL);
}
