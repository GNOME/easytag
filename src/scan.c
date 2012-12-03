/* scan.c - 2000/06/16 */
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <gtk/gtk.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>
#include <glib/gi18n-lib.h>

#include "gtk2_compat.h"
#include "scan.h"
#include "easytag.h"
#include "prefs.h"
#include "setting.h"
#include "id3_tag.h"
#include "bar.h"
#include "browser.h"
#include "log.h"
#include "misc.h"
#include "et_core.h"
#include "crc32.h"
#include "charset.h"


#define step(a,b) (b-a)+1


/****************
 * Declarations *
 ****************/
GtkWidget *DummyEntry = NULL;    /* Used to simulate a gtkentry widget for mask code '%i' */
GtkWidget *ScanTagMaskCombo           = NULL;
GtkWidget *RenameFileMaskCombo        = NULL;
GtkWidget *ScannerOptionCombo         = NULL;
GtkWidget *RenameFilePrefixPathButton = NULL;

GtkWidget *ScanTagFrame;
GtkWidget *RenameFileFrame;
GtkWidget *ProcessFieldsFrame;
GtkWidget *FillTagPreviewLabel        = NULL;
GtkWidget *RenameFilePreviewLabel     = NULL;

GtkListStore *RenameFileListModel;
GtkListStore *ScanTagListModel;

GtkWidget *ProcessFileNameField;
GtkWidget *ProcessTitleField;
GtkWidget *ProcessArtistField;
GtkWidget *ProcessAlbumArtistField;
GtkWidget *ProcessAlbumField;
GtkWidget *ProcessGenreField;
GtkWidget *ProcessCommentField;
GtkWidget *ProcessComposerField;
GtkWidget *ProcessOrigArtistField;
GtkWidget *ProcessCopyrightField;
GtkWidget *ProcessURLField;
GtkWidget *ProcessEncodedByField;
GtkWidget *ProcessFieldsConvertIntoSpace      = NULL;
GtkWidget *ProcessFieldsConvertSpace          = NULL;
GtkWidget *ProcessFieldsConvert               = NULL;
GtkWidget *ProcessFieldsConvertLabelTo;
GtkWidget *ProcessFieldsConvertTo             = NULL;
GtkWidget *ProcessFieldsConvertFrom           = NULL;
GtkWidget *ProcessFieldsAllUppercase          = NULL;
GtkWidget *ProcessFieldsAllDowncase           = NULL;
GtkWidget *ProcessFieldsFirstLetterUppercase  = NULL;
GtkWidget *ProcessFieldsFirstLettersUppercase = NULL;
GtkWidget *ProcessFieldsDetectRomanNumerals   = NULL;
GtkWidget *ProcessFieldsRemoveSpace           = NULL;
GtkWidget *ProcessFieldsInsertSpace           = NULL;
GtkWidget *ProcessFieldsOnlyOneSpace          = NULL;

GtkWidget *LegendFrame    = NULL;
GtkWidget *LegendButton   = NULL;

GtkWidget *MaskEditorButton = NULL;
GtkWidget *MaskEditorFrame  = NULL;
GtkWidget *MaskEditorVBox;
GtkWidget *MaskEditorHBox;
GtkWidget *MaskEditorScrollWindow;
GtkWidget *MaskEditorList;
GtkWidget *MaskEditorEntry;
GtkWidget *MaskEditorNewButton;
GtkWidget *MaskEditorCopyButton;
GtkWidget *MaskEditorAddButton;
GtkWidget *MaskEditorRemoveButton;
GtkWidget *MaskEditorUpButton;
GtkWidget *MaskEditorDownButton;
GtkWidget *MaskEditorSaveButton;

/* Some predefined masks -- IMPORTANT: Null-terminate me! */
gchar *Scan_Masks [] =
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

gchar *Rename_File_Masks [] =
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


gchar *Scanner_Option_Menu_Items [] =
{
    N_("Fill Tag"),
    N_("Rename File and Directory"),
    N_("Process Fields")
};

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
void     ScannerWindow_Quit      (void);
gboolean ScannerWindow_Key_Press (GtkWidget *window, GdkEvent *event);
void     Scan_Toggle_Legend_Button      (void);
void     Scan_Toggle_Mask_Editor_Button (void);
gchar   *Scan_Replace_String (gchar *string, gchar *last, gchar *new);
void     Scan_Option_Button (void);
gboolean Scan_Check_Scan_Tag_Mask    (GtkWidget *widget_to_show_hide, GtkEntry *widget_source);
gboolean Scan_Check_Rename_File_Mask (GtkWidget *widget_to_show_hide, GtkEntry *widget_source);
gboolean Scan_Check_Editor_Mask      (GtkWidget *widget_to_show_hide, GtkEntry *widget_source);

gchar   *Scan_Generate_New_Filename_From_Mask (ET_File *ETFile, gchar *mask, gboolean no_dir_check_or_conversion);
GList   *Scan_Generate_New_Tag_From_Mask      (ET_File *ETFile, gchar *mask);
void     Scan_Rename_File_Generate_Preview    (void);
void     Scan_Rename_File_Prefix_Path         (void);
void     Scan_Fill_Tag_Generate_Preview       (void);
void     Scan_Free_File_Rename_List           (GList *list);
void     Scan_Free_File_Fill_Tag_List         (GList *list);
void     Scan_Rename_Directory_Generate_Preview (void);

gchar  **Scan_Return_File_Tag_Field_From_Mask_Code (File_Tag *FileTag, gchar code);
void     Scan_Process_Fields_Functions (gchar **string);

gint     Scan_Word_Is_Roman_Numeral (gchar *text);

void Process_Fields_Check_Button_Toggled               (GtkWidget *object, GList *list);
void Process_Fields_Convert_Check_Button_Toggled       (GtkWidget *object);
void Process_Fields_First_Letters_Check_Button_Toggled (GtkWidget *object);
void Select_Fields_Invert_Selection    (void);
void Select_Fields_Select_Unselect_All (void);
void Select_Fields_Set_Sensitive       (void);

void Mask_Editor_List_Row_Selected    (GtkTreeSelection* selection, gpointer data);
void Mask_Editor_List_Set_Row_Visible (GtkTreeModel *treeModel, GtkTreeIter *rowIter);
void Mask_Editor_List_New             (void);
void Mask_Editor_List_Duplicate       (void);
void Mask_Editor_List_Add             (void);
void Mask_Editor_List_Remove          (void);
void Mask_Editor_List_Move_Up         (void);
void Mask_Editor_List_Move_Down       (void);
void Mask_Editor_List_Save_Button     (void);
void Mask_Editor_Entry_Changed        (void);
gboolean Mask_Editor_List_Key_Press   (GtkWidget *widget, GdkEvent *event);

void Mask_Editor_Clean_Up_Masks_List (void);

void Scanner_Option_Menu_Activate_Item (GtkWidget *widget, gpointer data);

int roman2int (const char *str);
const char * int2roman (int num);
char * int2roman_r (int num, char * str, size_t len);



/*************
 *           *
 * Functions *
 *           *
 *************/

void Init_ScannerWindow (void)
{
    ScannerWindow     = NULL;
    ScannerOptionCombo= NULL;
    SWScanButton      = NULL;
}


/*
 * Uses the filename and path to fill tag information
 * Note: mask and source are read from the right to the left
 */
void Scan_Tag_With_Mask (ET_File *ETFile)
{
    GList *fill_tag_list = NULL;
    gchar **dest = NULL;
    gchar *mask; // The 'mask' in the entry
    gchar *filename_utf8;
    File_Tag *FileTag;

    if (!ScannerWindow || !ScanTagMaskCombo || !ETFile) return;
    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ScanTagMaskCombo)))));
    if (!mask) return;

    // Create a new File_Tag item
    FileTag = ET_File_Tag_Item_New();
    ET_Copy_File_Tag_Item(ETFile,FileTag);

    // Process this mask with file
    fill_tag_list = Scan_Generate_New_Tag_From_Mask(ETFile,mask);
    while (fill_tag_list)
    {
        Scan_Mask_Item *mask_item = fill_tag_list->data;

        // Get the target entry for this code
        dest = Scan_Return_File_Tag_Field_From_Mask_Code(FileTag,mask_item->code);

        // We display the text affected to the code
        if ( dest && ( OVERWRITE_TAG_FIELD || *dest==NULL || strlen(*dest)==0 ) )
            ET_Set_Field_File_Tag_Item(dest,mask_item->string);

        if (!fill_tag_list->next) break;
            fill_tag_list = fill_tag_list->next;
    }
    Scan_Free_File_Fill_Tag_List(fill_tag_list);

    // Set the default text to comment
    if (SET_DEFAULT_COMMENT && (OVERWRITE_TAG_FIELD || FileTag->comment==NULL || strlen(FileTag->comment)==0 ) )
        ET_Set_Field_File_Tag_Item((void *)&FileTag->comment,DEFAULT_COMMENT);

    // Set CRC-32 value as default comment (for files with ID3 tag only ;-)
    if (SET_CRC32_COMMENT && (OVERWRITE_TAG_FIELD || FileTag->comment==NULL || strlen(FileTag->comment)==0 ) )
    {
        gulong crc32_value = 0;
        gchar *buffer;
        ET_File_Description *ETFileDescription;

        ETFileDescription = ETFile->ETFileDescription;
        switch (ETFileDescription->TagType)
        {
            case ID3_TAG:
                crc32_file_with_ID3_tag( ((File_Name *)((GList *)ETFile->FileNameNew)->data)->value, &crc32_value);

                if (crc32_value > 0)
                {
                    buffer = g_strdup_printf("%.8lx",crc32_value);
                    ET_Set_Field_File_Tag_Item((void *)&FileTag->comment,buffer);
                    g_free(buffer);
                }
                break;
            default:
                break;
        }
    }


    // Save changes of the 'File_Tag' item
    ET_Manage_Changes_Of_File_Data(ETFile,NULL,FileTag);

    g_free(mask);
    Statusbar_Message(_("Tag successfully scanned…"),TRUE);
    filename_utf8 = g_path_get_basename( ((File_Name *)ETFile->FileNameNew->data)->value_utf8 );
    Log_Print(LOG_OK,_("Tag successfully scanned…(%s)"),filename_utf8);
    g_free(filename_utf8);
}

GList *Scan_Generate_New_Tag_From_Mask (ET_File *ETFile, gchar *mask)
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

    if (!ETFile || !mask) return NULL;

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
        Log_Print(LOG_ERROR,_("Tag scanner: strange… the extension '%s' was not found in filename '%s'"),tmp,tmp1);
        g_free(tmp1);
    }

    // Replace characters into mask and filename before parsing
    if (FTS_CONVERT_UNDERSCORE_AND_P20_INTO_SPACE)
    {
        Scan_Convert_Underscore_Into_Space(mask);
        Scan_Convert_Underscore_Into_Space(filename_utf8);
        Scan_Convert_P20_Into_Space(mask);
        Scan_Convert_P20_Into_Space(filename_utf8);
    }
    if (FTS_CONVERT_SPACE_INTO_UNDERSCORE)
    {
        Scan_Convert_Space_Into_Undescore(mask);
        Scan_Convert_Space_Into_Undescore(filename_utf8);
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
            mask_item = g_malloc0(sizeof(Scan_Mask_Item));

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
                    Log_Print(LOG_ERROR,_("Scan Error: can't find separator '%s' within '%s'"),buf,file_seq_utf8);
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
                    Log_Print(LOG_ERROR,_("Scan Error: can't find separator '%s' within '%s'"),separator,file_seq_utf8);
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

void Scan_Fill_Tag_Generate_Preview (void)
{
    gchar *mask = NULL;
    gchar *preview_text = NULL;
    GList *fill_tag_list = NULL;

    if (!ETCore->ETFileDisplayedList
    ||  !ScannerWindow || !RenameFileMaskCombo || !FillTagPreviewLabel
    ||  gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) != SCANNER_FILL_TAG)
        return;

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ScanTagMaskCombo)))));
    if (!mask)
        return;

    preview_text = g_strdup("");
    fill_tag_list = Scan_Generate_New_Tag_From_Mask(ETCore->ETFileDisplayed,mask);
    while (fill_tag_list)
    {
        Scan_Mask_Item *mask_item = fill_tag_list->data;
        gchar *tmp_code   = g_strdup_printf("%c",mask_item->code);
        gchar *tmp_string = g_markup_printf_escaped("%s",mask_item->string); // To avoid problem with strings containing characters like '&'
        gchar *tmp_preview_text = preview_text;

        preview_text = g_strconcat(tmp_preview_text,"<b>","%",tmp_code," = ",
                                   "</b>","<i>",tmp_string,"</i>",NULL);
        g_free(tmp_code);
        g_free(tmp_string);
        g_free(tmp_preview_text);

        if (!fill_tag_list->next) break;
        fill_tag_list = fill_tag_list->next;

        tmp_preview_text = preview_text;
        preview_text = g_strconcat(tmp_preview_text,"  ||  ",NULL);
        g_free(tmp_preview_text);
    }
    Scan_Free_File_Fill_Tag_List(fill_tag_list);

    if (GTK_IS_LABEL(FillTagPreviewLabel))
    {
        if (preview_text)
        {
            //gtk_label_set_text(GTK_LABEL(FillTagPreviewLabel),preview_text);
            gtk_label_set_markup(GTK_LABEL(FillTagPreviewLabel),preview_text);
        } else
        {
            gtk_label_set_text(GTK_LABEL(FillTagPreviewLabel),"");
        }

        // Force the window to be redrawed
        gtk_widget_queue_resize(ScannerWindow);
    }

    g_free(mask);
    g_free(preview_text);
}

void Scan_Free_File_Fill_Tag_List (GList *list)
{
    // Free the list
    list = g_list_first(list);
    while (list)
    {
        if (list->data)
        {
            g_free(((Scan_Mask_Item *)list->data)->string);
            g_free( (Scan_Mask_Item *)list->data );
        }
        if (!list->next) break;
        list = list->next;
    }
    g_list_free(list);
    list = NULL;
}



/**************************
 * Scanner To Rename File *
 **************************/
/*
 * Uses tag information (displayed into tag entries) to rename file
 * Note: mask and source are read from the right to the left.
 * Note1: a mask code may be used severals times...
 */
