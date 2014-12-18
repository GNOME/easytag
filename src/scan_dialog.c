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

#include "config.h"

#include "scan_dialog.h"

#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "application_window.h"
#include "easytag.h"
#include "enums.h"
#include "preferences_dialog.h"
#include "scan.h"
#include "setting.h"
#include "id3_tag.h"
#include "browser.h"
#include "log.h"
#include "misc.h"
#include "et_core.h"
#include "crc32.h"
#include "charset.h"

/* TODO: Use G_DEFINE_TYPE_WITH_PRIVATE. */
G_DEFINE_TYPE (EtScanDialog, et_scan_dialog, GTK_TYPE_DIALOG)

#define et_scan_dialog_get_instance_private(dialog) (dialog->priv)

struct _EtScanDialogPrivate
{
    GtkListStore *rename_masks_model;
    GtkListStore *scan_tag_masks_model;

    GtkWidget *mask_editor_entry;
    GtkWidget *mask_editor_view;

    GtkWidget *notebook;
    GtkWidget *fill_grid;
    GtkWidget *rename_grid;

    GtkWidget *scan_tag_mask_combo;
    GtkWidget *rename_file_mask_combo;

    GtkWidget *legend_frame;
    GtkWidget *mask_editor_frame;

    GtkWidget *legend_toggle;
    GtkWidget *mask_editor_toggle;

    GtkWidget *process_convert_to_space_toggle;
    GtkWidget *process_convert_to_underscores_toggle;
    GtkWidget *process_convert_toggle;
    GtkWidget *process_convert_label;

    GtkWidget *process_all_uppercase_toggle;
    GtkWidget *process_all_lowercase_toggle;
    GtkWidget *process_first_uppercase_toggle;
    GtkWidget *process_first_style_uppercase_toggle;
    GtkWidget *process_roman_numerals_check;

    GtkWidget *process_remove_space_toggle;
    GtkWidget *process_insert_space_toggle;
    GtkWidget *process_insert_one_space_toggle;

    GtkWidget *process_convert_to_entry;
    GtkWidget *process_convert_from_entry;

    GtkWidget *fill_tag_preview_label;
    GtkWidget *rename_file_preview_label;
};

static const guint BOX_SPACING = 6;

/* Some predefined masks -- IMPORTANT: Null-terminate me! */
static const gchar *Scan_Masks [] =
{
    "%a - %b"G_DIR_SEPARATOR_S"%n - %t",
    "%a_-_%b"G_DIR_SEPARATOR_S"%n_-_%t",
    "%a - %b (%y)"G_DIR_SEPARATOR_S"%n - %a - %t",
    "%a_-_%b_(%y)"G_DIR_SEPARATOR_S"%n_-_%a_-_%t",
    "%a - %b (%y) - %g"G_DIR_SEPARATOR_S"%n - %a - %t",
    "%a_-_%b_(%y)_-_%g"G_DIR_SEPARATOR_S"%n_-_%a_-_%t",
    "%a - %b"G_DIR_SEPARATOR_S"%n. %t",
    "%a_-_%b"G_DIR_SEPARATOR_S"%n._%t",
    "%a-%b"G_DIR_SEPARATOR_S"%n-%t",
    "%b"G_DIR_SEPARATOR_S"%n. %a - %t",
    "%b"G_DIR_SEPARATOR_S"%n._%a_-_%t",
    "%b"G_DIR_SEPARATOR_S"%n - %a - %t",
    "%b"G_DIR_SEPARATOR_S"%n_-_%a_-_%t",
    "%b"G_DIR_SEPARATOR_S"%n-%a-%t",
    "%a-%b"G_DIR_SEPARATOR_S"%n-%t",
    "%a"G_DIR_SEPARATOR_S"%b"G_DIR_SEPARATOR_S"%n. %t",
    "%g"G_DIR_SEPARATOR_S"%a"G_DIR_SEPARATOR_S"%b"G_DIR_SEPARATOR_S"%t",
    "%a_-_%b-%n-%t-%y",
    "%a - %b"G_DIR_SEPARATOR_S"%n. %t(%c)",
    "%t",
    "Track%n",
    "Track%i %n",
    NULL
};

static const gchar *Rename_File_Masks [] =
{
    "%n - %a - %t",
    "%n_-_%a_-_%t",
    "%n. %a - %t",
    "%n._%a_-_%t",
    "%a - %b"G_DIR_SEPARATOR_S"%n - %t",
    "%a_-_%b"G_DIR_SEPARATOR_S"%n_-_%t",
    "%a - %b (%y) - %g"G_DIR_SEPARATOR_S"%n - %t",
    "%a_-_%b_(%y)_-_%g"G_DIR_SEPARATOR_S"%n_-_%t",
    "%n - %t",
    "%n_-_%t",
    "%n. %t",
    "%n._%t",
    "%n - %a - %b - %t",
    "%n_-_%a_-_%b_-_%t",
    "%a - %b - %t",
    "%a_-_%b_-_%t",
    "%a - %b - %n - %t",
    "%a_-_%b_-_%n_-_%t",
    "%a - %t",
    "%a_-_%t",
    "Track %n",
    NULL
};

/**gchar *Rename_Directory_Masks [] =
{
    "%a - %b",
    "%a_-_%b",
    "%a - %b (%y) - %g",
    "%a_-_%b_(%y)_-_%g",
    "VA - %b (%y)",
    "VA_-_%b_(%y)",
    NULL
};**/


typedef enum
{
    UNKNOWN = 0,           /* Default value when initialized */
    LEADING_SEPARATOR,     /* characters before the first code */
    TRAILING_SEPARATOR,    /* characters after the last code */
    SEPARATOR,             /* item is a separator between two codes */
    DIRECTORY_SEPARATOR,   /* item is a separator between two codes with character '/' (G_DIR_SEPARATOR) */
    FIELD,                 /* item contains text (not empty) of entry */
    EMPTY_FIELD            /* item when entry contains no text */
} Mask_Item_Type;


enum {
    MASK_EDITOR_TEXT,
    MASK_EDITOR_COUNT
};

/*
 * Used into Rename File Scanner
 */
typedef struct _File_Mask_Item File_Mask_Item;
struct _File_Mask_Item
{
    Mask_Item_Type  type;
    gchar          *string;
};

/*
 * Used into Scan Tag Scanner
 */
typedef struct _Scan_Mask_Item Scan_Mask_Item;
struct _Scan_Mask_Item
{
    gchar  code;   // The code of the mask without % (ex: %a => a)
    gchar *string; // The string found by the scanner for the code defined the line above
};



/**************
 * Prototypes *
 **************/
static void Scan_Option_Button (void);
static void entry_check_scan_tag_mask (GtkEntry *entry, gpointer user_data);

static GList *Scan_Generate_New_Tag_From_Mask (ET_File *ETFile, gchar *mask);
static void Scan_Free_File_Rename_List (GList *list);
static void Scan_Free_File_Fill_Tag_List (GList *list);

static gchar **Scan_Return_File_Tag_Field_From_Mask_Code (File_Tag *FileTag,
                                                          gchar code);
static GList *Scan_Generate_New_Tag_From_Mask (ET_File *ETFile, gchar *mask);

static void et_scan_on_response (GtkDialog *dialog, gint response_id,
                                 gpointer user_data);


/*************
 * Functions *
 *************/

/*
 * Uses the filename and path to fill tag information
 * Note: mask and source are read from the right to the left
 */
static void
Scan_Tag_With_Mask (EtScanDialog *self, ET_File *ETFile)
{
    EtScanDialogPrivate *priv;
    GList *fill_tag_list = NULL;
    GList *l;
    gchar **dest = NULL;
    gchar *mask; // The 'mask' in the entry
    gchar *filename_utf8;
    File_Tag *FileTag;

    g_return_if_fail (ETFile != NULL);

    priv = et_scan_dialog_get_instance_private (self);

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->scan_tag_mask_combo)))));
    if (!mask) return;

    // Create a new File_Tag item
    FileTag = ET_File_Tag_Item_New();
    ET_Copy_File_Tag_Item(ETFile,FileTag);

    // Process this mask with file
    fill_tag_list = Scan_Generate_New_Tag_From_Mask(ETFile,mask);

    for (l = fill_tag_list; l != NULL; l = g_list_next (l))
    {
        Scan_Mask_Item *mask_item = l->data;

        // Get the target entry for this code
        dest = Scan_Return_File_Tag_Field_From_Mask_Code(FileTag,mask_item->code);

        /* We display the text affected to the code. */
        if (dest && (g_settings_get_boolean (MainSettings,
                                             "fill-overwrite-tag-fields")
            || *dest == NULL || strlen (*dest) == 0))
        {
            ET_Set_Field_File_Tag_Item (dest, mask_item->string);
        }

    }

    Scan_Free_File_Fill_Tag_List(fill_tag_list);

    /* Set the default text to comment. */
    if (g_settings_get_boolean (MainSettings, "fill-set-default-comment")
        && (g_settings_get_boolean (MainSettings, "fill-overwrite-tag-fields")
            || FileTag->comment == NULL || strlen (FileTag->comment) == 0))
    {
        gchar *default_comment = g_settings_get_string (MainSettings,
                                                        "fill-default-comment");
        ET_Set_Field_File_Tag_Item ((void *)&FileTag->comment,
                                    default_comment);
        g_free (default_comment);
    }

    /* Set CRC-32 value as default comment (for files with ID3 tag only). */
    if (g_settings_get_boolean (MainSettings, "fill-crc32-comment")
        && (g_settings_get_boolean (MainSettings, "fill-overwrite-tag-fields")
            || FileTag->comment == NULL || strlen (FileTag->comment) == 0))
    {
        GFile *file;
        GError *error = NULL;
        guint32 crc32_value;
        gchar *buffer;
        const ET_File_Description *ETFileDescription;

        ETFileDescription = ETFile->ETFileDescription;
        switch (ETFileDescription->TagType)
        {
            case ID3_TAG:
                file = g_file_new_for_path (((File_Name *)((GList *)ETFile->FileNameNew)->data)->value);

                if (crc32_file_with_ID3_tag (file, &crc32_value, &error))
                {
                    buffer = g_strdup_printf ("%.8" G_GUINT32_FORMAT,
                                              crc32_value);
                    ET_Set_Field_File_Tag_Item((void *)&FileTag->comment,buffer);
                    g_free(buffer);
                }
                else
                {
                    Log_Print (LOG_ERROR,
                               _("Cannot calculate CRC value of file ‘%s’"),
                               error->message);
                    g_error_free (error);
                }

                g_object_unref (file);
                break;
            default:
                break;
        }
    }


    // Save changes of the 'File_Tag' item
    ET_Manage_Changes_Of_File_Data(ETFile,NULL,FileTag);

    g_free(mask);
    et_application_window_status_bar_message (ET_APPLICATION_WINDOW (MainWindow),
                                              _("Tag successfully scanned"),
                                              TRUE);
    filename_utf8 = g_path_get_basename( ((File_Name *)ETFile->FileNameNew->data)->value_utf8 );
    Log_Print (LOG_OK, _("Tag successfully scanned ‘%s’"), filename_utf8);
    g_free(filename_utf8);
}

