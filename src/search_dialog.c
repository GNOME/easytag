/* EasyTAG - tag editor for audio files
 * Copyright (C) 2013-2014  David King <amigadave@amigadave.com>
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

/* TODO: Use G_DEFINE_TYPE_WITH_PRIVATE. */
G_DEFINE_TYPE (EtSearchDialog, et_search_dialog, GTK_TYPE_DIALOG)

#define et_search_dialog_get_instance_private(dialog) (dialog->priv)

static const guint BOX_SPACING = 6;

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

struct _EtSearchDialogPrivate
{
    GtkWidget *search_string_combo;
    GtkListStore *search_string_model;
    GtkWidget *search_in_filename;
    GtkWidget *search_in_tag;
    GtkWidget *search_case_sensitive;
    GtkWidget *search_results_view;
    GtkListStore *search_results_model;
    GtkWidget *status_bar;
    guint status_bar_context;
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

static void
Add_Row_To_Search_Result_List (EtSearchDialog *self,
                               ET_File *ETFile, const gchar *string_to_search)
{
    EtSearchDialogPrivate *priv;
    gchar *SearchResultList_Text[15]; // Because : 15 columns to display
    gint SearchResultList_Weight[15] = {PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL};
    GdkRGBA *SearchResultList_Color[15] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                            NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                            NULL};
    gchar *track, *track_total;
    gchar *disc_number, *disc_total;
    gboolean case_sensitive;
    gint column;

    priv = et_search_dialog_get_instance_private (self);

    if (!ETFile || !string_to_search)
        return;