void Scan_Rename_File_With_Mask (ET_File *ETFile)
{
    gchar *filename_generated_utf8 = NULL;
    gchar *filename_generated = NULL;
    gchar *filename_new_utf8 = NULL;
    gchar *mask = NULL;
    File_Name *FileName;

    if (!ScannerWindow || !RenameFileMaskCombo || !ETFile) return;

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo)))));
    if (!mask) return;

    // Note : if the first character is '/', we have a path with the filename,
    // else we have only the filename. The both are in UTF-8.
    filename_generated_utf8 = Scan_Generate_New_Filename_From_Mask(ETFile,mask,FALSE);
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
        msgdialog = gtk_message_dialog_new(GTK_WINDOW(ScannerWindow),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_ERROR,
                             GTK_BUTTONS_CLOSE,
                             _("Could not convert filename '%s' into system filename encoding"),
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

    Statusbar_Message(_("New file name successfully scanned…"),TRUE);

    filename_new_utf8 = g_path_get_basename(((File_Name *)ETFile->FileNameNew->data)->value_utf8);
    Log_Print(LOG_OK,_("New file name successfully scanned…(%s)"),filename_new_utf8);
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
gchar *Scan_Generate_New_Filename_From_Mask (ET_File *ETFile, gchar *mask, gboolean no_dir_check_or_conversion)
{
    gchar *tmp;
    gchar **source = NULL;
    gchar *path_utf8_cur = NULL;
    gchar *filename_new_utf8 = NULL;
    gchar *filename_tmp = NULL;
    GList *rename_file_list = NULL;
    File_Mask_Item *mask_item;
    File_Mask_Item *mask_item_prev;
    File_Mask_Item *mask_item_next;
    gint counter = 0;


    if (!ETFile || !mask) return NULL;

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
            mask_item = g_malloc0(sizeof(File_Mask_Item));
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
        mask_item = g_malloc0(sizeof(File_Mask_Item));
        if (source && *source && strlen(*source)>0)
        {
            mask_item->type = FIELD;
            mask_item->string = g_strdup(*source);

            // Replace invalid characters for this field
            // Note : shouldn't be done always as for the content of a playlist, we don't need to replace...
            if (!no_dir_check_or_conversion)
            {
                ET_File_Name_Convert_Character(mask_item->string);
            }

            // Replace characters (rules) (!! don't convert in directory path_utf8_cur)
            if (RFS_CONVERT_UNDERSCORE_AND_P20_INTO_SPACE)
            {
                Scan_Convert_Underscore_Into_Space(mask_item->string);
                Scan_Convert_P20_Into_Space(mask_item->string);
            }
            if (RFS_CONVERT_SPACE_INTO_UNDERSCORE)
						 {
                Scan_Convert_Space_Into_Undescore(mask_item->string);
						 }
						 if (RFS_REMOVE_SPACES)
						 {
						    Scan_Remove_Spaces(mask_item->string);
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
        mask_item = g_malloc0(sizeof(File_Mask_Item));
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
    rename_file_list = g_list_last(rename_file_list);
    filename_new_utf8 = g_strdup("");

    while (rename_file_list)
    {
        File_Mask_Item *mask_item = rename_file_list->data;

        if ( mask_item->type==TRAILING_SEPARATOR ) // Trailing characters of mask
        {
            // Doesn't write it if previous field is empty
            if (rename_file_list->prev && ((File_Mask_Item *)rename_file_list->prev->data)->type!=EMPTY_FIELD)
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
            if (rename_file_list->prev)
            {
                // The empty field isn't the first.
                // If previous string is a separator, we don't use it, except if the next
                // string is a FIELD (not empty)
                mask_item_prev = rename_file_list->prev->data;
                if ( mask_item_prev->type==SEPARATOR )
                {
                    if ( !(rename_file_list->next && (mask_item_next=rename_file_list->next->data)
                    &&  mask_item_next->type==FIELD) )
                    {
                        rename_file_list = rename_file_list->prev;
                    }
                }
            }else
            if (rename_file_list->next && (mask_item_next=rename_file_list->next->data)
            &&  mask_item_next->type==SEPARATOR)
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

        if (!rename_file_list->prev) break;
        rename_file_list = rename_file_list->prev;
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

void Scan_Rename_File_Generate_Preview (void)
{
    gchar *preview_text = NULL;
    gchar *mask = NULL;

    if (!ETCore->ETFileDisplayed
    ||  !ScannerWindow || !RenameFileMaskCombo || !RenameFilePreviewLabel)
        return;

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) != SCANNER_RENAME_FILE)
        return;

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo)))));
    if (!mask)
        return;

    preview_text = Scan_Generate_New_Filename_From_Mask(ETCore->ETFileDisplayed,mask,FALSE);

    if (GTK_IS_LABEL(RenameFilePreviewLabel))
    {
        if (preview_text)
        {
            //gtk_label_set_text(GTK_LABEL(RenameFilePreviewLabel),preview_text);
            gchar *tmp_string = g_markup_printf_escaped("%s",preview_text); // To avoid problem with strings containing characters like '&'
            gchar *str = g_strdup_printf("<i>%s</i>",tmp_string);
            gtk_label_set_markup(GTK_LABEL(RenameFilePreviewLabel),str);
            g_free(tmp_string);
            g_free(str);
        } else
        {
            gtk_label_set_text(GTK_LABEL(RenameFilePreviewLabel),"");
        }

        // Force the window to be redrawed
        gtk_widget_queue_resize(ScannerWindow);
    }

    g_free(mask);
    g_free(preview_text);
}


void Scan_Free_File_Rename_List (GList *list)
{
    // Free the list
    list = g_list_last(list);
    while (list)
    {
        if (list->data)
        {
            g_free(((File_Mask_Item *)list->data)->string);
            g_free( (File_Mask_Item *)list->data );
        }
        if (!list->prev) break;
        list = list->prev;
    }
    g_list_free(list);
    list = NULL;
}

/*
 * Adds the current path of the file to the mask on the "Rename File Scanner" entry
 */
void Scan_Rename_File_Prefix_Path (void)
{
    gint pos;
    gchar *path_tmp;
    const gchar *combo_text = NULL;
    gchar *combo_tmp;
    ET_File *ETFile          = ETCore->ETFileDisplayed;
    gchar *filename_utf8_cur = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;
    gchar *path_utf8_cur;


    // The path to prefix
    path_utf8_cur = g_path_get_dirname(filename_utf8_cur);

    // The current text in the combobox
    combo_text = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo))));
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
        gtk_editable_insert_text(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo))),path_tmp, -1, &pos);
        g_free(path_tmp);
    }

    g_free(path_utf8_cur);
}


/*******************************
 * Scanner To Rename Directory *
 *******************************/
void Scan_Rename_Directory_Generate_Preview (void)
{
    gchar *preview_text = NULL;
    gchar *mask = NULL;

    if (!ETCore->ETFileDisplayed
    ||  !RenameDirectoryWindow || !RenameDirectoryMaskCombo || !RenameDirectoryPreviewLabel)
        return;

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryMaskCombo)))));
    if (!mask)
        return;

    preview_text = Scan_Generate_New_Filename_From_Mask(ETCore->ETFileDisplayed,mask,FALSE);

    if (GTK_IS_LABEL(RenameDirectoryPreviewLabel))
    {
        if (preview_text)
        {
            //gtk_label_set_text(GTK_LABEL(RenameFilePreviewLabel),preview_text);
            gchar *tmp_string = g_markup_printf_escaped("%s",preview_text); // To avoid problem with strings containing characters like '&'
            gchar *str = g_strdup_printf("<i>%s</i>",tmp_string);
            gtk_label_set_markup(GTK_LABEL(RenameDirectoryPreviewLabel),str);
            g_free(tmp_string);
            g_free(str);
        } else
        {
            gtk_label_set_text(GTK_LABEL(RenameDirectoryPreviewLabel),"");
        }

        // Force the window to be redrawed else the preview label may be not placed correctly
        gtk_widget_queue_resize(RenameDirectoryWindow);
    }

    g_free(mask);
    g_free(preview_text);
}

gchar *Scan_Generate_New_Directory_Name_From_Mask (ET_File *ETFile, gchar *mask, gboolean no_dir_check_or_conversion)
{
    return Scan_Generate_New_Filename_From_Mask(ETFile,mask,no_dir_check_or_conversion);
}



/*****************************
 * Scanner To Process Fields *
 *****************************/
/* See also functions : Convert_P20_And_Undescore_Into_Spaces, ... in easytag.c */
void Scan_Process_Fields (ET_File *ETFile)
{
    File_Name *FileName = NULL;
    File_Tag  *FileTag  = NULL;
    File_Name *st_filename;
    File_Tag  *st_filetag;
    gchar     *filename_utf8;
    gchar     *string;


    if (!ScannerWindow || !ETFile) return;

    st_filename = (File_Name *)ETFile->FileNameNew->data;
    st_filetag  = (File_Tag  *)ETFile->FileTag->data;

    /* Process the filename */
    if (st_filename != NULL)
    {
        if (st_filename->value_utf8 && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFileNameField))) // File name field
        {
            gchar *string_utf8;
            gchar *pos;

            filename_utf8 = st_filename->value_utf8;

            if (!FileName)
                FileName = ET_File_Name_Item_New();

            string = g_path_get_basename(filename_utf8);
            // Remove the extension to set it to lower case (to avoid problem with undo)
            if ((pos=strrchr(string,'.'))!=NULL) *pos = 0;

            Scan_Process_Fields_Functions(&string);

            string_utf8 = ET_File_Name_Generate(ETFile,string);
            ET_Set_Filename_File_Name_Item(FileName,string_utf8,NULL);
            g_free(string_utf8);
            g_free(string);
        }
    }

    /* Process data of the tag */
    if (st_filetag != NULL)
    {
        // Title field
        if (st_filetag->title && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessTitleField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->title);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->title,string);

            g_free(string);
        }

        // Artist field
        if (st_filetag->artist && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessArtistField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->artist);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->artist,string);

            g_free(string);
        }

		// Album Artist field
        if (st_filetag->album_artist && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessAlbumArtistField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->album_artist);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->album_artist,string);

            g_free(string);
        }
        // Album field
        if (st_filetag->album && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessAlbumField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->album);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->album,string);

            g_free(string);
        }

        // Genre field
        if (st_filetag->genre && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessGenreField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->genre);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->genre,string);

            g_free(string);
        }

        // Comment field
        if (st_filetag->comment && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessCommentField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->comment);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->comment,string);

            g_free(string);
        }

        // Composer field
        if (st_filetag->composer && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessComposerField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->composer);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->composer,string);

            g_free(string);
        }

        // Original artist field
        if (st_filetag->orig_artist && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessOrigArtistField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->orig_artist);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->orig_artist,string);

            g_free(string);
        }

        // Copyright field
        if (st_filetag->copyright && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessCopyrightField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->copyright);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->copyright,string);

            g_free(string);
        }

        // URL field
        if (st_filetag->url && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessURLField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->url);

            Scan_Process_Fields_Functions(&string);

            ET_Set_Field_File_Tag_Item(&FileTag->url,string);

            g_free(string);
        }

        // 'Encoded by' field
        if (st_filetag->encoded_by && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessEncodedByField)))
        {
            if (!FileTag)
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(ETFile,FileTag);
            }

            string = g_strdup(st_filetag->encoded_by);

            Scan_Process_Fields_Functions(&string);

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


void Scan_Process_Fields_Functions (gchar **string)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvertIntoSpace)))
    {
        Scan_Convert_Underscore_Into_Space(*string);
        Scan_Convert_P20_Into_Space(*string);
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvertSpace)))
        Scan_Convert_Space_Into_Undescore(*string);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsInsertSpace)))
        Scan_Process_Fields_Insert_Space(string);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsOnlyOneSpace)))
        Scan_Process_Fields_Keep_One_Space(*string);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvert)))
        Scan_Convert_Character(string);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsAllUppercase)))
        Scan_Process_Fields_All_Uppercase(*string);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsAllDowncase)))
        Scan_Process_Fields_All_Downcase(*string);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsFirstLetterUppercase)))
         Scan_Process_Fields_Letter_Uppercase(*string);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsFirstLettersUppercase)))
        Scan_Process_Fields_First_Letters_Uppercase(*string);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsRemoveSpace)))
        Scan_Process_Fields_Remove_Space(*string);

}

void Scan_Process_Fields_All_Uppercase (gchar *string)
{
    gchar *temp;
    gchar temp2[6]; // Must have at least 6 bytes of space
    gunichar c;

    for (temp = string; *temp; temp = g_utf8_next_char(temp))
    {
        c = g_utf8_get_char(temp);
        if (g_unichar_islower(c))
            strncpy(temp, temp2, g_unichar_to_utf8(g_unichar_toupper(c), temp2));
    }
}

void Scan_Process_Fields_All_Downcase (gchar *string)
{
    gchar *temp;
    gchar temp2[6];
    gunichar c;

    for (temp = string; *temp; temp = g_utf8_next_char(temp))
    {
        c = g_utf8_get_char(temp);
        if (g_unichar_isupper(c))
            strncpy(temp, temp2, g_unichar_to_utf8(g_unichar_tolower(c), temp2));
    }
}

void Scan_Process_Fields_Letter_Uppercase (gchar *string)
{
    gchar *temp;
    gchar temp2[6];
    gboolean set_to_upper_case = TRUE;
    gunichar c;
    gchar utf8_character[6];
    gchar *word, *word1, *word2;

    for (temp = string; *temp; temp = g_utf8_next_char(temp))
    {
        c = g_utf8_get_char(temp);
        if (set_to_upper_case && g_unichar_islower(c))
            strncpy(temp, temp2, g_unichar_to_utf8(g_unichar_toupper(c), temp2));
        else if (!set_to_upper_case && g_unichar_isupper(c))
            strncpy(temp, temp2, g_unichar_to_utf8(g_unichar_tolower(c), temp2));
        set_to_upper_case = FALSE; // After the first time, all will be down case
    }

    temp = string;

    // Uppercase again the word 'I' in english
    while ( temp )
    {
        word = temp; // Needed if there is only one word
        word1 = g_utf8_strchr(temp,-1,' ');
        word2 = g_utf8_strchr(temp,-1,'_');

        // Take the first string found (near beginning of string)
        if (word1 && word2)
            word = MIN(word1,word2);
        else if (word1)
            word = word1;
        else if (word2)
            word = word2;
        else
            // Last word of the string
            break;

        // Go to first character of the word (char. after ' ' or '_')
        word = word+1;

        // Set uppercase word 'I'
        if (g_ascii_strncasecmp("I ", word, strlen("I ")) == 0)
        {
            c = g_utf8_get_char(word);
            strncpy(word, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));
        }

        temp = word;
    }
}

