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

#include "search_dialog.h"

#include <glib/gi18n.h>

#include "application_window.h"
#include "browser.h"
#include "charset.h"
#include "easytag.h"
#include "log.h"
#include "misc.h"
#include "picture.h"
#include "scan_dialog.h"
#include "setting.h"

typedef struct
{
    GtkWidget *search_find_button;
    GtkWidget *search_string_combo;
    GtkListStore *search_string_model;
    GtkWidget *search_filename_check;
    GtkWidget *search_tag_check;
    GtkWidget *search_case_check;
    GtkWidget *search_results_view;
    GtkListStore *search_results_model;
    GtkWidget *status_bar;
    guint status_bar_context;
} EtSearchDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtSearchDialog, et_search_dialog, GTK_TYPE_DIALOG)

enum
{
    // Columns for titles
    SEARCH_RESULT_FILENAME = 0,
    SEARCH_RESULT_TITLE,
    SEARCH_RESULT_ARTIST,
    SEARCH_RESULT_ALBUM_ARTIST,
    SEARCH_RESULT_ALBUM,
    SEARCH_RESULT_DISC_NUMBER,
    SEARCH_RESULT_YEAR,
    SEARCH_RESULT_TRACK,
    SEARCH_RESULT_GENRE,
    SEARCH_RESULT_COMMENT,
    SEARCH_RESULT_COMPOSER,
    SEARCH_RESULT_ORIG_ARTIST,
    SEARCH_RESULT_COPYRIGHT,
    SEARCH_RESULT_URL,
    SEARCH_RESULT_ENCODED_BY,

    // Columns for pango style (normal/bold)
    SEARCH_RESULT_FILENAME_WEIGHT,
    SEARCH_RESULT_TITLE_WEIGHT,
    SEARCH_RESULT_ARTIST_WEIGHT,
    SEARCH_RESULT_ALBUM_ARTIST_WEIGHT,
    SEARCH_RESULT_ALBUM_WEIGHT,
    SEARCH_RESULT_DISC_NUMBER_WEIGHT,
    SEARCH_RESULT_YEAR_WEIGHT,
    SEARCH_RESULT_TRACK_WEIGHT,
    SEARCH_RESULT_GENRE_WEIGHT,
    SEARCH_RESULT_COMMENT_WEIGHT,
    SEARCH_RESULT_COMPOSER_WEIGHT,
    SEARCH_RESULT_ORIG_ARTIST_WEIGHT,
    SEARCH_RESULT_COPYRIGHT_WEIGHT,
    SEARCH_RESULT_URL_WEIGHT,
    SEARCH_RESULT_ENCODED_BY_WEIGHT,

    // Columns for color (normal/red)
    SEARCH_RESULT_FILENAME_FOREGROUND,
    SEARCH_RESULT_TITLE_FOREGROUND,
    SEARCH_RESULT_ARTIST_FOREGROUND,
    SEARCH_RESULT_ALBUM_ARTIST_FOREGROUND,
    SEARCH_RESULT_ALBUM_FOREGROUND,
    SEARCH_RESULT_DISC_NUMBER_FOREGROUND,
    SEARCH_RESULT_YEAR_FOREGROUND,
    SEARCH_RESULT_TRACK_FOREGROUND,
    SEARCH_RESULT_GENRE_FOREGROUND,
    SEARCH_RESULT_COMMENT_FOREGROUND,
    SEARCH_RESULT_COMPOSER_FOREGROUND,
    SEARCH_RESULT_ORIG_ARTIST_FOREGROUND,
    SEARCH_RESULT_COPYRIGHT_FOREGROUND,
    SEARCH_RESULT_URL_FOREGROUND,
    SEARCH_RESULT_ENCODED_BY_FOREGROUND,

    SEARCH_RESULT_POINTER,
    SEARCH_COLUMN_COUNT
};

/*
 * Callback to select-row event
 * Select all results that are selected in the search result list also in the browser list
 */
