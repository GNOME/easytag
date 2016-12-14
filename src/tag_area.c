/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014,2015  David King <amigadave@amigadave.com>
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

#include "tag_area.h"

#include <glib/gi18n.h>

#include "application_window.h"
#include "charset.h"
#include "easytag.h"
#include "file_list.h"
#include "genres.h"
#include "log.h"
#include "misc.h"
#include "picture.h"
#include "scan.h"
#include "scan_dialog.h"

typedef struct
{
    GtkWidget *tag_label;
    GtkWidget *tag_notebook;

    GtkWidget *common_grid;

    GtkWidget *title_label;
    GtkWidget *title_entry;
    GtkWidget *artist_label;
    GtkWidget *artist_entry;
    GtkWidget *album_artist_label;
    GtkWidget *album_artist_entry;
    GtkWidget *album_label;
    GtkWidget *album_entry;
    GtkWidget *disc_number_label;
    GtkWidget *disc_number_entry;
    GtkWidget *year_label;
    GtkWidget *year_entry;
    GtkWidget *track_label;
    GtkWidget *track_combo_entry;
    GtkWidget *track_total_entry;
    GtkWidget *genre_label;
    GtkWidget *genre_combo_entry;
    GtkWidget *comment_label;
    GtkWidget *comment_entry;
    GtkWidget *composer_label;
    GtkWidget *composer_entry;
    GtkWidget *orig_artist_label;
    GtkWidget *orig_artist_entry;
    GtkWidget *copyright_label;
    GtkWidget *copyright_entry;
    GtkWidget *url_label;
    GtkWidget *url_entry;
    GtkWidget *encoded_by_label;
    GtkWidget *encoded_by_entry;

    GtkListStore *genre_combo_model;
    GtkListStore *track_combo_model;

    GtkWidget *images_view;

    /* Other for picture. */
    GtkWidget *remove_image_toolitem;
    GtkWidget *add_image_toolitem;
    GtkWidget *save_image_toolitem;
    GtkWidget *image_properties_toolitem;

    /* Notebook tabs. */
    GtkWidget *images_grid;

    /* Image treeview model. */
    GtkListStore *images_model;

    /* Mini buttons. */
    GtkWidget *track_sequence_button;
    GtkWidget *track_number_button;
    GtkWidget *apply_image_toolitem;
} EtTagAreaPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtTagArea, et_tag_area, GTK_TYPE_BIN)

enum
{
    TRACK_COLUMN_TRACK_NUMBER,
    TRACK_COLUMN_COUNT
};

enum
{
    GENRE_COLUMN_GENRE,
    GENRE_COLUMN_COUNT
};

enum
{
    TARGET_URI_LIST
};

enum /* Columns for images_view. */
{
    PICTURE_COLUMN_SURFACE,
    PICTURE_COLUMN_TEXT,
    PICTURE_COLUMN_DATA,
    PICTURE_COLUMN_COUNT
};

enum /* Columns for list in properties window. */
{
    PICTURE_TYPE_COLUMN_TEXT,
    PICTURE_TYPE_COLUMN_TYPE_CODE,
    PICTURE_TYPE_COLUMN_COUNT
};

static void
on_apply_to_selection (GObject *object,
                       EtTagArea *self)
{
    EtTagAreaPrivate *priv;
    EtApplicationWindow *window;
    GList *etfilelist;
    GList *l;
    const gchar *string_to_set;
    const gchar *string_to_set1;
    gchar *msg = NULL;
    ET_File *etfile;
    File_Tag *FileTag;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    priv = et_tag_area_get_instance_private (self);

    window = ET_APPLICATION_WINDOW (MainWindow);

    et_application_window_update_et_file_from_ui (window);

    etfilelist = et_application_window_browser_get_selected_files (window);

    if (object == G_OBJECT (priv->title_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->title_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_title (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with title ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed title from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->artist_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->artist_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_artist (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with artist ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed artist from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->album_artist_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->album_artist_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_album_artist (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with album artist ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed album artist from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->album_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->album_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_album (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with album ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed album name from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->disc_number_entry))
    {
        gchar *separator;
        gchar *disc_number = NULL;

        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->disc_number_entry));
        /* strchr() does not allocate a new string, and should probably return
         * a const gchar *. */
        separator = strchr (string_to_set, '/');

        if (separator)
        {
            string_to_set1 = separator + 1;
            disc_number = g_strndup (string_to_set, separator - string_to_set);
        }
        else
        {
            string_to_set1 = NULL;
        }

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_disc_number (FileTag, disc_number ? disc_number
                                                              : string_to_set);
            et_file_tag_set_disc_total (FileTag, string_to_set1);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            if (!et_str_empty (string_to_set1))
            {
                msg = g_strdup_printf (_("Selected files tagged with disc number ‘%s/%s’"),
                                       disc_number ? disc_number
                                                   : string_to_set,
                                       string_to_set1);
            }
            else
            {
                msg = g_strdup_printf (_("Selected files tagged with disc number like ‘xx’"));
            }
        }
        else
        {
            msg = g_strdup (_("Removed disc number from selected files"));
        }

        g_free (disc_number);
    }
    else if (object == G_OBJECT (priv->year_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->year_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_year (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with year ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed year from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->track_total_entry))
    {
        /* Used of Track and Total Track values */
        string_to_set = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->track_combo_entry))));
        string_to_set1 = gtk_entry_get_text (GTK_ENTRY (priv->track_total_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);

            // We apply the TrackEntry field to all others files only if it is to delete
            // the field (string=""). Else we don't overwrite the track number
            if (et_str_empty (string_to_set))
            {
                et_file_tag_set_track_number (FileTag, string_to_set);
            }

            et_file_tag_set_track_total (FileTag, string_to_set1);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            if (!et_str_empty (string_to_set1))
            {
                msg = g_strdup_printf (_("Selected files tagged with track like ‘xx/%s’"),
                                       string_to_set1);
            }else
            {
                msg = g_strdup_printf (_("Selected files tagged with track like ‘xx’"));
            }
        }
        else
        {
            msg = g_strdup (_("Removed track number from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->track_sequence_button))
    {
        /* This part doesn't set the same track number to all files, but sequence the tracks.
         * So we must browse the whole 'etfilelistfull' to get position of each selected file.
         * Note : 'etfilelistfull' and 'etfilelist' must be sorted in the same order */
        GList *etfilelistfull = NULL;
        gint sort_mode;
        gchar *path = NULL;
        gchar *path1 = NULL;
        gint i = 0;

        /* FIX ME!: see to fill also the Total Track (it's a good idea?) */
        etfilelistfull = ETCore->ETFileList;

        /* Sort 'etfilelistfull' and 'etfilelist' in the same order. */
        sort_mode = g_settings_get_enum (MainSettings, "sort-mode");
        etfilelist = ET_Sort_File_List (etfilelist, sort_mode);
        etfilelistfull = ET_Sort_File_List (etfilelistfull, sort_mode);

        while (etfilelist && etfilelistfull)
        {
            gchar *track_string;
            // To get the path of the file
            const File_Name *FileNameCur = (File_Name *)((ET_File *)etfilelistfull->data)->FileNameCur->data;
            // The ETFile in the selected file list
            etfile = etfilelist->data;

            // Restart counter when entering a new directory
            g_free(path1);
            path1 = g_path_get_dirname(FileNameCur->value);
            if ( path && path1 && strcmp(path,path1)!=0 )
                i = 0;

            track_string = et_track_number_to_string (++i);

            // The file is in the selection?
            if ( (ET_File *)etfilelistfull->data == etfile )
            {
                FileTag = et_file_tag_new ();
                et_file_tag_copy_into (FileTag, etfile->FileTag->data);
                et_file_tag_set_track_number (FileTag, track_string);
                ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

                if (!etfilelist->next) break;
                etfilelist = g_list_next(etfilelist);
            }

            g_free (track_string);
            g_free(path);
            path = g_strdup(path1);

            etfilelistfull = g_list_next(etfilelistfull);
        }
        g_free(path);
        g_free(path1);
        //msg = g_strdup_printf(_("All %d tracks numbered sequentially."), ETCore->ETFileSelectionList_Length);
        msg = g_strdup_printf (_("Selected tracks numbered sequentially"));
    }
    else if (object==G_OBJECT(priv->track_number_button))
    {
        gchar *track_total = NULL;

        /* Used of Track and Total Track values */
        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            const gchar *filename_utf8;
            gchar *path_utf8;
            gchar *track_string;

            etfile        = (ET_File *)l->data;
            filename_utf8 = ((File_Name *)etfile->FileNameNew->data)->value_utf8;
            path_utf8     = g_path_get_dirname(filename_utf8);

            track_string = et_track_number_to_string (et_file_list_get_n_files_in_path (ETCore->ETFileList, path_utf8));

            g_free (path_utf8);

            if (!track_total)
            {
                /* Just for the message below, and only the first directory. */
                track_total = g_strdup (track_string);
            }

            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_track_total (FileTag, track_string);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            g_free (track_string);
        }

        if (!et_str_empty (track_total))
        {
            msg = g_strdup_printf (_("Selected files tagged with track like ‘xx/%s’"),
                                   track_total);
        }
        else
        {
            msg = g_strdup (_("Removed track number from selected files"));
        }

        g_free (track_total);
    }
    else if (object == G_OBJECT (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry))))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry))));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_genre (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with genre ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed genre from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->comment_entry))
    {
        //GtkTextBuffer *textbuffer;
        //GtkTextIter    start_iter;
        //GtkTextIter    end_iter;
        //textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(CommentView));
        //gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(textbuffer),&start_iter,&end_iter);
        //string_to_set = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(textbuffer),&start_iter,&end_iter,TRUE);

        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->comment_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_comment (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with comment ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed comment from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->composer_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->composer_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_composer (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with composer ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed composer from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->orig_artist_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->orig_artist_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_orig_artist (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with original artist ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed original artist from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->copyright_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->copyright_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_copyright (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with copyright ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed copyright from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->url_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY( priv->url_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_url (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with URL ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed URL from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->encoded_by_entry))
    {
        string_to_set = gtk_entry_get_text (GTK_ENTRY (priv->encoded_by_entry));

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_encoded_by (FileTag, string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (!et_str_empty (string_to_set))
        {
            msg = g_strdup_printf (_("Selected files tagged with encoder name ‘%s’"),
                                   string_to_set);
        }
        else
        {
            msg = g_strdup (_("Removed encoder name from selected files"));
        }
    }
    else if (object == G_OBJECT (priv->apply_image_toolitem))
    {
        EtPicture *res = NULL, *pic, *prev_pic = NULL;
        GtkTreeModel *model;
        GtkTreeIter iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->images_view));

        if (gtk_tree_model_get_iter_first(model, &iter))
        {
            do
            {
                gtk_tree_model_get (model, &iter, PICTURE_COLUMN_DATA, &pic,
                                    -1);

                if (!res)
                    res = pic;
                else
                    prev_pic->next = pic;
                prev_pic = pic;
            } while (gtk_tree_model_iter_next(model, &iter));
        }

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = et_file_tag_new ();
            et_file_tag_copy_into (FileTag, etfile->FileTag->data);
            et_file_tag_set_picture (FileTag, res);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (res)
        {
            msg = g_strdup (_("Selected files tagged with images"));
        }
        else
        {
            msg = g_strdup (_("Removed images from selected files"));
        }

        et_picture_free (res);
    }

    g_list_free(etfilelist);

    /* Refresh the whole list (faster than file by file) to show changes. */
    et_application_window_browser_refresh_list (window);

    /* Display the current file (Needed when sequencing tracks) */
    et_application_window_display_et_file (window, ETCore->ETFileDisplayed);

    if (msg)
    {
        Log_Print(LOG_OK,"%s",msg);
        et_application_window_status_bar_message (window, msg,TRUE);
        g_free(msg);
    }

    /* To update state of Undo button */
    et_application_window_update_actions (window);
}

static void
on_entry_icon_release (GtkEntry *entry,
                       GtkEntryIconPosition icon_pos,
                       GdkEvent *event,
                       EtTagArea *self)
{
    on_apply_to_selection (G_OBJECT (entry), self);
}

static void
Convert_P20_And_Underscore_Into_Spaces (GtkWidget *entry)
{
    gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    Scan_Convert_Underscore_Into_Space (string);
    Scan_Convert_P20_Into_Space (string);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_Space_Into_Underscore (GtkWidget *entry)
{
    gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    Scan_Convert_Space_Into_Underscore (string);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_All_Uppercase (GtkWidget *entry)
{
    gchar *res;
    const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));

    res = Scan_Process_Fields_All_Uppercase (string);
    gtk_entry_set_text (GTK_ENTRY (entry), res);
    g_free (res);
}

static void
Convert_All_Lowercase (GtkWidget *entry)
{
    gchar *res;
    const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));

    res = Scan_Process_Fields_All_Downcase (string);
    gtk_entry_set_text (GTK_ENTRY (entry), res);
    g_free (res);
}

static void
Convert_Letter_Uppercase (GtkWidget *entry)
{
    gchar *res;
    const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));

    res = Scan_Process_Fields_Letter_Uppercase (string);
    gtk_entry_set_text (GTK_ENTRY (entry), res);
    g_free (res);
}