/*
 * Function to set the first letter of each word to uppercase, according the "Chicago Manual of Style" (http://www.docstyles.com/cmscrib.htm#Note2)
 * No needed to reallocate
 */
void Scan_Process_Fields_First_Letters_Uppercase (gchar *string)
{
/**** DANIEL TEST *****
    gchar *iter;
    gchar utf8_character[6];
    gboolean set_to_upper_case = TRUE;
    gunichar c;

    for (iter = text; *iter; iter = g_utf8_next_char(iter))
    {
        c = g_utf8_get_char(iter);
        if (set_to_upper_case && g_unichar_islower(c))
            strncpy(iter, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));
        else if (!set_to_upper_case && g_unichar_isupper(c))
            strncpy(iter, utf8_character, g_unichar_to_utf8(g_unichar_tolower(c), utf8_character));

        set_to_upper_case = (g_unichar_isalpha(c)
                            || c == (gunichar)'.'
                            || c == (gunichar)'\''
                            || c == (gunichar)'`') ? FALSE : TRUE;
    }
****/
/**** Barış Çiçek version ****/
    gchar *word, *word1, *word2, *temp;
    gint i, len;
    gchar utf8_character[6];
    gunichar c;
    gboolean set_to_upper_case, set_to_upper_case_tmp;
    // There have to be space at the end of words to seperate them from prefix
    // Chicago Manual of Style "Heading caps" Capitalization Rules (CMS 1993, 282) (http://www.docstyles.com/cmscrib.htm#Note2)
    gchar *exempt[] =
    {
        "a ",       "a_",
        "against ", "against_",
        "an ",      "an_",
        "and ",     "and_",
        "at ",      "at_",
        "between ", "between_",
        "but ",     "but_",
        //"feat. ",   "feat._", // Removed by Slash Bunny
        "for ",     "for_",
        "in ",      "in_",
        "nor ",     "nor_",
        "of ",      "of_",
        //"off ",     "off_",   // Removed by Slash Bunny
        "on ",      "on_",
        "or ",      "or_",
        //"over ",    "over_",  // Removed by Slash Bunny
        "so ",      "so_",
        "the ",     "the_",
        "to ",      "to_",
        "with ",    "with_",
        "yet ",     "yet_",
        NULL
    };

    if (!PFS_DONT_UPPER_SOME_WORDS)
    {
        exempt[0] = NULL;
    }
    Scan_Process_Fields_All_Downcase(string);

    if (!g_utf8_validate(string,-1,NULL))
    {
        Log_Print(LOG_ERROR,"Scan_Process_Fields_First_Letters_Uppercase: Not a valid utf8! quiting");
        return;
    }
    // Removes trailing whitespace
    string = g_strchomp(string);

    temp = string;

    // If the word is a roman numeral, capitalize all of it
    if ((len = Scan_Word_Is_Roman_Numeral(temp)))
    {
        strncpy(string, g_utf8_strup(temp, len), len);
    } else
    {
        // Set first character to uppercase
        c = g_utf8_get_char(temp);
        strncpy(string, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));
    }

    // Uppercase first character of each word, except for 'exempt[]' words lists
    while ( temp )
    {
        word = temp; // Needed if there is only one word
        word1 = g_utf8_strchr(temp,-1,' ');
        word2 = g_utf8_strchr(temp,-1,'_');

        // Take the first string found (near beginning of string)
        if (word1 && word2)
            word = MIN(word1,word2);
        else if (word1)
            word = word1;
        else if (word2)
            word = word2;
        else
        {
            // Last word of the string: the first letter is always uppercase,
            // even if it's in the exempt list. This is a Chicago Manual of Style rule.
            // Last Word In String - Should Capitalize Regardless of Word (Chicago Manual of Style)
            c = g_utf8_get_char(word);
            strncpy(word, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));
            break;
        }

        // Go to first character of the word (char. after ' ' or '_')
        word = word+1;

        // If the word is a roman numeral, capitalize all of it
        if ((len = Scan_Word_Is_Roman_Numeral(word)))
        {
            strncpy(word, g_utf8_strup(word, len), len);
        } else
        {
            // Set uppercase the first character of this word
            c = g_utf8_get_char(word);
            strncpy(word, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));

            // Set lowercase the first character of this word if found in the exempt words list
            for (i=0; exempt[i]!=NULL; i++)
            {
                if (g_ascii_strncasecmp(exempt[i], word, strlen(exempt[i])) == 0)
                {
                    c = g_utf8_get_char(word);
                    strncpy(word, utf8_character, g_unichar_to_utf8(g_unichar_tolower(c), utf8_character));
                    break;
                }
            }
        }

        temp = word;
    }

    // Uppercase letter placed after some characters like '(', '[', '{'
    set_to_upper_case = FALSE;
    for (temp = string; *temp; temp = g_utf8_next_char(temp))
    {
        c = g_utf8_get_char(temp);
        set_to_upper_case_tmp = (  c == (gunichar)'('
                                || c == (gunichar)'['
                                || c == (gunichar)'{'
                                || c == (gunichar)'"'
                                || c == (gunichar)':'
                                || c == (gunichar)'.'
                                || c == (gunichar)'`'
                                || c == (gunichar)'-'
                                ) ? TRUE : FALSE;

        if (set_to_upper_case && g_unichar_islower(c))
            strncpy(temp, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));

        set_to_upper_case = set_to_upper_case_tmp;
    }

}

void Scan_Process_Fields_Remove_Space (gchar *string)
{
    gchar *tmp, *tmp1;

    tmp = tmp1 = string;

    while (*tmp)
    {
        while (*tmp == ' ')
            tmp++;
        if (*tmp)
            *(tmp1++) = *(tmp++);
    }
    *tmp1 = '\0';
}

/*
 * The function inserts a space before an uppercase letter
 * It is needed to realloc the memory!
 */
void Scan_Process_Fields_Insert_Space (gchar **string)
{
    gchar *iter;
    gunichar c;
    gint j;
    guint string_length;
    gchar *string1;

    // FIX ME : we suppose that it will not grow more than 2 times its size...
    string_length = 2 * strlen(*string);
    //string1 = g_realloc(*string, string_length+1);
    string1       = g_malloc(string_length+1);
    strncpy(string1,*string,string_length);
    string1[string_length]='\0';
    g_free(*string);
    *string = string1;

    for (iter = g_utf8_next_char(*string); *iter; iter = g_utf8_next_char(iter)) // At start : g_utf8_next_char to not consider first "uppercase" letter
    {
        c = g_utf8_get_char(iter);

        if (g_unichar_isupper(c))
        {
            for (j = strlen(iter); j > 0; j--)
                *(iter + j) = *(iter + j - 1);
            *iter = ' ';
            iter++;
        }
    }
}

/*
 * The function removes the duplicated spaces
 * No need to reallocate
 */
void Scan_Process_Fields_Keep_One_Space (gchar *string)
{
    gchar *tmp, *tmp1;

    tmp = tmp1 = string;

    // Remove multiple consecutive underscores and spaces.
    while (*tmp1)
    {
        while (*tmp1 && *tmp1 != ' ' && *tmp1 != '_')
            *(tmp++) = *(tmp1++);
        if (!*tmp1)
            break;
        *(tmp++) = *(tmp1++);
        while (*tmp1 == ' ' || *tmp1 == '_')
            tmp1++;
    }
    *tmp = '\0';
}

/*
 * Function to replace underscore '_' by a space
 * No need to reallocate
 */
void Scan_Convert_Underscore_Into_Space (gchar *string)
{
    gchar *tmp;

    while ((tmp=strchr(string,'_'))!=NULL)
        *tmp = ' ';
}

/*
 * Function to replace %20 by a space
 * No need to reallocate
 */
void Scan_Convert_P20_Into_Space (gchar *string)
{
    gchar *tmp, *tmp1;

    while ((tmp=strstr(string,"%20"))!=NULL)
    {
        tmp1 = tmp + 3;
        *(tmp++) = ' ';
        while (*tmp1)
            *(tmp++) = *(tmp1++);
        *tmp = '\0';
    }
}

/*
 * Function to remove spaces
 * No need to reallocate
 */
void Scan_Remove_Spaces (gchar *string)
{
  int nextnotspace = 0, pos = 0;

  while(string[pos] != '\0')
  {
    if(string[pos] == ' ')
    {
      nextnotspace = pos;
      while(string[++nextnotspace] == ' ');
      string[pos] = string[nextnotspace];
      string[nextnotspace] = ' ';
      continue;
    }
    pos++;
  }
}

/*
 * Function to replace space by '_'
 * No need to reallocate
 */
void Scan_Convert_Space_Into_Undescore (gchar *string)
{
    gchar *tmp;

    while ((tmp=strchr(string,' '))!=NULL)
        *tmp = '_';
}

/*
 * Replace something with something else ;)
 * Currently this only works with one character for each
 */
void Scan_Convert_Character (gchar **string)
{
    gchar *from = gtk_editable_get_chars(GTK_EDITABLE(ProcessFieldsConvertFrom),0,-1 );
    gchar *to   = gtk_editable_get_chars(GTK_EDITABLE(ProcessFieldsConvertTo),0,-1 );
    const gchar *s;
    const gchar *current;
    GSList *tokens_list, *list;
    gchar  *token;
    gint    n_tokens;
    gchar **result;


    tokens_list = NULL;
    n_tokens = 0;
    s = current = *string;

    // Find tokens
    if (strlen(from) > 0)
    {
        while (s
        &&    *s != '\0'
        &&    (s = g_strstr_len(current,strlen(current),from)) )
        {
            token = g_strndup(current, s - current);
            tokens_list = g_slist_prepend(tokens_list, token);
            n_tokens++;
            current = s + strlen(from);
        }
        // For the last token
        if (s == NULL)
        {
            token = g_strndup(current,strlen(current));
            tokens_list = g_slist_prepend(tokens_list, token);
            n_tokens++;
        }
    }else
    {
        // We search an empty string... we suppose it exists between each UTF-8 character
        while (s
        &&    *s != '\0'
        &&    (s = g_utf8_find_next_char(current,NULL)) )
        {
            token = g_strndup(current, s - current);
            tokens_list = g_slist_prepend(tokens_list, token);
            n_tokens++;
            current = s;
        }
    }

    // Load the tokens list in an array for g_strjoinv
    result = g_new (gchar *, n_tokens + 1);
    result[n_tokens] = NULL;
    for (list = tokens_list; list != NULL; list = list->next)
        result[--n_tokens] = list->data;
    g_slist_free(tokens_list);

    g_free(*string);

    // Join all the tokens with the 'to' separator
    *string = g_strjoinv(to,result);

    g_strfreev(result);
}

/*
 * Quick roman numeral check (non-roman numerals may also return true)
 * Patch from Slash Bunny (2007.08.12)
 * (http://home.hiwaay.net/~lkseitz/math/roman/numerals.shtml)
 *    I = 1    (one)
 *    V = 5    (five)
 *    X = 10   (ten)
 *    L = 50   (fifty)
 *    C = 100  (one hundred)
 *    D = 500  (five hundred)
 *    M = 1000 (one thousand)
 */
gint Scan_Word_Is_Roman_Numeral (gchar *text)
{
    gchar *tmp;
    gint  len;
    gchar *buf = NULL;
    gint   rn_int;
    gchar *rn_char;

    if (!ScannerWindow
    ||  !ProcessFieldsDetectRomanNumerals
    ||  !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsDetectRomanNumerals)))
        return 0;
    
    tmp = text;
    len = 0;

    while (*tmp)
    {
        if (*tmp == (gunichar)'i'
        ||  *tmp == (gunichar)'v'
        ||  *tmp == (gunichar)'x'
        ||  *tmp == (gunichar)'l'
        ||  *tmp == (gunichar)'c'
        ||  *tmp == (gunichar)'d'
        ||  *tmp == (gunichar)'m')
        {
            // Found a roman numeral => continue
            tmp++;
            len++;
        } else if (*tmp == ' '
               ||  *tmp == '_'
               ||  *tmp == '.'
               ||  *tmp == ','
               ||  *tmp == '-')
        {
            // A separator was found => end of word
            // Check if it is a valid roman numeral
            goto roman_numeral_found;
            
        } else
        {
            return 0;
        }
    }

    // Found in last word of the string
    
roman_numeral_found:
    // Check if it is a valid roman numeral
    buf = g_strndup(text,len);
    rn_int = roman2int(buf); // Convert the Roman numeral string to integer
    
    if (rn_int >= 0 )
    {
        // Some strings as "IIIII" or "CCCCC" are returned as valid, which is not correct...
        // Same problem with: IC MIC IM MIM IL CIL XM MXM VC MVC VM MVM VL MVL LC MLC LD MLD LM MLM MDM 
        // So we convert it again to a string, and compare to the initial one.
        rn_char = (gchar *)int2roman(rn_int); // Convert the Roman numeral integer to string
        if (rn_char
        && strcasecmp(buf,rn_char)==0)
        {
            g_free(buf);
            g_free(rn_char);
            return len; // Roman numeral valid
        }else
        {
            g_free(buf);
            g_free(rn_char);
            return 0;
        }
    }else
    {
        g_free(buf);
        return 0;
    }
}



/*
 * Taken from :
 *   Roman Numeral Conversion API (http://sourceforge.net/project/showfiles.php?group_id=211070)
 *    Copyright (c) 2007 David M. Syzdek <roman-project@syzdek.net>
 */