static void
Search_Result_List_Row_Selected (GtkTreeSelection *selection,
                                 gpointer user_data)
{
    EtSearchDialogPrivate *priv;
    GList       *selectedRows;
    GList *l;
    ET_File     *ETFile;
    GtkTreeIter  currentFile;

    priv = et_search_dialog_get_instance_private (ET_SEARCH_DIALOG (user_data));

    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    /* We might be called with no rows selected */
    if (!selectedRows)
    {
        return;
    }

    /* Unselect files in the main list before re-selecting them... */
    et_application_window_browser_unselect_all (ET_APPLICATION_WINDOW (MainWindow));

    for (l = selectedRows; l != NULL; l = g_list_next (l))
    {
        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->search_results_model),
                                     &currentFile, (GtkTreePath *)l->data))
        {
            gtk_tree_model_get(GTK_TREE_MODEL(priv->search_results_model), &currentFile, 
                               SEARCH_RESULT_POINTER, &ETFile, -1);
            /* Select the files (but don't display them to increase speed). */
            et_application_window_browser_select_file_by_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                                                  ETFile,
                                                                  TRUE);
            /* Display only the last file (to increase speed). */
            if (!selectedRows->next)
            {
                et_application_window_select_file_by_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                                              ETFile);
            }
        }
    }

    g_list_free_full (selectedRows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * Add_Row_To_Search_Result_List:
 * @self: an #EtSearchDialog
 * @ETFile: a file with tags in which to search
 * @string_to_search: the search term
 *
 * Search for the given @string_to_search in tags and the filename from
 * @ETFile. Add the result row, corresctly-formatted to highlight matches, to
 * the tree view in @self.
 */
static void
Add_Row_To_Search_Result_List (EtSearchDialog *self,
                               const ET_File *ETFile,
                               const gchar *string_to_search)
{
    EtSearchDialogPrivate *priv;
    const gchar *haystacks[15]; /* 15 columns to display. */
    gint weights[15] = { PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                         PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                         PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                         PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                         PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                         PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                         PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                         PANGO_WEIGHT_NORMAL };
    GdkRGBA *colors[15] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                            NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    gchar *display_basename;
    const gchar *track;
    const gchar *track_total;
    const gchar *disc_number;
    const gchar *disc_total;
    gchar *discs = NULL;
    gchar *tracks = NULL;
    gboolean case_sensitive;
    gsize column;

    priv = et_search_dialog_get_instance_private (self);

    if (!ETFile || !string_to_search)
        return;

    case_sensitive = g_settings_get_boolean (MainSettings, "search-case-sensitive");

    /* Most fields can be taken from the tag as-is. */
    haystacks[SEARCH_RESULT_TITLE] = ((File_Tag *)ETFile->FileTag->data)->title;
    haystacks[SEARCH_RESULT_ARTIST] = ((File_Tag *)ETFile->FileTag->data)->artist;
    haystacks[SEARCH_RESULT_ALBUM_ARTIST] = ((File_Tag *)ETFile->FileTag->data)->album_artist;
    haystacks[SEARCH_RESULT_ALBUM] = ((File_Tag *)ETFile->FileTag->data)->album;
    haystacks[SEARCH_RESULT_YEAR] = ((File_Tag *)ETFile->FileTag->data)->year;
    haystacks[SEARCH_RESULT_GENRE] = ((File_Tag *)ETFile->FileTag->data)->genre;
    haystacks[SEARCH_RESULT_COMMENT] = ((File_Tag *)ETFile->FileTag->data)->comment;
    haystacks[SEARCH_RESULT_COMPOSER] = ((File_Tag *)ETFile->FileTag->data)->composer;
    haystacks[SEARCH_RESULT_ORIG_ARTIST] = ((File_Tag *)ETFile->FileTag->data)->orig_artist;
    haystacks[SEARCH_RESULT_COPYRIGHT] = ((File_Tag *)ETFile->FileTag->data)->copyright;
    haystacks[SEARCH_RESULT_URL] = ((File_Tag *)ETFile->FileTag->data)->url;
    haystacks[SEARCH_RESULT_ENCODED_BY] = ((File_Tag *)ETFile->FileTag->data)->encoded_by;

    /* Some fields need extra allocations. */
    display_basename = g_path_get_basename (((File_Name *)ETFile->FileNameNew->data)->value_utf8);
    haystacks[SEARCH_RESULT_FILENAME] = display_basename;

    /* Disc Number. */
    disc_number = ((File_Tag *)ETFile->FileTag->data)->disc_number;
    disc_total = ((File_Tag *)ETFile->FileTag->data)->disc_total;

    if (disc_number)
    {
        if (disc_total)
        {
            discs = g_strconcat (disc_number, "/", disc_total, NULL);
            haystacks[SEARCH_RESULT_DISC_NUMBER] = discs;
        }
        else
        {
            haystacks[SEARCH_RESULT_DISC_NUMBER] = disc_number;
        }
    }
    else
    {
        haystacks[SEARCH_RESULT_DISC_NUMBER] = NULL;
    }

    /* Track. */
    track = ((File_Tag *)ETFile->FileTag->data)->track;
    track_total = ((File_Tag *)ETFile->FileTag->data)->track_total;

    if (track)
    {
        if (track_total)
        {
            tracks = g_strconcat (track, "/", track_total, NULL);
            haystacks[SEARCH_RESULT_TRACK] = tracks;
        }
        else
        {
            haystacks[SEARCH_RESULT_TRACK] = track;
        }
    }
    else
    {
        haystacks[SEARCH_RESULT_TRACK] = NULL;
    }

    /* Highlight the keywords in the result list. Don't display files in red if
     * the searched string is '' (to display all files). */
    for (column = 0; column < G_N_ELEMENTS (haystacks); column++)
    {
        gchar *needle;
        gchar *haystack;

        /* Already checked if string_to_search is NULL. */
        needle = g_utf8_normalize (string_to_search, -1, G_NORMALIZE_DEFAULT);
        haystack = haystacks[column] ? g_utf8_normalize (haystacks[column], -1,
                                                         G_NORMALIZE_DEFAULT)
                                     : NULL;

        if (case_sensitive)
        {
            if (haystack && !et_str_empty (needle)
                && strstr (haystack, needle))
            {

                if (g_settings_get_boolean (MainSettings, "file-changed-bold"))
                {
                    weights[column] = PANGO_WEIGHT_BOLD;
                }
                else
                {
                    colors[column] = &RED;
                }
            }
        }
        else
        {
            /* Search wasn't case-sensitive. */
            gchar *list_text;
            gchar *string_to_search2;

            if (!haystack)
            {
                g_free (needle);
                continue;
            }

            string_to_search2 = g_utf8_casefold (needle, -1);
            list_text = g_utf8_casefold (haystack, -1);

            if (!et_str_empty (string_to_search2)
                && strstr (list_text, string_to_search2))
            {
                if (g_settings_get_boolean (MainSettings, "file-changed-bold"))
                {
                    weights[column] = PANGO_WEIGHT_BOLD;
                }
                else
                {
                    colors[column] = &RED;
                }
            }

            g_free(list_text);
            g_free(string_to_search2);
        }

        g_free (haystack);
        g_free (needle);
    }

    /* Load the row in the list. */
    gtk_list_store_insert_with_values (priv->search_results_model, NULL, G_MAXINT,
                                       SEARCH_RESULT_FILENAME, haystacks[SEARCH_RESULT_FILENAME],
                                       SEARCH_RESULT_TITLE, haystacks[SEARCH_RESULT_TITLE],
                                       SEARCH_RESULT_ARTIST, haystacks[SEARCH_RESULT_ARTIST],
                                       SEARCH_RESULT_ALBUM_ARTIST,haystacks[SEARCH_RESULT_ALBUM_ARTIST],
                                       SEARCH_RESULT_ALBUM, haystacks[SEARCH_RESULT_ALBUM],
                                       SEARCH_RESULT_DISC_NUMBER, haystacks[SEARCH_RESULT_DISC_NUMBER],
                                       SEARCH_RESULT_YEAR, haystacks[SEARCH_RESULT_YEAR],
                                       SEARCH_RESULT_TRACK, haystacks[SEARCH_RESULT_TRACK],
                                       SEARCH_RESULT_GENRE, haystacks[SEARCH_RESULT_GENRE],
                                       SEARCH_RESULT_COMMENT, haystacks[SEARCH_RESULT_COMMENT],
                                       SEARCH_RESULT_COMPOSER, haystacks[SEARCH_RESULT_COMPOSER],
                                       SEARCH_RESULT_ORIG_ARTIST, haystacks[SEARCH_RESULT_ORIG_ARTIST],
                                       SEARCH_RESULT_COPYRIGHT, haystacks[SEARCH_RESULT_COPYRIGHT],
                                       SEARCH_RESULT_URL, haystacks[SEARCH_RESULT_URL],
                                       SEARCH_RESULT_ENCODED_BY, haystacks[SEARCH_RESULT_ENCODED_BY],

                                       SEARCH_RESULT_FILENAME_WEIGHT, weights[SEARCH_RESULT_FILENAME],
                                       SEARCH_RESULT_TITLE_WEIGHT, weights[SEARCH_RESULT_TITLE],
                                       SEARCH_RESULT_ARTIST_WEIGHT, weights[SEARCH_RESULT_ARTIST],
                                       SEARCH_RESULT_ALBUM_ARTIST_WEIGHT, weights[SEARCH_RESULT_ALBUM_ARTIST],
                                       SEARCH_RESULT_ALBUM_WEIGHT, weights[SEARCH_RESULT_ALBUM],
                                       SEARCH_RESULT_DISC_NUMBER_WEIGHT, weights[SEARCH_RESULT_DISC_NUMBER],
                                       SEARCH_RESULT_YEAR_WEIGHT, weights[SEARCH_RESULT_YEAR],
                                       SEARCH_RESULT_TRACK_WEIGHT, weights[SEARCH_RESULT_TRACK],
                                       SEARCH_RESULT_GENRE_WEIGHT, weights[SEARCH_RESULT_GENRE],
                                       SEARCH_RESULT_COMMENT_WEIGHT, weights[SEARCH_RESULT_COMMENT],
                                       SEARCH_RESULT_COMPOSER_WEIGHT, weights[SEARCH_RESULT_COMPOSER],
                                       SEARCH_RESULT_ORIG_ARTIST_WEIGHT, weights[SEARCH_RESULT_ORIG_ARTIST],
                                       SEARCH_RESULT_COPYRIGHT_WEIGHT, weights[SEARCH_RESULT_COPYRIGHT],
                                       SEARCH_RESULT_URL_WEIGHT, weights[SEARCH_RESULT_URL],
                                       SEARCH_RESULT_ENCODED_BY_WEIGHT, weights[SEARCH_RESULT_ENCODED_BY],

                                       SEARCH_RESULT_FILENAME_FOREGROUND, colors[SEARCH_RESULT_FILENAME],
                                       SEARCH_RESULT_TITLE_FOREGROUND, colors[SEARCH_RESULT_TITLE],
                                       SEARCH_RESULT_ARTIST_FOREGROUND, colors[SEARCH_RESULT_ARTIST],
                                       SEARCH_RESULT_ALBUM_ARTIST_FOREGROUND, colors[SEARCH_RESULT_ALBUM_ARTIST],
                                       SEARCH_RESULT_ALBUM_FOREGROUND, colors[SEARCH_RESULT_ALBUM],
                                       SEARCH_RESULT_DISC_NUMBER_FOREGROUND, colors[SEARCH_RESULT_DISC_NUMBER],
                                       SEARCH_RESULT_YEAR_FOREGROUND, colors[SEARCH_RESULT_YEAR],
                                       SEARCH_RESULT_TRACK_FOREGROUND, colors[SEARCH_RESULT_TRACK],
                                       SEARCH_RESULT_GENRE_FOREGROUND, colors[SEARCH_RESULT_GENRE],
                                       SEARCH_RESULT_COMMENT_FOREGROUND, colors[SEARCH_RESULT_COMMENT],
                                       SEARCH_RESULT_COMPOSER_FOREGROUND, colors[SEARCH_RESULT_COMPOSER],
                                       SEARCH_RESULT_ORIG_ARTIST_FOREGROUND, colors[SEARCH_RESULT_ORIG_ARTIST],
                                       SEARCH_RESULT_COPYRIGHT_FOREGROUND, colors[SEARCH_RESULT_COPYRIGHT],
                                       SEARCH_RESULT_URL_FOREGROUND, colors[SEARCH_RESULT_URL],
                                       SEARCH_RESULT_ENCODED_BY_FOREGROUND, colors[SEARCH_RESULT_ENCODED_BY],

                                       SEARCH_RESULT_POINTER, ETFile, -1);

    /* Frees allocated data. */
    g_free (display_basename);
    g_free (discs);
    g_free (tracks);
}

/*
 * This function and the one below could do with improving
 * as we are looking up tag data twice (once when searching, once when adding to list)
 */
/*
 * Search_File:
 * @search_button: the search button which was clicked
 * @user_data: the #EtSearchDialog which contains @search_button
 *
 * Search for the search term (in the search entry of @user_data) in the list
 * of open files.
 */
static void
Search_File (GtkWidget *search_button,
             gpointer user_data)
{
    EtSearchDialog *self;
    EtSearchDialogPrivate *priv;
    const gchar *string_to_search = NULL;
    GList *l;
    const ET_File *ETFile;
    gchar *msg;
    gint resultCount = 0;

    self = ET_SEARCH_DIALOG (user_data);
    priv = et_search_dialog_get_instance_private (self);

    if (!priv->search_string_combo || !priv->search_filename_check || !priv->search_tag_check || !priv->search_results_view)
        return;

    string_to_search = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->search_string_combo))));

    if (!string_to_search)
        return;

    Add_String_To_Combo_List (priv->search_string_model, string_to_search);

    gtk_widget_set_sensitive (GTK_WIDGET (search_button), FALSE);
    gtk_list_store_clear (priv->search_results_model);
    gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                        priv->status_bar_context, "");

    for (l = ETCore->ETFileList; l != NULL; l = g_list_next (l))
    {
        ETFile = (ET_File *)l->data;

        /* Search in the filename. */
        if (g_settings_get_boolean (MainSettings, "search-filename"))
        {
            const gchar *filename_utf8 = ((File_Name *)ETFile->FileNameNew->data)->value_utf8;
            gchar *basename;
            gchar *haystack;
            gchar *needle;

            basename = g_path_get_basename (filename_utf8);

            /* To search without case sensitivity. */
            if (!g_settings_get_boolean (MainSettings, "search-case-sensitive"))
            {
                haystack = g_utf8_casefold (basename, -1);
                needle = g_utf8_casefold (string_to_search, -1);
            }
            else
            {
                haystack = g_utf8_normalize (basename, -1, G_NORMALIZE_DEFAULT);
                needle = g_utf8_normalize (string_to_search, -1,
                                           G_NORMALIZE_DEFAULT);
            }

            g_free (basename);

            if (haystack && strstr (haystack, needle))
            {
                Add_Row_To_Search_Result_List (self, ETFile, needle);
                g_free (haystack);
                g_free (needle);
                continue;
            }

            g_free (haystack);
            g_free (needle);
        }

        /* Search in the tag. */
        if (g_settings_get_boolean (MainSettings, "search-tag"))
        {
            gchar *title2, *artist2, *album_artist2, *album2, *disc_number2,
                  *disc_total2, *year2, *track2, *track_total2, *genre2,
                  *comment2, *composer2, *orig_artist2, *copyright2, *url2,
                  *encoded_by2;
            gchar *needle;
            const File_Tag *FileTag = (File_Tag *)ETFile->FileTag->data;

            if (!g_settings_get_boolean (MainSettings, "search-case-sensitive"))
            {
                /* To search without case sensitivity. */
                title2 = FileTag->title ? g_utf8_casefold (FileTag->title, -1)
                                        : NULL;
                artist2 = FileTag->artist ? g_utf8_casefold (FileTag->artist,
                                                             -1)
                                          : NULL;
                album_artist2 = FileTag->album_artist ? g_utf8_casefold (FileTag->album_artist,
                                                                         -1)
                                          : NULL;
                album2 = FileTag->album ? g_utf8_casefold (FileTag->album, -1)
                                          : NULL;
                disc_number2 = FileTag->disc_number ? g_utf8_casefold (FileTag->disc_number,
                                                                       -1)
                                                    : NULL;
                disc_total2 = FileTag->disc_total ? g_utf8_casefold (FileTag->disc_total,
                                                                     -1)
                                                  : NULL;
                year2 = FileTag->year ? g_utf8_casefold (FileTag->year, -1)
                                      : NULL;
                track2 = FileTag->track ? g_utf8_casefold (FileTag->track, -1)
                                        : NULL;
                track_total2 = FileTag->track_total ? g_utf8_casefold (FileTag->track_total,
                                                                       -1)
                                                    : NULL;
                genre2 = FileTag->genre ? g_utf8_casefold (FileTag->genre, -1)
                                        : NULL;
                comment2 = FileTag->comment ? g_utf8_casefold (FileTag->comment,
                                                               -1)
                                            : NULL;
                composer2 = FileTag->composer ? g_utf8_casefold (FileTag->composer,
                                                                 -1)
                                              : NULL;
                orig_artist2 = FileTag->orig_artist ? g_utf8_casefold (FileTag->orig_artist,
                                                                       -1)
                                                    : NULL;
                copyright2 = FileTag->copyright ? g_utf8_casefold (FileTag->copyright,
                                                                   -1)
                                                : NULL;
                url2 = FileTag->url ? g_utf8_casefold (FileTag->url, -1)
                                    : NULL;
                encoded_by2 = FileTag->encoded_by ? g_utf8_casefold (FileTag->encoded_by,
                                                                     -1)
                                                  : NULL;
                needle = g_utf8_casefold (string_to_search, -1);
            }
            else
            {
                /* To search with case-sensitivity. */
                title2 = FileTag->disc_number ? g_utf8_normalize (FileTag->title,
                                                                  -1,
                                                                  G_NORMALIZE_DEFAULT)
                                              : NULL;
                artist2 = FileTag->artist ? g_utf8_normalize (FileTag->artist,
                                                              -1,
                                                              G_NORMALIZE_DEFAULT)
                                          : NULL;
                album_artist2 = FileTag->album_artist ? g_utf8_normalize (FileTag->album_artist,
                                                                          -1,
                                                                          G_NORMALIZE_DEFAULT)
                                                      : NULL;
                album2 = FileTag->album ? g_utf8_normalize (FileTag->album, -1,
                                                            G_NORMALIZE_DEFAULT)
                                        : NULL;
                disc_number2 = FileTag->disc_number ? g_utf8_normalize (FileTag->disc_number,
                                                                        -1,
                                                                        G_NORMALIZE_DEFAULT)
                                                     : NULL;
                disc_total2 = FileTag->disc_total ? g_utf8_normalize (FileTag->disc_total,
                                                                      -1,
                                                                      G_NORMALIZE_DEFAULT)
                                                  : NULL;
                year2 = FileTag->year ? g_utf8_normalize (FileTag->year, -1,
                                                          G_NORMALIZE_DEFAULT)
                                      : NULL;
                track2 = FileTag->track ? g_utf8_normalize (FileTag->track, -1,
                                                            G_NORMALIZE_DEFAULT)
                                        : NULL;
                track_total2 = FileTag->track_total ? g_utf8_normalize (FileTag->track_total,
                                                                        -1,
                                                                        G_NORMALIZE_DEFAULT)
                                                    : NULL;
                genre2 = FileTag->genre ? g_utf8_normalize (FileTag->genre, -1,
                                                            G_NORMALIZE_DEFAULT)
                                        : NULL;
                comment2 = FileTag->comment ? g_utf8_normalize (FileTag->comment,
                                                                -1,
                                                                G_NORMALIZE_DEFAULT)
                                            : NULL;
                composer2 = FileTag->composer ? g_utf8_normalize (FileTag->composer,
                                                                  -1,
                                                                  G_NORMALIZE_DEFAULT)
                                              : NULL;
                orig_artist2 = FileTag->orig_artist ? g_utf8_normalize (FileTag->orig_artist,
                                                                        -1,
                                                                        G_NORMALIZE_DEFAULT)
                                                    : NULL;
                copyright2 = FileTag->copyright ? g_utf8_normalize (FileTag->copyright,
                                                                    -1,
                                                                    G_NORMALIZE_DEFAULT)
                                                : NULL;
                url2 = FileTag->url ? g_utf8_normalize (FileTag->url, -1,
                                                        G_NORMALIZE_DEFAULT)
                                    : NULL;
                encoded_by2 = FileTag->encoded_by ? g_utf8_normalize (FileTag->encoded_by,
                                                                      -1,
                                                                      G_NORMALIZE_DEFAULT)
                                                  : NULL;
                needle = g_utf8_normalize (string_to_search, -1,
                                           G_NORMALIZE_DEFAULT);
            }

            if ((title2 && strstr (title2, needle))
                || (artist2 && strstr (artist2, needle))
                || (album_artist2 && strstr (album_artist2, needle))
                || (album2 && strstr (album2, needle))
                || (disc_number2 && strstr (disc_number2, needle))
                || (disc_total2 && strstr (disc_total2, needle))
                || (year2 && strstr (year2, needle))
                || (track2 && strstr (track2, needle))
                || (track_total2 && strstr (track_total2, needle))
                || (genre2 && strstr (genre2, needle))
                || (comment2 && strstr (comment2, needle))
                || (composer2 && strstr (composer2, needle))
                || (orig_artist2 && strstr (orig_artist2, needle))
                || (copyright2 && strstr (copyright2, needle))
                || (url2 && strstr (url2, needle))
                || (encoded_by2 && strstr (encoded_by2, needle)))
            {
                Add_Row_To_Search_Result_List (self, ETFile, string_to_search);
            }

            g_free (title2);
            g_free (artist2);
            g_free (album_artist2);
            g_free (album2);
            g_free (disc_number2);
            g_free (disc_total2);
            g_free (year2);
            g_free (track2);
            g_free (track_total2);
            g_free (genre2);
            g_free (comment2);
            g_free (composer2);
            g_free (orig_artist2);
            g_free (copyright2);
            g_free (url2);
            g_free (encoded_by2);
            g_free (needle);
        }
    }

    gtk_widget_set_sensitive (GTK_WIDGET (search_button), TRUE);

    /* Display the number of matches in the statusbar. */
    resultCount = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->search_results_model),
                                                  NULL);
    msg = g_strdup_printf (ngettext ("Found one file", "Found %d files",
                           resultCount), resultCount);
    gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                        priv->status_bar_context, msg);
    g_free (msg);

    /* Disable result list if no row inserted. */
    if (resultCount > 0)
    {
        gtk_widget_set_sensitive (GTK_WIDGET (priv->search_results_view),
                                  TRUE);
    }
    else
    {
        gtk_widget_set_sensitive (GTK_WIDGET (priv->search_results_view),
                                  FALSE);
    }
}