static void
Convert_First_Letters_Uppercase (GtkWidget *entry)
{
    gchar *string;
    gboolean uppercase_preps;
    gboolean handle_roman;

    string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
    uppercase_preps = g_settings_get_boolean (MainSettings,
                                              "process-uppercase-prepositions");
    handle_roman = g_settings_get_boolean (MainSettings,
                                           "process-detect-roman-numerals");

    Scan_Process_Fields_First_Letters_Uppercase (&string, uppercase_preps,
                                                 handle_roman);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_Remove_Space (GtkWidget *entry)
{
    gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    Scan_Process_Fields_Remove_Space (string);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_Insert_Space (GtkWidget *entry)
{
    gchar *res;
    const gchar *string = (gtk_entry_get_text (GTK_ENTRY (entry)));

    res = Scan_Process_Fields_Insert_Space (string);
    gtk_entry_set_text (GTK_ENTRY (entry), res);
    g_free (res);
}

static void
Convert_Only_One_Space (GtkWidget *entry)
{
    gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    Scan_Process_Fields_Keep_One_Space (string);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_Remove_All_Text (GtkWidget *entry)
{
    gtk_entry_set_text (GTK_ENTRY (entry), "");
}

static void
on_apply_to_selection_menu_item (GObject *entry,
                                 GtkMenuItem *menu_item)
{
    EtTagArea *self;

    self = g_object_get_data (G_OBJECT (menu_item), "tag-area");

    on_apply_to_selection (entry, self);
}

/* TODO: Support populate-all and do not assume the widget is a GtkMenu.
 * Popup menu attached to all entries of tag + filename + rename combobox.
 * Displayed when pressing the right mouse button and contains functions to process ths strings.
 */
void
on_entry_populate_popup (GtkEntry *entry,
                         GtkWidget *menu,
                         EtTagArea *self)
{
    GtkWidget *menu_item;
    GtkWidget *label;

    /* Menu items */
    menu_item = gtk_menu_item_new_with_label (_("Tag selected files with this field"));
    label = gtk_bin_get_child (GTK_BIN (menu_item));
    gtk_accel_label_set_accel (GTK_ACCEL_LABEL (label), GDK_KEY_Return,
                               GDK_CONTROL_MASK);
    g_object_set_data (G_OBJECT (menu_item), "tag-area", self);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (on_apply_to_selection_menu_item),
                              G_OBJECT (entry));

    /* Separator */
    menu_item = gtk_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    menu_item = gtk_menu_item_new_with_label (_("Convert ‘_’ and ‘%20’ to spaces"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_P20_And_Underscore_Into_Spaces),
                              G_OBJECT (entry));

    menu_item = gtk_menu_item_new_with_label (_("Convert spaces to underscores"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Space_Into_Underscore),
                              G_OBJECT (entry));

    /* Separator */
    menu_item = gtk_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    menu_item = gtk_menu_item_new_with_label (_("All uppercase"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_All_Uppercase),
                              G_OBJECT (entry));

    menu_item = gtk_menu_item_new_with_label (_("All lowercase"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_All_Lowercase),
                              G_OBJECT (entry));

    menu_item = gtk_menu_item_new_with_label (_("First letter uppercase"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Letter_Uppercase),
                              G_OBJECT (entry));

    menu_item = gtk_menu_item_new_with_label (_("First letter uppercase of each word"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_First_Letters_Uppercase),
                              G_OBJECT (entry));

    /* Separator */
    menu_item = gtk_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    menu_item = gtk_menu_item_new_with_label (_("Remove spaces"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Remove_Space),
                              G_OBJECT (entry));

    menu_item = gtk_menu_item_new_with_label (_("Insert space before uppercase letter"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Insert_Space),
                              G_OBJECT (entry));

    menu_item = gtk_menu_item_new_with_label (_("Remove duplicate spaces or underscores"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Only_One_Space),
                              G_OBJECT (entry));

    menu_item = gtk_menu_item_new_with_label (_("Remove all text"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Remove_All_Text),
                              G_OBJECT (entry));

    gtk_widget_show_all (menu);
}

/*
 * et_tag_field_on_key_press_event:
 * @entry: the tag entry field on which the event was generated
 * @event: the generated event
 * @user_data: user data set when the signal was connected
 *
 * Handle the Ctrl+Return combination being pressed in the tag field GtkEntrys
 * and apply the tag to selected files.
 *
 * Returns: %TRUE if the event was handled, %FALSE if the event should
 * propagate further
 */
static gboolean
et_tag_field_on_key_press_event (GtkEntry *entry, GdkEventKey *event,
                                 gpointer user_data)
{
    GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask ();

    switch (event->keyval)
    {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
            if ((event->state & modifiers) == GDK_CONTROL_MASK)
            {
                on_apply_to_selection (G_OBJECT (entry),
                                       ET_TAG_AREA (user_data));
            }
            return TRUE;
        default:
            return FALSE;
    }
}

/*
 * et_tag_field_connect_signals:
 * @entry: the entry for which to connect signals
 *
 * Connect the GtkWidget::key-press-event and GtkEntry::icon-release signals
 * of @entry to appropriate handlers for tag entry fields.
 */
static void
et_tag_field_connect_signals (GtkEntry *entry,
                              EtTagArea *self)
{
    g_signal_connect_after (entry, "key-press-event",
                            G_CALLBACK (et_tag_field_on_key_press_event),
                            self);
    g_signal_connect (entry, "icon-release",
                      G_CALLBACK (on_entry_icon_release),
                      self);
    g_signal_connect (entry, "populate-popup",
                      G_CALLBACK (on_entry_populate_popup), self);
}

/*
 * Load the genres list to the combo, and sorts it
 */
static void
populate_genre_combo (EtTagArea *self)
{
    EtTagAreaPrivate *priv;
    gsize i;

    priv = et_tag_area_get_instance_private (self);

    gtk_list_store_insert_with_values (priv->genre_combo_model, NULL,
                                       G_MAXINT, GENRE_COLUMN_GENRE, "", -1);
    gtk_list_store_insert_with_values (priv->genre_combo_model, NULL,
                                       G_MAXINT, GENRE_COLUMN_GENRE, "Unknown",
                                       -1);

    for (i = 0; i <= GENRE_MAX; i++)
    {
        gtk_list_store_insert_with_values (priv->genre_combo_model, NULL,
                                           G_MAXINT, GENRE_COLUMN_GENRE,
                                           id3_genres[i], -1);
    }
}

/*
 * Load the track numbers into the track combo list
 * We limit the preloaded values to 30 to avoid lost of time with lot of files...
 */
static void
populate_track_combo (EtTagArea *self)
{
    EtTagAreaPrivate *priv;
    /* Length limited to 30 (instead to the number of files)! */
    const gsize len = 30;
    gsize i;
    gchar *text;

    priv = et_tag_area_get_instance_private (self);

    /* Remove the entries in the list to avoid duplicates. */
    gtk_list_store_clear (priv->track_combo_model);

    /* Create list of tracks. */
    for (i = 1; i <= len; i++)
    {
        text = et_track_number_to_string (i);

        gtk_list_store_insert_with_values (priv->track_combo_model, NULL,
                                           G_MAXINT, TRACK_COLUMN_TRACK_NUMBER,
                                           text, -1);
        g_free (text);
    }
}

/*
 * Iter compare func: Sort alphabetically
 */
static gint
tree_iter_alphabetical_sort (GtkTreeModel *model,
                             GtkTreeIter *a,
                             GtkTreeIter *b,
                             gpointer data)
{
    gchar *text1, *text1_folded;
    gchar *text2, *text2_folded;
    gint ret;

    gtk_tree_model_get (model, a, GENRE_COLUMN_GENRE, &text1, -1);
    gtk_tree_model_get (model, b, GENRE_COLUMN_GENRE, &text2, -1);

    if (text1 == text2)
    {
        g_free (text1);
        g_free (text2);
        return 0;
    }

    if (text1 == NULL)
    {
        g_free (text2);
        return -1;
    }

    if (text2 == NULL)
    {
        g_free (text1);
        return 1;
    }

    text1_folded = g_utf8_casefold (text1, -1);
    text2_folded = g_utf8_casefold (text2, -1);
    ret = g_utf8_collate (text1_folded, text2_folded);

    g_free (text1);
    g_free (text2);
    g_free (text1_folded);
    g_free (text2_folded);

    return ret;
}

/*
 * To insert only digits in an entry. If the text contains only digits: returns it,
 * else only first digits.
 */
static void
Insert_Only_Digit (GtkEditable *editable,
                   const gchar *inserted_text,
                   gint length,
                   gint *position,
                   gpointer data)
{
    int i = 1; // Ignore first character
    int j = 1;
    gchar *result;

    if (length<=0 || !inserted_text)
        return;

    if (!g_ascii_isdigit (inserted_text[0]) && inserted_text[0] != '-')
    {
        g_signal_stop_emission_by_name(G_OBJECT(editable),"insert_text");
        return;
    } else if (length == 1)
    {
        // We already checked the first digit...
        return;
    }

    g_signal_stop_emission_by_name(G_OBJECT(editable),"insert_text");
    result = g_malloc0(length+1);
    result[0] = inserted_text[0];

    // Check the rest, if any...
    for (i = 1; i < length; i++)
    {
        if (g_ascii_isdigit (inserted_text[i]))
        {
            result[j++] = inserted_text[i];
        }
    }
    // Null terminate for the benefit of glib/gtk
    result[j] = '\0';

    if (result[0] == '\0')
    {
        g_free(result);
        return;
    }

    g_signal_handlers_block_by_func(G_OBJECT(editable),G_CALLBACK(Insert_Only_Digit),data);
    gtk_editable_insert_text(editable, result, j, position);
    g_signal_handlers_unblock_by_func(G_OBJECT(editable),G_CALLBACK(Insert_Only_Digit),data);
    g_free(result);
}

/*
 * Parse and auto complete date entry if you don't type the 4 digits.
 */
#include <stdlib.h>
static void
Parse_Date (EtTagArea *self)
{
    EtTagAreaPrivate *priv;
    const gchar *year;
    gchar *current_year;

    priv = et_tag_area_get_instance_private (self);

    /* Early return. */
    if (!g_settings_get_boolean (MainSettings, "tag-date-autocomplete"))
    {
        return;
    }

    /* Get the info entered by user */
    year = gtk_entry_get_text (GTK_ENTRY (priv->year_entry));

    if (!et_str_empty (year) && strlen (year) < 4)
    {
        GDateTime *dt;
        gchar *tmp, *tmp1;

        dt = g_date_time_new_now_local ();
        current_year = g_date_time_format (dt, "%Y");
        g_date_time_unref (dt);

        tmp = &current_year[4-strlen(year)];

        if (atoi (year) <= atoi (tmp))
        {
            sprintf (current_year, "%d", atoi (current_year) - atoi (tmp));
            tmp1 = g_strdup_printf ("%d", atoi (current_year) + atoi (year));
            gtk_entry_set_text (GTK_ENTRY (priv->year_entry), tmp1);
            g_free (tmp1);
        }
        else
        {
            sprintf (current_year, "%d", atoi (current_year) - atoi (tmp)
                     - (strlen (year) <= 0 ? 1 : strlen (year) <= 1 ? 10 :          // pow(10,strlen(year)) returns 99 instead of 100 under Win32...
                     strlen (year) <= 2 ? 100 : strlen (year) <= 3 ? 1000 : 0));
            tmp1 = g_strdup_printf ("%d", atoi (current_year) + atoi (year));
            gtk_entry_set_text (GTK_ENTRY (priv->year_entry), tmp1);
            g_free (tmp1);
        }

        g_free (current_year);
    }
}

static gboolean
on_year_entry_focus_out_event (GtkWidget *widget,
                               GdkEvent *event,
                               gpointer user_data)
{
    Parse_Date (ET_TAG_AREA (user_data));

    return GDK_EVENT_PROPAGATE;
}

static void
on_year_entry_activate (GtkEntry *entry,
                        gpointer user_data)
{
    Parse_Date (ET_TAG_AREA (user_data));
}

static void
on_picture_view_selection_changed (GtkTreeSelection *selection,
                                   gpointer user_data)
{
    EtTagArea *self;
    EtTagAreaPrivate *priv;

    self = ET_TAG_AREA (user_data);
    priv = et_tag_area_get_instance_private (self);

    if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (selection)) >= 1)
    {
        gtk_widget_set_sensitive (GTK_WIDGET (priv->remove_image_toolitem),
                                  TRUE);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->save_image_toolitem),
                                  TRUE);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->image_properties_toolitem),
                                  TRUE);
    }
    else
    {
        gtk_widget_set_sensitive (GTK_WIDGET (priv->remove_image_toolitem),
                                  FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->save_image_toolitem),
                                  FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->image_properties_toolitem),
                                  FALSE);
    }
}