/* Convert Roman numeral from integer to string */
const char * int2roman (int num)
{
    #define ROMAN_BUFF_LEN 512
    
    /* buffer for storing conversions */
    char roman_string[ROMAN_BUFF_LEN];

    /* wrap long2roman_r() with library buffer */
    char *result = int2roman_r(num, roman_string, ROMAN_BUFF_LEN);
   
    if (!result)
         return NULL;
    return g_strdup(roman_string);
}
char * int2roman_r (int num, char * str, size_t len)
{
   // local variables
   unsigned pos;
   unsigned u;
   unsigned dividend;

   // checks arguments
   if (!str)
   {
      return NULL;
   };
   // verify that number is withing boundaries
   if ((num > 5000) || (num < 0))
   {
      return NULL;
   };

   // sets initial values
   pos = 0;
   memset(str, 0, len);
   len--;

   // checks for nullae
   if (!(num))
   {
      str[0] = 'N';
      return str;
   };

   // calculates sign
   if (num < 0)
   {
      num *= -1;
      if (1 > len)
      {
         return NULL;
      };
      str[pos] = '-';
      pos++;
   };

   // calculates thousands
   dividend = num/1000;
   if (dividend > (len-1))
   {
      return NULL;
   };
   for (u = 0; u < dividend; u++)
      str[pos+u] = 'M';
   num %= 1000;
   pos += u;

   // calculates hundreds
   dividend = num/100;
   if (dividend > (len-1-pos))
   {
      return NULL;
   };
   if (dividend == 9)
   {
      str[pos+0] = 'C';
      str[pos+1] = 'M';
      pos += 2;
      dividend = 0;
   };
   if (dividend >= 5)
   {
      str[pos] = 'D';
      dividend -= 5;
      pos++;
   };
   if (dividend == 4)
   {
      str[pos+0] = 'C';
      str[pos+1] = 'D';
      dividend -= 4;
      pos += 2;
   };
   for(u = 0; u < dividend; u++)
      str[pos+u] = 'C';
   pos += u;
   num %= 100;

   // calculates tens
   dividend = num/10;
   if (dividend > (len-1-pos))
   {
      return NULL;
   };
   if (dividend == 9)
   {
      str[pos+0] = 'X';
      str[pos+1] = 'C';
      dividend = 0;
      pos += 2;
   };
   if (dividend >= 5)
   {
      str[pos+0] = 'L';
      dividend -= 5;
      pos++;
   };
   if (dividend == 4)
   {
      str[pos+0] = 'X';
      str[pos+1] = 'L';
      pos += 2;
      dividend -= 4;
   };
   for (u = 0; u < dividend; u++)
      str[pos+u] = 'X';
   pos += u;
   num %= 10;

   // calculates ones
   dividend = num;
   if (dividend > (len-1-pos))
   {
      return NULL;
   };
   if (dividend == 9)
   {
      str[pos+0] = 'I';
      str[pos+1] = 'X';
      dividend = 0;
      pos += 2;
   };
   if (dividend >= 5)
   {
      str[pos+0] = 'V';
      dividend -= 5;
      pos++;
   };
   if (dividend == 4)
   {
      str[pos+0] = 'I';
      str[pos+1] = 'V';
      pos += 2;
      dividend -= 4;
   };
   for(u = 0; u < dividend; u++)
      str[pos+u] = 'I';

   /* ends function */
   return str;
}
/* Convert Roman numeral from string to integer */
int roman2int (const char * str)
{
   // declares local vars
   int      num;
   unsigned i;
   unsigned len;
   unsigned last;
   unsigned prevlast;

   // checks args
   if (!(str))
   {
      return(0);
   };

   // sets initial values 
   num  = 0;
   len  = strlen(str);
   last = 1000;
   prevlast = 1000;

   // loops through characters
   for(i = 0; i < len; i++)
   {
      switch(str[i])
      {
         case 'n':
         case 'N':
            if (strlen(str) > 1)
            {
               return(-1);
            };
            return(0);
            break;
         case 'i':
         case 'I':
            num  += 1;
            // prevent patterns like IXI
            if ((prevlast == 1) && (last != 1))
            {
               return(-1);
            };
            // prevent patterns like IIIII and VIIIII
            if ((!(num%5)) || (!(num%10)))
            {
              return(-1);
            };
            // rotates value into history
            prevlast   = last;
            last  = 1;
            break;
         case 'v':
         case 'V':
            num += 5;
            // tests for invalid syntax
            if ( ((last <= 5) && (last != 1)) || (prevlast <= 5) )
            {
               return(-1);
            }
            // applies subtraction method & rotates value into history
            if (last < 5)
               num -= (last * 2);
            prevlast  = last;
            last = 5;
            break;
         case 'x':
         case 'X':
            num += 10;
            // tests for invalid syntax
            if ( ((prevlast < 10) && (last <= 10)) || ((last < 10) && (last != 1)) )
            {
               return(-1);
            };
            // prevent patterns like XXXXX and VXXXXX
            if ((!(num%50)) || (!(num%100)))
            {
               return(-1);
            };
            // applies subtraction method & rotates value into history
            if (last == 1)
               num -= (last * 2);
            prevlast  = last;
            last = 10;
            break;
         case 'l':
         case 'L':
            num += 50;
            // tests for invalid syntax
            if ( ((last <= 50) && (last != 10)) || (prevlast <= 50) )
            {
               return(-1);
            }
            // applies subtraction method & rotates value into history
            if (last < 50)
               num -= (last * 2);
            prevlast  = last;
            last = 50;
            break;
         case 'c':
         case 'C':
            num += 100;
            // tests for invalid syntax
            if ( ((prevlast < 100) && (last <= 100)) || ((last < 100) && (last != 10)) )
            {
               return(-1);
            };
            // prevent patterns like CCCCC and VCCCCC
            if ((!(num%500)) || (!(num%1000)))
            {
               return(-1);
            };
            // applies subtraction method & rotates value into history
            if (last == 10)
               num -= (last * 2);
            prevlast  = last;
            last = 100;
            break;
         case 'd':
         case 'D':
            num += 500;
            // tests for invalid syntax
            if ( ((last <= 500) && (last != 100)) || (prevlast <= 500) )
            {
               return(-1);
            }
            // applies subtraction method & rotates value into history
            if (last < 500)
               num -= (last * 2);
            prevlast  = last;
            last = 500;
            break;
         case 'm':
         case 'M':
            num += 1000;
            // tests for invalid syntax
            if ( ((prevlast < 1000) && (last <= 1000)) || ((last < 1000) && (last != 100)) )
            {
               return(-1);
            };
            // prevent patterns like MMMMM and VMMMMM
            if ((!(num%5000)) || (!(num%10000)))
            {
               return(-1);
            };
            // applies subtraction method & rotates value into history
            if (last == 100)
               num -= (last * 2);
            prevlast  = last;
            last = 1000;
            break;
         default:
            return(-1);
      };
   };

   // ends function
   return(num);
}



/*
 * Return the field of a 'File_Tag' structure corresponding to the mask code
 */