    case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->search_case_sensitive));

    // Filename
    SearchResultList_Text[SEARCH_RESULT_FILENAME]    = g_path_get_basename( ((File_Name *)ETFile->FileNameNew->data)->value_utf8 );
    // Title
    SearchResultList_Text[SEARCH_RESULT_TITLE]       = g_strdup(((File_Tag *)ETFile->FileTag->data)->title);
    // Artist
    SearchResultList_Text[SEARCH_RESULT_ARTIST]      = g_strdup(((File_Tag *)ETFile->FileTag->data)->artist);
    // Album Artist
    SearchResultList_Text[SEARCH_RESULT_ALBUM_ARTIST]= g_strdup(((File_Tag *)ETFile->FileTag->data)->album_artist);
    // Album
    SearchResultList_Text[SEARCH_RESULT_ALBUM]       = g_strdup(((File_Tag *)ETFile->FileTag->data)->album);
    // Year
    SearchResultList_Text[SEARCH_RESULT_YEAR]        = g_strdup(((File_Tag *)ETFile->FileTag->data)->year);
    //Genre
    SearchResultList_Text[SEARCH_RESULT_GENRE]       = g_strdup(((File_Tag *)ETFile->FileTag->data)->genre);
    // Comment
    SearchResultList_Text[SEARCH_RESULT_COMMENT]     = g_strdup(((File_Tag *)ETFile->FileTag->data)->comment);
    // Composer
    SearchResultList_Text[SEARCH_RESULT_COMPOSER]    = g_strdup(((File_Tag *)ETFile->FileTag->data)->composer);
    // Orig. Artist
    SearchResultList_Text[SEARCH_RESULT_ORIG_ARTIST] = g_strdup(((File_Tag *)ETFile->FileTag->data)->orig_artist);
    // Copyright
    SearchResultList_Text[SEARCH_RESULT_COPYRIGHT]   = g_strdup(((File_Tag *)ETFile->FileTag->data)->copyright);
    // URL
    SearchResultList_Text[SEARCH_RESULT_URL]         = g_strdup(((File_Tag *)ETFile->FileTag->data)->url);
    // Encoded by
    SearchResultList_Text[SEARCH_RESULT_ENCODED_BY]  = g_strdup(((File_Tag *)ETFile->FileTag->data)->encoded_by);

    /* Disc Number. */
    disc_number = ((File_Tag *)ETFile->FileTag->data)->disc_number;
    disc_total = ((File_Tag *)ETFile->FileTag->data)->disc_total;

    if (disc_number)
    {
        if (disc_total)
        {
            SearchResultList_Text[SEARCH_RESULT_DISC_NUMBER] = g_strconcat (disc_number, "/", disc_total, NULL);
        }
        else
        {
            SearchResultList_Text[SEARCH_RESULT_DISC_NUMBER] = g_strdup (disc_number);
        }
    }
    else
    {
        SearchResultList_Text[SEARCH_RESULT_DISC_NUMBER] = NULL;
    }

    // Track
    track       = ((File_Tag *)ETFile->FileTag->data)->track;
    track_total = ((File_Tag *)ETFile->FileTag->data)->track_total;
    if (track)
    {
        if (track_total)
            SearchResultList_Text[SEARCH_RESULT_TRACK] = g_strconcat(track,"/",track_total,NULL);
        else
            SearchResultList_Text[SEARCH_RESULT_TRACK] = g_strdup(track);
    } else
    {
        SearchResultList_Text[SEARCH_RESULT_TRACK] = NULL;
    }


    // Highlight the keywords in the result list
    // Don't display files to red if the searched string is '' (to display all files)
    for (column=0;column<14;column++)
    {
        if (case_sensitive)
        {
            if ( SearchResultList_Text[column] && strlen(string_to_search) && strstr(SearchResultList_Text[column],string_to_search) )
            {
                if (g_settings_get_boolean (MainSettings, "file-changed-bold"))
                {
                    SearchResultList_Weight[column] = PANGO_WEIGHT_BOLD;
                }
                else
                {
                    SearchResultList_Color[column] = &RED;
                }
            }

        } else
        {
            // Search wasn't case sensitive
            gchar *list_text = NULL;
            gchar *string_to_search2 = g_utf8_casefold(string_to_search, -1);

            if (!SearchResultList_Text[column])
            {
                g_free(string_to_search2);
                continue;
            }

            list_text = g_utf8_casefold(SearchResultList_Text[column], -1);

            if ( list_text && strlen(string_to_search2) && strstr(list_text,string_to_search2) )
            {
                if (g_settings_get_boolean (MainSettings, "file-changed-bold"))
                {
                    SearchResultList_Weight[column] = PANGO_WEIGHT_BOLD;
                }
                else
                {
                    SearchResultList_Color[column] = &RED;
                }
            }

            g_free(list_text);
            g_free(string_to_search2);
        }
    }

    // Load the row in the list
    gtk_list_store_insert_with_values (priv->search_results_model, NULL, G_MAXINT,
                       SEARCH_RESULT_FILENAME,    SearchResultList_Text[SEARCH_RESULT_FILENAME],
                       SEARCH_RESULT_TITLE,       SearchResultList_Text[SEARCH_RESULT_TITLE],
                       SEARCH_RESULT_ARTIST,      SearchResultList_Text[SEARCH_RESULT_ARTIST],
                       SEARCH_RESULT_ALBUM_ARTIST,SearchResultList_Text[SEARCH_RESULT_ALBUM_ARTIST],
                       SEARCH_RESULT_ALBUM,       SearchResultList_Text[SEARCH_RESULT_ALBUM],
                       SEARCH_RESULT_DISC_NUMBER, SearchResultList_Text[SEARCH_RESULT_DISC_NUMBER],
                       SEARCH_RESULT_YEAR,        SearchResultList_Text[SEARCH_RESULT_YEAR],
                       SEARCH_RESULT_TRACK,       SearchResultList_Text[SEARCH_RESULT_TRACK],
                       SEARCH_RESULT_GENRE,       SearchResultList_Text[SEARCH_RESULT_GENRE],
                       SEARCH_RESULT_COMMENT,     SearchResultList_Text[SEARCH_RESULT_COMMENT],
                       SEARCH_RESULT_COMPOSER,    SearchResultList_Text[SEARCH_RESULT_COMPOSER],
                       SEARCH_RESULT_ORIG_ARTIST, SearchResultList_Text[SEARCH_RESULT_ORIG_ARTIST],
                       SEARCH_RESULT_COPYRIGHT,   SearchResultList_Text[SEARCH_RESULT_COPYRIGHT],
                       SEARCH_RESULT_URL,         SearchResultList_Text[SEARCH_RESULT_URL],
                       SEARCH_RESULT_ENCODED_BY,  SearchResultList_Text[SEARCH_RESULT_ENCODED_BY],

                       SEARCH_RESULT_FILENAME_WEIGHT,    SearchResultList_Weight[SEARCH_RESULT_FILENAME],
                       SEARCH_RESULT_TITLE_WEIGHT,       SearchResultList_Weight[SEARCH_RESULT_TITLE],
                       SEARCH_RESULT_ARTIST_WEIGHT,      SearchResultList_Weight[SEARCH_RESULT_ARTIST],
                       SEARCH_RESULT_ALBUM_ARTIST_WEIGHT, SearchResultList_Weight[SEARCH_RESULT_ALBUM_ARTIST],
		       SEARCH_RESULT_ALBUM_WEIGHT,       SearchResultList_Weight[SEARCH_RESULT_ALBUM],
                       SEARCH_RESULT_DISC_NUMBER_WEIGHT, SearchResultList_Weight[SEARCH_RESULT_DISC_NUMBER],
                       SEARCH_RESULT_YEAR_WEIGHT,        SearchResultList_Weight[SEARCH_RESULT_YEAR],
                       SEARCH_RESULT_TRACK_WEIGHT,       SearchResultList_Weight[SEARCH_RESULT_TRACK],
                       SEARCH_RESULT_GENRE_WEIGHT,       SearchResultList_Weight[SEARCH_RESULT_GENRE],
                       SEARCH_RESULT_COMMENT_WEIGHT,     SearchResultList_Weight[SEARCH_RESULT_COMMENT],
                       SEARCH_RESULT_COMPOSER_WEIGHT,    SearchResultList_Weight[SEARCH_RESULT_COMPOSER],
                       SEARCH_RESULT_ORIG_ARTIST_WEIGHT, SearchResultList_Weight[SEARCH_RESULT_ORIG_ARTIST],
                       SEARCH_RESULT_COPYRIGHT_WEIGHT,   SearchResultList_Weight[SEARCH_RESULT_COPYRIGHT],
                       SEARCH_RESULT_URL_WEIGHT,         SearchResultList_Weight[SEARCH_RESULT_URL],
                       SEARCH_RESULT_ENCODED_BY_WEIGHT,  SearchResultList_Weight[SEARCH_RESULT_ENCODED_BY],

                       SEARCH_RESULT_FILENAME_FOREGROUND,    SearchResultList_Color[SEARCH_RESULT_FILENAME],
                       SEARCH_RESULT_TITLE_FOREGROUND,       SearchResultList_Color[SEARCH_RESULT_TITLE],
                       SEARCH_RESULT_ARTIST_FOREGROUND,      SearchResultList_Color[SEARCH_RESULT_ARTIST],
                       SEARCH_RESULT_ALBUM_ARTIST_FOREGROUND,       SearchResultList_Color[SEARCH_RESULT_ALBUM_ARTIST],
		       SEARCH_RESULT_ALBUM_FOREGROUND,       SearchResultList_Color[SEARCH_RESULT_ALBUM],
                       SEARCH_RESULT_DISC_NUMBER_FOREGROUND, SearchResultList_Color[SEARCH_RESULT_DISC_NUMBER],
                       SEARCH_RESULT_YEAR_FOREGROUND,        SearchResultList_Color[SEARCH_RESULT_YEAR],
                       SEARCH_RESULT_TRACK_FOREGROUND,       SearchResultList_Color[SEARCH_RESULT_TRACK],
                       SEARCH_RESULT_GENRE_FOREGROUND,       SearchResultList_Color[SEARCH_RESULT_GENRE],
                       SEARCH_RESULT_COMMENT_FOREGROUND,     SearchResultList_Color[SEARCH_RESULT_COMMENT],
                       SEARCH_RESULT_COMPOSER_FOREGROUND,    SearchResultList_Color[SEARCH_RESULT_COMPOSER],
                       SEARCH_RESULT_ORIG_ARTIST_FOREGROUND, SearchResultList_Color[SEARCH_RESULT_ORIG_ARTIST],
                       SEARCH_RESULT_COPYRIGHT_FOREGROUND,   SearchResultList_Color[SEARCH_RESULT_COPYRIGHT],
                       SEARCH_RESULT_URL_FOREGROUND,         SearchResultList_Color[SEARCH_RESULT_URL],
                       SEARCH_RESULT_ENCODED_BY_FOREGROUND,  SearchResultList_Color[SEARCH_RESULT_ENCODED_BY],

                       SEARCH_RESULT_POINTER, ETFile,
                       -1);

    // Frees allocated data
    g_free(SearchResultList_Text[SEARCH_RESULT_FILENAME]);
    g_free(SearchResultList_Text[SEARCH_RESULT_TITLE]);
    g_free(SearchResultList_Text[SEARCH_RESULT_ARTIST]);
    g_free(SearchResultList_Text[SEARCH_RESULT_ALBUM_ARTIST]);
    g_free(SearchResultList_Text[SEARCH_RESULT_ALBUM]);
    g_free(SearchResultList_Text[SEARCH_RESULT_DISC_NUMBER]);
    g_free(SearchResultList_Text[SEARCH_RESULT_YEAR]);
    g_free(SearchResultList_Text[SEARCH_RESULT_TRACK]);
    g_free(SearchResultList_Text[SEARCH_RESULT_GENRE]);
    g_free(SearchResultList_Text[SEARCH_RESULT_COMMENT]);
    g_free(SearchResultList_Text[SEARCH_RESULT_COMPOSER]);
    g_free(SearchResultList_Text[SEARCH_RESULT_ORIG_ARTIST]);
    g_free(SearchResultList_Text[SEARCH_RESULT_COPYRIGHT]);
    g_free(SearchResultList_Text[SEARCH_RESULT_URL]);
    g_free(SearchResultList_Text[SEARCH_RESULT_ENCODED_BY]);
}