static void
PictureEntry_Clear (EtTagArea *self)
{
    EtTagAreaPrivate *priv;

    priv = et_tag_area_get_instance_private (self);

    gtk_list_store_clear (priv->images_model);
}

static void
PictureEntry_Update (EtTagArea *self,
                     EtPicture *pic,
                     gboolean select_it)
{
    EtTagAreaPrivate *priv;
    GdkPixbufLoader *loader = 0;
    GError *error = NULL;
    
    g_return_if_fail (pic != NULL);

    priv = et_tag_area_get_instance_private (self);

    if (g_bytes_get_size (pic->bytes) == 0)
    {
        goto next;
    }

    loader = gdk_pixbuf_loader_new ();

    if (loader)
    {
        if (gdk_pixbuf_loader_write_bytes (loader, pic->bytes, &error))
        {
            GtkTreeSelection *selection;
            GdkPixbuf *pixbuf;

            if (!gdk_pixbuf_loader_close(loader, &error))
            {
                Log_Print (LOG_ERROR, _("Error parsing image data ‘%s’"),
                           error->message);
                g_error_free (error);
            }

            selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->images_view));

            pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
            if (pixbuf)
            {
                GtkTreeIter iter1;
                gint scale_factor;
                GdkPixbuf *scaled_pixbuf;
                cairo_surface_t *surface;
                GdkWindow *view_window;
                gint scaled_pixbuf_width;
                gint scaled_pixbuf_height;
                gchar *pic_info;

                g_object_ref(pixbuf);
                g_object_unref(loader);
                
                // Keep aspect ratio of the picture
                pic->width  = gdk_pixbuf_get_width(pixbuf);
                pic->height = gdk_pixbuf_get_height(pixbuf);
                /* TODO: Connect to notify:scale-factor and update when the
                 * scale changes. */
                scale_factor = gtk_widget_get_scale_factor (priv->images_view);

                if (pic->width > pic->height)
                {
                    scaled_pixbuf_width = 96 * scale_factor;
                    scaled_pixbuf_height = 96 * scale_factor * pic->height
                                           / pic->width;
                }else
                {
                    scaled_pixbuf_width = 96 * scale_factor * pic->width
                                          / pic->height;
                    scaled_pixbuf_height = 96 * scale_factor;
                }

                scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                         scaled_pixbuf_width,
                                                         scaled_pixbuf_height,
                                                         GDK_INTERP_BILINEAR);
                g_object_unref (pixbuf);

                /* This ties the model to the view, so if the model is to be
                 * shared in the future, the surface should be per-view. */
                view_window = gtk_widget_get_window (priv->images_view);
                surface = gdk_cairo_surface_create_from_pixbuf (scaled_pixbuf,
                                                                scale_factor,
                                                                view_window);
                pic_info = et_picture_format_info (pic,
                                                   ETCore->ETFileDisplayed->ETFileDescription->TagType);
                gtk_list_store_insert_with_values (priv->images_model, &iter1,
                                                   G_MAXINT,
                                                   PICTURE_COLUMN_SURFACE,
                                                   surface,
                                                   PICTURE_COLUMN_TEXT,
                                                   pic_info,
                                                   PICTURE_COLUMN_DATA,
                                                   pic, -1);
                g_free(pic_info);

                if (select_it)
                    gtk_tree_selection_select_iter(selection, &iter1);
                g_object_unref(scaled_pixbuf);
            }else
            {
                GtkWidget *msgdialog;
                
                g_object_unref(loader);
                
                Log_Print (LOG_ERROR, "%s",
                           _("Cannot display the image because not enough data has been read to determine how to create the image buffer"));

                msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "%s",
                                                   _("Cannot display the image"));
                gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
                                                          _("Not enough data has been read to determine how to create the image buffer."));
                gtk_window_set_title (GTK_WINDOW (msgdialog),
                                      _("Load Image File"));
                gtk_dialog_run(GTK_DIALOG(msgdialog));
                gtk_widget_destroy(msgdialog);
            }
        }
        else
        {
            Log_Print (LOG_ERROR, _("Error parsing image data ‘%s’"),
                       error->message);
            g_error_free (error);
        }
    }