gchar **Scan_Return_File_Tag_Field_From_Mask_Code (File_Tag *FileTag, gchar code)
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
#include "../pixmaps/black.xpm"
#include "../pixmaps/blackwhite.xpm"
void Open_ScannerWindow (gint scanner_type)
{
    GtkWidget *ScanVBox;
    GtkWidget *HBox1, *HBox2, *HBox4, *VBox, *hbox, *vbox;
    GtkWidget *Table;
    GtkWidget *Label;
    GtkWidget *Button;
    GtkWidget *Separator;
    GtkWidget *Icon;
    GtkWidget *EventBox;
    GtkWidget *MaskStatusIconBox;
    GList *pf_cb_group1 = NULL;
    GList *pf_cb_group2 = NULL;
    GList *pf_cb_group3 = NULL;
    GtkTreeViewColumn * column;
    GtkCellRenderer *renderer;

    /* Check if already opened */
    if (ScannerWindow)
    {
        //gdk_window_show(ScannerWindow->window);
        gtk_window_present(GTK_WINDOW(ScannerWindow));
        if (ScannerOptionCombo)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(ScannerOptionCombo), scanner_type);
        }
        return;
    }

    if ( scanner_type < SCANNER_FILL_TAG
    ||   scanner_type > SCANNER_PROCESS_FIELDS)
        scanner_type = SCANNER_FILL_TAG;

    /* The window */
    ScannerWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    /* Config */
    gtk_container_set_border_width(GTK_CONTAINER(ScannerWindow), 5);
    gtk_window_set_resizable(GTK_WINDOW(ScannerWindow), FALSE);
    gtk_window_set_wmclass(GTK_WINDOW(ScannerWindow),"EasyTag_Scanner","easytag"); // Patch from Nikolai Prokoschenko (Debian)
    if (SCANNER_WINDOW_ON_TOP)
        gtk_window_set_transient_for(GTK_WINDOW(ScannerWindow),GTK_WINDOW(MainWindow));

    /* The init position is define below, cause the scanner window must be showed before
     * to be able to move it. */

    /* Title */
    gtk_window_set_title(GTK_WINDOW(ScannerWindow),_("Tag and File Name scan"));

    /* Signals connection */
    g_signal_connect(G_OBJECT(ScannerWindow),"destroy",G_CALLBACK(ScannerWindow_Quit),NULL);
    g_signal_connect(G_OBJECT(ScannerWindow),"delete_event",G_CALLBACK(ScannerWindow_Quit),NULL);
    g_signal_connect(G_OBJECT(ScannerWindow),"key_press_event",G_CALLBACK(ScannerWindow_Key_Press),NULL);

    /* The main vbox */
    ScanVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(ScannerWindow),ScanVBox);


    /*
     * The hbox for mode buttons + buttons + what to scan
     */
    HBox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(ScanVBox),HBox1,FALSE,FALSE,0);

    /* Option Menu */
    Label = gtk_label_new(_("Scanner:"));
    gtk_box_pack_start(GTK_BOX(HBox1),Label,FALSE,FALSE,0);

    EventBox = gtk_event_box_new();
    ScannerOptionCombo = gtk_combo_box_text_new();
    gtk_container_add(GTK_CONTAINER(EventBox),ScannerOptionCombo);
    gtk_box_pack_start(GTK_BOX(HBox1),EventBox,TRUE,TRUE,2);
    gtk_widget_set_size_request(ScannerOptionCombo, 160, -1);

    /* Option for Tag */
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ScannerOptionCombo),
                                   _(Scanner_Option_Menu_Items[SCANNER_FILL_TAG]));

    /* Option for FileName */
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ScannerOptionCombo),
                                   _(Scanner_Option_Menu_Items[SCANNER_RENAME_FILE]));

    /* Option for ProcessFields */
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ScannerOptionCombo),
                              _(Scanner_Option_Menu_Items[SCANNER_PROCESS_FIELDS]));

    // Selection of the item made at the end of the function
    gtk_widget_set_tooltip_text(EventBox, _("Select the type of scanner to use"));
    g_signal_connect(G_OBJECT(ScannerOptionCombo), "changed", G_CALLBACK(Scanner_Option_Menu_Activate_Item), NULL);

    /* 'Scan selected files' button */
    SWScanButton = gtk_button_new();
    Icon = gtk_image_new_from_icon_name ("document-properties", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(SWScanButton),Icon);
    gtk_box_pack_start(GTK_BOX(HBox1),SWScanButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(SWScanButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(SWScanButton,_("Open scanner window / Scan selected files"));
    g_signal_connect(G_OBJECT(SWScanButton),"clicked",G_CALLBACK(Action_Scan_Selected_Files),NULL);

    /* Separator line */
    Separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(HBox1),Separator,FALSE,FALSE,2);

    /* Options button */
    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(HBox1),Button,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Scanner Options"));
    g_signal_connect(G_OBJECT(Button),"clicked",G_CALLBACK(Scan_Option_Button),NULL);

    /* Mask Editor button */
    MaskEditorButton = gtk_toggle_button_new();
    Icon = gtk_image_new_from_stock("easytag-mask", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(MaskEditorButton),Icon);
    gtk_box_pack_start(GTK_BOX(HBox1),MaskEditorButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(MaskEditorButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(MaskEditorButton,_("Show / Hide Masks Editor"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(MaskEditorButton),SCAN_MASK_EDITOR_BUTTON);
    g_signal_connect(G_OBJECT(MaskEditorButton),"toggled",G_CALLBACK(Scan_Toggle_Mask_Editor_Button),NULL);

    /* Legend button */
    LegendButton = gtk_toggle_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_HELP, GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(LegendButton),Icon);
    gtk_box_pack_start(GTK_BOX(HBox1),LegendButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(LegendButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(LegendButton,_("Show / Hide Legend"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(LegendButton),SCAN_LEGEND_BUTTON);
    g_signal_connect(G_OBJECT(LegendButton),"toggled",G_CALLBACK(Scan_Toggle_Legend_Button),NULL);

    /* Close button */
    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(HBox1),Button,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Close this window"));
    g_signal_connect(G_OBJECT(Button),"clicked",G_CALLBACK(ScannerWindow_Quit),NULL);

    /*
     * Frame for Scan Tag
     */
    ScanTagFrame = gtk_frame_new (_(Scanner_Option_Menu_Items[0]));
    gtk_box_pack_start(GTK_BOX(ScanVBox),ScanTagFrame,FALSE,FALSE,0);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(ScanTagFrame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);

    /* The combo box + Status icon */
    HBox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(vbox),HBox2,TRUE,TRUE,0);

    // Set up list model which is used both by the combobox and the editor
    ScanTagListModel = gtk_list_store_new(MASK_EDITOR_COUNT, G_TYPE_STRING);

    // The combo box to select the mask to apply
    ScanTagMaskCombo = gtk_combo_box_new_with_entry();
    gtk_combo_box_set_model(GTK_COMBO_BOX(ScanTagMaskCombo), GTK_TREE_MODEL(ScanTagListModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(ScanTagMaskCombo), MASK_EDITOR_TEXT);

    gtk_box_pack_start(GTK_BOX(HBox2),ScanTagMaskCombo,TRUE,TRUE,2);
    gtk_widget_set_tooltip_text(GTK_WIDGET(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ScanTagMaskCombo)))),
        _("Select or type in a mask using codes (see Legend) to parse file name and "
        "path. Used to fill in tag fields."));
    // Signal to generate preview (preview of the new tag values)
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ScanTagMaskCombo)))),"changed",
        G_CALLBACK(Scan_Fill_Tag_Generate_Preview),NULL);

    // Load masks into the combobox from a file
    Load_Scan_Tag_Masks_List(ScanTagListModel, MASK_EDITOR_TEXT, Scan_Masks);
    if (SCAN_TAG_DEFAULT_MASK)
    {
        Add_String_To_Combo_List(ScanTagListModel, SCAN_TAG_DEFAULT_MASK);
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ScanTagMaskCombo))), SCAN_TAG_DEFAULT_MASK);
    }else
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(ScanTagMaskCombo), 0);
    }

    // Mask status icon
    MaskStatusIconBox = Create_Pixmap_Icon_With_Event_Box("easytag-forbidden");
    gtk_box_pack_start(GTK_BOX(HBox2),MaskStatusIconBox,FALSE,FALSE,0);
    gtk_widget_set_tooltip_text(MaskStatusIconBox,_("Invalid Scanner Mask"));
    // Signal connection to check if mask is correct into the mask entry
    g_signal_connect_swapped(G_OBJECT(gtk_bin_get_child(GTK_BIN(ScanTagMaskCombo))),"changed",
        G_CALLBACK(Scan_Check_Scan_Tag_Mask),G_OBJECT(MaskStatusIconBox));

    // Preview label
    FillTagPreviewLabel = gtk_label_new(_("Fill tag preview…"));
    gtk_label_set_line_wrap(GTK_LABEL(FillTagPreviewLabel),TRUE);
    gtk_widget_show(FillTagPreviewLabel);
    gtk_box_pack_start(GTK_BOX(vbox),FillTagPreviewLabel,TRUE,TRUE,0);

    /*
     * Frame for Rename File
     */
    RenameFileFrame = gtk_frame_new (_(Scanner_Option_Menu_Items[1]));
    gtk_box_pack_start(GTK_BOX(ScanVBox),RenameFileFrame,FALSE,FALSE,0);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(RenameFileFrame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);

    /* The button to prefix path + combo box + Status icon */
    HBox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(vbox),HBox4,TRUE,TRUE,0);

    // Button to prefix path
    RenameFilePrefixPathButton = gtk_button_new();
    Icon = gtk_image_new_from_stock("easytag-add-folder", GTK_ICON_SIZE_SMALL_TOOLBAR); // On Win32, GTK_ICON_SIZE_BUTTON enlarge the combobox...
    gtk_container_add(GTK_CONTAINER(RenameFilePrefixPathButton),Icon);
    gtk_box_pack_start(GTK_BOX(HBox4),RenameFilePrefixPathButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(RenameFilePrefixPathButton),GTK_RELIEF_NONE);
    g_signal_connect(G_OBJECT(RenameFilePrefixPathButton),"clicked",G_CALLBACK(Scan_Rename_File_Prefix_Path),NULL);
    gtk_widget_set_tooltip_text(RenameFilePrefixPathButton,_("Prefix mask with current path"));

    // Set up list model which is used both by the combobox and the editor
    RenameFileListModel = gtk_list_store_new(MASK_EDITOR_COUNT, G_TYPE_STRING);

    // The combo box to select the mask to apply
    RenameFileMaskCombo = gtk_combo_box_new_with_entry();
    gtk_combo_box_set_model(GTK_COMBO_BOX(RenameFileMaskCombo), GTK_TREE_MODEL(RenameFileListModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(RenameFileMaskCombo), MASK_EDITOR_TEXT);

    gtk_box_pack_start(GTK_BOX(HBox4),RenameFileMaskCombo,TRUE,TRUE,2);
    gtk_container_set_border_width(GTK_CONTAINER(HBox4), 2);
    gtk_widget_set_tooltip_text(GTK_WIDGET(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo)))),
        _("Select or type in a mask using codes (see Legend) to parse tag fields. "
        "Used to rename the file.\nUse / to make directories. If the first character "
        "is /, it's a absolute path, otherwise is relative to the old path."));
    // Signal to generate preview (preview of the new filename)
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo)))),"changed",
        G_CALLBACK(Scan_Rename_File_Generate_Preview),NULL);

    // Load masks into the combobox from a file
    Load_Rename_File_Masks_List(RenameFileListModel, MASK_EDITOR_TEXT, Rename_File_Masks);
    if (RENAME_FILE_DEFAULT_MASK)
    {
        Add_String_To_Combo_List(RenameFileListModel, RENAME_FILE_DEFAULT_MASK);
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo))), RENAME_FILE_DEFAULT_MASK);
    }else
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(RenameFileMaskCombo), 0);
    }

    // Mask status icon
    MaskStatusIconBox = Create_Pixmap_Icon_With_Event_Box("easytag-forbidden");
    gtk_box_pack_start(GTK_BOX(HBox4),MaskStatusIconBox,FALSE,FALSE,0);
    gtk_widget_set_tooltip_text(MaskStatusIconBox,_("Invalid Scanner Mask"));
    // Signal connection to check if mask is correct into the mask entry
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo)))),"changed",
        G_CALLBACK(Scan_Check_Rename_File_Mask),G_OBJECT(MaskStatusIconBox));

    /* Preview label */
    RenameFilePreviewLabel = gtk_label_new(_("Rename file preview…"));
    gtk_label_set_line_wrap(GTK_LABEL(RenameFilePreviewLabel),TRUE);
    gtk_widget_show(RenameFilePreviewLabel);
    gtk_box_pack_start(GTK_BOX(vbox),RenameFilePreviewLabel,TRUE,TRUE,0);

    /*
     * Frame for Processing Fields
     */
    ProcessFieldsFrame = gtk_frame_new (_(Scanner_Option_Menu_Items[2]));
    gtk_box_pack_start(GTK_BOX(ScanVBox),ProcessFieldsFrame,FALSE,FALSE,0);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(ProcessFieldsFrame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 4);
    gtk_widget_show(VBox);

    /* Group: select entry fields to process */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(VBox),hbox,FALSE,FALSE,2);
    EventBox = gtk_event_box_new();
    Label = gtk_label_new(_("Select fields:"));
    gtk_box_pack_start(GTK_BOX(hbox),EventBox,FALSE,FALSE,2);
    gtk_container_add(GTK_CONTAINER(EventBox),Label);
    gtk_widget_set_tooltip_text(EventBox,_("The buttons on the right represent the fields which can "
        "be processed. Select those who interest you."));
    // Advice for Translators: set the first letter of filename translated
    ProcessFileNameField = gtk_toggle_button_new_with_label(   _("F"));
    gtk_widget_set_tooltip_text(ProcessFileNameField,          _("Process file name field"));
    // Advice for Translators: set the first letter of title translated
    ProcessTitleField = gtk_toggle_button_new_with_label(      _("T"));
    gtk_widget_set_tooltip_text(ProcessTitleField,             _("Process title field"));
    // Advice for Translators: set the first letter of artist translated
    ProcessArtistField = gtk_toggle_button_new_with_label(     _("Ar"));
    gtk_widget_set_tooltip_text(ProcessArtistField,            _("Process file artist field"));
    // Advice for Translators: set the first letter of album artist translated
    ProcessAlbumArtistField = gtk_toggle_button_new_with_label(_("AA"));
    gtk_widget_set_tooltip_text(ProcessAlbumArtistField,       _("Process album artist field"));
    // Advice for Translators: set the first letter of album translated
    ProcessAlbumField = gtk_toggle_button_new_with_label(      _("Al"));
    gtk_widget_set_tooltip_text(ProcessAlbumField,             _("Process album field"));
    // Advice for Translators: set the first letter of genre translated
    ProcessGenreField = gtk_toggle_button_new_with_label(      _("G"));
    gtk_widget_set_tooltip_text(ProcessGenreField,             _("Process genre field"));
    // Advice for Translators: set the first letter of comment translated
    ProcessCommentField = gtk_toggle_button_new_with_label(    _("Cm"));
    gtk_widget_set_tooltip_text(ProcessCommentField,           _("Process comment field"));
    // Advice for Translators: set the first letter of composer translated
    ProcessComposerField = gtk_toggle_button_new_with_label(   _("Cp"));
    gtk_widget_set_tooltip_text(ProcessComposerField,          _("Process composer field"));
    // Advice for Translators: set the first letter of orig artist translated
    ProcessOrigArtistField = gtk_toggle_button_new_with_label( _("O"));
    gtk_widget_set_tooltip_text(ProcessOrigArtistField,        _("Process original artist field"));
    // Advice for Translators: set the first letter of copyright translated
    ProcessCopyrightField = gtk_toggle_button_new_with_label(  _("Cr"));
    gtk_widget_set_tooltip_text(ProcessCopyrightField,         _("Process copyright field"));
    // Advice for Translators: set the first letter of URL translated
    ProcessURLField = gtk_toggle_button_new_with_label(        _("U"));
    gtk_widget_set_tooltip_text(ProcessURLField,               _("Process URL field"));
    // Advice for Translators: set the first letter of encoder name translated
    ProcessEncodedByField = gtk_toggle_button_new_with_label(  _("E"));
    gtk_widget_set_tooltip_text(ProcessEncodedByField,         _("Process encoder name field"));
    gtk_box_pack_start(GTK_BOX(hbox),ProcessFileNameField,   TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessTitleField,      TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessArtistField,     TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessAlbumArtistField,TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessAlbumField,      TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessGenreField,      TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessCommentField,    TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessComposerField,   TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessOrigArtistField, TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessCopyrightField,  TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessURLField,        TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessEncodedByField,  TRUE,TRUE,2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFileNameField),   PROCESS_FILENAME_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessTitleField),      PROCESS_TITLE_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessArtistField),     PROCESS_ARTIST_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessAlbumArtistField),PROCESS_ALBUM_ARTIST_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessAlbumField),      PROCESS_ALBUM_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessGenreField),      PROCESS_GENRE_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessCommentField),    PROCESS_COMMENT_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessComposerField),   PROCESS_COMPOSER_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessOrigArtistField), PROCESS_ORIG_ARTIST_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessCopyrightField),  PROCESS_COPYRIGHT_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessURLField),        PROCESS_URL_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessEncodedByField),  PROCESS_ENCODED_BY_FIELD);
    g_signal_connect(G_OBJECT(ProcessFileNameField),   "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessTitleField),      "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessArtistField),     "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessAlbumArtistField),"toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessAlbumField),      "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessGenreField),      "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessCommentField),    "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessComposerField),   "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessOrigArtistField), "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessCopyrightField),  "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessURLField),        "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    g_signal_connect(G_OBJECT(ProcessEncodedByField),  "toggled",G_CALLBACK(Select_Fields_Set_Sensitive),NULL);
    /* The small buttons */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_box_pack_start(GTK_BOX(hbox),vbox,FALSE,FALSE,0);
    Button = gtk_button_new();
    g_signal_connect(G_OBJECT(Button),"clicked",G_CALLBACK(Select_Fields_Invert_Selection),NULL);
    gtk_box_pack_start(GTK_BOX(vbox),Button,FALSE,FALSE,0);
    gtk_widget_set_size_request(Button, 12, 12);
    //Icon = gtk_image_new_from_stock("easytag-blackwhite", GTK_ICON_SIZE_BUTTON);
    Icon = Create_Xpm_Image((const char **)blackwhite_xpm);
    gtk_widget_set_can_default(Button,FALSE); // To have enough space to display the icon
    gtk_widget_set_can_focus(Button,FALSE);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_widget_set_tooltip_text(Button,_("Invert Selection"));
    Button = gtk_button_new();
    g_signal_connect(G_OBJECT(Button),"clicked",G_CALLBACK(Select_Fields_Select_Unselect_All),NULL);
    gtk_box_pack_start(GTK_BOX(vbox),Button,FALSE,FALSE,0);
    gtk_widget_set_size_request(Button, 12, 12);
    Icon = Create_Xpm_Image((const char **)black_xpm);
    gtk_widget_set_can_default(Button,FALSE); // To have enough space to display the icon
    gtk_widget_set_can_focus(Button,FALSE);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_widget_set_tooltip_text(Button,_("Select/Unselect All"));

    /* Separator line */
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    /* Group: character conversion */
    ProcessFieldsConvertIntoSpace = gtk_check_button_new_with_label(_("Convert '_' and '%20' to spaces"));
    ProcessFieldsConvertSpace     = gtk_check_button_new_with_label(_("Convert ' ' to '_'"));
    gtk_box_pack_start(GTK_BOX(VBox),ProcessFieldsConvertIntoSpace,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(VBox),ProcessFieldsConvertSpace,    FALSE,FALSE,0);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(VBox),hbox,FALSE,FALSE,0);
    ProcessFieldsConvert          = gtk_check_button_new_with_label(_("Convert:"));  // Patch from Ben Hearsum, Oct. 3, 2003
    ProcessFieldsConvertTo        = gtk_entry_new();
    ProcessFieldsConvertLabelTo   = gtk_label_new(_("to: ")); // A "space" at the end to allow another translation for "to :" (needed in French!)
    ProcessFieldsConvertFrom      = gtk_entry_new();
    //gtk_entry_set_max_length(GTK_ENTRY(ProcessFieldsConvertTo), 1); // Now, it isn't limited to one character
    //gtk_entry_set_max_length(GTK_ENTRY(ProcessFieldsConvertFrom), 1);
    gtk_widget_set_size_request(ProcessFieldsConvertTo,120,-1);
    gtk_widget_set_size_request(ProcessFieldsConvertFrom,120,-1);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessFieldsConvert,       FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessFieldsConvertFrom,   FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessFieldsConvertLabelTo,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessFieldsConvertTo,     FALSE,FALSE,0);
    if (PROCESS_FIELDS_CONVERT_FROM)
        gtk_entry_set_text(GTK_ENTRY(ProcessFieldsConvertFrom),PROCESS_FIELDS_CONVERT_FROM);
    if (PROCESS_FIELDS_CONVERT_TO)
        gtk_entry_set_text(GTK_ENTRY(ProcessFieldsConvertTo),PROCESS_FIELDS_CONVERT_TO);
    /* List creation for check buttons in group */
    pf_cb_group1 = g_list_append (pf_cb_group1,ProcessFieldsConvertIntoSpace);
    pf_cb_group1 = g_list_append (pf_cb_group1,ProcessFieldsConvertSpace);
    pf_cb_group1 = g_list_append (pf_cb_group1,ProcessFieldsConvert);
    /* Toggled signals */
    g_signal_connect(G_OBJECT(ProcessFieldsConvertIntoSpace),"toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group1);
    g_signal_connect(G_OBJECT(ProcessFieldsConvertSpace),    "toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group1);
    g_signal_connect(G_OBJECT(ProcessFieldsConvert),         "toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group1);
    g_signal_connect(G_OBJECT(ProcessFieldsConvert),         "toggled",G_CALLBACK(Process_Fields_Convert_Check_Button_Toggled),NULL);
    /* Set check buttons to init value */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvertIntoSpace),PF_CONVERT_INTO_SPACE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvertSpace),PF_CONVERT_SPACE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvert),PF_CONVERT);
    /* Tooltips */
    gtk_widget_set_tooltip_text(ProcessFieldsConvertIntoSpace,
        _("The underscore character or the string '%20' are replaced by one space. "
          "Example, before: 'Text%20In%20An_Entry', after: 'Text In An Entry'."));
    gtk_widget_set_tooltip_text(ProcessFieldsConvertSpace,
        _("The space character is replaced by one underscore character. "
          "Example, before: 'Text In An Entry', after: 'Text_In_An_Entry'."));
    gtk_widget_set_tooltip_text(ProcessFieldsConvert,
        _("Replace a string by another one. Note that the search is case sensitive."));

    /* Separator line */
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    
    /* Group: capitalize, ... */
    ProcessFieldsAllUppercase = gtk_check_button_new_with_label         (_("All uppercase"));
    ProcessFieldsAllDowncase  = gtk_check_button_new_with_label         (_("All lowercase"));
    ProcessFieldsFirstLetterUppercase  = gtk_check_button_new_with_label(_("First letter uppercase"));
    ProcessFieldsFirstLettersUppercase = gtk_check_button_new_with_label(_("First letter uppercase of each word"));
    ProcessFieldsDetectRomanNumerals = gtk_check_button_new_with_label(_("Detect Roman numerals"));
    gtk_box_pack_start(GTK_BOX(VBox),ProcessFieldsAllUppercase,         FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(VBox),ProcessFieldsAllDowncase,          FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(VBox),ProcessFieldsFirstLetterUppercase, FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(VBox),hbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessFieldsFirstLettersUppercase,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),ProcessFieldsDetectRomanNumerals,FALSE,FALSE,0);
    /* List creation for check buttons in group */
    pf_cb_group2 = g_list_append(pf_cb_group2,ProcessFieldsAllUppercase);
    pf_cb_group2 = g_list_append(pf_cb_group2,ProcessFieldsAllDowncase);
    pf_cb_group2 = g_list_append(pf_cb_group2,ProcessFieldsFirstLetterUppercase);
    pf_cb_group2 = g_list_append(pf_cb_group2,ProcessFieldsFirstLettersUppercase);
    /* Toggled signals */
    g_signal_connect(G_OBJECT(ProcessFieldsAllUppercase),"toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group2);
    g_signal_connect(G_OBJECT(ProcessFieldsAllDowncase), "toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group2);
    g_signal_connect(G_OBJECT(ProcessFieldsFirstLetterUppercase),"toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group2);
    g_signal_connect(G_OBJECT(ProcessFieldsFirstLettersUppercase),"toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group2);
    g_signal_connect(G_OBJECT(ProcessFieldsFirstLettersUppercase),"toggled",G_CALLBACK(Process_Fields_First_Letters_Check_Button_Toggled),NULL);
    g_signal_connect(G_OBJECT(ProcessFieldsDetectRomanNumerals),"toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),NULL);
    /* Set check buttons to init value */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsAllUppercase),PF_CONVERT_ALL_UPPERCASE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsAllDowncase),PF_CONVERT_ALL_DOWNCASE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsFirstLetterUppercase),PF_CONVERT_FIRST_LETTER_UPPERCASE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsFirstLettersUppercase),PF_CONVERT_FIRST_LETTERS_UPPERCASE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsDetectRomanNumerals),PF_DETECT_ROMAN_NUMERALS);
    /* Tooltips */
    gtk_widget_set_tooltip_text(ProcessFieldsAllUppercase,
        _("Convert all words in all fields to upper case. "
          "Example, before: 'Text IN AN entry', after: 'TEXT IN AN ENTRY'."));
    gtk_widget_set_tooltip_text(ProcessFieldsAllDowncase,
        _("Convert all words in all fields to lower case. "
          "Example, before: 'TEXT IN an entry', after: 'text in an entry'."));
    gtk_widget_set_tooltip_text(ProcessFieldsFirstLetterUppercase,
        _("Convert the initial of the first word in all fields to upper case. "
          "Example, before: 'text IN An ENTRY', after: 'Text in an entry'."));
    gtk_widget_set_tooltip_text(ProcessFieldsFirstLettersUppercase,
        _("Convert the initial of each word in all fields to upper case. "
          "Example, before: 'Text in an ENTRY', after: 'Text In An Entry'."));
    gtk_widget_set_tooltip_text(ProcessFieldsDetectRomanNumerals,
        _("Force to convert to upper case the Roman numerals. "
          "Example, before: 'ix. text in an entry', after: 'IX. Text In An Entry'."));

    /* Separator line */
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    /* Group: insert/remove spaces */
    ProcessFieldsRemoveSpace  = gtk_check_button_new_with_label(_("Remove spaces"));
    ProcessFieldsInsertSpace  = gtk_check_button_new_with_label(_("Insert a space before an uppercase letter"));
    ProcessFieldsOnlyOneSpace = gtk_check_button_new_with_label(_("Remove duplicates of space or underscore"));
    gtk_box_pack_start(GTK_BOX(VBox),ProcessFieldsRemoveSpace,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(VBox),ProcessFieldsInsertSpace,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(VBox),ProcessFieldsOnlyOneSpace,FALSE,FALSE,0);
    /* List creation for check buttons in group */
    pf_cb_group3 = g_list_append(pf_cb_group3,ProcessFieldsRemoveSpace);
    pf_cb_group3 = g_list_append(pf_cb_group3,ProcessFieldsInsertSpace);
    pf_cb_group3 = g_list_append(pf_cb_group3,ProcessFieldsOnlyOneSpace);
    /* Toggled signals */
    g_signal_connect(G_OBJECT(ProcessFieldsRemoveSpace), "toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group3);
    g_signal_connect(G_OBJECT(ProcessFieldsInsertSpace), "toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group3);
    g_signal_connect(G_OBJECT(ProcessFieldsOnlyOneSpace),"toggled",G_CALLBACK(Process_Fields_Check_Button_Toggled),pf_cb_group3);
    /* Set check buttons to init value */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsRemoveSpace),PF_REMOVE_SPACE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsInsertSpace),PF_INSERT_SPACE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFieldsOnlyOneSpace),PF_ONLY_ONE_SPACE);
    /* Tooltips */
    gtk_widget_set_tooltip_text(ProcessFieldsRemoveSpace,
        _("All spaces between words are removed. "
          "Example, before: 'Text In An Entry', after: 'TextInAnEntry'."));
    gtk_widget_set_tooltip_text(ProcessFieldsInsertSpace,
        _("A space is inserted before each upper case letter. "
          "Example, before: 'TextInAnEntry', after: 'Text In An Entry'."));
    gtk_widget_set_tooltip_text(ProcessFieldsOnlyOneSpace,
        _("Duplicated spaces or underscores are removed. "
          "Example, before: 'Text__In__An   Entry', after: 'Text_In_An Entry'."));
    Select_Fields_Set_Sensitive();

    /*
     * Frame to display codes legend
     */
    LegendFrame = gtk_frame_new (_("Legend"));
    gtk_box_pack_start(GTK_BOX(ScanVBox),LegendFrame,FALSE,FALSE,0);
    /* Legend labels */
    Table = gtk_table_new (3, 6, FALSE);
    gtk_container_add(GTK_CONTAINER(LegendFrame),Table);
    gtk_container_set_border_width(GTK_CONTAINER(Table),4);
    Label = gtk_label_new(_("%a: artist"));
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,0,1,0,1);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%z: album artist"));
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,0,1,1,2);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%b: album"));
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,0,1,2,3);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%c: comment"));
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,0,1,3,4);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%p: composer"));
    gtk_table_attach_defaults (GTK_TABLE (Table), Label, 0, 1, 4, 5);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%r: copyright"));
    gtk_table_attach_defaults (GTK_TABLE (Table), Label, 0, 1, 5, 6);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%d: disc number"));
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,1,2,0,1);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%e: encoded by"));
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,1,2,1,2);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%g: genre"));
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,1,2,2,3);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%i: ignored"));
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,1,2,3,4);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%l: number of tracks"));
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,1,2,4,5);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%o: orig. artist"));
    gtk_table_attach_defaults (GTK_TABLE (Table), Label, 1, 2, 5, 6);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%n: track"));
    gtk_table_attach_defaults (GTK_TABLE (Table), Label, 2, 3, 0, 1);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%t: title"));
    gtk_table_attach_defaults (GTK_TABLE (Table), Label, 2, 3, 1, 2);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%u: URL"));
    gtk_table_attach_defaults (GTK_TABLE (Table), Label, 2, 3, 2, 3);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);
    Label = gtk_label_new(_("%y: year"));
    gtk_table_attach_defaults (GTK_TABLE (Table), Label, 2, 3, 3, 4);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);

    /*
     * Masks Editor
     */
    MaskEditorFrame = gtk_frame_new (_("Mask Editor"));
    gtk_box_pack_start(GTK_BOX(ScanVBox),MaskEditorFrame,FALSE,FALSE,0);
    MaskEditorHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_container_add(GTK_CONTAINER(MaskEditorFrame),MaskEditorHBox);
    gtk_container_set_border_width(GTK_CONTAINER(MaskEditorHBox), 4);

    /* The editor part */
    MaskEditorVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_box_pack_start(GTK_BOX(MaskEditorHBox),MaskEditorVBox,TRUE,TRUE,0);
    MaskEditorScrollWindow = gtk_scrolled_window_new(NULL,NULL);
    gtk_box_pack_start(GTK_BOX(MaskEditorVBox),MaskEditorScrollWindow,TRUE,TRUE,0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(MaskEditorScrollWindow),
                                   GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(MaskEditorScrollWindow), -1, 101);

    /* The list */
    MaskEditorList = gtk_tree_view_new();
    gtk_tree_view_set_model(GTK_TREE_VIEW(MaskEditorList), GTK_TREE_MODEL(ScanTagListModel));

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(NULL,
            renderer, "text", MASK_EDITOR_TEXT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(MaskEditorList), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(MaskEditorList), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(MaskEditorList)),
                GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(MaskEditorList), TRUE);
    gtk_container_add(GTK_CONTAINER(MaskEditorScrollWindow), MaskEditorList);
    g_signal_connect_after(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(MaskEditorList))),
            "changed", G_CALLBACK(Mask_Editor_List_Row_Selected), NULL);
    g_signal_connect(G_OBJECT(MaskEditorList), "key-press-event",
        G_CALLBACK(Mask_Editor_List_Key_Press), NULL);
    /* The entry */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(MaskEditorVBox),hbox,FALSE,FALSE,0);
    MaskEditorEntry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox),MaskEditorEntry,TRUE,TRUE,2);
    g_signal_connect(G_OBJECT(MaskEditorEntry),"changed",
        G_CALLBACK(Mask_Editor_Entry_Changed),NULL);
    // Mask status icon
    MaskStatusIconBox = Create_Pixmap_Icon_With_Event_Box("easytag-forbidden");
    gtk_box_pack_start(GTK_BOX(hbox),MaskStatusIconBox,FALSE,FALSE,0);
    gtk_widget_set_tooltip_text(MaskStatusIconBox,_("Invalid Scanner Mask"));
    // Signal connection to check if mask is correct into the mask entry
    g_signal_connect_swapped(G_OBJECT(MaskEditorEntry),"changed",
        G_CALLBACK(Scan_Check_Editor_Mask),G_OBJECT(MaskStatusIconBox));

    /* The buttons part */
    MaskEditorVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start(GTK_BOX(MaskEditorHBox),MaskEditorVBox,FALSE,FALSE,0);

    /* New mask button */
    MaskEditorNewButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(MaskEditorNewButton),Icon);
    gtk_box_pack_start(GTK_BOX(MaskEditorVBox),MaskEditorNewButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(MaskEditorNewButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(MaskEditorNewButton,_("Create New Mask"));
    g_signal_connect(G_OBJECT(MaskEditorNewButton),"clicked",
        G_CALLBACK(Mask_Editor_List_New),NULL);

    /* Move up mask button */
    MaskEditorUpButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(MaskEditorUpButton),Icon);
    gtk_box_pack_start(GTK_BOX(MaskEditorVBox),MaskEditorUpButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(MaskEditorUpButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(MaskEditorUpButton,_("Move Up this Mask"));
    g_signal_connect(G_OBJECT(MaskEditorUpButton),"clicked",
        G_CALLBACK(Mask_Editor_List_Move_Up),NULL);

    /* Move down mask button */
    MaskEditorDownButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(MaskEditorDownButton),Icon);
    gtk_box_pack_start(GTK_BOX(MaskEditorVBox),MaskEditorDownButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(MaskEditorDownButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(MaskEditorDownButton,_("Move Down this Mask"));
    g_signal_connect(G_OBJECT(MaskEditorDownButton),"clicked",
        G_CALLBACK(Mask_Editor_List_Move_Down),NULL);

    /* Copy mask button */
    MaskEditorCopyButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(MaskEditorCopyButton),Icon);
    gtk_box_pack_start(GTK_BOX(MaskEditorVBox),MaskEditorCopyButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(MaskEditorCopyButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(MaskEditorCopyButton,_("Duplicate Mask"));
    g_signal_connect(G_OBJECT(MaskEditorCopyButton),"clicked",
        G_CALLBACK(Mask_Editor_List_Duplicate),NULL);

    /* Add mask button */
    MaskEditorAddButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(MaskEditorAddButton),Icon);
    gtk_box_pack_start(GTK_BOX(MaskEditorVBox),MaskEditorAddButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(MaskEditorAddButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(MaskEditorAddButton,_("Add Default Masks"));
    g_signal_connect(G_OBJECT(MaskEditorAddButton),"clicked",
        G_CALLBACK(Mask_Editor_List_Add),NULL);

    /* Remove mask button */
    MaskEditorRemoveButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(MaskEditorRemoveButton),Icon);
    gtk_box_pack_start(GTK_BOX(MaskEditorVBox),MaskEditorRemoveButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(MaskEditorRemoveButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(MaskEditorRemoveButton,_("Remove Mask"));
    g_signal_connect(G_OBJECT(MaskEditorRemoveButton),"clicked",
        G_CALLBACK(Mask_Editor_List_Remove),NULL);

    /* Save mask button */
    MaskEditorSaveButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(MaskEditorSaveButton),Icon);
    gtk_box_pack_end(GTK_BOX(MaskEditorVBox),MaskEditorSaveButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(MaskEditorSaveButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(MaskEditorSaveButton,_("Save Masks"));
    g_signal_connect(G_OBJECT(MaskEditorSaveButton),"clicked",
        G_CALLBACK(Mask_Editor_List_Save_Button),NULL);

    gtk_widget_show(ScanVBox);
    gtk_widget_show_all(HBox1);
    gtk_widget_show_all(HBox2);
    gtk_widget_show_all(HBox4);
    gtk_widget_show(ScannerWindow);

    /* Init position of the scanner window */
    Scan_Set_Scanner_Window_Init_Position();

    /* To initialize the mask status icon and visibility */
    g_signal_emit_by_name(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ScanTagMaskCombo)))),"changed");
    g_signal_emit_by_name(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo)))),"changed");
    g_signal_emit_by_name(G_OBJECT(MaskEditorEntry),"changed");
    g_signal_emit_by_name(G_OBJECT(LegendButton),"toggled");        /* To hide legend frame */
    g_signal_emit_by_name(G_OBJECT(MaskEditorButton),"toggled");    /* To hide mask editor frame */
    g_signal_emit_by_name(G_OBJECT(ProcessFieldsConvert),"toggled");/* To enable / disable entries */
    g_signal_emit_by_name(G_OBJECT(ProcessFieldsDetectRomanNumerals),"toggled");/* To enable / disable entries */

    // Activate the current menu in the option menu
    gtk_combo_box_set_active(GTK_COMBO_BOX(ScannerOptionCombo), scanner_type);
}