static void
on_close_clicked (GtkButton *button, gpointer user_data)
{
    EtSearchDialog *self;

    self = ET_SEARCH_DIALOG (user_data);

    et_search_dialog_apply_changes (self);
    gtk_widget_hide (GTK_WIDGET (self));
}

static gboolean
on_delete_event (GtkWidget *widget)
{
    et_search_dialog_apply_changes (ET_SEARCH_DIALOG (widget));
    gtk_widget_hide (widget);

    return G_SOURCE_CONTINUE;
}

/*
 * The window to search keywords in the list of files.
 */
static void
create_search_dialog (EtSearchDialog *self)
{
    EtSearchDialogPrivate *priv;

    priv = et_search_dialog_get_instance_private (self);

    /* Words to search. */
    priv->search_string_model = gtk_list_store_new (MISC_COMBO_COUNT,
                                                    G_TYPE_STRING);
    gtk_combo_box_set_model (GTK_COMBO_BOX (priv->search_string_combo),
                             GTK_TREE_MODEL (priv->search_string_model));
    g_object_unref (priv->search_string_model);
    /* History List. */
    Load_Search_File_List (priv->search_string_model, MISC_COMBO_TEXT);
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->search_string_combo))),
                        "");

    /* Set content of the clipboard if available. */
    gtk_editable_paste_clipboard (GTK_EDITABLE (gtk_bin_get_child (GTK_BIN (priv->search_string_combo))));

    g_settings_bind (MainSettings, "search-filename",
                     priv->search_filename_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "search-tag", priv->search_tag_check,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Property of the search. */
    g_settings_bind (MainSettings, "search-case-sensitive",
                     priv->search_case_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Button to run the search. */
    gtk_widget_grab_default (priv->search_find_button);
    g_signal_connect (gtk_bin_get_child (GTK_BIN (priv->search_string_combo)),
                      "activate", G_CALLBACK (Search_File), self);

    /* Status bar. */
    priv->status_bar_context = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->status_bar),
                                                             "Messages");
    gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                        priv->status_bar_context, _("Ready to search…"));
}