/*
 * This function and the one below could do with improving
 * as we are looking up tag data twice (once when searching, once when adding to list)
 */
static void
Search_File (GtkWidget *search_button, gpointer user_data)
{
    EtSearchDialog *self;
    EtSearchDialogPrivate *priv;
    const gchar *string_to_search = NULL;
    GList *l;
    ET_File *ETFile;
    gchar *msg;
    gchar *temp = NULL;
    gchar *title2, *artist2, *album_artist2, *album2, *disc_number2,
          *disc_total2, *year2, *track2, *track_total2, *genre2, *comment2,
          *composer2, *orig_artist2, *copyright2, *url2, *encoded_by2,
          *string_to_search2;
    gint resultCount = 0;

    self = ET_SEARCH_DIALOG (user_data);
    priv = et_search_dialog_get_instance_private (self);

    if (!priv->search_string_combo || !priv->search_in_filename || !priv->search_in_tag || !priv->search_results_view)
        return;

    string_to_search = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->search_string_combo))));
    if (!string_to_search)
        return;

    Add_String_To_Combo_List (priv->search_string_model, string_to_search);

    gtk_widget_set_sensitive(GTK_WIDGET(search_button),FALSE);
    gtk_list_store_clear(priv->search_results_model);
    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,"");

    for (l = g_list_first (ETCore->ETFileList); l != NULL; l = g_list_next (l))
    {
        ETFile = (ET_File *)l->data;

        // Search in the filename
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->search_in_filename)))
        {
            gchar *filename_utf8 = ((File_Name *)ETFile->FileNameNew->data)->value_utf8;
            gchar *basename_utf8;

            // To search without case sensivity
            if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->search_case_sensitive)))
            {
                temp = g_path_get_basename(filename_utf8);
                basename_utf8 = g_utf8_casefold(temp, -1);
                g_free(temp);
                string_to_search2 = g_utf8_casefold(string_to_search, -1);
            } else
            {
                basename_utf8 = g_path_get_basename(filename_utf8);
                string_to_search2 = g_strdup(string_to_search);
            }

            if ( basename_utf8 && strstr(basename_utf8,string_to_search2) )
            {
                Add_Row_To_Search_Result_List (self, ETFile,
                                               string_to_search2);
                g_free(basename_utf8);
                g_free(string_to_search2);
                continue;
            }
            g_free(basename_utf8);
            g_free(string_to_search2);
        }

        // Search in the tag
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->search_in_tag)))
        {
            File_Tag *FileTag   = (File_Tag *)ETFile->FileTag->data;

            // To search with or without case sensivity
            if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->search_case_sensitive)))
            {
                // To search without case sensivity...
                // Duplicate and convert the strings into UTF-8 in loxer case
                if (FileTag->title)       title2       = g_utf8_casefold(FileTag->title, -1);        else title2       = NULL;
                if (FileTag->artist)      artist2      = g_utf8_casefold(FileTag->artist, -1);       else artist2      = NULL;
                if (FileTag->album_artist) album_artist2 = g_utf8_casefold(FileTag->album_artist, -1); else album_artist2= NULL;
                if (FileTag->album)       album2       = g_utf8_casefold(FileTag->album, -1);        else album2       = NULL;
                if (FileTag->disc_number) disc_number2 = g_utf8_casefold(FileTag->disc_number, -1);  else disc_number2 = NULL;
                if (FileTag->disc_total)
                {
                    disc_total2 = g_utf8_casefold (FileTag->disc_total, -1);
                }
                else
                {
                    disc_total2 = NULL;
                }
                if (FileTag->year)        year2        = g_utf8_casefold(FileTag->year, -1);         else year2        = NULL;
                if (FileTag->track)       track2       = g_utf8_casefold(FileTag->track, -1);        else track2       = NULL;
                if (FileTag->track_total) track_total2 = g_utf8_casefold(FileTag->track_total, -1);  else track_total2 = NULL;
                if (FileTag->genre)       genre2       = g_utf8_casefold(FileTag->genre, -1);        else genre2       = NULL;
                if (FileTag->comment)     comment2     = g_utf8_casefold(FileTag->comment, -1);      else comment2     = NULL;
                if (FileTag->composer)    composer2    = g_utf8_casefold(FileTag->composer, -1);     else composer2    = NULL;
                if (FileTag->orig_artist) orig_artist2 = g_utf8_casefold(FileTag->orig_artist, -1);  else orig_artist2 = NULL;
                if (FileTag->copyright)   copyright2   = g_utf8_casefold(FileTag->copyright, -1);    else copyright2   = NULL;
                if (FileTag->url)         url2         = g_utf8_casefold(FileTag->url, -1);          else url2         = NULL;
                if (FileTag->encoded_by)  encoded_by2  = g_utf8_casefold(FileTag->encoded_by, -1);   else encoded_by2  = NULL;
                string_to_search2 = g_utf8_strdown(string_to_search, -1);
            }else
            {
                // To search with case sensivity...
                // Duplicate and convert the strings into UTF-8
                title2       = g_strdup(FileTag->title);
                artist2      = g_strdup(FileTag->artist);
                album_artist2= g_strdup(FileTag->album_artist);
                album2       = g_strdup(FileTag->album);
                disc_number2 = g_strdup(FileTag->disc_number);
                disc_total2 = g_strdup (FileTag->disc_total);
                year2        = g_strdup(FileTag->year);
                track2       = g_strdup(FileTag->track);
                track_total2 = g_strdup(FileTag->track_total);
                genre2       = g_strdup(FileTag->genre);
                comment2     = g_strdup(FileTag->comment);
                composer2    = g_strdup(FileTag->composer);
                orig_artist2 = g_strdup(FileTag->orig_artist);
                copyright2   = g_strdup(FileTag->copyright);
                url2         = g_strdup(FileTag->url);
                encoded_by2  = g_strdup(FileTag->encoded_by);
                string_to_search2 = g_strdup(string_to_search);
            }

            // FIX ME : should use UTF-8 functions?
            if ( (title2       && strstr(title2,       string_to_search2) )
             ||  (artist2      && strstr(artist2,      string_to_search2) )
             ||  (album_artist2 && strstr(album_artist2,string_to_search2) )
             ||  (album2       && strstr(album2,       string_to_search2) )
             ||  (disc_number2 && strstr(disc_number2, string_to_search2) )
             ||  (disc_total2 && strstr (disc_total2, string_to_search2))
             ||  (year2        && strstr(year2,        string_to_search2) )
             ||  (track2       && strstr(track2,       string_to_search2) )
             ||  (track_total2 && strstr(track_total2, string_to_search2) )
             ||  (genre2       && strstr(genre2,       string_to_search2) )
             ||  (comment2     && strstr(comment2,     string_to_search2) )
             ||  (composer2    && strstr(composer2,    string_to_search2) )
             ||  (orig_artist2 && strstr(orig_artist2, string_to_search2) )
             ||  (copyright2   && strstr(copyright2,   string_to_search2) )
             ||  (url2         && strstr(url2,         string_to_search2) )
             ||  (encoded_by2  && strstr(encoded_by2,  string_to_search2) ) )
            {
                Add_Row_To_Search_Result_List (self, ETFile, string_to_search);
            }
            g_free(title2);
            g_free(artist2);
            g_free(album_artist2);
            g_free(album2);
            g_free(disc_number2);
            g_free (disc_total2);
            g_free(year2);
            g_free(track2);
            g_free(track_total2);
            g_free(genre2);
            g_free(comment2);
            g_free(composer2);
            g_free(orig_artist2);
            g_free(copyright2);
            g_free(url2);
            g_free(encoded_by2);
            g_free(string_to_search2);
        }
    }

    gtk_widget_set_sensitive(GTK_WIDGET(search_button),TRUE);

    // Display the number of files in the statusbar
    resultCount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->search_results_model), NULL);
    msg = g_strdup_printf (ngettext ("Found one file", "Found %d files",
                           resultCount), resultCount);
    gtk_statusbar_push(GTK_STATUSBAR(priv->status_bar),priv->status_bar_context,msg);
    g_free(msg);

    // Disable result list if no row inserted
    if (resultCount > 0 )
    {
        gtk_widget_set_sensitive(GTK_WIDGET(priv->search_results_view), TRUE);
    } else
    {
        gtk_widget_set_sensitive(GTK_WIDGET(priv->search_results_view), FALSE);
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

    return TRUE;
}