gboolean ScannerWindow_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_KEY_Escape:
                ScannerWindow_Quit();
                break;
        }
    }
    return FALSE;
}

/*
 * Select the scanner to run for the current ETFile
 */
void Scan_Select_Mode_And_Run_Scanner (ET_File *ETFile)
{
    if (!ScannerWindow || !ETFile) return;

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) == SCANNER_FILL_TAG)
    {
        /* Run Scanner Type: Scan Tag */
        Scan_Tag_With_Mask(ETFile);
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) == SCANNER_RENAME_FILE)
    {
        /* Run Scanner Type: Rename File */
        Scan_Rename_File_With_Mask(ETFile);
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) == SCANNER_PROCESS_FIELDS)
    {
        /* Run Scanner Type: Process Fields */
        Scan_Process_Fields(ETFile);
    }
}

void Scan_Use_Fill_Tag_Scanner (void)
{
    if (!ScannerWindow || gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) != SCANNER_FILL_TAG)
        Open_ScannerWindow(SCANNER_FILL_TAG);
    else
        Action_Scan_Selected_Files();
}


void Scan_Use_Rename_File_Scanner (void)
{
    if (!ScannerWindow || gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) != SCANNER_RENAME_FILE)
        Open_ScannerWindow(SCANNER_RENAME_FILE);
    else
        Action_Scan_Selected_Files();
}