static GList *
Scan_Generate_New_Tag_From_Mask (ET_File *ETFile, gchar *mask)
{
    GList *fill_tag_list = NULL;
    gchar *filename_utf8;
    gchar *tmp;
    gchar *buf;
    gchar *separator;
    gchar *string;
    gint  len, i, loop=0;
    gchar **mask_splitted;
    gchar **file_splitted;
    guint mask_splitted_number;
    guint file_splitted_number;
    guint mask_splitted_index;
    guint file_splitted_index;
    Scan_Mask_Item *mask_item;
    EtConvertSpaces convert_mode;

    g_return_val_if_fail (ETFile != NULL && mask != NULL, NULL);

    filename_utf8 = g_strdup(((File_Name *)((GList *)ETFile->FileNameNew)->data)->value_utf8);
    if (!filename_utf8) return NULL;

    // Remove extension of file (if found)
    tmp = strrchr(filename_utf8,'.');
    for (i=0; i<=(gint)ET_FILE_DESCRIPTION_SIZE; i++)
    {
        if ( strcasecmp(tmp,ETFileDescription[i].Extension)==0 )
        {
            *tmp = 0; //strrchr(source,'.') = 0;
            break;
        }
    }

    if (i==ET_FILE_DESCRIPTION_SIZE)
    {
        gchar *tmp1 = g_path_get_basename(filename_utf8);
        Log_Print (LOG_ERROR,
                   _("The extension ‘%s’ was not found in filename ‘%s’"), tmp,
                   tmp1);
        g_free(tmp1);
    }

    /* Replace characters into mask and filename before parsing. */
    convert_mode = g_settings_get_enum (MainSettings, "fill-convert-spaces");

    switch (convert_mode)
    {
        case ET_CONVERT_SPACES_SPACES:
            Scan_Convert_Underscore_Into_Space (mask);
            Scan_Convert_Underscore_Into_Space (filename_utf8);
            Scan_Convert_P20_Into_Space (mask);
            Scan_Convert_P20_Into_Space (filename_utf8);
            break;
        case ET_CONVERT_SPACES_UNDERSCORES:
            Scan_Convert_Space_Into_Underscore (mask);
            Scan_Convert_Space_Into_Underscore (filename_utf8);
            break;
        case ET_CONVERT_SPACES_NO_CHANGE:
            break;
        default:
            g_assert_not_reached ();
    }

    // Split the Scanner mask
    mask_splitted = g_strsplit(mask,G_DIR_SEPARATOR_S,0);
    // Get number of arguments into 'mask_splitted'
    for (mask_splitted_number=0;mask_splitted[mask_splitted_number];mask_splitted_number++);

    // Split the File Path
    file_splitted = g_strsplit(filename_utf8,G_DIR_SEPARATOR_S,0);
    // Get number of arguments into 'file_splitted'
    for (file_splitted_number=0;file_splitted[file_splitted_number];file_splitted_number++);

    // Set the starting position for each tab
    if (mask_splitted_number <= file_splitted_number)
    {
        mask_splitted_index = 0;
        file_splitted_index = file_splitted_number - mask_splitted_number;
    }else
    {
        mask_splitted_index = mask_splitted_number - file_splitted_number;
        file_splitted_index = 0;
    }

    loop = 0;
    while ( mask_splitted[mask_splitted_index]!= NULL && file_splitted[file_splitted_index]!=NULL )
    {
        gchar *mask_seq = mask_splitted[mask_splitted_index];
        gchar *file_seq = file_splitted[file_splitted_index];
        gchar *file_seq_utf8 = filename_to_display(file_seq);

        //g_print(">%d> seq '%s' '%s'\n",loop,mask_seq,file_seq);
        while ( mask_seq && strlen(mask_seq)>0 )
        {

            /*
             * Determine (first) code and destination
             */
            if ( (tmp=strchr(mask_seq,'%')) == NULL || strlen(tmp) < 2 )
            {
                break;
            }

            /*
             * Allocate a new iten for the fill_tag_list
             */
            mask_item = g_slice_new0 (Scan_Mask_Item);

            // Get the code (used to determine the corresponding target entry)
            mask_item->code = tmp[1];

            /*
             * Delete text before the code
             */
            if ( (len = strlen(mask_seq) - strlen(tmp)) > 0 )
            {
                // Get this text in 'mask_seq'
                buf = g_strndup(mask_seq,len);
                // We remove it in 'mask_seq'
                mask_seq = mask_seq + len;
                // Find the same text at the begining of 'file_seq' ?
                if ( (strstr(file_seq,buf)) == file_seq )
                {
                    file_seq = file_seq + len; // We remove it
                }else
                {
                    Log_Print (LOG_ERROR,
                               _("Cannot find separator ‘%s’ within ‘%s’"),
                               buf, file_seq_utf8);
                }
                g_free(buf);
            }

            // Remove the current code into 'mask_seq'
            mask_seq = mask_seq + 2;

            /*
             * Determine separator between two code or trailing text (after code)
             */
            if ( mask_seq && strlen(mask_seq)>0 )
            {
                if ( (tmp=strchr(mask_seq,'%')) == NULL || strlen(tmp) < 2 )
                {
                    // No more code found
                    len = strlen(mask_seq);
                }else
                {
                    len = strlen(mask_seq) - strlen(tmp);
                }
                separator = g_strndup(mask_seq,len);

                // Remove the current separator in 'mask_seq'
                mask_seq = mask_seq + len;

                // Try to find the separator in 'file_seq'
                if ( (tmp=strstr(file_seq,separator)) == NULL )
                {
                    Log_Print (LOG_ERROR,
                               _("Cannot find separator ‘%s’ within ‘%s’"),
                               separator, file_seq_utf8);
                    separator[0] = 0; // Needed to avoid error when calculting 'len' below
                }

                // Get the string affected to the code (or the corresponding entry field)
                len = strlen(file_seq) - (tmp!=NULL?strlen(tmp):0);
                string = g_strndup(file_seq,len);

                // Remove the current separator in 'file_seq'
                file_seq = file_seq + strlen(string) + strlen(separator);
                g_free(separator);

                // We get the text affected to the code
                mask_item->string = string;
            }else
            {
                // We display the remaining text, affected to the code (no more data in 'mask_seq')
                mask_item->string = g_strdup(file_seq);
            }

            // Add the filled mask_iten to the list
            fill_tag_list = g_list_append(fill_tag_list,mask_item);
        }

        g_free(file_seq_utf8);

        // Next sequences
        mask_splitted_index++;
        file_splitted_index++;
        loop++;
    }

    g_free(filename_utf8);
    g_strfreev(mask_splitted);
    g_strfreev(file_splitted);

    // The 'fill_tag_list' must be freed after use
    return fill_tag_list;
}

static void
Scan_Fill_Tag_Generate_Preview (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    gchar *mask = NULL;
    gchar *preview_text = NULL;
    GList *fill_tag_list = NULL;
    GList *l;

    priv = et_scan_dialog_get_instance_private (self);

    if (!ETCore->ETFileDisplayedList
        || gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) != ET_SCAN_MODE_FILL_TAG)
        return;

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->scan_tag_mask_combo)))));
    if (!mask)
        return;

    preview_text = g_strdup("");
    fill_tag_list = Scan_Generate_New_Tag_From_Mask(ETCore->ETFileDisplayed,mask);
    for (l = fill_tag_list; l != NULL; l = g_list_next (l))
    {
        Scan_Mask_Item *mask_item = l->data;
        gchar *tmp_code   = g_strdup_printf("%c",mask_item->code);
        gchar *tmp_string = g_markup_printf_escaped("%s",mask_item->string); // To avoid problem with strings containing characters like '&'
        gchar *tmp_preview_text = preview_text;

        preview_text = g_strconcat(tmp_preview_text,"<b>","%",tmp_code," = ",
                                   "</b>","<i>",tmp_string,"</i>",NULL);
        g_free(tmp_code);
        g_free(tmp_string);
        g_free(tmp_preview_text);

        tmp_preview_text = preview_text;
        preview_text = g_strconcat(tmp_preview_text,"  ||  ",NULL);
        g_free(tmp_preview_text);
    }

    Scan_Free_File_Fill_Tag_List(fill_tag_list);

    if (GTK_IS_LABEL(priv->fill_tag_preview_label))
    {
        if (preview_text)
        {
            //gtk_label_set_text(GTK_LABEL(priv->fill_tag_preview_label),preview_text);
            gtk_label_set_markup(GTK_LABEL(priv->fill_tag_preview_label),preview_text);
        } else
        {
            gtk_label_set_text(GTK_LABEL(priv->fill_tag_preview_label),"");
        }

        /* Force the window to be redrawn. */
        gtk_widget_queue_resize (GTK_WIDGET (self));
    }

    g_free(mask);
    g_free(preview_text);
}

static void
Scan_Rename_File_Generate_Preview (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    gchar *preview_text = NULL;
    gchar *mask = NULL;

    priv = et_scan_dialog_get_instance_private (self);

    if (!ETCore->ETFileDisplayed
    || !priv->rename_file_mask_combo || !priv->rename_file_preview_label)
        return;

    if (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) != ET_SCAN_MODE_RENAME_FILE)
        return;

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->rename_file_mask_combo)))));
    if (!mask)
        return;

    preview_text = et_scan_generate_new_filename_from_mask (ETCore->ETFileDisplayed,
                                                            mask, FALSE);

    if (GTK_IS_LABEL(priv->rename_file_preview_label))
    {
        if (preview_text)
        {
            //gtk_label_set_text(GTK_LABEL(priv->rename_file_preview_label),preview_text);
            gchar *tmp_string = g_markup_printf_escaped("%s",preview_text); // To avoid problem with strings containing characters like '&'
            gchar *str = g_strdup_printf("<i>%s</i>",tmp_string);
            gtk_label_set_markup(GTK_LABEL(priv->rename_file_preview_label),str);
            g_free(tmp_string);
            g_free(str);
        } else
        {
            gtk_label_set_text(GTK_LABEL(priv->rename_file_preview_label),"");
        }

        /* Force the window to be redrawn. */
        gtk_widget_queue_resize (GTK_WIDGET (self));
    }

    g_free(mask);
    g_free(preview_text);
}


void
et_scan_dialog_update_previews (EtScanDialog *self)
{
    g_return_if_fail (ET_SCAN_DIALOG (self));

    Scan_Fill_Tag_Generate_Preview (self);
    Scan_Rename_File_Generate_Preview (self);
}

static void
Scan_Free_File_Fill_Tag_List (GList *list)
{
    GList *l;

    list = g_list_first (list);

    for (l = list; l != NULL; l = g_list_next (l))
    {
        if (l->data)
        {
            g_free (((Scan_Mask_Item *)l->data)->string);
            g_slice_free (Scan_Mask_Item, l->data);
        }
    }

    g_list_free (list);
}



/**************************
 * Scanner To Rename File *
 **************************/
/*
 * Uses tag information (displayed into tag entries) to rename file
 * Note: mask and source are read from the right to the left.
 * Note1: a mask code may be used severals times...
 */