next:
    /* Do also for next picture. */
    if (pic->next)
    {
        PictureEntry_Update (self, pic->next, select_it);
    }

    return;
}


/*
 * load_picture_from_file:
 * @file: the image file to load
 * @self: the #EtTagArea
 *
 * Load the image file @file and update the images tree model.
 */
static void
load_picture_from_file (GFile *file,
                        EtTagArea *self)
{
    GBytes *bytes;
    const gchar *filename_utf8;
    GFileInfo *info;
    GError *error = NULL;

    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                              G_FILE_QUERY_INFO_NONE, NULL, &error);

    if (!info)
    {
        Log_Print (LOG_ERROR, _("Image file not loaded ‘%s’"), error->message);
        g_error_free (error);
        return;
    }

    filename_utf8 = g_file_info_get_display_name (info);
    bytes = et_picture_load_file_data (file, &error);

    if (!bytes)
    {
        GtkWidget *msgdialog;

        /* Picture file not opened */
        msgdialog = gtk_message_dialog_new (GTK_WINDOW (MainWindow),
                                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE,
                                            _("Cannot open file ‘%s’"),
                                            filename_utf8);
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(msgdialog),
                                                  "%s", error->message);
        gtk_window_set_title (GTK_WINDOW (msgdialog), _("Image File Error"));
        gtk_dialog_run (GTK_DIALOG (msgdialog));
        gtk_widget_destroy (msgdialog);

        Log_Print (LOG_ERROR, _("Image file not loaded ‘%s’"),
                   error->message);
        g_error_free (error);
        return;
    }
    else
    {
        Log_Print (LOG_OK, _("Image file loaded"));
    }

    if (filename_utf8)
    {
        EtPicture *pic;
        EtPictureType type;
        const gchar *description;

        // Behaviour following the tag type...
        switch (ETCore->ETFileDisplayed->ETFileDescription->TagType)
        {
            // Only one picture supported for MP4
            case MP4_TAG:
                description = "";
                type = ET_PICTURE_TYPE_FRONT_COVER;
                break;

            // Other tag types
            case ID3_TAG:
            case OGG_TAG:
            case OPUS_TAG:
            case APE_TAG:
            case FLAC_TAG:
            case WAVPACK_TAG:
                description = filename_utf8;

                if (g_settings_get_boolean (MainSettings,
                                            "tag-image-type-automatic"))
                {
                    type = et_picture_type_from_filename (description);
                }
                else
                {
                    type = ET_PICTURE_TYPE_FRONT_COVER;
                }
                break;

            case UNKNOWN_TAG:
            default:
                g_assert_not_reached ();
        }

        pic = et_picture_new (type, description, 0, 0, bytes);

        PictureEntry_Update (self, pic, TRUE);

        et_picture_free (pic);
    }

    g_bytes_unref (bytes);
    g_object_unref (info);
}

/*
 * To add a picture in the list -> call a FileSelectionWindow
 */
static void
on_picture_add_button_clicked (GObject *object,
                               gpointer user_data)
{
    EtTagArea *self;
    EtTagAreaPrivate *priv;
    GtkWidget *FileSelectionWindow;
    GtkFileFilter *filter;
    GtkWindow *parent_window;
    gchar *init_dir;
    gint response;

    self = ET_TAG_AREA (user_data);
    priv = et_tag_area_get_instance_private (self);

    g_return_if_fail (ETCore->ETFileDisplayed);

    parent_window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (object)));

    if (!gtk_widget_is_toplevel (GTK_WIDGET (parent_window)))
    {
        g_warning("Could not get parent window\n");
        return;
    }


    FileSelectionWindow = gtk_file_chooser_dialog_new (_("Add Images"),
                                                       parent_window,
                                                       GTK_FILE_CHOOSER_ACTION_OPEN,
                                                       _("_Cancel"),
                                                       GTK_RESPONSE_CANCEL,
                                                       _("_Open"),
                                                       GTK_RESPONSE_OK, NULL);

    /* "All Files" filter. */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All Files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (FileSelectionWindow),
                                 filter);

    /* "PNG and JPEG" filter. */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("PNG and JPEG"));
    gtk_file_filter_add_mime_type (filter, "image/jpeg");
    gtk_file_filter_add_mime_type (filter, "image/png");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (FileSelectionWindow),
                                 filter);
    /* Make this filter the default. */
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (FileSelectionWindow),
                                 filter);

    // Behaviour following the tag type...
    if (ETCore->ETFileDisplayed->ETFileDescription->TagType == MP4_TAG)
    {
        /* Only one file can be selected. */
        gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (FileSelectionWindow),
                                              FALSE);
    }
    else
    {
        /* Other tag types .*/
        gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (FileSelectionWindow),
                                              TRUE);
    }

    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (FileSelectionWindow),
                                     FALSE);

    /* Starting directory (the same as the current file). */
    init_dir = g_path_get_dirname (((File_Name *)((GList *)ETCore->ETFileDisplayed->FileNameCur)->data)->value);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (FileSelectionWindow),
                                         init_dir);
    g_free (init_dir);

    response = gtk_dialog_run (GTK_DIALOG (FileSelectionWindow));

    if (response == GTK_RESPONSE_OK)
    {
        GtkTreeSelection *selection;
        GSList *list;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->images_view));
        gtk_tree_selection_unselect_all (selection);

        list = gtk_file_chooser_get_files (GTK_FILE_CHOOSER (FileSelectionWindow));
        g_slist_foreach (list, (GFunc) load_picture_from_file, self);
        g_slist_free_full (list, g_object_unref);
    }

    et_application_window_update_et_file_from_ui (ET_APPLICATION_WINDOW (MainWindow));
    et_application_window_display_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                           ETCore->ETFileDisplayed);

    gtk_widget_destroy(FileSelectionWindow);
}


/*
 * Open the window to select and type the picture properties
 */
static void
on_picture_properties_button_clicked (GObject *object,
                                      gpointer user_data)
{
    EtTagArea *self;
    EtTagAreaPrivate *priv;
    GtkWidget *type, *desc;
    GtkTreeSelection *selection;
    GtkListStore *store;
    GtkTreeIter type_iter_to_select, iter;
    GtkTreeModel *model;
    GtkWindow *parent_window = NULL;
    GList *selection_list = NULL;
    GList *l;
    gint selection_nbr, selection_i = 1;
    gint response;
    EtPictureType pic_type;

    self = ET_TAG_AREA (user_data);
    priv = et_tag_area_get_instance_private (self);

    parent_window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (object)));

    if (!gtk_widget_is_toplevel (GTK_WIDGET (parent_window)))
    {
        g_warning ("Could not get parent window");
        return;
    }

    model = GTK_TREE_MODEL (priv->images_model);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->images_view));
    selection_list = gtk_tree_selection_get_selected_rows (selection, NULL);
    selection_nbr = gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (selection));

    for (l = selection_list; l != NULL; l = g_list_next (l))
    {
        GtkWidget *PictureTypesWindow;
        GtkTreePath *path = l->data;
        EtPicture *pic = NULL;
        GtkTreeSelection *selectiontype;
        gchar *title;
        GtkTreePath *rowPath;
        gboolean valid;
        GtkBuilder *builder;

        /* Get corresponding picture. */
        valid = gtk_tree_model_get_iter (model, &iter, path);

        if (valid)
        {
            gtk_tree_model_get (model, &iter, PICTURE_COLUMN_DATA, &pic, -1);
        }
        else
        {
            g_warning ("Iter not found in picture model");
            break;
        }

        builder = gtk_builder_new_from_resource ("/org/gnome/EasyTAG/image_properties_dialog.ui");

        title = g_strdup_printf (_("Image Properties %d/%d"), selection_i++,
                                 selection_nbr);
        PictureTypesWindow = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                 "image_properties_dialog"));
        gtk_window_set_title (GTK_WINDOW (PictureTypesWindow), title);
        g_free (title);
        gtk_window_set_transient_for (GTK_WINDOW (PictureTypesWindow),
                                      parent_window);

        store = gtk_list_store_new (PICTURE_TYPE_COLUMN_COUNT, G_TYPE_STRING,
                                    G_TYPE_INT);
        type = GTK_WIDGET (gtk_builder_get_object (builder, "types_view"));
        gtk_tree_view_set_model (GTK_TREE_VIEW (type), GTK_TREE_MODEL (store));
        g_object_unref (store);

        /* Behaviour following the tag type. */
        if (ETCore->ETFileDisplayed->ETFileDescription->TagType == MP4_TAG)
        {
            /* Load picture type (only Front Cover!). */
            GtkTreeIter itertype;

            gtk_list_store_insert_with_values (store, &itertype,
                                               G_MAXINT,
                                               PICTURE_TYPE_COLUMN_TEXT,
                                               _(Picture_Type_String (ET_PICTURE_TYPE_FRONT_COVER)),
                                               PICTURE_TYPE_COLUMN_TYPE_CODE,
                                               ET_PICTURE_TYPE_FRONT_COVER,
                                               -1);
            /* Line to select by default. */
            type_iter_to_select = itertype;
        }
        else
        /* Other tag types. */
        {
            /* Load pictures types. */
            for (pic_type = ET_PICTURE_TYPE_OTHER; pic_type < ET_PICTURE_TYPE_UNDEFINED; pic_type++)
            {
                GtkTreeIter itertype;

                gtk_list_store_insert_with_values (store, &itertype,
                                                   G_MAXINT,
                                                   PICTURE_TYPE_COLUMN_TEXT,
                                                   _(Picture_Type_String (pic_type)),
                                                   PICTURE_TYPE_COLUMN_TYPE_CODE,
                                                   pic_type, -1);
                /* Line to select by default. */
                if (pic->type == pic_type)
                {
                    type_iter_to_select = itertype;
                }
            }
        }

        /* Select the line by default. */
        selectiontype = gtk_tree_view_get_selection (GTK_TREE_VIEW (type));
        gtk_tree_selection_select_iter (selectiontype, &type_iter_to_select);

        /* Set visible the current selected line. */
        rowPath = gtk_tree_model_get_path (GTK_TREE_MODEL (store),
                                           &type_iter_to_select);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (type), rowPath, NULL,
                                      FALSE, 0, 0);
        gtk_tree_path_free (rowPath);

        /* Entry for the description. */
        desc = GTK_WIDGET (gtk_builder_get_object (builder,
                                                   "description_entry"));

        g_object_unref (builder);

        if (pic->description)
        {
            gchar *tmp = Try_To_Validate_Utf8_String (pic->description);
            gtk_entry_set_text (GTK_ENTRY (desc), tmp);
            g_free (tmp);
        }

        /* Behaviour following the tag type. */
        if (ETCore->ETFileDisplayed->ETFileDescription->TagType == MP4_TAG)
        {
            gtk_widget_set_sensitive (GTK_WIDGET (desc), FALSE);
        }

        gtk_widget_show_all (PictureTypesWindow);

        response = gtk_dialog_run (GTK_DIALOG (PictureTypesWindow));

        if (response == GTK_RESPONSE_ACCEPT)
        {
            GtkTreeModel *modeltype;
            GtkTreeIter itertype;

            modeltype = gtk_tree_view_get_model (GTK_TREE_VIEW (type));
            selectiontype = gtk_tree_view_get_selection (GTK_TREE_VIEW (type));

            if (gtk_tree_selection_get_selected (selectiontype, &modeltype,
                                                 &itertype))
            {
                gchar *buffer, *pic_info;
                gint t;

                gtk_tree_model_get (modeltype, &itertype,
                                   PICTURE_TYPE_COLUMN_TYPE_CODE, &t, -1);
                pic->type = t;

                buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (desc)));
                g_strstrip (buffer);

                g_free (pic->description);

                /* If the entry was empty, buffer will be the empty string "".
                 * This can be safely passed to the underlying
                 * FLAC__metadata_object_picture_set_description(). See
                 * https://bugs.launchpad.net/ubuntu/+source/easytag/+bug/558804
                 * and https://bugzilla.redhat.com/show_bug.cgi?id=559828 for
                 * downstream bugs when 0 was passed instead. */
                pic->description = buffer;

                /* Update value in the PictureEntryView. */
                pic_info = et_picture_format_info (pic,
                                                   ETCore->ETFileDisplayed->ETFileDescription->TagType);
                gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                                    PICTURE_COLUMN_TEXT, pic_info,
                                    PICTURE_COLUMN_DATA, pic, -1);
                g_free (pic_info);
            }
        }

        gtk_widget_destroy (PictureTypesWindow);
        et_picture_free (pic);
    }

    g_list_free_full (selection_list, (GDestroyNotify)gtk_tree_path_free);
}