void Scan_Use_Process_Fields_Scanner (void)
{
    if (!ScannerWindow || gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) != SCANNER_PROCESS_FIELDS)
        Open_ScannerWindow(SCANNER_PROCESS_FIELDS);
    else
        Action_Scan_Selected_Files();
}


/* Callback from Open_ScannerWindow */
void ScannerWindow_Quit (void)
{
    if (ScannerWindow)
    {
        ScannerWindow_Apply_Changes();

        gtk_widget_destroy(ScannerWindow);
        gtk_list_store_clear(ScanTagListModel);
        gtk_list_store_clear(RenameFileListModel);
        ScannerWindow     = NULL;
        ScannerOptionCombo= NULL;
        SWScanButton      = NULL;

        // To avoid crashs after tests
        ScanTagMaskCombo              = NULL;
        RenameFileMaskCombo           = NULL;
        MaskEditorEntry               = NULL;
        LegendFrame                   = NULL;
        ProcessFieldsConvertIntoSpace = NULL;
        ProcessFieldsConvertSpace     = NULL;
        FillTagPreviewLabel           = NULL;
        RenameFilePreviewLabel        = NULL;
    }
}


/*
 * For the configuration file...
 */
void ScannerWindow_Apply_Changes (void)
{
    if (ScannerWindow)
    {
        gint x, y;//, width, height;
        GdkWindow *window;

        window = gtk_widget_get_window(ScannerWindow);

        if ( window && gdk_window_is_visible(window) && gdk_window_get_state(window)!=GDK_WINDOW_STATE_MAXIMIZED )
        {
            // Position and Origin of the scanner window
            gdk_window_get_root_origin(window,&x,&y);
            SCANNER_WINDOW_X = x;
            SCANNER_WINDOW_Y = y;
            //gdk_window_get_size(window,&width,&height);
            //SCANNER_WINDOW_WIDTH  = width;
            //SCANNER_WINDOW_HEIGHT = height;
        }

        // The scanner selected
        SCANNER_TYPE = gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo));

        SCAN_MASK_EDITOR_BUTTON   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(MaskEditorButton));
        SCAN_LEGEND_BUTTON        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(LegendButton));

        /* Group: select entries to process */
        PROCESS_FILENAME_FIELD    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFileNameField));
        PROCESS_TITLE_FIELD       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessTitleField));
        PROCESS_ARTIST_FIELD      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessArtistField));
        PROCESS_ALBUM_ARTIST_FIELD= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessAlbumArtistField));
        PROCESS_ALBUM_FIELD       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessAlbumField));
        PROCESS_GENRE_FIELD       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessGenreField));
        PROCESS_COMMENT_FIELD     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessCommentField));
        PROCESS_COMPOSER_FIELD    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessComposerField));
        PROCESS_ORIG_ARTIST_FIELD = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessOrigArtistField));
        PROCESS_COPYRIGHT_FIELD   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessCopyrightField));
        PROCESS_URL_FIELD         = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessURLField));
        PROCESS_ENCODED_BY_FIELD  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessEncodedByField));

        if (PROCESS_FIELDS_CONVERT_FROM) g_free(PROCESS_FIELDS_CONVERT_FROM);
        PROCESS_FIELDS_CONVERT_FROM = g_strdup(gtk_entry_get_text(GTK_ENTRY(ProcessFieldsConvertFrom)));
        if (PROCESS_FIELDS_CONVERT_TO) g_free(PROCESS_FIELDS_CONVERT_TO);
        PROCESS_FIELDS_CONVERT_TO   = g_strdup(gtk_entry_get_text(GTK_ENTRY(ProcessFieldsConvertTo)));

        /* Group: convert one character */
        PF_CONVERT_INTO_SPACE     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvertIntoSpace));
        PF_CONVERT_SPACE          = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvertSpace));
        PF_CONVERT                = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvert));

        /* Group: capitalize */
        PF_CONVERT_ALL_UPPERCASE           = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsAllUppercase));
        PF_CONVERT_ALL_DOWNCASE            = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsAllDowncase));
        PF_CONVERT_FIRST_LETTER_UPPERCASE  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsFirstLetterUppercase));
        PF_CONVERT_FIRST_LETTERS_UPPERCASE = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsFirstLettersUppercase));
        PF_DETECT_ROMAN_NUMERALS           = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsDetectRomanNumerals));

        /* Group: remove/insert space */
        PF_REMOVE_SPACE   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsRemoveSpace));
        PF_INSERT_SPACE   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsInsertSpace));
        PF_ONLY_ONE_SPACE = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsOnlyOneSpace));

        // Save default masks...
        if (SCAN_TAG_DEFAULT_MASK) g_free(SCAN_TAG_DEFAULT_MASK);
        SCAN_TAG_DEFAULT_MASK = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ScanTagMaskCombo)))));
        Add_String_To_Combo_List(ScanTagListModel, SCAN_TAG_DEFAULT_MASK);
        Save_Rename_File_Masks_List(ScanTagListModel, MASK_EDITOR_TEXT);

        if (RENAME_FILE_DEFAULT_MASK) g_free(RENAME_FILE_DEFAULT_MASK);
        RENAME_FILE_DEFAULT_MASK = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameFileMaskCombo)))));
        Add_String_To_Combo_List(RenameFileListModel, RENAME_FILE_DEFAULT_MASK);
        Save_Rename_File_Masks_List(RenameFileListModel, MASK_EDITOR_TEXT);

    }
}


/* Callback from Option button */
void Scan_Option_Button (void)
{
    Open_OptionsWindow();
    gtk_notebook_set_current_page(GTK_NOTEBOOK(OptionsNoteBook), OptionsNoteBook_Scanner_Page_Num);
}



/*
 * Check if mask of the "Scan Tag" is valid. Return TRUE if valid, else FALSE.
 */
gboolean Scan_Check_Scan_Tag_Mask (GtkWidget *widget_to_show_hide, GtkEntry *widget_source)
{
    gchar *tmp  = NULL;
    gchar *mask = NULL;
    gint loop = 0;


    if (!widget_to_show_hide || !widget_source)
        goto Bad_Mask;

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget_source)));
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
        gtk_widget_show(GTK_WIDGET(widget_to_show_hide));
        return FALSE;

    Good_Mask:
        g_free(mask);
        gtk_widget_hide(GTK_WIDGET(widget_to_show_hide));
        return TRUE;
}
/*
 * Check if mask of the "Rename File" is valid. Return TRUE if valid, else FALSE.
 */
gboolean Scan_Check_Rename_File_Mask (GtkWidget *widget_to_show_hide, GtkEntry *widget_source)
{
    gchar *tmp = NULL;
    gchar *mask = NULL;


    if (!widget_to_show_hide || !widget_source)
        goto Bad_Mask;

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget_source)));
    if (!mask || strlen(mask)<1)
        goto Bad_Mask;

    // Since version 1.99.4, it is available! => OK
    /*if ( strchr(mask,G_DIR_SEPARATOR)!=NULL ) // Renaming directory is not yet available
        goto Bad_Mask;*/
    // Not a valid path....
    if ( strstr(mask,"//") != NULL
    ||   strstr(mask,"./") != NULL
    ||   strstr(mask,"../") != NULL)
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
        gtk_widget_show(GTK_WIDGET(widget_to_show_hide));
        return FALSE;

    Good_Mask:
        g_free(mask);
        gtk_widget_hide(GTK_WIDGET(widget_to_show_hide));
        return TRUE;
}


/*
 * Check if the selected mask in the Mask Editor is valid, else display the mask status icon.
 */
gboolean Scan_Check_Editor_Mask (GtkWidget *widget_to_show_hide, GtkEntry *widget_source)
{
    /* Select and get result of check scanner */
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) == SCANNER_FILL_TAG)
    {
        return Scan_Check_Scan_Tag_Mask(widget_to_show_hide,widget_source);
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) == SCANNER_RENAME_FILE)
    {
        return Scan_Check_Rename_File_Mask(widget_to_show_hide,widget_source);
    } else
        return FALSE;
}


void Scan_Toggle_Legend_Button (void)
{
    if (!LegendButton || !LegendFrame) return;

    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(LegendButton)) )
        gtk_widget_show_all(LegendFrame);
    else
        gtk_widget_hide(LegendFrame);
}


void Scan_Toggle_Mask_Editor_Button (void)
{
    GtkTreeModel *treemodel;
    GtkTreeSelection *selection;
    GtkTreeIter iter;

    if (!MaskEditorButton || !MaskEditorFrame || !MaskEditorList) return;

    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(MaskEditorButton)) )
    {
        gtk_widget_show_all(MaskEditorFrame);

        // Select first row in list
        treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));
        if (gtk_tree_model_get_iter_first(treemodel, &iter))
        {
            selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MaskEditorList));
            gtk_tree_selection_unselect_all(selection);
            gtk_tree_selection_select_iter(selection, &iter);
        }

        // Update status of the icon box cause prev instruction show it for all cases
        g_signal_emit_by_name(G_OBJECT(MaskEditorEntry),"changed");
    }else
    {
        gtk_widget_hide(MaskEditorFrame);
    }
}



/*
 * Manage/Toggle check buttons into 'Process Fields' frame
 */
void Process_Fields_Check_Button_Toggled (GtkWidget *object, GList *list)
{
    gint i = 0;

    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object)) )
    {
        while (list)
        {
            if ( list->data!=NULL && list->data!=object )
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(list->data),FALSE);
            i++;
            if (!list->next) break;
            list = list->next;
        }
    }
}


void Process_Fields_Convert_Check_Button_Toggled (GtkWidget *object)
{
    gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertTo),gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object)));
    gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertFrom),gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object)));
}

void Process_Fields_First_Letters_Check_Button_Toggled (GtkWidget *object)
{
    gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsDetectRomanNumerals),gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object)));
}


/*
 * Small buttons of Process Fields scanner
 */
void Select_Fields_Invert_Selection (void)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFileNameField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFileNameField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessTitleField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessTitleField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessArtistField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessArtistField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessAlbumArtistField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessAlbumArtistField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessAlbumField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessAlbumField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessGenreField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessGenreField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessCommentField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessCommentField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessComposerField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessComposerField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessOrigArtistField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessOrigArtistField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessCopyrightField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessCopyrightField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessURLField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessURLField)));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessEncodedByField),
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessEncodedByField)));
}
void Select_Fields_Select_Unselect_All (void)
{
    static gboolean state = TRUE;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessFileNameField),   state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessTitleField),      state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessArtistField),     state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessAlbumArtistField),state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessAlbumField),      state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessGenreField),      state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessCommentField),    state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessComposerField),   state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessOrigArtistField), state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessCopyrightField),  state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessURLField),        state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ProcessEncodedByField),  state);
    state = !state;
}

/*
 * Set sensitive state of the processing check boxes : if no one is selected => all disabled
 */
void Select_Fields_Set_Sensitive (void)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFileNameField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessTitleField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessArtistField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessAlbumArtistField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessAlbumField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessGenreField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessCommentField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessComposerField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessOrigArtistField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessCopyrightField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessURLField))
    ||  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessEncodedByField)))
    {
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertIntoSpace),     TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertSpace),         TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvert),              TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertLabelTo),       TRUE);
        // Activate the two entries only if the check box is activated, else keep them disabled
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ProcessFieldsConvert)))
        {
            gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertTo),        TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertFrom),      TRUE);
        }
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsAllUppercase),         TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsAllDowncase),          TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsFirstLetterUppercase), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsFirstLettersUppercase),TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsDetectRomanNumerals),  TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsRemoveSpace),          TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsInsertSpace),          TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsOnlyOneSpace),         TRUE);
    }else
    {
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertIntoSpace),     FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertSpace),         FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvert),              FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertTo),            FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertLabelTo),       FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsConvertFrom),          FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsAllUppercase),         FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsAllDowncase),          FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsFirstLetterUppercase), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsFirstLettersUppercase),FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsDetectRomanNumerals),  FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsRemoveSpace),          FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsInsertSpace),          FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ProcessFieldsOnlyOneSpace),         FALSE);
    }
}