static void
Scan_Rename_File_With_Mask (EtScanDialog *self, ET_File *ETFile)
{
    EtScanDialogPrivate *priv;
    gchar *filename_generated_utf8 = NULL;
    gchar *filename_generated = NULL;
    gchar *filename_new_utf8 = NULL;
    gchar *mask = NULL;
    File_Name *FileName;

    g_return_if_fail (ETFile != NULL);

    priv = et_scan_dialog_get_instance_private (self);

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->rename_file_mask_combo)))));
    if (!mask) return;

    // Note : if the first character is '/', we have a path with the filename,
    // else we have only the filename. The both are in UTF-8.
    filename_generated_utf8 = et_scan_generate_new_filename_from_mask (ETFile,
                                                                       mask,
                                                                       FALSE);
    g_free(mask);

    if (!filename_generated_utf8)
        return;
    if (g_utf8_strlen(filename_generated_utf8,-1)<1)
    {
        g_free(filename_generated_utf8);
        return;
    }

    // Convert filename to file-system encoding
    filename_generated = filename_from_display(filename_generated_utf8);
    if (!filename_generated)
    {
        GtkWidget *msgdialog;
        msgdialog = gtk_message_dialog_new (GTK_WINDOW (self),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_ERROR,
                             GTK_BUTTONS_CLOSE,
                             _("Could not convert filename ‘%s’ into system filename encoding"),
                             filename_generated_utf8);
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Filename translation"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        g_free(filename_generated_utf8);
        return;
    }

    /* Build the filename with the full path or relative to old path */
    filename_new_utf8 = ET_File_Name_Generate(ETFile,filename_generated_utf8);
    g_free(filename_generated);
    g_free(filename_generated_utf8);

    /* Set the new filename */
    // Create a new 'File_Name' item
    FileName = ET_File_Name_Item_New();
    // Save changes of the 'File_Name' item
    ET_Set_Filename_File_Name_Item(FileName,filename_new_utf8,NULL);

    ET_Manage_Changes_Of_File_Data(ETFile,FileName,NULL);
    g_free(filename_new_utf8);

    et_application_window_status_bar_message (ET_APPLICATION_WINDOW (MainWindow),
                                              _("New filename successfully scanned"),
                                              TRUE);

    filename_new_utf8 = g_path_get_basename(((File_Name *)ETFile->FileNameNew->data)->value_utf8);
    Log_Print (LOG_OK, _("New filename successfully scanned ‘%s’"),
               filename_new_utf8);
    g_free(filename_new_utf8);

    return;
}

/*
 * Build the new filename using tag + mask
 * Used also to rename the directory (from the browser)
 * @param ETFile                     : the etfile to process
 * @param mask                       : the pattern to parse
 * @param no_dir_check_or_conversion : if FALSE, disable checking of a directory
 *      in the mask, and don't convert "illegal" characters. This is used in the
 *      function "Write_Playlist" for the content of the playlist.
 * Returns filename in UTF-8
 */
gchar *
et_scan_generate_new_filename_from_mask (const ET_File *ETFile,
                                         const gchar *mask,
                                         gboolean no_dir_check_or_conversion)
{
    gchar *tmp;
    gchar **source = NULL;
    gchar *path_utf8_cur = NULL;
    gchar *filename_new_utf8 = NULL;
    gchar *filename_tmp = NULL;
    GList *rename_file_list = NULL;
    GList *l;
    File_Mask_Item *mask_item;
    File_Mask_Item *mask_item_prev;
    File_Mask_Item *mask_item_next;
    gint counter = 0;

    g_return_val_if_fail (ETFile != NULL && mask != NULL, NULL);

    /*
     * Check for a directory in the mask
     */
    if (!no_dir_check_or_conversion)
    {
        if (g_path_is_absolute(mask))
        {
            // Absolute directory
        }else if (strrchr(mask,G_DIR_SEPARATOR)!=NULL) // This is '/' on UNIX machines and '\' under Windows
        {
            // Relative path => set beginning of the path
            path_utf8_cur = g_path_get_dirname( ((File_Name *)ETFile->FileNameCur->data)->value_utf8 );
        }
    }


    /*
     * Parse the codes to generate a list (1rst item = 1rst code)
     */
    while ( mask!=NULL && (tmp=strrchr(mask,'%'))!=NULL && strlen(tmp)>1 )
    {
        // Mask contains some characters after the code ('%b__')
        if (strlen(tmp)>2)
        {
            mask_item = g_slice_new0 (File_Mask_Item);
            if (counter)
            {
                if (strchr(tmp+2,G_DIR_SEPARATOR))
                    mask_item->type = DIRECTORY_SEPARATOR;
                else
                    mask_item->type = SEPARATOR;
            } else
            {
                mask_item->type = TRAILING_SEPARATOR;
            }
            mask_item->string = g_strdup(tmp+2);
            rename_file_list = g_list_prepend(rename_file_list,mask_item);
        }

        // Now, parses the code to get the corresponding string (from tag)
        source = Scan_Return_File_Tag_Field_From_Mask_Code((File_Tag *)ETFile->FileTag->data,tmp[1]);
        mask_item = g_slice_new0 (File_Mask_Item);
        if (source && *source && strlen(*source)>0)
        {
            mask_item->type = FIELD;
            mask_item->string = g_strdup(*source);

            // Replace invalid characters for this field
            /* Do not replace characters in a playlist information field. */
            if (!no_dir_check_or_conversion)
            {
                EtConvertSpaces convert_mode;

                ET_File_Name_Convert_Character (mask_item->string);
                convert_mode = g_settings_get_enum (MainSettings,
                                                    "rename-convert-spaces");

                switch (convert_mode)
                {
                    case ET_CONVERT_SPACES_SPACES:
                        Scan_Convert_Underscore_Into_Space (mask_item->string);
                        Scan_Convert_P20_Into_Space (mask_item->string);
                        break;
                    case ET_CONVERT_SPACES_UNDERSCORES:
                        Scan_Convert_Space_Into_Underscore (mask_item->string);
                        break;
                    case ET_CONVERT_SPACES_REMOVE:
                        Scan_Remove_Spaces (mask_item->string);
                        break;
                    default:
                        g_assert_not_reached ();
                }
            }
        }else
        {
            mask_item->type = EMPTY_FIELD;
            mask_item->string = NULL;
        }
        rename_file_list = g_list_prepend(rename_file_list,mask_item);
        *tmp = '\0'; // Cut parsed data of mask
        counter++; // To indicate that we made at least one loop to identifiate 'separator' or 'trailing_separator'
    }

    // It may have some characters before the last remaining code ('__%a')
    if (mask!=NULL && strlen(mask)>0)
    {
        mask_item = g_slice_new0 (File_Mask_Item);
        mask_item->type = LEADING_SEPARATOR;
        mask_item->string = g_strdup(mask);
        rename_file_list = g_list_prepend(rename_file_list,mask_item);
    }

    if (!rename_file_list) return NULL;

    /*
     * For Debugging : display the "rename_file_list" list
     */
    /***{
        GList *list = g_list_first(rename_file_list);
        gint i = 0;
        g_print("## rename_file_list - start\n");
        while (list)
        {
            File_Mask_Item *mask_item = (File_Mask_Item *)list->data;
            Mask_Item_Type  type      = mask_item->type;
            gchar          *string    = mask_item->string;

            //g_print("item %d : \n",i++);
            //g_print("  - type   : '%s'\n",type==UNKNOWN?"UNKNOWN":type==LEADING_SEPARATOR?"LEADING_SEPARATOR":type==TRAILING_SEPARATOR?"TRAILING_SEPARATOR":type==SEPARATOR?"SEPARATOR":type==DIRECTORY_SEPARATOR?"DIRECTORY_SEPARATOR":type==FIELD?"FIELD":type==EMPTY_FIELD?"EMPTY_FIELD":"???");
            //g_print("  - string : '%s'\n",string);
            g_print("%d -> %s (%s) | ",i++,type==UNKNOWN?"UNKNOWN":type==LEADING_SEPARATOR?"LEADING_SEPARATOR":type==TRAILING_SEPARATOR?"TRAILING_SEPARATOR":type==SEPARATOR?"SEPARATOR":type==DIRECTORY_SEPARATOR?"DIRECTORY_SEPARATOR":type==FIELD?"FIELD":type==EMPTY_FIELD?"EMPTY_FIELD":"???",string);

            list = list->next;
        }
        g_print("\n## rename_file_list - end\n\n");
    }***/

    /*
     * Build the new filename with items placed into the list
     * (read the list from the end to the beginning)
     */
    filename_new_utf8 = g_strdup("");

    for (l = g_list_last (rename_file_list); l != NULL;
         l = g_list_previous (l))
    {
        File_Mask_Item *mask_item = l->data;

        if ( mask_item->type==TRAILING_SEPARATOR ) // Trailing characters of mask
        {
            // Doesn't write it if previous field is empty
            if (l->prev
                && ((File_Mask_Item *)l->prev->data)->type != EMPTY_FIELD)
            {
                filename_tmp = filename_new_utf8;
                filename_new_utf8 = g_strconcat(mask_item->string,filename_new_utf8,NULL);
                g_free(filename_tmp);
            }
        }else
        if ( mask_item->type==EMPTY_FIELD )
        // We don't concatenate the field value (empty) and the previous
        // separator (except leading separator) to the filename.
        // If the empty field is the 'first', we don't concatenate it, and the
        // next separator too.
        {
            if (l->prev)
            {
                // The empty field isn't the first.
                // If previous string is a separator, we don't use it, except if the next
                // string is a FIELD (not empty)
                mask_item_prev = l->prev->data;
                if ( mask_item_prev->type==SEPARATOR )
                {
                    if (!(l->next
                        && (mask_item_next = rename_file_list->next->data)
                        && mask_item_next->type == FIELD))
                    {
                        l = l->prev;
                    }
                }
            }else
            if (l->next && (mask_item_next = l->next->data)
                && mask_item_next->type == SEPARATOR)
            // We are at the 'beginning' of the mask (so empty field is the first)
            // and next field is a separator. As the separator may have been already added, we remove it
            {
                if ( filename_new_utf8 && mask_item_next->string && (strncmp(filename_new_utf8,mask_item_next->string,strlen(mask_item_next->string))==0) ) // To avoid crash if filename_new_utf8 is 'empty'
                {
                    filename_tmp = filename_new_utf8;
                    filename_new_utf8 = g_strdup(filename_new_utf8+strlen(mask_item_next->string));
                    g_free(filename_tmp);
                 }
            }

        }else // SEPARATOR, FIELD, LEADING_SEPARATOR, DIRECTORY_SEPARATOR
        {
            filename_tmp = filename_new_utf8;
            filename_new_utf8 = g_strconcat(mask_item->string,filename_new_utf8,NULL);
            g_free(filename_tmp);
        }
    }

    // Free the list
    Scan_Free_File_Rename_List(rename_file_list);


    // Add current path if relative path entered
    if (path_utf8_cur)
    {
        filename_tmp = filename_new_utf8; // in UTF-8!
        filename_new_utf8 = g_strconcat(path_utf8_cur,G_DIR_SEPARATOR_S,filename_new_utf8,NULL);
        g_free(filename_tmp);
        g_free(path_utf8_cur);
    }

    return filename_new_utf8; // in UTF-8!
}