static void
on_picture_save_button_clicked (GObject *object,
                                gpointer user_data)
{
    EtTagArea *self;
    EtTagAreaPrivate *priv;
    GtkWidget *FileSelectionWindow;
    GtkFileFilter *filter;
    GtkWindow *parent_window = NULL;
    static gchar *init_dir = NULL;

    GtkTreeSelection *selection;
    GList *selection_list = NULL;
    GList *l;
    GtkTreeModel *model;
    gint selection_nbr, selection_i = 1;

    self = ET_TAG_AREA (user_data);
    priv = et_tag_area_get_instance_private (self);

    parent_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(object)));
    if (!gtk_widget_is_toplevel(GTK_WIDGET(parent_window)))
    {
        g_warning("Could not get parent window\n");
        return;
    }

    model = GTK_TREE_MODEL (priv->images_model);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->images_view));
    selection_list = gtk_tree_selection_get_selected_rows (selection, NULL);
    selection_nbr = gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (selection));

    for (l = selection_list; l != NULL; l = g_list_next (l))
    {
        GtkTreePath *path = l->data;
        GtkTreeIter iter;
        EtPicture *pic;
        gchar *title;
        gboolean valid;
        gint response;

        // Get corresponding picture
        valid = gtk_tree_model_get_iter (model, &iter, path);

        if (valid)
        {
            gtk_tree_model_get (model, &iter, PICTURE_COLUMN_DATA, &pic, -1);
        }
        else
        {
            g_warning ("Iter not found in picture model");
            break;
        }

        title = g_strdup_printf (_("Save Image %d/%d"), selection_i++,
                                 selection_nbr);
        FileSelectionWindow = gtk_file_chooser_dialog_new (title,
                                                           parent_window,
                                                           GTK_FILE_CHOOSER_ACTION_SAVE,
                                                           _("_Cancel"),
                                                           GTK_RESPONSE_CANCEL,
                                                           _("_Save"),
                                                           GTK_RESPONSE_OK,
                                                           NULL);
        g_free(title);

        /* "All Files" filter. */
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, _("All Files"));
        gtk_file_filter_add_pattern (filter, "*");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (FileSelectionWindow),
                                     filter);

        /* "PNG and JPEG" filter. */
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, _("PNG and JPEG"));
        gtk_file_filter_add_mime_type (filter, "image/jpeg");
        gtk_file_filter_add_mime_type (filter, "image/png");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (FileSelectionWindow),
                                     filter);
        /* Make this filter the default. */
        gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (FileSelectionWindow),
                                     filter);

        // Set the default folder if defined
        if (init_dir)
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(FileSelectionWindow),init_dir);

        /* Suggest a filename to the user. */
        if (!et_str_empty (pic->description))
        {
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(FileSelectionWindow), pic->description); //filename in UTF8
        }else
        {
            gchar *image_name = NULL;
            switch (Picture_Format_From_Data (pic))
            {
                case PICTURE_FORMAT_JPEG :
                    image_name = g_strdup("image_name.jpg");
                    break;
                case PICTURE_FORMAT_PNG :
                    image_name = g_strdup("image_name.png");
                    break;
                case PICTURE_FORMAT_GIF:
                    image_name = g_strdup ("image_name.gif");
                    break;
                case PICTURE_FORMAT_UNKNOWN:
                default:
                    image_name = g_strdup("image_name.ext");
                    break;
            }
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(FileSelectionWindow), image_name); //filename in UTF8
            g_free(image_name);
        }

        gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (FileSelectionWindow),
                                                        TRUE);
        gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (FileSelectionWindow),
                                         FALSE);

        response = gtk_dialog_run(GTK_DIALOG(FileSelectionWindow));
        if (response == GTK_RESPONSE_OK)
        {
            GFile *file;
            GError *error = NULL;

            // Save the directory selected for initialize next time
            g_free(init_dir);
            init_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(FileSelectionWindow));

            file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (FileSelectionWindow));

            if (!et_picture_save_file_data (pic, file, &error))
            {
                 Log_Print (LOG_ERROR, _("Image file not saved ‘%s’"),
                            error->message);
                 g_error_free (error);
            }

            g_object_unref (file);
        }
        gtk_widget_destroy(FileSelectionWindow);
    }

    g_list_free_full (selection_list, (GDestroyNotify)gtk_tree_path_free);
}


/*
 * If double clicking the PictureEntryView :
 *  - over a selected row : opens properties window
 *  - over an empty area : open the adding window
 */
static gboolean
on_picture_view_button_pressed (GtkTreeView *treeview,
                                GdkEventButton *event,
                                gpointer user_data)
{
    EtTagArea *self;
    EtTagAreaPrivate *priv;

    self = ET_TAG_AREA (user_data);
    priv = et_tag_area_get_instance_private (self);

    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY)
    {
        if (event->window == gtk_tree_view_get_bin_window (treeview))
        {
            if (!gtk_tree_view_get_path_at_pos (treeview, event->x, event->y,
                                                NULL, NULL, NULL, NULL))
            {
                gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (treeview));
            }
        }
    }

    if (event->type == GDK_2BUTTON_PRESS
        && event->button == GDK_BUTTON_PRIMARY)
    {
        GtkTreeSelection *selection;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->images_view));

        if (gtk_tree_selection_count_selected_rows (selection) >= 1)
        {
            on_picture_properties_button_clicked (G_OBJECT (priv->image_properties_toolitem),
                                                  self);
        }
        else
        {
            on_picture_add_button_clicked (G_OBJECT (priv->add_image_toolitem),
                                           self);
        }

        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

static void
on_picture_view_drag_data (GtkWidget *widget, GdkDragContext *dc,
                           gint x, gint y, GtkSelectionData *selection_data,
                           guint info, guint t, gpointer user_data)
{
    EtTagArea *self;
    EtTagAreaPrivate *priv;
    GtkTreeSelection *selection;
    gchar **uri_list, **uri;

    gtk_drag_finish(dc, TRUE, FALSE, t);

    if (info != TARGET_URI_LIST || !selection_data)
        return;

    self = ET_TAG_AREA (user_data);
    priv = et_tag_area_get_instance_private (self);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->images_view));
    gtk_tree_selection_unselect_all(selection);

    uri = uri_list = g_strsplit ((const gchar *)gtk_selection_data_get_data (selection_data),
                                 "\r\n", 0);

    while (!et_str_empty (*uri))
    {
        GFile *file = g_file_new_for_uri (*uri);

        load_picture_from_file (file, self);

        g_object_unref (file);
        uri++;
    }

    g_strfreev (uri_list);
}