/*
 * Callbacks from Mask Editor buttons
 */

/*
 * Callback from the mask edit list
 * Previously known as Mask_Editor_List_Select_Row
 */
void Mask_Editor_List_Row_Selected (GtkTreeSelection* selection, gpointer data)
{
    GList *selectedRows;
    gchar *text = NULL;
    GtkTreePath *lastSelected;
    GtkTreeIter lastFile;
    GtkTreeModel *treemodel;
    gboolean valid;

    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));

    /* We must block the function, else the previous selected row will be modified */
    g_signal_handlers_block_by_func(G_OBJECT(MaskEditorEntry),
                                    G_CALLBACK(Mask_Editor_Entry_Changed),NULL);

    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    /*
     * At some point, we might get called when no rows are selected?
     */
    if (g_list_length(selectedRows) == 0)
    {
        g_list_foreach(selectedRows, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selectedRows);
        g_signal_handlers_unblock_by_func(G_OBJECT(MaskEditorEntry),
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
            gtk_entry_set_text(GTK_ENTRY(MaskEditorEntry),text);
            g_free(text);
        }
    }

    g_signal_handlers_unblock_by_func(G_OBJECT(MaskEditorEntry),
                                      G_CALLBACK(Mask_Editor_Entry_Changed),NULL);
}


/*
 * Add a new mask to the list
 */
void Mask_Editor_List_New (void)
{
    gchar *text = _("New_mask");
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreeModel *treemodel;

    if (!MaskEditorList) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MaskEditorList));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));

    gtk_list_store_insert(GTK_LIST_STORE(treemodel), &iter, 0);
    gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter, MASK_EDITOR_TEXT, text, -1);

    gtk_tree_selection_unselect_all(selection);
    gtk_tree_selection_select_iter(selection, &iter);
}

/*
 * Duplicate a mask on the list
 */
void Mask_Editor_List_Duplicate (void)
{
    gchar *text = NULL;
    GList *selectedRows;
    GList *toInsert = NULL;
    GtkTreeSelection *selection;
    GtkTreeIter rowIter;
    GtkTreeModel *treeModel;
    gboolean valid;

    if (!MaskEditorList) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MaskEditorList));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);
    treeModel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));

    if (g_list_length(selectedRows) == 0)
    {
        Log_Print(LOG_ERROR,_("Copy: No row selected"));
        g_list_foreach(selectedRows, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selectedRows);
        return;
    }

    /* Loop through selected rows, duplicating them into a GList
     * We cannot directly insert because the paths in selectedRows
     * get out of date after an insertion */
    while (selectedRows)
    {
        valid = gtk_tree_model_get_iter(treeModel, &rowIter, (GtkTreePath*) selectedRows->data);
        if (valid)
        {
            gtk_tree_model_get(treeModel, &rowIter, MASK_EDITOR_TEXT, &text, -1);
            toInsert = g_list_append(toInsert, text);
        }

        selectedRows = selectedRows->next;
        if (!selectedRows) break;
    }

    /* Duplicate the relevant entries, by looping through the list backwards
     * (to preserve original order) */
    toInsert = g_list_last(toInsert);
    while (toInsert)
    {
        gtk_list_store_insert(GTK_LIST_STORE(treeModel), &rowIter, 0);
        gtk_list_store_set(GTK_LIST_STORE(treeModel), &rowIter, MASK_EDITOR_TEXT, (gchar*) toInsert->data, -1);
        g_free(toInsert->data);

        toInsert = toInsert->prev;
        if (!toInsert) break;
    }
    // Set focus to the last inserted line
    if (toInsert)
        Mask_Editor_List_Set_Row_Visible(treeModel,&rowIter);

    /* Free data no longer needed */
    selectedRows = g_list_first(selectedRows);
    toInsert = g_list_first(toInsert);
    g_list_foreach(selectedRows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selectedRows);
    g_list_foreach(toInsert, (GFunc) g_free, NULL);
    g_list_free(toInsert);
}

void Mask_Editor_List_Add (void)
{
    gint i = 0;
    GtkTreeIter iter;
    GtkTreeModel *treemodel;
    gchar *temp;

    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) == SCANNER_FILL_TAG)
    {
        while(Scan_Masks[i])
        {
            /*if (!g_utf8_validate(Scan_Masks[i], -1, NULL))
                temp = convert_to_utf8(Scan_Masks[i]);
            else
                temp = g_strdup(Scan_Masks[i]);*/
            temp = Try_To_Validate_Utf8_String(Scan_Masks[i]);

            gtk_list_store_append(GTK_LIST_STORE(treemodel), &iter);
            gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter,
                               MASK_EDITOR_TEXT, temp, -1);
            g_free(temp);
            i++;
        }
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) == SCANNER_RENAME_FILE)
    {
        while(Rename_File_Masks[i])
        {
            /*if (!g_utf8_validate(Rename_File_Masks[i], -1, NULL))
                temp = convert_to_utf8(Rename_File_Masks[i]);
            else
                temp = g_strdup(Rename_File_Masks[i]);*/
            temp = Try_To_Validate_Utf8_String(Rename_File_Masks[i]);

            gtk_list_store_append(GTK_LIST_STORE(treemodel), &iter);
            gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter,
                               MASK_EDITOR_TEXT, temp, -1);
            g_free(temp);
            i++;
        }
    }
}

/*
 * Remove the selected rows from the mask editor list
 */
void Mask_Editor_List_Remove (void)
{
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkTreeModel *treemodel;

    if (!MaskEditorList) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MaskEditorList));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));

    if (gtk_tree_selection_count_selected_rows(selection) == 0) {
        Log_Print(LOG_ERROR,_("Remove: No row selected"));
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
 * Move all selected rows up one place in the mask list
 */
void Mask_Editor_List_Move_Up (void)
{
    GtkTreeSelection *selection;
    GList *selectedRows;
    GList *selectedRowsCopy;
    GtkTreeIter currentFile;
    GtkTreeIter nextFile;
    GtkTreePath *currentPath;
    GtkTreeModel *treemodel;
    gboolean valid;

    if (!MaskEditorList) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MaskEditorList));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (g_list_length(selectedRows) == 0)
    {
        Log_Print(LOG_ERROR,_("Move Up: No row selected"));
        g_list_foreach(selectedRows, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(selectedRows);
        return;
    }

    selectedRowsCopy = selectedRows;

    while (selectedRows)
    {
        currentPath = (GtkTreePath*) selectedRows->data;
        valid = gtk_tree_model_get_iter(treemodel, &currentFile, currentPath);
        if (valid)
        {
            /* Find the entry above the node... */
            if (gtk_tree_path_prev(currentPath))
            {
                /* ...and if it exists, swap the two rows by iter */
                gtk_tree_model_get_iter(treemodel, &nextFile, currentPath);
                gtk_list_store_swap(GTK_LIST_STORE(treemodel), &currentFile, &nextFile);
            }
        }

        selectedRows = selectedRows->next;
        if (!selectedRows) break;
    }

    g_list_foreach(selectedRowsCopy, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selectedRowsCopy);
}

/*
 * Move all selected rows down one place in the mask list
 */
void Mask_Editor_List_Move_Down (void)
{
    GtkTreeSelection *selection;
    GList *selectedRows;
    GList *selectedRowsCopy;
    GtkTreeIter currentFile;
    GtkTreeIter nextFile;
    GtkTreePath *currentPath;
    GtkTreeModel *treemodel;
    gboolean valid;

    if (!MaskEditorList) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MaskEditorList));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (g_list_length(selectedRows) == 0)
    {
        Log_Print(LOG_ERROR,_("Move Down: No row selected"));
        g_list_foreach(selectedRows, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(selectedRows);
        return;
    }

    selectedRowsCopy = selectedRows;

    while (selectedRows)
    {
        currentPath = (GtkTreePath*) selectedRows->data;
        valid = gtk_tree_model_get_iter(treemodel, &currentFile, currentPath);
        if (valid)
        {
            /* Find the entry below the node and swap the two nodes by iter */
            gtk_tree_path_next(currentPath);
            if (gtk_tree_model_get_iter(treemodel, &nextFile, currentPath))
                gtk_list_store_swap(GTK_LIST_STORE(treemodel), &currentFile, &nextFile);
        }

        if (!selectedRows->next) break;
        selectedRows = selectedRows->next;
    }

    g_list_foreach(selectedRowsCopy, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selectedRowsCopy);
}

/*
 * Set a row visible in the mask editor list (by scrolling the list)
 */
void Mask_Editor_List_Set_Row_Visible (GtkTreeModel *treeModel, GtkTreeIter *rowIter)
{
    /*
     * TODO: Make this only scroll to the row if it is not visible
     * (like in easytag GTK1)
     * See function gtk_tree_view_get_visible_rect() ??
     */
    GtkTreePath *rowPath;

    if (!treeModel) return;

    rowPath = gtk_tree_model_get_path(treeModel, rowIter);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(MaskEditorList), rowPath, NULL, FALSE, 0, 0);
    gtk_tree_path_free(rowPath);
}

/*
 * Save the currently displayed mask list in the mask editor
 */
void Mask_Editor_List_Save_Button (void)
{
    Mask_Editor_Clean_Up_Masks_List();

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) == SCANNER_FILL_TAG)
    {
        Save_Scan_Tag_Masks_List(ScanTagListModel, MASK_EDITOR_TEXT);
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(ScannerOptionCombo)) == SCANNER_RENAME_FILE)
    {
        Save_Rename_File_Masks_List(RenameFileListModel, MASK_EDITOR_TEXT);
    }
}

/*
 * Clean up the currently displayed masks lists, ready for saving
 */
void Mask_Editor_Clean_Up_Masks_List (void)
{
    gchar *text = NULL;
    gchar *text1 = NULL;
    GtkTreeIter currentIter;
    GtkTreeIter itercopy;
    GtkTreeModel *treemodel;

    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));

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
 * Update the Mask List with the new value of the entry box
 */
void Mask_Editor_Entry_Changed (void)
{
    GtkTreeSelection *selection;
    GtkTreePath *firstSelected;
    GtkTreeModel *treemodel;
    GList *selectedRows;
    GtkTreeIter row;
    const gchar* text;
    gboolean valid;

    if (!MaskEditorList) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MaskEditorList));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(MaskEditorList));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (g_list_length(selectedRows) == 0)
    {
        g_list_foreach(selectedRows, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selectedRows);
        return;
    }

    firstSelected = (GtkTreePath *)g_list_first(selectedRows)->data;
    text = gtk_entry_get_text(GTK_ENTRY(MaskEditorEntry));

    valid = gtk_tree_model_get_iter(treemodel, &row, firstSelected);
    if (valid)
        gtk_list_store_set(GTK_LIST_STORE(treemodel), &row, MASK_EDITOR_TEXT, text, -1);

    g_list_foreach(selectedRows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selectedRows);
}

/*
 * Actions when the a key is pressed into the masks editor clist
 */
gboolean Mask_Editor_List_Key_Press (GtkWidget *widget, GdkEvent *event)
{
    if (event && event->type == GDK_KEY_PRESS) {
        GdkEventKey *kevent = (GdkEventKey *)event;

        switch(kevent->keyval)
        {
            case GDK_KEY_Delete:
                Mask_Editor_List_Remove();
                break;
/*          case GDK_KEY_Up:
                Mask_Editor_Clist_Move_Up();
                break;
            case GDK_KEY_Down:
                Mask_Editor_Clist_Move_Down();
                break;
*/      }
    }
    return TRUE;
}

/*
 * Function when you select an item of the option menu
 */
void Scanner_Option_Menu_Activate_Item (GtkWidget *combo, gpointer data)
{
    switch (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)))
    {
        case SCANNER_FILL_TAG:
            gtk_widget_show(MaskEditorButton);
            gtk_widget_show(LegendButton);
            gtk_widget_show(ScanTagFrame);
            gtk_widget_hide(RenameFileFrame);
            gtk_widget_hide(ProcessFieldsFrame);
            gtk_tree_view_set_model(GTK_TREE_VIEW(MaskEditorList), GTK_TREE_MODEL(ScanTagListModel));
            Scan_Fill_Tag_Generate_Preview();
            g_signal_emit_by_name(G_OBJECT(LegendButton),"toggled");        /* To hide or show legend frame */
            g_signal_emit_by_name(G_OBJECT(MaskEditorButton),"toggled");    /* To hide or show mask editor frame */
            break;

        case SCANNER_RENAME_FILE:
            gtk_widget_show(MaskEditorButton);
            gtk_widget_show(LegendButton);
            gtk_widget_hide(ScanTagFrame);
            gtk_widget_show(RenameFileFrame);
            gtk_widget_hide(ProcessFieldsFrame);
            gtk_tree_view_set_model(GTK_TREE_VIEW(MaskEditorList), GTK_TREE_MODEL(RenameFileListModel));
            Scan_Rename_File_Generate_Preview();
            g_signal_emit_by_name(G_OBJECT(LegendButton),"toggled");        /* To hide or show legend frame */
            g_signal_emit_by_name(G_OBJECT(MaskEditorButton),"toggled");    /* To hide or show mask editor frame */
            break;

        case SCANNER_PROCESS_FIELDS:
            gtk_widget_hide(MaskEditorButton);
            gtk_widget_hide(LegendButton);
            gtk_widget_hide(ScanTagFrame);
            gtk_widget_hide(RenameFileFrame);
            gtk_widget_show_all(ProcessFieldsFrame);
            // Hide directly the frames to don't change state of the buttons!
            gtk_widget_hide(LegendFrame);
            gtk_widget_hide(MaskEditorFrame);

            gtk_tree_view_set_model(GTK_TREE_VIEW(MaskEditorList), NULL);
            break;
    }
}

/*
 * Init the position of the scanner window
 */
void Scan_Set_Scanner_Window_Init_Position (void)
{
    if (ScannerWindow && SET_SCANNER_WINDOW_POSITION)
    {
        gtk_widget_realize(ScannerWindow);
        gtk_window_move(GTK_WINDOW(ScannerWindow),SCANNER_WINDOW_X,SCANNER_WINDOW_Y);
    }
}