static void
Scan_Free_File_Rename_List (GList *list)
{
    GList *l;

    for (l = list; l != NULL; l = g_list_next (l))
    {
        if (l->data)
        {
            g_free (((File_Mask_Item *)l->data)->string);
            g_slice_free (File_Mask_Item, l->data);
        }
    }

    g_list_free (list);
}

/*
 * Adds the current path of the file to the mask on the "Rename File Scanner" entry
 */
static void
Scan_Rename_File_Prefix_Path (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    gint pos;
    gchar *path_tmp;
    const gchar *combo_text = NULL;
    gchar *combo_tmp;
    ET_File *ETFile = ETCore->ETFileDisplayed;
    gchar *filename_utf8_cur;
    gchar *path_utf8_cur;

    if (!ETFile)
    {
        return;
    }

    filename_utf8_cur = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;
    priv = et_scan_dialog_get_instance_private (self);

    // The path to prefix
    path_utf8_cur = g_path_get_dirname(filename_utf8_cur);

    // The current text in the combobox
    combo_text = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->rename_file_mask_combo))));
    /*if (!g_utf8_validate(combo_text, -1, NULL))
    {
        combo_tmp = convert_to_utf8(combo_text);
    }else
    {
        combo_tmp = g_strdup(combo_text);
    }*/
    combo_tmp = Try_To_Validate_Utf8_String(combo_text);

    // If the path already exists we don't add it again
    // Use g_utf8_collate_key instead of strncmp
    if (combo_tmp && path_utf8_cur && strncmp(combo_tmp,path_utf8_cur,strlen(path_utf8_cur))!=0)
    {
        if (g_path_is_absolute(combo_tmp))
        {
            path_tmp = g_strdup(path_utf8_cur);
        } else
        {
            path_tmp = g_strconcat(path_utf8_cur,G_DIR_SEPARATOR_S,NULL);
        }
	pos = 0;
        gtk_editable_insert_text(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(priv->rename_file_mask_combo))),path_tmp, -1, &pos);
        g_free(path_tmp);
    }

    g_free(path_utf8_cur);
}


gchar *
et_scan_generate_new_directory_name_from_mask (const ET_File *ETFile,
                                               const gchar *mask,
                                               gboolean no_dir_check_or_conversion)
{
    return et_scan_generate_new_filename_from_mask (ETFile, mask,
                                                    no_dir_check_or_conversion);
}


/*
 * Replace something with something else ;)
 * Here use Regular Expression, to search and replace.
 */
static void
Scan_Convert_Character (EtScanDialog *self, gchar **string)
{
    EtScanDialogPrivate *priv;
    gchar *from;
    gchar *to;
    GRegex *regex;
    GError *regex_error = NULL;
    gchar *new_string;

    priv = et_scan_dialog_get_instance_private (self);

    from = gtk_editable_get_chars (GTK_EDITABLE (priv->process_convert_from_entry), 0,
                                 -1);
    to = gtk_editable_get_chars (GTK_EDITABLE (priv->process_convert_to_entry), 0, -1);

    regex = g_regex_new (from, 0, 0, &regex_error);
    if (regex_error != NULL)
    {
        goto handle_error;
    }

    new_string = g_regex_replace (regex, *string, -1, 0, to, 0, &regex_error);
    if (regex_error != NULL)
    {
        g_free (new_string);
        g_regex_unref (regex);
        goto handle_error;
    }

    /* Success. */
    g_regex_unref (regex);
    g_free (*string);
    *string = new_string;

out:
    g_free (from);
    g_free (to);
    return;

handle_error:
    Log_Print (LOG_ERROR, _("Error while processing fields ‘%s’"),
               regex_error->message);

    g_error_free (regex_error);

    goto out;
}

static void
Scan_Process_Fields_Functions (EtScanDialog *self, gchar **string)
{
    EtScanDialogPrivate *priv;
    EtProcessFieldsConvert process;

    priv = et_scan_dialog_get_instance_private (self);

    process = g_settings_get_enum (MainSettings, "process-convert");

    switch (process)
    {
        case ET_PROCESS_FIELDS_CONVERT_SPACES:
            Scan_Convert_Underscore_Into_Space (*string);
            Scan_Convert_P20_Into_Space (*string);
            break;
        case ET_PROCESS_FIELDS_CONVERT_UNDERSCORES:
            Scan_Convert_Space_Into_Underscore (*string);
            break;
        case ET_PROCESS_FIELDS_CONVERT_CHARACTERS:
            Scan_Convert_Character (self, string);
            break;
        case ET_PROCESS_FIELDS_CONVERT_NO_CHANGE:
            break;
    }

    /* FIXME: Use GSettings keys instead of toggle buton states. */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->process_insert_space_toggle)))
    {
        gchar *res;
        res = Scan_Process_Fields_Insert_Space (*string);
        g_free (*string);
        *string = res;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->process_insert_one_space_toggle)))
        Scan_Process_Fields_Keep_One_Space(*string);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->process_all_uppercase_toggle)))
    {
        gchar *res;
        res = Scan_Process_Fields_All_Uppercase (*string);
        g_free (*string);
        *string = res;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->process_all_lowercase_toggle)))
    {
        gchar *res;
        res = Scan_Process_Fields_All_Downcase (*string);
        g_free (*string);
        *string = res;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->process_first_uppercase_toggle)))
    {
        gchar *res;
        res = Scan_Process_Fields_Letter_Uppercase (*string);
        g_free (*string);
        *string = res;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->process_first_style_uppercase_toggle)))
    {
        gboolean uppercase_preps;
        gboolean handle_roman;

        uppercase_preps = g_settings_get_boolean (MainSettings,
                                                  "process-uppercase-prepositions");
        handle_roman = g_settings_get_boolean (MainSettings,
                                               "process-detect-roman-numerals");
        Scan_Process_Fields_First_Letters_Uppercase (string, uppercase_preps,
                                                     handle_roman);
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->process_remove_space_toggle)))
        Scan_Process_Fields_Remove_Space(*string);

}


/*****************************
 * Scanner To Process Fields *
 *****************************/
/* See also functions : Convert_P20_And_Undescore_Into_Spaces, ... in easytag.c */
static void
Scan_Process_Fields (EtScanDialog *self, ET_File *ETFile)
{
    File_Name *FileName = NULL;
    File_Tag  *FileTag  = NULL;
    File_Name *st_filename;
    File_Tag  *st_filetag;
    guint process_fields;
    gchar     *filename_utf8;
    gchar     *string;

    g_return_if_fail (ETFile != NULL);

    st_filename = (File_Name *)ETFile->FileNameNew->data;
    st_filetag  = (File_Tag  *)ETFile->FileTag->data;
    process_fields = g_settings_get_flags (MainSettings, "process-fields");

    /* Process the filename */
    if (st_filename != NULL)
    {
        if (st_filename->value_utf8 
            && (process_fields & ET_PROCESS_FIELD_FILENAME))
        {
            gchar *string_utf8;
            gchar *pos;

            filename_utf8 = st_filename->value_utf8;

            if (!FileName)
                FileName = ET_File_Name_Item_New();

            string = g_path_get_basename(filename_utf8);
            // Remove the extension to set it to lower case (to avoid problem with undo)
            if ((pos=strrchr(string,'.'))!=NULL) *pos = 0;

            Scan_Process_Fields_Functions (self, &string);

            string_utf8 = ET_File_Name_Generate(ETFile,string);
            ET_Set_Filename_File_Name_Item(FileName,string_utf8,NULL);
            g_free(string_utf8);
            g_free(string);
        }
    }

    /* Process data of the tag */
    if (st_filetag != NULL)
    {
        /* Title field. */
        if (st_filetag->title
            && (process_fields & ET_PROCESS_FIELD_TITLE))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->title);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->title,string);

            g_free(string);
        }

        /* Artist field. */
        if (st_filetag->artist
            && (process_fields & ET_PROCESS_FIELD_ARTIST))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->artist);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->artist,string);

            g_free(string);
        }

        /* Album Artist field. */
        if (st_filetag->album_artist
            && (process_fields & ET_PROCESS_FIELD_ALBUM_ARTIST))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->album_artist);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->album_artist,string);

            g_free(string);
        }

        /* Album field. */
        if (st_filetag->album
            && (process_fields & ET_PROCESS_FIELD_ALBUM))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->album);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->album,string);

            g_free(string);
        }

        /* Genre field. */
        if (st_filetag->genre
            && (process_fields & ET_PROCESS_FIELD_GENRE))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->genre);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->genre,string);

            g_free(string);
        }

        /* Comment field. */
        if (st_filetag->comment
            && (process_fields & ET_PROCESS_FIELD_COMMENT))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->comment);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->comment,string);

            g_free(string);
        }

        /* Composer field. */
        if (st_filetag->composer
            && (process_fields & ET_PROCESS_FIELD_COMPOSER))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->composer);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->composer,string);

            g_free(string);
        }

        /* Original artist field. */
        if (st_filetag->orig_artist
            && (process_fields & ET_PROCESS_FIELD_ORIGINAL_ARTIST))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->orig_artist);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->orig_artist,string);

            g_free(string);
        }

        /* Copyright field. */
        if (st_filetag->copyright
            && (process_fields & ET_PROCESS_FIELD_COPYRIGHT))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->copyright);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->copyright,string);

            g_free(string);
        }

        /* URL field. */
        if (st_filetag->url
            && (process_fields & ET_PROCESS_FIELD_URL))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->url);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->url,string);

            g_free(string);
        }

        /* 'Encoded by' field. */
        if (st_filetag->encoded_by
            && (process_fields & ET_PROCESS_FIELD_ENCODED_BY))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->encoded_by);

            Scan_Process_Fields_Functions (self, &string);

            ET_Set_Field_File_Tag_Item(&FileTag->encoded_by,string);

            g_free(string);
        }
    }

    if (FileName && FileTag)
    {
        // Synchronize undo key of the both structures (used for the
        // undo functions, as they are generated as the same time)
        FileName->key = FileTag->key;
    }
    ET_Manage_Changes_Of_File_Data(ETFile,FileName,FileTag);

}

/*
 * Return the field of a 'File_Tag' structure corresponding to the mask code
 */
static gchar
**Scan_Return_File_Tag_Field_From_Mask_Code (File_Tag *FileTag, gchar code)
{
    switch (code)
    {
        case 't':    /* Title */
            return &FileTag->title;
        case 'a':    /* Artist */
            return &FileTag->artist;
        case 'b':    /* Album */
            return &FileTag->album;
        case 'd':    /* Disc Number */
            return &FileTag->disc_number;
        case 'x': /* Total number of discs. */
            return &FileTag->disc_total;
        case 'y':    /* Year */
            return &FileTag->year;
        case 'n':    /* Track */
            return &FileTag->track;
        case 'l':    /* Track Total */
            return &FileTag->track_total;
        case 'g':    /* Genre */
            return &FileTag->genre;
        case 'c':    /* Comment */
            return &FileTag->comment;
        case 'p':    /* Composer */
            return &FileTag->composer;
        case 'o':    /* Orig. Artist */
            return &FileTag->orig_artist;
        case 'r':    /* Copyright */
            return &FileTag->copyright;
        case 'u':    /* URL */
            return &FileTag->url;
        case 'e':    /* Encoded by */
            return &FileTag->encoded_by;
        case 'z':    /* Album Artist */
            return &FileTag->album_artist;
        case 'i':    /* Ignored */
            return NULL;
        default:
            Log_Print(LOG_ERROR,"Scanner: Invalid code '%%%c' found!",code);
            return NULL;
    }
}