/*
 * The window to search keywords in the list of files.
 */
static void
create_search_dialog (EtSearchDialog *self)
{
    EtSearchDialogPrivate *priv;
    GtkWidget *content_area;
    GtkBuilder *builder;
    GError *error = NULL;
    GtkWidget *grid;
    GtkWidget *button;

    priv = et_search_dialog_get_instance_private (self);

    gtk_window_set_title (GTK_WINDOW (self), _("Find Files"));
    g_signal_connect (self, "delete-event", G_CALLBACK (on_delete_event),
                      NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (self));
    gtk_box_set_spacing (GTK_BOX (content_area), BOX_SPACING);
    gtk_container_set_border_width (GTK_CONTAINER (self),
                                    BOX_SPACING);

    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder,
                                   "/org/gnome/EasyTAG/search_dialog.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get search dialog from resource: %s",
                 error->message);
    }

    grid = GTK_WIDGET (gtk_builder_get_object (builder, "search_grid"));
    gtk_container_add (GTK_CONTAINER (content_area), grid);

    /* Words to search. */
    priv->search_string_model = gtk_list_store_new (MISC_COMBO_COUNT,
                                                    G_TYPE_STRING);
    priv->search_string_combo = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                    "search_combo"));
    gtk_combo_box_set_model (GTK_COMBO_BOX (priv->search_string_combo),
                             GTK_TREE_MODEL (priv->search_string_model));
    g_object_unref (priv->search_string_model);
    /* History List. */
    Load_Search_File_List (priv->search_string_model, MISC_COMBO_TEXT);
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->search_string_combo))),
                        "");

    /* Set content of the clipboard if available. */
    gtk_editable_paste_clipboard (GTK_EDITABLE (gtk_bin_get_child (GTK_BIN (priv->search_string_combo))));

    priv->search_in_filename = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                   "search_filename_check"));
    priv->search_in_tag = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "search_tag_check"));
    g_settings_bind (MainSettings, "search-filename", priv->search_in_filename,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "search-tag", priv->search_in_tag,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Property of the search. */
    priv->search_case_sensitive = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                      "search_case_check"));
    g_settings_bind (MainSettings, "search-case-sensitive",
                     priv->search_case_sensitive, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Results list. */
    priv->search_results_model = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                                         "search_model"));
    priv->search_results_view = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                    "search_view"));

    g_signal_connect (gtk_builder_get_object (builder, "search_selection"),
                      "changed", G_CALLBACK (Search_Result_List_Row_Selected),
                      self);

    /* Button to run the search. */
    button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                 "search_find_button"));
    gtk_widget_grab_default (button);
    g_signal_connect (button, "clicked", G_CALLBACK (Search_File), self);
    g_signal_connect (gtk_bin_get_child (GTK_BIN (priv->search_string_combo)),
                      "activate", G_CALLBACK (Search_File), self);

    /* Button to cancel. */
    button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                 "search_close_button"));
    g_signal_connect (button, "clicked", G_CALLBACK (on_close_clicked), self);

    /* Status bar. */
    priv->status_bar = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "search_status"));
    priv->status_bar_context = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->status_bar),
                                                             "Messages");
    gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                        priv->status_bar_context, _("Ready to search…"));

    g_object_unref (builder);
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
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, ET_TYPE_SEARCH_DIALOG,
                                              EtSearchDialogPrivate);

    create_search_dialog (self);
}

static void
et_search_dialog_class_init (EtSearchDialogClass *klass)
{
    g_type_class_add_private (klass, sizeof (EtSearchDialogPrivate));
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