static void
on_picture_clear_button_clicked (GObject *object,
                                 gpointer user_data)
{
    EtTagArea *self;
    EtTagAreaPrivate *priv;
    GList *paths, *refs = NULL;
    GList *l;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;

    self = ET_TAG_AREA (user_data);
    priv = et_tag_area_get_instance_private (self);

    model = GTK_TREE_MODEL (priv->images_model);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->images_view));
    paths = gtk_tree_selection_get_selected_rows (selection, NULL);

    /* List of items to delete. */
    for (l = paths; l != NULL; l = g_list_next (l))
    {
        refs = g_list_prepend (refs, gtk_tree_row_reference_new (model,
                                                                 l->data));
    }

    g_list_free_full (paths, (GDestroyNotify)gtk_tree_path_free);

    for (l = refs; l != NULL; l = g_list_next (l))
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path (l->data);

        if (gtk_tree_model_get_iter (model, &iter, path))
        {
            gtk_list_store_remove (priv->images_model, &iter);
        }

        gtk_tree_path_free(path);
        gtk_tree_row_reference_free (l->data);
    }

    et_application_window_update_et_file_from_ui (ET_APPLICATION_WINDOW (MainWindow));

    if (ETCore->ETFileDisplayed)
    {
        et_application_window_display_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                               ETCore->ETFileDisplayed);
    }

    g_list_free (refs);
}


/*
 * Key press into picture entry
 *   - Delete = delete selected picture files
 */
static gboolean
on_picture_view_key_pressed (GtkTreeView *treeview,
                             GdkEvent *event,
                             gpointer user_data)
{
    EtTagArea *self;
    EtTagAreaPrivate *priv;
    GdkEventKey *kevent;

    self = ET_TAG_AREA (user_data);
    priv = et_tag_area_get_instance_private (self);

    kevent = (GdkEventKey *)event;

    if (event && event->type == GDK_KEY_PRESS)
    {
        switch (kevent->keyval)
        {
            case GDK_KEY_Delete:
                on_picture_clear_button_clicked (G_OBJECT (priv->remove_image_toolitem),
                                                 self);
                return GDK_EVENT_STOP;
            default:
                /* Ignore all other keypresses. */
                break;
        }
    }

    return GDK_EVENT_PROPAGATE;
}
static void
create_tag_area (EtTagArea *self)
{
    EtTagAreaPrivate *priv;
    GList *focus_chain = NULL;
    GtkEntryCompletion *completion;

    /* For Picture. Ignore const string warning. */
    static const GtkTargetEntry drops[] = { { (gchar *)"text/uri-list", 0,
                                              TARGET_URI_LIST } };

    priv = et_tag_area_get_instance_private (self);

    /* Page for common tag fields. */
    et_tag_field_connect_signals (GTK_ENTRY (priv->title_entry), self);
    et_tag_field_connect_signals (GTK_ENTRY (priv->artist_entry), self);
    et_tag_field_connect_signals (GTK_ENTRY (priv->album_artist_entry), self);
    et_tag_field_connect_signals (GTK_ENTRY (priv->album_entry), self);
    /* FIXME should allow to type only something like : 1/3. */
    et_tag_field_connect_signals (GTK_ENTRY (priv->disc_number_entry), self);
    /* Year */
    et_tag_field_connect_signals (GTK_ENTRY (priv->year_entry), self);

    /* Track and Track total */
    populate_track_combo (self);

    gtk_entry_set_width_chars (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->track_combo_entry))),
                               2);
    g_signal_connect (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->track_combo_entry))),
                      "insert-text", G_CALLBACK (Insert_Only_Digit), NULL);

    et_tag_field_connect_signals (GTK_ENTRY (priv->track_total_entry), self);

    /* Genre */
    completion = gtk_entry_completion_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry))),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    gtk_entry_set_completion (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry))),
                              completion);
    g_object_unref (completion);
    gtk_entry_completion_set_model (completion,
                                    GTK_TREE_MODEL (priv->genre_combo_model));
    gtk_entry_completion_set_text_column (completion, 0);
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->genre_combo_model),
                                     GENRE_COLUMN_GENRE,
                                     tree_iter_alphabetical_sort, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->genre_combo_model),
                                          GENRE_COLUMN_GENRE,
                                          GTK_SORT_ASCENDING);
    populate_genre_combo (self);

    et_tag_field_connect_signals (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry))), self);
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry))),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this genre"));

    et_tag_field_connect_signals (GTK_ENTRY (priv->comment_entry), self);
    et_tag_field_connect_signals (GTK_ENTRY (priv->composer_entry), self);
    /* Translators: Original Artist / Performer. Please try to keep this string
     * as short as possible, as it must fit into a narrow column. */
    et_tag_field_connect_signals (GTK_ENTRY (priv->orig_artist_entry), self);
    et_tag_field_connect_signals (GTK_ENTRY (priv->copyright_entry), self);
    et_tag_field_connect_signals (GTK_ENTRY (priv->url_entry), self);
    et_tag_field_connect_signals (GTK_ENTRY (priv->encoded_by_entry), self);

    /* Set focus chain. */
    /* TODO: Use focus-chain GtkBuilder element in GTK+ 3.16. */
    focus_chain = g_list_prepend (focus_chain, priv->title_entry);
    focus_chain = g_list_prepend (focus_chain, priv->artist_entry);
    focus_chain = g_list_prepend (focus_chain, priv->album_artist_entry);
    focus_chain = g_list_prepend (focus_chain, priv->album_entry);
    focus_chain = g_list_prepend (focus_chain, priv->disc_number_entry);
    focus_chain = g_list_prepend (focus_chain, priv->year_entry);
    focus_chain = g_list_prepend (focus_chain, priv->track_combo_entry);
    focus_chain = g_list_prepend (focus_chain, priv->track_total_entry);
    focus_chain = g_list_prepend (focus_chain, priv->genre_combo_entry);
    focus_chain = g_list_prepend (focus_chain, priv->comment_entry);
    focus_chain = g_list_prepend (focus_chain, priv->composer_entry);
    focus_chain = g_list_prepend (focus_chain, priv->orig_artist_entry);
    focus_chain = g_list_prepend (focus_chain, priv->copyright_entry);
    focus_chain = g_list_prepend (focus_chain, priv->url_entry);
    focus_chain = g_list_prepend (focus_chain, priv->encoded_by_entry);
    /* More efficient than using g_list_append(), which must traverse the
     * whole list. */
    focus_chain = g_list_reverse (focus_chain);
    gtk_container_set_focus_chain (GTK_CONTAINER (priv->common_grid),
                                   focus_chain);
    g_list_free (focus_chain);

    /* Activate Drag'n'Drop for the priv->images_view. */
    gtk_drag_dest_set (GTK_WIDGET (priv->images_view),
                       GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                       drops, sizeof(drops) / sizeof(GtkTargetEntry),
                       GDK_ACTION_COPY);

    /* Activate Drag'n'Drop for the add_image_toolitem. */
    gtk_drag_dest_set (GTK_WIDGET (priv->add_image_toolitem),
                       GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                       drops, sizeof(drops) / sizeof(GtkTargetEntry),
                       GDK_ACTION_COPY);
}

static void
et_tag_area_init (EtTagArea *self)
{
    /* Ensure that the boxed type is registered before using it in
     * GtkBuilder. */
    et_picture_get_type ();

    gtk_widget_init_template (GTK_WIDGET (self));
    create_tag_area (self);
}

static void
et_tag_area_class_init (EtTagAreaClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/EasyTAG/tag_area.ui");
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  tag_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  tag_notebook);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  title_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  common_grid);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  title_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  artist_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  artist_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  album_artist_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  album_artist_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  album_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  album_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  disc_number_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  disc_number_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  year_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  year_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  track_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  track_combo_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  track_total_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  genre_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  genre_combo_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  comment_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  comment_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  composer_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  composer_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  orig_artist_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  orig_artist_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  copyright_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  copyright_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  url_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  url_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  encoded_by_label);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  encoded_by_entry);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  genre_combo_model);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  track_combo_model);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  images_view);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  add_image_toolitem);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  apply_image_toolitem);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  remove_image_toolitem);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  save_image_toolitem);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  image_properties_toolitem);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  images_grid);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  images_model);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  track_number_button);
    gtk_widget_class_bind_template_child_private (widget_class, EtTagArea,
                                                  track_sequence_button);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_picture_add_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_picture_clear_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_picture_save_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_picture_properties_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_apply_to_selection);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_picture_view_button_pressed);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_picture_view_drag_data);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_picture_view_key_pressed);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_picture_view_selection_changed);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_year_entry_activate);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_year_entry_focus_out_event);
    gtk_widget_class_bind_template_callback (widget_class,
                                             Insert_Only_Digit);
}

/*
 * et_tag_area_new:
 *
 * Create a new EtTagArea instance.
 *
 * Returns: a new #EtTagArea
 */
GtkWidget *
et_tag_area_new (void)
{
    return g_object_new (ET_TYPE_TAG_AREA, NULL);
}

static void
et_tag_area_hide_images_tab (EtTagArea *self)
{
    EtTagAreaPrivate *priv;

    priv = et_tag_area_get_instance_private (self);

    gtk_widget_hide (priv->images_grid);
}

static void
et_tag_area_show_images_tab (EtTagArea *self)
{
    EtTagAreaPrivate *priv;

    priv = et_tag_area_get_instance_private (self);

    gtk_widget_show (priv->images_grid);
}

/*
 * et_tag_area_update_controls:
 *
 * Update the visibility of entry fields depending on the type of file.
 */