/******************
 * Scanner Window *
 ******************/
/*
 * Function when you select an item of the option menu
 */
static void
on_scan_mode_changed (EtScanDialog *self, gchar *key, GSettings *settings)
{
    EtScanDialogPrivate *priv;
    EtScanMode mode;

    priv = et_scan_dialog_get_instance_private (self);

    mode = g_settings_get_enum (settings, key);

    switch (mode)
    {
        case ET_SCAN_MODE_FILL_TAG:
            gtk_widget_show(priv->mask_editor_toggle);
            gtk_widget_show(priv->legend_toggle);
            gtk_tree_view_set_model(GTK_TREE_VIEW(priv->mask_editor_view), GTK_TREE_MODEL(priv->scan_tag_masks_model));
            Scan_Fill_Tag_Generate_Preview (self);
            g_signal_emit_by_name(G_OBJECT(priv->legend_toggle),"toggled");        /* To hide or show legend frame */
            g_signal_emit_by_name(G_OBJECT(priv->mask_editor_toggle),"toggled");    /* To hide or show mask editor frame */
            break;

        case ET_SCAN_MODE_RENAME_FILE:
            gtk_widget_show(priv->mask_editor_toggle);
            gtk_widget_show(priv->legend_toggle);
            gtk_tree_view_set_model(GTK_TREE_VIEW(priv->mask_editor_view), GTK_TREE_MODEL(priv->rename_masks_model));
            Scan_Rename_File_Generate_Preview (self);
            g_signal_emit_by_name(G_OBJECT(priv->legend_toggle),"toggled");        /* To hide or show legend frame */
            g_signal_emit_by_name(G_OBJECT(priv->mask_editor_toggle),"toggled");    /* To hide or show mask editor frame */
            break;

        case ET_SCAN_MODE_PROCESS_FIELDS:
            gtk_widget_hide(priv->mask_editor_toggle);
            gtk_widget_hide(priv->legend_toggle);
            // Hide directly the frames to don't change state of the buttons!
            gtk_widget_hide(priv->legend_frame);
            gtk_widget_hide(priv->mask_editor_frame);

            gtk_tree_view_set_model(GTK_TREE_VIEW(priv->mask_editor_view), NULL);
            break;
        default:
            g_assert_not_reached ();
    }

    /* TODO: Either duplicate the legend and mask editor, or split the dialog.
     */
    if (mode == ET_SCAN_MODE_FILL_TAG || mode == ET_SCAN_MODE_RENAME_FILE)
    {
        GtkWidget *parent;

        parent = gtk_widget_get_parent (priv->mask_editor_frame);

        if ((mode == ET_SCAN_MODE_RENAME_FILE && parent != priv->rename_grid)
            || (mode == ET_SCAN_MODE_FILL_TAG && parent != priv->fill_grid))
        {
            g_object_ref (priv->mask_editor_frame);
            g_object_ref (priv->legend_frame);
            gtk_container_remove (GTK_CONTAINER (parent),
                                  priv->mask_editor_frame);
            gtk_container_remove (GTK_CONTAINER (parent), priv->legend_frame);

            if (mode == ET_SCAN_MODE_RENAME_FILE)
            {
                gtk_container_add (GTK_CONTAINER (priv->rename_grid),
                                   priv->mask_editor_frame);
                gtk_container_add (GTK_CONTAINER (priv->rename_grid),
                                   priv->legend_frame);
            }
            else
            {
                gtk_container_add (GTK_CONTAINER (priv->fill_grid),
                                   priv->mask_editor_frame);
                gtk_container_add (GTK_CONTAINER (priv->fill_grid),
                                   priv->legend_frame);
            }

            g_object_unref (priv->mask_editor_frame);
            g_object_unref (priv->legend_frame);
        }
    }
}

void
et_scan_dialog_open (EtScanDialog *self, EtScanMode scanner_type)
{
    g_return_if_fail (ET_SCAN_DIALOG (self));
    g_return_if_fail (scanner_type >= ET_SCAN_MODE_FILL_TAG
                      && scanner_type <= ET_SCAN_MODE_PROCESS_FIELDS);

    if (g_settings_get_enum (MainSettings, "scan-mode") != scanner_type)
    {
        g_settings_set_enum (MainSettings, "scan-mode", scanner_type);
    }
}

static void
Mask_Editor_List_Add (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    gint i = 0;
    GtkTreeModel *treemodel;
    gchar *temp;

    priv = et_scan_dialog_get_instance_private (self);

    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));

    if (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) != ET_SCAN_MODE_FILL_TAG)
    {
        while(Scan_Masks[i])
        {
            /*if (!g_utf8_validate(Scan_Masks[i], -1, NULL))
                temp = convert_to_utf8(Scan_Masks[i]);
            else
                temp = g_strdup(Scan_Masks[i]);*/
            temp = Try_To_Validate_Utf8_String(Scan_Masks[i]);

            gtk_list_store_insert_with_values (GTK_LIST_STORE (treemodel),
                                               NULL, G_MAXINT,
                                               MASK_EDITOR_TEXT, temp, -1);
            g_free(temp);
            i++;
        }
    } else if (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) == ET_SCAN_MODE_RENAME_FILE)
    {
        while(Rename_File_Masks[i])
        {
            /*if (!g_utf8_validate(Rename_File_Masks[i], -1, NULL))
                temp = convert_to_utf8(Rename_File_Masks[i]);
            else
                temp = g_strdup(Rename_File_Masks[i]);*/
            temp = Try_To_Validate_Utf8_String(Rename_File_Masks[i]);

            gtk_list_store_insert_with_values (GTK_LIST_STORE (treemodel),
                                               NULL, G_MAXINT,
                                               MASK_EDITOR_TEXT, temp, -1);
            g_free(temp);
            i++;
        }
    }
}

/*
 * Clean up the currently displayed masks lists, ready for saving
 */
static void
Mask_Editor_Clean_Up_Masks_List (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    gchar *text = NULL;
    gchar *text1 = NULL;
    GtkTreeIter currentIter;
    GtkTreeIter itercopy;
    GtkTreeModel *treemodel;

    priv = et_scan_dialog_get_instance_private (self);

    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));

    /* Remove blank and duplicate items */
    if (gtk_tree_model_get_iter_first(treemodel, &currentIter))
    {

        while(TRUE)
        {
            gtk_tree_model_get(treemodel, &currentIter, MASK_EDITOR_TEXT, &text, -1);

            /* Check for blank entry */
            if (text && g_utf8_strlen(text, -1) == 0)
            {
                g_free(text);

                if (!gtk_list_store_remove(GTK_LIST_STORE(treemodel), &currentIter))
                    break; /* No following entries */
                else
                    continue; /* Go on to next entry, which the remove function already moved onto for us */
            }

            /* Check for duplicate entries */
            itercopy = currentIter;
            if (!gtk_tree_model_iter_next(treemodel, &itercopy))
            {
                g_free(text);
                break;
            }

            while(TRUE)
            {
                gtk_tree_model_get(treemodel, &itercopy, MASK_EDITOR_TEXT, &text1, -1);
                if (text1 && g_utf8_collate(text,text1) == 0)
                {
                    g_free(text1);

                    if (!gtk_list_store_remove(GTK_LIST_STORE(treemodel), &itercopy))
                        break; /* No following entries */
                    else
                        continue; /* Go on to next entry, which the remove function already set iter to for us */

                }
                g_free(text1);
                if (!gtk_tree_model_iter_next(treemodel, &itercopy))
                    break;
            }

            g_free(text);

            if (!gtk_tree_model_iter_next(treemodel, &currentIter))
                break;
        }
    }
}

/*
 * Save the currently displayed mask list in the mask editor
 */
static void
Mask_Editor_List_Save_Button (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;

    priv = et_scan_dialog_get_instance_private (self);

    Mask_Editor_Clean_Up_Masks_List (self);

    if (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) == ET_SCAN_MODE_FILL_TAG)
    {
        Save_Scan_Tag_Masks_List(priv->scan_tag_masks_model, MASK_EDITOR_TEXT);
    }
    else if (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) == ET_SCAN_MODE_RENAME_FILE)
    {
        Save_Rename_File_Masks_List(priv->rename_masks_model, MASK_EDITOR_TEXT);
    }
}

static void
Process_Fields_First_Letters_Check_Button_Toggled (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;

    priv = et_scan_dialog_get_instance_private (self);

    gtk_widget_set_sensitive (GTK_WIDGET (priv->process_roman_numerals_check),
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->process_first_style_uppercase_toggle)));
}

/*
 * Set sensitive state of the processing check boxes : if no one is selected => all disabled
 */
static void
on_process_fields_changed (EtScanDialog *self,
                           gchar *key,
                           GSettings *settings)
{
    EtScanDialogPrivate *priv;

    priv = et_scan_dialog_get_instance_private (self);

    if (g_settings_get_flags (settings, key) != 0)
    {
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_to_space_toggle),     TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_to_underscores_toggle),         TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_toggle),              TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_label),       TRUE);
        // Activate the two entries only if the check box is activated, else keep them disabled
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->process_convert_toggle)))
        {
            gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_to_entry),        TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_from_entry),      TRUE);
        }
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_all_uppercase_toggle),         TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_all_lowercase_toggle),          TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_first_uppercase_toggle), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_first_style_uppercase_toggle),TRUE);
        Process_Fields_First_Letters_Check_Button_Toggled (self);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_remove_space_toggle),          TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_insert_space_toggle),          TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_insert_one_space_toggle),         TRUE);
    }else
    {
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_to_space_toggle),     FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_to_underscores_toggle),         FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_toggle),              FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_to_entry),            FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_label),       FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_convert_from_entry),          FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_all_uppercase_toggle),         FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_all_lowercase_toggle),          FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_first_uppercase_toggle), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_first_style_uppercase_toggle),FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_roman_numerals_check),  FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_remove_space_toggle),          FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_insert_space_toggle),          FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(priv->process_insert_one_space_toggle),         FALSE);
    }
}

static void
Scan_Toggle_Legend_Button (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;

    priv = et_scan_dialog_get_instance_private (self);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->legend_toggle)))
    {
        gtk_widget_show_all (priv->legend_frame);
    }
    else
    {
        gtk_widget_hide (priv->legend_frame);
    }
}

static void
Scan_Toggle_Mask_Editor_Button (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    GtkTreeModel *treemodel;
    GtkTreeSelection *selection;
    GtkTreeIter iter;

    priv = et_scan_dialog_get_instance_private (self);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->mask_editor_toggle)))
    {
        gtk_widget_show_all(priv->mask_editor_frame);

        // Select first row in list
        treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));
        if (gtk_tree_model_get_iter_first(treemodel, &iter))
        {
            selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->mask_editor_view));
            gtk_tree_selection_unselect_all(selection);
            gtk_tree_selection_select_iter(selection, &iter);
        }

        // Update status of the icon box cause prev instruction show it for all cases
        g_signal_emit_by_name(G_OBJECT(priv->mask_editor_entry),"changed");
    }else
    {
        gtk_widget_hide(priv->mask_editor_frame);
    }
}