/*
 * For the configuration file...
 */
void
et_search_dialog_apply_changes (EtSearchDialog *self)
{
    EtSearchDialogPrivate *priv;

    g_return_if_fail (ET_SEARCH_DIALOG (self));

    priv = et_search_dialog_get_instance_private (self);

    Save_Search_File_List (priv->search_string_model, MISC_COMBO_TEXT);
}

static void
et_search_dialog_init (EtSearchDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
    create_search_dialog (self);
}

static void
et_search_dialog_class_init (EtSearchDialogClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/EasyTAG/search_dialog.ui");
    gtk_widget_class_bind_template_child_private (widget_class, EtSearchDialog,
                                                  search_find_button);
    gtk_widget_class_bind_template_child_private (widget_class, EtSearchDialog,
                                                  search_string_combo);
    gtk_widget_class_bind_template_child_private (widget_class, EtSearchDialog,
                                                  search_filename_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtSearchDialog,
                                                  search_tag_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtSearchDialog,
                                                  search_case_check);
    gtk_widget_class_bind_template_child_private (widget_class, EtSearchDialog,
                                                  search_results_model);
    gtk_widget_class_bind_template_child_private (widget_class, EtSearchDialog,
                                                  search_results_view);
    gtk_widget_class_bind_template_child_private (widget_class, EtSearchDialog,
                                                  status_bar);
    gtk_widget_class_bind_template_callback (widget_class, on_close_clicked);
    gtk_widget_class_bind_template_callback (widget_class, on_delete_event);
    gtk_widget_class_bind_template_callback (widget_class, Search_File);
    gtk_widget_class_bind_template_callback (widget_class,
                                             Search_Result_List_Row_Selected);
}

/*
 * et_search_dialog_new:
 *
 * Create a new EtSearchDialog instance.
 *
 * Returns: a new #EtSearchDialog
 */
EtSearchDialog *
et_search_dialog_new (GtkWindow *parent)
{
    g_return_val_if_fail (GTK_WINDOW (parent), NULL);

    return g_object_new (ET_TYPE_SEARCH_DIALOG, "transient-for", parent, NULL);
}