void
et_tag_area_update_controls (EtTagArea *self,
                             const ET_File *ETFile)
{
    EtTagAreaPrivate *priv;

    g_return_if_fail (ET_TAG_AREA (self));

    priv = et_tag_area_get_instance_private (self);

    /* Common controls for all tags. */
    gtk_widget_show (priv->title_label);
    gtk_widget_show (priv->title_entry);
    gtk_widget_show (priv->artist_label);
    gtk_widget_show (priv->artist_entry);
    gtk_widget_show (priv->album_artist_label);
    gtk_widget_show (priv->album_artist_entry);
    gtk_widget_show (priv->album_label);
    gtk_widget_show (priv->album_entry);
    gtk_widget_show (priv->year_label);
    gtk_widget_show (priv->year_entry);
    gtk_widget_show (priv->track_label);
    gtk_widget_show (priv->track_combo_entry);
    gtk_widget_show (priv->track_total_entry);
    gtk_widget_show (priv->track_sequence_button);
    gtk_widget_show (priv->track_number_button);
    gtk_widget_show (priv->genre_label);
    gtk_widget_show (priv->genre_combo_entry);
    gtk_widget_show (priv->comment_label);
    gtk_widget_show (priv->comment_entry);

    /* Special controls to display or not! */
    switch (ETFile->ETFileDescription->TagType)
    {
        case ID3_TAG:
            if (!g_settings_get_boolean (MainSettings, "id3v2-enabled"))
            {
                /* ID3v1 : Hide specifics ID3v2 fields if not activated! */
                gtk_widget_hide (priv->disc_number_label);
                gtk_widget_hide (priv->disc_number_entry);
                gtk_widget_hide (priv->composer_label);
                gtk_widget_hide (priv->composer_entry);
                gtk_widget_hide (priv->orig_artist_label);
                gtk_widget_hide (priv->orig_artist_entry);
                gtk_widget_hide (priv->copyright_label);
                gtk_widget_hide (priv->copyright_entry);
                gtk_widget_hide (priv->url_label);
                gtk_widget_hide (priv->url_entry);
                gtk_widget_hide (priv->encoded_by_label);
                gtk_widget_hide (priv->encoded_by_entry);
                et_tag_area_hide_images_tab (self);
            }
            else
            {
                gtk_widget_show (priv->disc_number_label);
                gtk_widget_show (priv->disc_number_entry);
                gtk_widget_show (priv->composer_label);
                gtk_widget_show (priv->composer_entry);
                gtk_widget_show (priv->orig_artist_label);
                gtk_widget_show (priv->orig_artist_entry);
                gtk_widget_show (priv->copyright_label);
                gtk_widget_show (priv->copyright_entry);
                gtk_widget_show (priv->url_label);
                gtk_widget_show (priv->url_entry);
                gtk_widget_show (priv->encoded_by_label);
                gtk_widget_show (priv->encoded_by_entry);
                et_tag_area_show_images_tab (self);
            }
            break;

#ifdef ENABLE_OGG
        case OGG_TAG:
            gtk_widget_show (priv->disc_number_label);
            gtk_widget_show (priv->disc_number_entry);
            gtk_widget_show (priv->composer_label);
            gtk_widget_show (priv->composer_entry);
            gtk_widget_show (priv->orig_artist_label);
            gtk_widget_show (priv->orig_artist_entry);
            gtk_widget_show (priv->copyright_label);
            gtk_widget_show (priv->copyright_entry);
            gtk_widget_show (priv->url_label);
            gtk_widget_show (priv->url_entry);
            gtk_widget_show (priv->encoded_by_label);
            gtk_widget_show (priv->encoded_by_entry);
            et_tag_area_show_images_tab (self);
            break;
#endif

#ifdef ENABLE_OPUS
        case OPUS_TAG:
            gtk_widget_show (priv->disc_number_label);
            gtk_widget_show (priv->disc_number_entry);
            gtk_widget_show (priv->composer_label);
            gtk_widget_show (priv->composer_entry);
            gtk_widget_show (priv->orig_artist_label);
            gtk_widget_show (priv->orig_artist_entry);
            gtk_widget_show (priv->copyright_label);
            gtk_widget_show (priv->copyright_entry);
            gtk_widget_show (priv->url_label);
            gtk_widget_show (priv->url_entry);
            gtk_widget_show (priv->encoded_by_label);
            gtk_widget_show (priv->encoded_by_entry);
            et_tag_area_show_images_tab (self);
            break;
#endif

#ifdef ENABLE_FLAC
        case FLAC_TAG:
            gtk_widget_show (priv->disc_number_label);
            gtk_widget_show (priv->disc_number_entry);
            gtk_widget_show (priv->composer_label);
            gtk_widget_show (priv->composer_entry);
            gtk_widget_show (priv->orig_artist_label);
            gtk_widget_show (priv->orig_artist_entry);
            gtk_widget_show (priv->copyright_label);
            gtk_widget_show (priv->copyright_entry);
            gtk_widget_show (priv->url_label);
            gtk_widget_show (priv->url_entry);
            gtk_widget_show (priv->encoded_by_label);
            gtk_widget_show (priv->encoded_by_entry);
            et_tag_area_show_images_tab (self);
            break;
#endif

        case APE_TAG:
            gtk_widget_show (priv->disc_number_label);
            gtk_widget_show (priv->disc_number_entry);
            gtk_widget_show (priv->composer_label);
            gtk_widget_show (priv->composer_entry);
            gtk_widget_show (priv->orig_artist_label);
            gtk_widget_show (priv->orig_artist_entry);
            gtk_widget_show (priv->copyright_label);
            gtk_widget_show (priv->copyright_entry);
            gtk_widget_show (priv->url_label);
            gtk_widget_show (priv->url_entry);
            gtk_widget_show (priv->encoded_by_label);
            gtk_widget_show (priv->encoded_by_entry);
            et_tag_area_show_images_tab (self);
            break;

#ifdef ENABLE_MP4
        case MP4_TAG:
            gtk_widget_show (priv->disc_number_label);
            gtk_widget_show (priv->disc_number_entry);
            gtk_widget_show (priv->composer_label);
            gtk_widget_show (priv->composer_entry);
            gtk_widget_hide (priv->orig_artist_label);
            gtk_widget_hide (priv->orig_artist_entry);
            gtk_widget_show (priv->copyright_label);
            gtk_widget_show (priv->copyright_entry);
            gtk_widget_hide (priv->url_label);
            gtk_widget_hide (priv->url_entry);
            gtk_widget_show (priv->encoded_by_label);
            gtk_widget_show (priv->encoded_by_entry);
            et_tag_area_show_images_tab (self);
            break;
#endif

#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
            gtk_widget_show (priv->disc_number_label);
            gtk_widget_show (priv->disc_number_entry);
            gtk_widget_show (priv->composer_label);
            gtk_widget_show (priv->composer_entry);
            gtk_widget_show (priv->orig_artist_label);
            gtk_widget_show (priv->orig_artist_entry);
            gtk_widget_show (priv->copyright_label);
            gtk_widget_show (priv->copyright_entry);
            gtk_widget_show (priv->url_label);
            gtk_widget_show (priv->url_entry);
            gtk_widget_show (priv->encoded_by_label);
            gtk_widget_show (priv->encoded_by_entry);
            et_tag_area_hide_images_tab (self);
            break;
#endif /* ENABLE_WAVPACK */

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
            gtk_widget_hide (priv->disc_number_label);
            gtk_widget_hide (priv->disc_number_entry);
            gtk_widget_hide (priv->composer_label);
            gtk_widget_hide (priv->composer_entry);
            gtk_widget_hide (priv->orig_artist_label);
            gtk_widget_hide (priv->orig_artist_entry);
            gtk_widget_hide (priv->copyright_label);
            gtk_widget_hide (priv->copyright_entry);
            gtk_widget_hide (priv->url_label);
            gtk_widget_hide (priv->url_entry);
            gtk_widget_hide (priv->encoded_by_label);
            gtk_widget_hide (priv->encoded_by_entry);
            et_tag_area_hide_images_tab (self);
            break;
    }
}

void
et_tag_area_clear (EtTagArea *self)
{
    EtTagAreaPrivate *priv;

    g_return_if_fail (ET_TAG_AREA (self));

    priv = et_tag_area_get_instance_private (self);

    gtk_entry_set_text (GTK_ENTRY (priv->title_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->artist_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->album_artist_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->album_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->disc_number_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->year_entry), "");
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->track_combo_entry))),
                        "");
    gtk_entry_set_text (GTK_ENTRY (priv->track_total_entry), "");
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry))),
                        "");
    gtk_entry_set_text (GTK_ENTRY (priv->comment_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->composer_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->orig_artist_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->copyright_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->url_entry), "");
    gtk_entry_set_text (GTK_ENTRY (priv->encoded_by_entry), "");
    PictureEntry_Clear (self);
}

void
et_tag_area_title_grab_focus (EtTagArea *self)
{
    EtTagAreaPrivate *priv;

    g_return_if_fail (ET_TAG_AREA (self));

    priv = et_tag_area_get_instance_private (self);

    gtk_widget_grab_focus (priv->title_entry);
}

/*
 * et_tag_area_create_file_tag:
 *
 * Create a new File_Tag structure and poopulate it with values from the UI.
 */