/*
 * Update the Mask List with the new value of the entry box
 */
static void
Mask_Editor_Entry_Changed (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreePath *firstSelected;
    GtkTreeModel *treemodel;
    GList *selectedRows;
    GtkTreeIter row;
    const gchar* text;

    priv = et_scan_dialog_get_instance_private (self);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->mask_editor_view));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (!selectedRows)
    {
        return;
    }

    firstSelected = (GtkTreePath *)g_list_first(selectedRows)->data;
    text = gtk_entry_get_text(GTK_ENTRY(priv->mask_editor_entry));

    if (gtk_tree_model_get_iter (treemodel, &row, firstSelected))
    {
        gtk_list_store_set(GTK_LIST_STORE(treemodel), &row, MASK_EDITOR_TEXT, text, -1);
    }

    g_list_free_full (selectedRows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * Callback from the mask edit list
 * Previously known as Mask_Editor_List_Select_Row
 */
static void
Mask_Editor_List_Row_Selected (GtkTreeSelection* selection, EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    GList *selectedRows;
    gchar *text = NULL;
    GtkTreePath *lastSelected;
    GtkTreeIter lastFile;
    GtkTreeModel *treemodel;
    gboolean valid;

    priv = et_scan_dialog_get_instance_private (self);

    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));

    /* We must block the function, else the previous selected row will be modified */
    g_signal_handlers_block_by_func(G_OBJECT(priv->mask_editor_entry),
                                    G_CALLBACK(Mask_Editor_Entry_Changed),NULL);

    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    /*
     * At some point, we might get called when no rows are selected?
     */
    if (!selectedRows)
    {
        g_signal_handlers_unblock_by_func(G_OBJECT(priv->mask_editor_entry),
                                          G_CALLBACK(Mask_Editor_Entry_Changed),NULL);
        return;
    }

    /* Get the text of the last selected row */
    lastSelected = (GtkTreePath *)g_list_last(selectedRows)->data;

    valid= gtk_tree_model_get_iter(treemodel, &lastFile, lastSelected);
    if (valid)
    {
        gtk_tree_model_get(treemodel, &lastFile, MASK_EDITOR_TEXT, &text, -1);

        if (text)
        {
            gtk_entry_set_text(GTK_ENTRY(priv->mask_editor_entry),text);
            g_free(text);
        }
    }

    g_signal_handlers_unblock_by_func(G_OBJECT(priv->mask_editor_entry),
                                      G_CALLBACK(Mask_Editor_Entry_Changed),NULL);

    g_list_free_full (selectedRows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * Remove the selected rows from the mask editor list
 */
static void
Mask_Editor_List_Remove (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkTreeModel *treemodel;

    priv = et_scan_dialog_get_instance_private (self);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->mask_editor_view));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));

    if (gtk_tree_selection_count_selected_rows(selection) == 0) {
        g_critical ("%s", "Remove: No row selected");
        return;
    }

    if (!gtk_tree_model_get_iter_first(treemodel, &iter))
        return;

    while (TRUE)
    {
        if (gtk_tree_selection_iter_is_selected(selection, &iter))
        {
            if (!gtk_list_store_remove(GTK_LIST_STORE(treemodel), &iter))
            {
                break;
            }
        } else
        {
            if (!gtk_tree_model_iter_next(treemodel, &iter))
            {
                break;
            }
        }
    }
}

/*
 * Actions when the a key is pressed into the masks editor clist
 */
static gboolean
Mask_Editor_List_Key_Press (GtkWidget *widget,
                            GdkEvent *event,
                            EtScanDialog *self)
{
    if (event && event->type == GDK_KEY_PRESS)
    {
        GdkEventKey *kevent = (GdkEventKey *)event;

        switch (kevent->keyval)
        {
            case GDK_KEY_Delete:
                Mask_Editor_List_Remove (self);
                return GDK_EVENT_STOP;
                break;
        }
    }

    return GDK_EVENT_PROPAGATE;
}

/*
 * Add a new mask to the list
 */
static void
Mask_Editor_List_New (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    gchar *text;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreeModel *treemodel;

    priv = et_scan_dialog_get_instance_private (self);

    text = _("New_mask");
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->mask_editor_view));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));

    gtk_list_store_insert(GTK_LIST_STORE(treemodel), &iter, 0);
    gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter, MASK_EDITOR_TEXT, text, -1);

    gtk_tree_selection_unselect_all(selection);
    gtk_tree_selection_select_iter(selection, &iter);
}

/*
 * Move all selected rows up one place in the mask list
 */
static void
Mask_Editor_List_Move_Up (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    GtkTreeSelection *selection;
    GList *selectedRows;
    GList *l;
    GtkTreeIter currentFile;
    GtkTreeIter nextFile;
    GtkTreePath *currentPath;
    GtkTreeModel *treemodel;

    priv = et_scan_dialog_get_instance_private (self);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->mask_editor_view));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (!selectedRows)
    {
        g_critical ("%s", "Move Up: No row selected");
        return;
    }

    for (l = selectedRows; l != NULL; l = g_list_next (l))
    {
        currentPath = (GtkTreePath *)l->data;
        if (gtk_tree_model_get_iter(treemodel, &currentFile, currentPath))
        {
            /* Find the entry above the node... */
            if (gtk_tree_path_prev(currentPath))
            {
                /* ...and if it exists, swap the two rows by iter */
                gtk_tree_model_get_iter(treemodel, &nextFile, currentPath);
                gtk_list_store_swap(GTK_LIST_STORE(treemodel), &currentFile, &nextFile);
            }
        }
    }

    g_list_free_full (selectedRows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * Move all selected rows down one place in the mask list
 */
static void
Mask_Editor_List_Move_Down (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    GtkTreeSelection *selection;
    GList *selectedRows;
    GList *l;
    GtkTreeIter currentFile;
    GtkTreeIter nextFile;
    GtkTreePath *currentPath;
    GtkTreeModel *treemodel;

    priv = et_scan_dialog_get_instance_private (self);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->mask_editor_view));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (!selectedRows)
    {
        g_critical ("%s", "Move Down: No row selected");
        return;
    }

    for (l = selectedRows; l != NULL; l = g_list_next (l))
    {
        currentPath = (GtkTreePath *)l->data;

        if (gtk_tree_model_get_iter(treemodel, &currentFile, currentPath))
        {
            /* Find the entry below the node and swap the two nodes by iter */
            gtk_tree_path_next(currentPath);
            if (gtk_tree_model_get_iter(treemodel, &nextFile, currentPath))
                gtk_list_store_swap(GTK_LIST_STORE(treemodel), &currentFile, &nextFile);
        }
    }

    g_list_free_full (selectedRows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * Set a row visible in the mask editor list (by scrolling the list)
 */
static void
Mask_Editor_List_Set_Row_Visible (GtkTreeView *view,
                                  GtkTreeModel *treeModel,
                                  GtkTreeIter *rowIter)
{
    /*
     * TODO: Make this only scroll to the row if it is not visible
     * (like in easytag GTK1)
     * See function gtk_tree_view_get_visible_rect() ??
     */
    GtkTreePath *rowPath;

    g_return_if_fail (treeModel != NULL);

    rowPath = gtk_tree_model_get_path (treeModel, rowIter);
    gtk_tree_view_scroll_to_cell (view, rowPath, NULL, FALSE, 0, 0);
    gtk_tree_path_free (rowPath);
}

/*
 * Duplicate a mask on the list
 */
static void
Mask_Editor_List_Duplicate (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    gchar *text = NULL;
    GList *selectedRows;
    GList *l;
    GList *toInsert = NULL;
    GtkTreeSelection *selection;
    GtkTreeIter rowIter;
    GtkTreeModel *treeModel;

    priv = et_scan_dialog_get_instance_private (self);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->mask_editor_view));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);
    treeModel = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->mask_editor_view));

    if (!selectedRows)
    {
        g_critical ("%s", "Copy: No row selected");
        return;
    }

    /* Loop through selected rows, duplicating them into a GList
     * We cannot directly insert because the paths in selectedRows
     * get out of date after an insertion */
    for (l = selectedRows; l != NULL; l = g_list_next (l))
    {
        if (gtk_tree_model_get_iter (treeModel, &rowIter,
                                     (GtkTreePath*)l->data))
        {
            gtk_tree_model_get(treeModel, &rowIter, MASK_EDITOR_TEXT, &text, -1);
            toInsert = g_list_prepend (toInsert, text);
        }
    }

    for (l = toInsert; l != NULL; l = g_list_next (l))
    {
        gtk_list_store_insert_with_values (GTK_LIST_STORE(treeModel), &rowIter,
                                           0, MASK_EDITOR_TEXT,
                                           (gchar *)l->data, -1);
    }

    /* Set focus to the last inserted line. */
    if (toInsert)
    {
        Mask_Editor_List_Set_Row_Visible (GTK_TREE_VIEW (priv->mask_editor_view),
                                          treeModel, &rowIter);
    }

    /* Free data no longer needed */
    g_list_free_full (selectedRows, (GDestroyNotify)gtk_tree_path_free);
    g_list_free_full (toInsert, (GDestroyNotify)g_free);
}

static void
Process_Fields_Convert_Check_Button_Toggled (EtScanDialog *self, GtkWidget *object)
{
    EtScanDialogPrivate *priv;

    priv = et_scan_dialog_get_instance_private (self);

    gtk_widget_set_sensitive (priv->process_convert_to_entry,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->process_convert_toggle)));
    gtk_widget_set_sensitive (priv->process_convert_from_entry,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->process_convert_toggle)));
}

/* Make sure that the Show Scanner toggle action is updated. */
static void
et_scan_on_hide (GtkWidget *widget,
                 gpointer user_data)
{
    g_action_group_activate_action (G_ACTION_GROUP (MainWindow), "scanner",
                                    NULL);
}

static void
create_scan_dialog (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;
    GtkWidget *scan_button;
    GtkWidget *ScanVBox;
    GtkWidget *button;
    GtkWidget *icon;
    GtkWidget *process_fields_convert_none;
    GtkBuilder *builder;
    GError *error = NULL;

    priv = et_scan_dialog_get_instance_private (self);

    /* The window */
    gtk_window_set_title (GTK_WINDOW (self), _("Tag and Filename Scan"));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (self), TRUE);
    gtk_dialog_add_buttons (GTK_DIALOG (self), _("_Close"),
                            GTK_RESPONSE_CLOSE, NULL);

    /* 'Scan selected files' button */
    scan_button = gtk_button_new_with_label (_("Scan Files"));
    gtk_widget_set_can_default (scan_button, TRUE);
    gtk_dialog_add_action_widget (GTK_DIALOG (self), scan_button,
                                  GTK_RESPONSE_APPLY);
    gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_APPLY);
    gtk_widget_show (scan_button);
    gtk_widget_set_tooltip_text (scan_button, _("Scan selected files"));

    /* The response signal handles close, scan and the delete event. */
    g_signal_connect (self, "response", G_CALLBACK (et_scan_on_response),
                      NULL);
    g_signal_connect (self, "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    g_signal_connect (self, "hide", G_CALLBACK (et_scan_on_hide), NULL);

    /* The main vbox */
    ScanVBox = gtk_dialog_get_content_area (GTK_DIALOG (self));
    gtk_container_set_border_width (GTK_CONTAINER (self), 6);
    gtk_box_set_spacing (GTK_BOX (ScanVBox), 12);

    /*
     * The hbox for mode buttons + buttons + what to scan
     */
    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder,
                                   "/org/gnome/EasyTAG/scan_dialog.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get scanner dialog from resource: %s",
                 error->message);
    }

    priv->notebook = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "scan_notebook"));
    gtk_container_add (GTK_CONTAINER (ScanVBox), priv->notebook);
    g_settings_bind_with_mapping (MainSettings, "scan-mode", priv->notebook,
                                  "page", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_get, et_settings_enum_set,
                                  GSIZE_TO_POINTER (ET_TYPE_SCAN_MODE), NULL);
    g_signal_connect_swapped (MainSettings, "changed::scan-mode",
                              G_CALLBACK (on_scan_mode_changed),
                              self);

    priv->fill_grid = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "fill_grid"));
    priv->rename_grid = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "rename_grid"));

    /* Options button */
    button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                 "scanner_preferences_button"));
    g_signal_connect (button, "clicked", G_CALLBACK (Scan_Option_Button),
                      NULL);

    /* Mask Editor button */
    priv->mask_editor_toggle = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                   "scanner_mask_toggle"));
    icon = gtk_image_new_from_resource ("/org/gnome/EasyTAG/images/mask.png");
    gtk_button_set_image (GTK_BUTTON (priv->mask_editor_toggle), icon);
    g_settings_bind (MainSettings, "scan-mask-editor-show",
                     priv->mask_editor_toggle, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_signal_connect_swapped (priv->mask_editor_toggle, "toggled",
                              G_CALLBACK (Scan_Toggle_Mask_Editor_Button),
                              self);

    /* Legend button */
    priv->legend_toggle = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "scanner_legend_toggle"));
    g_settings_bind (MainSettings, "scan-legend-show", priv->legend_toggle,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_signal_connect_swapped (priv->legend_toggle, "toggled",
                              G_CALLBACK (Scan_Toggle_Legend_Button),
                              self);

    /* Frame for Scan Tag. */
    /* Set up list model which is used both by the combobox and the editor. */
    priv->scan_tag_masks_model = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                                         "fill_masks_model"));

    /* The combo box to select the mask to apply. */
    priv->scan_tag_mask_combo = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                    "fill_combo"));

    /* Signal to generate preview (preview of the new tag values). */
    g_signal_connect_swapped (gtk_bin_get_child (GTK_BIN (priv->scan_tag_mask_combo)),
                              "changed",
                              G_CALLBACK (Scan_Fill_Tag_Generate_Preview),
                              self);

    /* Load masks into the combobox from a file. */
    Load_Scan_Tag_Masks_List (priv->scan_tag_masks_model, MASK_EDITOR_TEXT,
                              Scan_Masks);
    g_settings_bind (MainSettings, "scan-tag-default-mask",
                     gtk_bin_get_child (GTK_BIN (priv->scan_tag_mask_combo)),
                     "text", G_SETTINGS_BIND_DEFAULT);
    Add_String_To_Combo_List (priv->scan_tag_masks_model,
                              gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->scan_tag_mask_combo)))));

    /* Mask status icon. Signal connection to check if mask is correct in the
     * mask entry. */
    g_signal_connect (gtk_bin_get_child (GTK_BIN (priv->scan_tag_mask_combo)),
                      "changed", G_CALLBACK (entry_check_scan_tag_mask),
                      NULL);

    /* Preview label. */
    priv->fill_tag_preview_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                       "fill_preview_label"));

    /* Frame for Rename File. */
    /* Button to prefix path. */
    button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                 "rename_prefix_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Scan_Rename_File_Prefix_Path), self);

    /* Set up list model which is used both by the combobox and the editor. */
    priv->rename_masks_model = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                                       "rename_masks_model"));

    /* The combo box to select the mask to apply. */
    priv->rename_file_mask_combo = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                       "rename_combo"));

    /* Signal to generate preview (preview of the new filename). */
    g_signal_connect_swapped (gtk_bin_get_child (GTK_BIN (priv->rename_file_mask_combo)),
                              "changed",
                              G_CALLBACK (Scan_Rename_File_Generate_Preview),
                              self);

    /* Load masks into the combobox from a file. */
    Load_Rename_File_Masks_List (priv->rename_masks_model, MASK_EDITOR_TEXT,
                                 Rename_File_Masks);
    g_settings_bind (MainSettings, "rename-file-default-mask",
                     gtk_bin_get_child (GTK_BIN (priv->rename_file_mask_combo)),
                     "text", G_SETTINGS_BIND_DEFAULT);
    Add_String_To_Combo_List (priv->rename_masks_model,
                              gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->rename_file_mask_combo)))));

    /* Mask status icon. Signal connection to check if mask is correct to the
     * mask entry. */
    g_signal_connect (gtk_bin_get_child (GTK_BIN (priv->rename_file_mask_combo)),
                      "changed", G_CALLBACK (entry_check_rename_file_mask),
                      NULL);

    /* Preview label */
    priv->rename_file_preview_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                          "rename_preview_label"));

    /* Frame for Processing Fields. */
    /* Group: select entry fields to process */
    {
        gsize i;
        static const struct
        {
            const gchar *id;
        } ids[] =
        {
            { "process_filename_check" },
            { "process_title_check" },
            { "process_artist_check" },
            { "process_album_artist_check" },
            { "process_album_check" },
            { "process_genre_check" },
            { "process_comment_check" },
            { "process_composer_check" },
            { "process_orig_artist_check" },
            { "process_copyright_check" },
            { "process_url_check" },
            { "process_encoded_by_check" }
        };

        for (i = 0; i < G_N_ELEMENTS (ids); i++)
        {
            GtkWidget *widget;

            widget = GTK_WIDGET (gtk_builder_get_object (builder, ids[i].id));
            g_object_set_data (G_OBJECT (widget), "flags-type",
                               GSIZE_TO_POINTER (ET_TYPE_PROCESS_FIELD));
            g_settings_bind_with_mapping (MainSettings, "process-fields",
                                          widget, "active",
                                          G_SETTINGS_BIND_DEFAULT,
                                          et_settings_flags_toggle_get,
                                          et_settings_flags_toggle_set, widget,
                                          NULL);
        }
    }

    g_signal_connect_swapped (MainSettings, "changed::process-fields",
                              G_CALLBACK (on_process_fields_changed), self);

    /* Group: character conversion */
    priv->process_convert_to_space_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "convert_space_radio"));
    priv->process_convert_to_underscores_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "convert_underscores_radio"));
    priv->process_convert_toggle = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                       "convert_string_radio"));
    priv->process_convert_from_entry = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                           "convert_from_entry"));
    priv->process_convert_to_entry = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                         "convert_to_entry"));
    priv->process_convert_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                      "convert_to_label"));
    process_fields_convert_none = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                      "convert_none_radio"));

    /* Toggled signals */
    g_signal_connect_swapped (priv->process_convert_toggle, "toggled",
                              G_CALLBACK (Process_Fields_Convert_Check_Button_Toggled),
                              self);
    /* Set check buttons to init value */
    g_settings_bind_with_mapping (MainSettings, "process-convert",
                                  priv->process_convert_to_space_toggle,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->process_convert_to_space_toggle, NULL);
    g_settings_bind_with_mapping (MainSettings, "process-convert",
                                  priv->process_convert_to_underscores_toggle,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->process_convert_to_underscores_toggle,
                                  NULL);
    g_settings_bind_with_mapping (MainSettings, "process-convert",
                                  priv->process_convert_toggle,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->process_convert_toggle, NULL);
    g_settings_bind_with_mapping (MainSettings, "process-convert",
                                  process_fields_convert_none,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  process_fields_convert_none, NULL);
    g_settings_bind (MainSettings, "process-convert-characters-from",
                     priv->process_convert_from_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "process-convert-characters-to",
                     priv->process_convert_to_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* Group: capitalize, ... */
    priv->process_all_uppercase_toggle = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                             "capitalize_all_radio"));
    priv->process_all_lowercase_toggle  = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                              "capitalize_lower_radio"));
    priv->process_first_uppercase_toggle  = GTK_WIDGET (gtk_builder_get_object (builder, "capitalize_first_radio"));
    priv->process_first_style_uppercase_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "capitalize_first_style_radio"));
    priv->process_roman_numerals_check = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                             "capitalize_roman_check"));
    /* Toggled signals */
    g_signal_connect_swapped (priv->process_first_style_uppercase_toggle,
                              "toggled",
                              G_CALLBACK (Process_Fields_First_Letters_Check_Button_Toggled),
                              self);
    /* Set check buttons to init value */
    g_settings_bind (MainSettings, "process-uppercase-all",
                     priv->process_all_uppercase_toggle, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "process-lowercase-all",
                     priv->process_all_lowercase_toggle, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "process-uppercase-first-letters",
                     priv->process_first_uppercase_toggle, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "process-detect-roman-numerals",
                     priv->process_roman_numerals_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Group: insert/remove spaces */
    priv->process_remove_space_toggle = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                            "spaces_remove_radio"));
    priv->process_insert_space_toggle = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                            "spaces_insert_radio"));
    priv->process_insert_one_space_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "spaces_insert_one_radio"));
    /* Set check buttons to init value */
    g_settings_bind (MainSettings, "process-remove-spaces",
                     priv->process_remove_space_toggle, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "process-insert-capital-spaces",
                     priv->process_insert_space_toggle, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "process-remove-duplicate-spaces",
                     priv->process_insert_one_space_toggle, "active",
                     G_SETTINGS_BIND_DEFAULT);
    on_process_fields_changed (self, "process-fields", MainSettings);

    /*
     * Frame to display codes legend
     */
    priv->legend_frame = GTK_WIDGET (gtk_builder_get_object (builder,
                                                             "legend_grid"));

    /* Masks Editor. */
    priv->mask_editor_frame = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                  "editor_grid"));

    /* The list */
    priv->mask_editor_view = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                 "mask_view"));

    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->mask_editor_view)),
                                 GTK_SELECTION_MULTIPLE);
    g_signal_connect_after (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->mask_editor_view)),
                            "changed",
                            G_CALLBACK (Mask_Editor_List_Row_Selected), self);
    g_signal_connect (priv->mask_editor_view, "key-press-event",
                      G_CALLBACK (Mask_Editor_List_Key_Press), self);
    /* The entry */
    priv->mask_editor_entry = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                  "mask_entry"));
    g_signal_connect_swapped (priv->mask_editor_entry, "changed",
                              G_CALLBACK (Mask_Editor_Entry_Changed), self);
    /* Mask status icon. */
    /* Signal connection to check if mask is correct into the mask entry. */
    g_signal_connect (priv->mask_editor_entry, "changed",
                      G_CALLBACK (entry_check_scan_tag_mask), NULL);

    /* The buttons part */
    /* New mask button */
    button = GTK_WIDGET (gtk_builder_get_object (builder, "mask_new_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Mask_Editor_List_New), self);

    /* Move up mask button */
    button = GTK_WIDGET (gtk_builder_get_object (builder, "mask_up_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Mask_Editor_List_Move_Up), self);

    /* Move down mask button */
    button = GTK_WIDGET (gtk_builder_get_object (builder, "mask_down_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Mask_Editor_List_Move_Down), self);

    /* Copy mask button */
    button = GTK_WIDGET (gtk_builder_get_object (builder, "mask_copy_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Mask_Editor_List_Duplicate), self);

    /* Add mask button */
    button = GTK_WIDGET (gtk_builder_get_object (builder, "mask_add_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Mask_Editor_List_Add), self);

    /* Remove mask button */
    button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                 "mask_remove_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Mask_Editor_List_Remove), self);

    /* Save mask button */
    button = GTK_WIDGET (gtk_builder_get_object (builder, "mask_save_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Mask_Editor_List_Save_Button), self);

    /* To initialize the mask status icon and visibility */
    g_signal_emit_by_name (gtk_bin_get_child (GTK_BIN (priv->scan_tag_mask_combo)),
                           "changed");
    g_signal_emit_by_name (gtk_bin_get_child (GTK_BIN (priv->rename_file_mask_combo)),
                           "changed");
    g_signal_emit_by_name (priv->mask_editor_entry, "changed");
    g_signal_emit_by_name (priv->legend_toggle, "toggled"); /* To hide legend frame */
    g_signal_emit_by_name (priv->mask_editor_toggle, "toggled"); /* To hide mask editor frame */
    g_signal_emit_by_name (priv->process_convert_toggle, "toggled"); /* To enable / disable entries */
    g_signal_emit_by_name (priv->process_roman_numerals_check, "toggled"); /* To enable / disable entries */

    gtk_widget_show_all (ScanVBox);

    /* Activate the current menu in the option menu. */
    on_scan_mode_changed (self, "scan-mode", MainSettings);

    g_object_unref (builder);
}

/*
 * Select the scanner to run for the current ETFile
 */
void
Scan_Select_Mode_And_Run_Scanner (EtScanDialog *self, ET_File *ETFile)
{
    EtScanDialogPrivate *priv;
    EtScanMode mode;

    g_return_if_fail (ET_SCAN_DIALOG (self));
    g_return_if_fail (ETFile != NULL);

    priv = et_scan_dialog_get_instance_private (self);
    mode = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook));

    switch (mode)
    {
        case ET_SCAN_MODE_FILL_TAG:
            Scan_Tag_With_Mask (self, ETFile);
            break;
        case ET_SCAN_MODE_RENAME_FILE:
            Scan_Rename_File_With_Mask (self, ETFile);
            break;
        case ET_SCAN_MODE_PROCESS_FIELDS:
            Scan_Process_Fields (self, ETFile);
            break;
        default:
            g_assert_not_reached ();
    }
}

/*
 * For the configuration file...
 */
void
et_scan_dialog_apply_changes (EtScanDialog *self)
{
    EtScanDialogPrivate *priv;

    g_return_if_fail (ET_SCAN_DIALOG (self));

    priv = et_scan_dialog_get_instance_private (self);

    /* Save default masks. */
    Add_String_To_Combo_List (priv->scan_tag_masks_model,
                              gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->scan_tag_mask_combo)))));
    Save_Rename_File_Masks_List(priv->scan_tag_masks_model, MASK_EDITOR_TEXT);

    Add_String_To_Combo_List(priv->rename_masks_model,
                             gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->rename_file_mask_combo)))));
    Save_Rename_File_Masks_List(priv->rename_masks_model, MASK_EDITOR_TEXT);
}


/* Callback from Option button */
static void
Scan_Option_Button (void)
{
    et_application_window_show_preferences_dialog_scanner (ET_APPLICATION_WINDOW (MainWindow));
}


/*
 * entry_check_rename_file_mask:
 * @entry: the entry for which to check the mask
 * @user_data: user data set when the signal was connected
 *
 * Display an icon in the entry if the current text contains an invalid mask
 * for scanning tags.
 */
static void
entry_check_scan_tag_mask (GtkEntry *entry, gpointer user_data)
{
    gchar *tmp  = NULL;
    gchar *mask = NULL;
    gint loop = 0;

    g_return_if_fail (entry != NULL);

    mask = g_strdup (gtk_entry_get_text (entry));
    if (!mask || strlen(mask)<1)
        goto Bad_Mask;

    while (mask)
    {
        if ( (tmp=strrchr(mask,'%'))==NULL )
        {
            if (loop==0)
                /* There is no code the first time => not accepted */
                goto Bad_Mask;
            else
                /* There is no more code => accepted */
                goto Good_Mask;
        }
        if ( strlen(tmp)>1
        && (tmp[1]=='a' || tmp[1]=='b' || tmp[1]=='c' || tmp[1]=='d' || tmp[1]=='p' ||
            tmp[1]=='r' || tmp[1]=='e' || tmp[1]=='g' || tmp[1]=='i' || tmp[1]=='l' ||
            tmp[1]=='o' || tmp[1]=='n' || tmp[1]=='t' || tmp[1]=='u' || tmp[1]=='y' ) )
        {
            /* Code is correct */
            *(mask+strlen(mask)-strlen(tmp)) = '\0';
        }else
        {
            goto Bad_Mask;
        }

        /* Check the following code and separator */
        if ( (tmp=strrchr(mask,'%'))==NULL )
            /* There is no more code => accepted */
            goto Good_Mask;

        if ( strlen(tmp)>2
        && (tmp[1]=='a' || tmp[1]=='b' || tmp[1]=='c' || tmp[1]=='d' || tmp[1]=='p' ||
            tmp[1]=='r' || tmp[1]=='e' || tmp[1]=='g' || tmp[1]=='i' || tmp[1]=='l' ||
            tmp[1]=='o' || tmp[1]=='n' || tmp[1]=='t' || tmp[1]=='u' || tmp[1]=='y' ) )
        {
            /* There is a separator and code is correct */
            *(mask+strlen(mask)-strlen(tmp)) = '\0';
        }else
        {
            goto Bad_Mask;
        }
        loop++;
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
}

/*
 * entry_check_rename_file_mask:
 * @entry: the entry for which to check the mask
 * @user_data: user data set when the signal was connected
 *
 * Display an icon in the entry if the current text contains an invalid mask
 * for renaming files.
 */
void
entry_check_rename_file_mask (GtkEntry *entry, gpointer user_data)
{
    gchar *tmp = NULL;
    gchar *mask = NULL;

    g_return_if_fail (entry != NULL);

    mask = g_strdup (gtk_entry_get_text (entry));
    if (!mask || strlen(mask)<1)
        goto Bad_Mask;

    // Not a valid path....
    if ( strstr(mask,"//") != NULL
    ||   strstr(mask,"./") != NULL
    ||   strstr(mask,"data/") != NULL)
        goto Bad_Mask;

    while (mask)
    {
        if ( (tmp=strrchr(mask,'%'))==NULL )
        {
            /* There is no more code. */
            /* No code in mask is accepted. */
            goto Good_Mask;
        }
        if ( strlen(tmp)>1
        && (tmp[1]=='a' || tmp[1]=='b' || tmp[1]=='c' || tmp[1]=='d' || tmp[1]=='p' ||
            tmp[1]=='r' || tmp[1]=='e' || tmp[1]=='g' || tmp[1]=='i' || tmp[1]=='l' ||
            tmp[1]=='o' || tmp[1]=='n' || tmp[1]=='t' || tmp[1]=='u' || tmp[1]=='y' ) )
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
}

void
et_scan_dialog_scan_selected_files (EtScanDialog *self)
{
    gint progress_bar_index;
    gint selectcount;
    gchar progress_bar_text[30];
    double fraction;
    GList *selfilelist = NULL;
    GList *l;
    ET_File *etfile;
    EtApplicationWindow *window;
    GtkTreeSelection *selection;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Initialize status bar */
    window = ET_APPLICATION_WINDOW (MainWindow);
    selection = et_application_window_browser_get_selection (window);
    selectcount = gtk_tree_selection_count_selected_rows (selection);
    et_application_window_progress_set_fraction (window, 0.0);
    progress_bar_index = 0;
    g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, selectcount);
    et_application_window_progress_set_text (window, progress_bar_text);

    /* Set to unsensitive all command buttons (except Quit button) */
    et_application_window_disable_command_actions (window);

    progress_bar_index = 0;

    selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);

    for (l = selfilelist; l != NULL; l = g_list_next (l))
    {
        etfile = et_application_window_browser_get_et_file_from_path (window,
                                                                      l->data);

        /* Run the current scanner. */
        Scan_Select_Mode_And_Run_Scanner (self, etfile);

        fraction = (++progress_bar_index) / (double) selectcount;
        et_application_window_progress_set_fraction (window, fraction);
        g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, selectcount);
        et_application_window_progress_set_text (window, progress_bar_text);

        /* Needed to refresh status bar */
        while (gtk_events_pending())
            gtk_main_iteration();
    }

    g_list_free_full (selfilelist, (GDestroyNotify)gtk_tree_path_free);

    /* Refresh the whole list (faster than file by file) to show changes. */
    et_application_window_browser_refresh_list (window);

    /* Display the current file */
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);

    /* To update state of command buttons */
    et_application_window_update_actions (window);

    et_application_window_progress_set_text (window, "");
    et_application_window_progress_set_fraction (window, 0.0);
    et_application_window_status_bar_message (window,
                                              _("All tags have been scanned"),
                                              TRUE);
}