File_Tag *
et_tag_area_create_file_tag (EtTagArea *self)
{
    EtTagAreaPrivate *priv;
    gchar *buffer;
    File_Tag *FileTag;

    g_return_val_if_fail (ET_TAG_AREA (self), NULL);

    priv = et_tag_area_get_instance_private (self);

    /* Save tag data and generate undo for tag. */
    FileTag = et_file_tag_new ();

    /* Title */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->title_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->title = buffer;
    }
    else
    {
        FileTag->title = NULL;
        g_free (buffer);
    }

    /* Artist */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->artist_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->artist = buffer;
    }
    else
    {
        FileTag->artist = NULL;
        g_free (buffer);
    }

	/* Album Artist */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->album_artist_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->album_artist = buffer;
    }
    else
    {
        FileTag->album_artist = NULL;
        g_free (buffer);
    }

    /* Album */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->album_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->album = buffer;
    }
    else
    {
        FileTag->album = NULL;
        g_free (buffer);
    }

    /* Disc number and total number of discs. */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->disc_number_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        gchar *separator;

        separator = strchr (buffer, '/');

        if (separator && *(separator + 1))
        {
            /* Copy before the separator for the disc number, beyond the
             * separator for the total number of discs. */
            FileTag->disc_number = g_strndup (buffer, separator - buffer);
            FileTag->disc_total = g_strdup (separator + 1);
            g_free (buffer);
        }
        else
        {
            FileTag->disc_number = buffer;
            FileTag->disc_total = NULL;
        }
    }
    else
    {
        FileTag->disc_number = NULL;
        FileTag->disc_total = NULL;
        g_free (buffer);
    }

    /* Year */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->year_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->year = buffer;
    }
    else
    {
        FileTag->year = NULL;
        g_free (buffer);
    }

    /* Track */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->track_combo_entry)))));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->track = et_track_number_to_string (atoi (buffer));
        g_free (buffer);
    }
    else
    {
        FileTag->track = NULL;
        g_free (buffer);
    }

    /* Track Total */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->track_total_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->track_total = et_track_number_to_string (atoi (buffer));
        g_free (buffer);
    }
    else
    {
        FileTag->track_total = NULL;
        g_free (buffer);
    }

    /* Genre */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry)))));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->genre = buffer;
    }
    else
    {
        FileTag->genre = NULL;
        g_free (buffer);
    }

    /* Comment */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->comment_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->comment = buffer;
    }
    else
    {
        FileTag->comment = NULL;
        g_free (buffer);
    }

    /* Composer */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->composer_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->composer = buffer;
    }
    else
    {
        FileTag->composer = NULL;
        g_free (buffer);
    }

    /* Original Artist */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->orig_artist_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->orig_artist = buffer;
    }
    else
    {
        FileTag->orig_artist = NULL;
        g_free (buffer);
    }

    /* Copyright */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->copyright_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->copyright = buffer;
    }
    else
    {
        FileTag->copyright = NULL;
        g_free (buffer);
    }

    /* URL */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->url_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->url = buffer;
    }
    else
    {
        FileTag->url = NULL;
        g_free (buffer);
    }

    /* Encoded by */
    buffer = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->encoded_by_entry)));
    g_strstrip (buffer);

    if (*buffer)
    {
        FileTag->encoded_by = buffer;
    }
    else
    {
        FileTag->encoded_by = NULL;
        g_free (buffer);
    }

    /* Picture */
    {
        EtPicture *pic, *prev_pic = NULL;
        GtkTreeModel *model;
        GtkTreeIter iter;

        et_file_tag_set_picture (FileTag, NULL);

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->images_view));

        if (gtk_tree_model_get_iter_first (model, &iter))
        {
            do
            {
                gtk_tree_model_get (model, &iter, PICTURE_COLUMN_DATA, &pic,
                                    -1);

                if (!FileTag->picture)
                {
                    FileTag->picture = pic;
                }
                else
                {
                    prev_pic->next = pic;
                }

                prev_pic = pic;
            } while (gtk_tree_model_iter_next (model, &iter));
        }
    }

    return FileTag;
}

gboolean
et_tag_area_display_et_file (EtTagArea *self,
                             const ET_File *ETFile)
{
    EtTagAreaPrivate *priv;
    const File_Tag *FileTag;

    g_return_val_if_fail (ET_TAG_AREA (self), FALSE);

    if (!ETFile || !ETFile->FileTag)
    {
        et_tag_area_clear (self);
        //Tag_Area_Set_Sensitive(FALSE);
        return FALSE;
    }

    priv = et_tag_area_get_instance_private (self);

    switch (ETFile->ETFileDescription->TagType)
    {
#ifdef ENABLE_MP3
        case ID3_TAG:
            gtk_label_set_text (GTK_LABEL (priv->tag_label), _("ID3 Tag"));
            break;
#endif
#ifdef ENABLE_OGG
        case OGG_TAG:
            gtk_label_set_text (GTK_LABEL (priv->tag_label), _("Ogg Vorbis Tag"));
            break;
#endif
#ifdef ENABLE_FLAC
        case FLAC_TAG:
            gtk_label_set_text (GTK_LABEL (priv->tag_label), _("FLAC Vorbis Tag"));
            break;
#endif
        case APE_TAG:
            gtk_label_set_text (GTK_LABEL (priv->tag_label), _("APE Tag"));
            break;
#ifdef ENABLE_MP4
        case MP4_TAG:
            gtk_label_set_text (GTK_LABEL (priv->tag_label), _("MP4/M4A/AAC Tag"));
            break;
#endif
#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
            gtk_label_set_text (GTK_LABEL (priv->tag_label), _("Wavpack Tag"));
            break;
#endif
#ifdef ENABLE_OPUS
        case OPUS_TAG:
            gtk_label_set_text (GTK_LABEL (priv->tag_label), _("Opus Tag"));
            break;
#endif
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
            gtk_label_set_text (GTK_LABEL (priv->tag_label), _("Tag"));
            /* FIXME: Translatable string. */
            Log_Print (LOG_ERROR,
                       "FileTag: Undefined tag type %d for file %s.",
                       (gint)ETFile->ETFileDescription->TagType,
                       ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value_utf8);
            break;
    }

    //Tag_Area_Set_Sensitive(TRUE); // Causes displaying problem when saving files

    FileTag = (File_Tag *)(ETFile->FileTag->data);

    /* Show title */
    if (FileTag && FileTag->title)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->title);
        gtk_entry_set_text (GTK_ENTRY (priv->title_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->title_entry), "");
    }

    /* Show artist */
    if (FileTag && FileTag->artist)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->artist);
        gtk_entry_set_text (GTK_ENTRY (priv->artist_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->artist_entry), "");
    }

	/* Show album artist */
    if (FileTag && FileTag->album_artist)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->album_artist);
        gtk_entry_set_text (GTK_ENTRY (priv->album_artist_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->album_artist_entry), "");
    }

    /* Show album */
    if (FileTag && FileTag->album)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->album);
        gtk_entry_set_text (GTK_ENTRY (priv->album_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->album_entry),"");
    }

    /* Show disc number and number of discs. */
    if (FileTag && FileTag->disc_number)
    {
        gchar *tmp;

        if (FileTag->disc_total)
        {
            gchar *total;

            total = g_strjoin ("/", FileTag->disc_number, FileTag->disc_total,
                               NULL);
            tmp = Try_To_Validate_Utf8_String (total);
            g_free (total);
        }
        else
        {
            tmp = Try_To_Validate_Utf8_String (FileTag->disc_number);
        }

        gtk_entry_set_text (GTK_ENTRY (priv->disc_number_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->disc_number_entry), "");
    }

    /* Show year */
    if (FileTag && FileTag->year)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->year);
        gtk_entry_set_text (GTK_ENTRY (priv->year_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->year_entry), "");
    }

    /* Show track */
    if (FileTag && FileTag->track)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->track);
        gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->track_combo_entry))),
                            tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->track_combo_entry))),
                            "");
    }

    /* Show number of tracks on the album */
    if (FileTag && FileTag->track_total)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->track_total);
        gtk_entry_set_text (GTK_ENTRY (priv->track_total_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->track_total_entry),
                            "");
    }

    /* Show genre */
    if (FileTag && FileTag->genre)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->genre);
        gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry))),
                            tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->genre_combo_entry))),
                            "");
    }

    /* Show comment */
    if (FileTag && FileTag->comment)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->comment);
        gtk_entry_set_text (GTK_ENTRY (priv->comment_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->comment_entry),
                            "");
    }

    /* Show composer */
    if (FileTag && FileTag->composer)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->composer);
        gtk_entry_set_text (GTK_ENTRY (priv->composer_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->composer_entry), "");
    }

    /* Show original artist */
    if (FileTag && FileTag->orig_artist)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->orig_artist);
        gtk_entry_set_text (GTK_ENTRY (priv->orig_artist_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->orig_artist_entry),
                            "");
    }

    /* Show copyright */
    if (FileTag && FileTag->copyright)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->copyright);
        gtk_entry_set_text (GTK_ENTRY (priv->copyright_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->copyright_entry), "");
    }

    /* Show URL */
    if (FileTag && FileTag->url)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->url);
        gtk_entry_set_text (GTK_ENTRY (priv->url_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->url_entry), "");
    }

    /* Show Encoded by */
    if (FileTag && FileTag->encoded_by)
    {
        gchar *tmp = Try_To_Validate_Utf8_String (FileTag->encoded_by);
        gtk_entry_set_text (GTK_ENTRY (priv->encoded_by_entry), tmp);
        g_free (tmp);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->encoded_by_entry), "");
    }

    /* Show picture */
    PictureEntry_Clear (self);

    if (FileTag && FileTag->picture)
    {
        EtPicture *pic;
        guint    nbr_pic = 0;
        GtkWidget *page;
        gchar *string;

        PictureEntry_Update (self, FileTag->picture, FALSE);

        // Count the number of items
        for (pic = FileTag->picture; pic != NULL; pic = pic->next)
        {
            nbr_pic++;
        }

        /* Get page "Images" of the notebook. */
        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->tag_notebook), 1);
        string = g_strdup_printf (_("Images (%u)"), nbr_pic);
        /* Update the notebook tab. */
        gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (priv->tag_notebook), page,
                                         string);
        /* Update the notebook menu. */
        gtk_notebook_set_menu_label_text (GTK_NOTEBOOK (priv->tag_notebook), page,
                                          string);
        g_free (string);

    }
    else
    {
        GtkWidget *page;

        /* Get page "Images" of the notebook. */
        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->tag_notebook),
                                          1);
        /* Update the notebook tab. */
        gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (priv->tag_notebook),
                                         page, _("Images"));
        /* Update the notebook menu. */
        gtk_notebook_set_menu_label_text (GTK_NOTEBOOK (priv->tag_notebook),
                                          page, _("Images"));
    }

    return TRUE;
}

gboolean
et_tag_area_select_all_if_focused (EtTagArea *self,
                                   GtkWidget *focused)
{
    EtTagAreaPrivate *priv;

    g_return_val_if_fail (ET_TAG_AREA (self), FALSE);

    priv = et_tag_area_get_instance_private (self);

    if (focused == priv->images_view)
    {
        GtkTreeSelection *selection;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->images_view));
        gtk_tree_selection_select_all (selection);
        return TRUE;
    }

    return FALSE;
}

gboolean
et_tag_area_unselect_all_if_focused (EtTagArea *self,
                                     GtkWidget *focused)
{
    EtTagAreaPrivate *priv;

    g_return_val_if_fail (ET_TAG_AREA (self), FALSE);

    priv = et_tag_area_get_instance_private (self);

    if (focused == priv->images_view)
    {
        GtkTreeSelection *selection;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->images_view));
        gtk_tree_selection_unselect_all (selection);
        return TRUE;
    }

    return FALSE;
}