/*
 * et_scan_on_response:
 * @dialog: the scanner window
 * @response_id: the #GtkResponseType corresponding to the dialog event
 * @user_data: user data set when the signal was connected
 *
 * Handle the response signal of the scanner dialog.
 */
static void
et_scan_on_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
    switch (response_id)
    {
        case GTK_RESPONSE_APPLY:
            et_scan_dialog_scan_selected_files (ET_SCAN_DIALOG (dialog));
            break;
        case GTK_RESPONSE_CLOSE:
            gtk_widget_hide (GTK_WIDGET (dialog));
            break;
        case GTK_RESPONSE_DELETE_EVENT:
            break;
        default:
            g_assert_not_reached ();
            break;
    }
}

static void
et_scan_dialog_init (EtScanDialog *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, ET_TYPE_SCAN_DIALOG,
                                              EtScanDialogPrivate);

    create_scan_dialog (self);
}

static void
et_scan_dialog_class_init (EtScanDialogClass *klass)
{
    g_type_class_add_private (klass, sizeof (EtScanDialogPrivate));
}

/*
 * et_scan_dialog_new:
 *
 * Create a new EtScanDialog instance.
 *
 * Returns: a new #EtScanDialog
 */
EtScanDialog *
et_scan_dialog_new (GtkWindow *parent)
{
    g_return_val_if_fail (GTK_WINDOW (parent), NULL);

    return g_object_new (ET_TYPE_SCAN_DIALOG, "transient-for", parent, NULL);
}
