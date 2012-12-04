/* misc.c - 2000/06/28 */
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
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n-lib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "gtk2_compat.h"
#include "misc.h"
#include "easytag.h"
#include "id3_tag.h"
#include "browser.h"
#include "setting.h"
#include "bar.h"
#include "prefs.h"
#include "scan.h"
#include "genres.h"
#include "log.h"
#include "charset.h"

#ifdef WIN32
#   include <windows.h>
#endif


/***************
 * Declaration *
 ***************/
// Playlist window defined in misc.h

// Search file window
GtkWidget *SearchFileWindow = NULL;
GtkWidget *SearchStringCombo;
GtkListStore *SearchStringModel = NULL;
GtkWidget *SearchInFilename;
GtkWidget *SearchInTag;
GtkWidget *SearchCaseSensitive;
GtkWidget *SearchResultList;
GtkListStore *SearchResultListModel;
GtkWidget *SearchStatusBar;
guint      SearchStatusBarContext;

// Load filename window
GtkWidget *LoadFilenameWindow  = NULL;
GtkWidget *FileToLoadCombo;
GtkListStore *FileToLoadModel = NULL;
GtkWidget *LoadFileContentList;
GtkListStore* LoadFileContentListModel;
GtkWidget *LoadFileNameList;
GtkListStore* LoadFileNameListModel;


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

enum
{
    LOAD_FILE_CONTENT_TEXT,
    LOAD_FILE_CONTENT_COUNT
};

enum
{
    LOAD_FILE_NAME_TEXT,
    LOAD_FILE_NAME_POINTER,
    LOAD_FILE_NAME_COUNT
};

/**************
 * Prototypes *
 **************/
void     Open_Write_Playlist_Window      (void);
gboolean Write_Playlist_Window_Key_Press (GtkWidget *window, GdkEvent *event);
void     Destroy_Write_Playlist_Window   (void);
void     Playlist_Write_Button_Pressed   (void);
gboolean Write_Playlist                  (gchar *play_list_name);
gboolean Playlist_Check_Content_Mask     (GtkWidget *widget_to_show_hide, GtkEntry *widget_source);
void     Playlist_Convert_Forwardslash_Into_Backslash (gchar *string);

void Open_Search_File_Window          (void);
void Destroy_Search_File_Window       (void);
gboolean Search_File_Window_Key_Press (GtkWidget *window, GdkEvent *event);
void Search_File                      (GtkWidget *search_button);
void Add_Row_To_Search_Result_List    (ET_File *ETFile,const gchar *string_to_search);
void Search_Result_List_Row_Selected  (GtkTreeSelection* selection, gpointer data);

void Open_Load_Filename_Window      (void);
void Destroy_Load_Filename_Window   (void);
gboolean Load_Filename_Window_Key_Press (GtkWidget *window, GdkEvent *event);
void Load_Filename_List_Key_Press   (GtkWidget *clist, GdkEvent *event);
void Load_File_Content              (GtkWidget *file_entry);
void Load_File_List                 (void);
void Load_Filename_Select_Row_In_Other_List (GtkWidget *target, gpointer selection_emit);
void Load_Filename_Set_Filenames            (void);
void Button_Load_Set_Sensivity              (GtkWidget *button, GtkWidget *entry);
GtkWidget *Create_Load_Filename_Popup_Menu     (GtkWidget *list);
void Load_Filename_List_Insert_Blank_Line      (GtkWidget *list);
void Load_Filename_List_Delete_Line            (GtkWidget *list);
void Load_Filename_List_Move_Up                (GtkWidget *list);
void Load_Filename_List_Move_Down              (GtkWidget *list);
void Load_Filename_List_Delete_All_Blank_Lines (GtkWidget *list);
void Load_Filename_List_Reload                 (GtkWidget *list);
void Load_Filename_Update_Text_Line            (GtkWidget *entry, GtkWidget *list);
void Load_Filename_Edit_Text_Line              (GtkTreeSelection *selection, gpointer data);

void Create_Xpm_Icon_Factory (const char **xpm_data, const char *name_in_factory);
void Create_Png_Icon_Factory (const char *png_file, const char *name_in_factory);

/* Browser */
static void Open_File_Selection_Window (GtkWidget *entry, gchar *title, GtkFileChooserAction action);
void        File_Selection_Window_For_File      (GtkWidget *entry);
void        File_Selection_Window_For_Directory (GtkWidget *entry);


/*************
 * Functions *
 *************/

/******************************
 * Functions managing pixmaps *
 ******************************/

/*
 * Create an icon into an event box to allow some events (as to display tooltips).
 */
GtkWidget *Create_Pixmap_Icon_With_Event_Box (const gchar *pixmap_name)
{
    GtkWidget *icon;
    GtkWidget *EventBox;

    EventBox = gtk_event_box_new();
    if (pixmap_name)
    {
        icon = gtk_image_new_from_stock(pixmap_name, GTK_ICON_SIZE_BUTTON);
        gtk_container_add(GTK_CONTAINER(EventBox),icon);
    }

    return EventBox;
}

/*
 * Return a button with an icon and a label
 */
GtkWidget *Create_Button_With_Icon_And_Label (const gchar *pixmap_name, gchar *label)
{
    GtkWidget *Button;
    GtkWidget *HBox;
    GtkWidget *Label;
    GtkWidget *Pixmap;

    Button = gtk_button_new();
    HBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_container_add(GTK_CONTAINER(Button),HBox);

    /* Add a pixmap if not null */
    if (pixmap_name != NULL)
    {
        gtk_widget_realize(MainWindow);
        Pixmap = gtk_image_new_from_stock(pixmap_name, GTK_ICON_SIZE_BUTTON);
        gtk_container_add(GTK_CONTAINER(HBox),Pixmap);
    }

    /* Add a label if not null */
    if (label != NULL)
    {
        Label = gtk_label_new(label);
        gtk_container_add(GTK_CONTAINER(HBox),Label);
    }

    /* Display a warning message if the both parameters are NULL */
    if (pixmap_name==NULL && label==NULL)
        g_warning("Empty button created 'adr=%p' (no icon and no label)!",Button);

    return Button;
}

/*
 * Add the 'string' passed in parameter to the list store
 * If this string already exists in the list store, it doesn't add it.
 * Returns TRUE if string was added.
 */
gboolean Add_String_To_Combo_List (GtkListStore *liststore, const gchar *str)
{
    GtkTreeIter iter;
    gchar *text;
    const gint HISTORY_MAX_LENGTH = 15;
    //gboolean found = FALSE;
    gchar *string = g_strdup(str);

    if (!string || g_utf8_strlen(string, -1) <= 0)
        return FALSE;

#if 0
    // We add the string to the beginning of the list store
    // So we will start to parse from the second line below
    gtk_list_store_prepend(liststore, &iter);
    gtk_list_store_set(liststore, &iter, MISC_COMBO_TEXT, string, -1);

    // Search in the list store if string already exists and remove other same strings in the list
    found = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);
    //gtk_tree_model_get(GTK_TREE_MODEL(liststore), &iter, MISC_COMBO_TEXT, &text, -1);
    while (found && gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(liststore), &iter, MISC_COMBO_TEXT, &text, -1);
        //g_print(">0>%s\n>1>%s\n",string,text);
        if (g_utf8_collate(text, string) == 0)
        {
            g_free(text);
            // FIX ME : it seems that after it selects the next item for the
            // combo (changes 'string')????
            // So should select the first item?
            gtk_list_store_remove(liststore, &iter);
            // Must be rewinded?
            found = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);
            //gtk_tree_model_get(GTK_TREE_MODEL(liststore), &iter, MISC_COMBO_TEXT, &text, -1);
            continue;
        }
        g_free(text);
    }

    // Limit list size to HISTORY_MAX_LENGTH
    while (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(liststore),NULL) > HISTORY_MAX_LENGTH)
    {
        if ( gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(liststore),
                                           &iter,NULL,HISTORY_MAX_LENGTH) )
        {
            gtk_list_store_remove(liststore, &iter);
        }
    }

    g_free(string);
    // Place again to the beginning of the list, to select the right value?
    //gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);

    return TRUE;

#else

    // Search in the list store if string already exists.
    // FIXME : insert string at the beginning of the list (if already exists),
    //         and remove other same strings in the list
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter))
    {
        do
        {
            gtk_tree_model_get(GTK_TREE_MODEL(liststore), &iter, MISC_COMBO_TEXT, &text, -1);
            if (g_utf8_collate(text, string) == 0)
            {
                g_free(text);
                return FALSE;
            }

            g_free(text);
        } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter));
    }

    // We add the string to the beginning of the list store
    gtk_list_store_prepend(liststore, &iter);
    gtk_list_store_set(liststore, &iter, MISC_COMBO_TEXT, string, -1);

    // Limit list size to HISTORY_MAX_LENGTH
    while (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(liststore),NULL) > HISTORY_MAX_LENGTH)
    {
        if ( gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(liststore),
                                           &iter,NULL,HISTORY_MAX_LENGTH) )
        {
            gtk_list_store_remove(liststore, &iter);
        }
    }

    g_free(string);
    return TRUE;
#endif
}

/*
 * Returns the text of the selected item in a combo box
 * Remember to free the returned value...
 */
gchar *Get_Active_Combo_Box_Item (GtkComboBox *combo)
{
    gchar *text;
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (!combo)
        return NULL;

    model = gtk_combo_box_get_model(combo);
    if (!gtk_combo_box_get_active_iter(combo, &iter))
        return NULL;

    gtk_tree_model_get(model, &iter, MISC_COMBO_TEXT, &text, -1);
    return text;
}

/*
 * Event attached to an entry to disable another widget (for example: a button)
 * when the entry is empty
 */
void Entry_Changed_Disable_Object(GtkWidget *widget_to_disable, GtkEditable *source_widget)
{
    gchar *text = NULL;

    if (!widget_to_disable || !source_widget) return;

    text = gtk_editable_get_chars(GTK_EDITABLE(source_widget),0,-1);
    if (!text || strlen(text)<1)
        gtk_widget_set_sensitive(widget_to_disable,FALSE);
    else
        gtk_widget_set_sensitive(widget_to_disable,TRUE);

    g_free(text);
}

/*
 * To insert only digits in an entry. If the text contains only digits: returns it,
 * else only first digits.
 */
void Insert_Only_Digit (GtkEditable *editable, const gchar *inserted_text, gint length,
                        gint *position, gpointer data)
{
    int i = 1; // Ignore first character
    int j = 1;
    gchar *result;

    if (length<=0 || !inserted_text)
        return;

    if (!isdigit((guchar)inserted_text[0]) && inserted_text[0] != '-')
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
        if (isdigit((guchar)inserted_text[i]))
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
#include <time.h>
#include <stdlib.h>
gboolean Parse_Date (void)
{
    const gchar *year;
    gchar *tmp, *tmp1;
    gchar current_year[5];
    time_t t;
    struct tm t0;

    if (!DATE_AUTO_COMPLETION) return FALSE;

    /* Get the info entered by user */
    year = gtk_entry_get_text(GTK_ENTRY(YearEntry));

    if ( strcmp(year,"")!=0 && strlen(year)<4 )
    {
        t = time(NULL);
        /* Get the current date */
        memcpy(&t0, localtime(&t), sizeof(struct tm));
        /* Put the current year in 'current_year' tab */
        sprintf(current_year,"%04d",1900+t0.tm_year);

        tmp = &current_year[4-strlen(year)];
        if ( atoi(year) <= atoi(tmp) )
        {
            sprintf(current_year,"%d",atoi(current_year)-atoi(tmp));
            tmp1 = g_strdup_printf("%d",atoi(current_year)+atoi(year));
            gtk_entry_set_text(GTK_ENTRY(YearEntry),tmp1);
            g_free(tmp1);

        }else
        {
            sprintf(current_year,"%d",atoi(current_year)-atoi(tmp)
                -(strlen(year)<=0 ? 1 : strlen(year)<=1 ? 10 :          // pow(10,strlen(year)) returns 99 instead of 100 under Win32...
                 strlen(year)<=2 ? 100 : strlen(year)<=3 ? 1000 : 0));
            tmp1 = g_strdup_printf("%d",atoi(current_year)+atoi(year));
            gtk_entry_set_text(GTK_ENTRY(YearEntry),tmp1);
            g_free(tmp1);
        }
    }
    return FALSE;
}

/*
 * Load the genres list to the combo, and sorts it
 */
void Load_Genres_List_To_UI (void)
{
    guint i;
    GtkTreeIter iter;

    if (!GenreComboModel) return;

    gtk_list_store_append(GTK_LIST_STORE(GenreComboModel), &iter);
    gtk_list_store_set(GTK_LIST_STORE(GenreComboModel), &iter, MISC_COMBO_TEXT, "", -1);

    gtk_list_store_append(GTK_LIST_STORE(GenreComboModel), &iter);
    gtk_list_store_set(GTK_LIST_STORE(GenreComboModel), &iter, MISC_COMBO_TEXT, "Unknown", -1);

    for (i=0; i<=GENRE_MAX; i++)
    {
        gtk_list_store_append(GTK_LIST_STORE(GenreComboModel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(GenreComboModel), &iter, MISC_COMBO_TEXT, id3_genres[i], -1);
    }
}

/*
 * Load the track numbers into the track combo list
 * We limit the preloaded values to 30 to avoid lost of time with lot of files...
 */
void Load_Track_List_To_UI (void)
{
    guint len;
    guint i;
    GtkTreeIter iter;
    gchar *text;

    if (!ETCore->ETFileDisplayedList || !TrackEntryComboModel) return;

    // Number mini of items
    //if ((len=ETCore->ETFileDisplayedList_Length) < 30)
    // Length limited to 30 (instead to the number of files)!
    len = 30;

    // Create list of tracks
    for (i=1; i<=len; i++)
    {

        if (NUMBER_TRACK_FORMATED)
        {
            text = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,i);
        } else
        {
            text = g_strdup_printf("%.2d",i);
        }

        gtk_list_store_append(GTK_LIST_STORE(TrackEntryComboModel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(TrackEntryComboModel), &iter, MISC_COMBO_TEXT, text, -1);
        g_free(text);
    }

}

/*
 * Change mouse cursor
 */
void Init_Mouse_Cursor (void)
{
    MouseCursor = NULL;
}

void Destroy_Mouse_Cursor (void)
{
    if (MouseCursor)
    {
        gdk_cursor_unref(MouseCursor);
        MouseCursor = NULL;
    }
}

void Set_Busy_Cursor (void)
{
    /* If still built, destroy it to avoid memory leak */
    Destroy_Mouse_Cursor();
    /* Create the new cursor */
    MouseCursor = gdk_cursor_new(GDK_WATCH);
    gdk_window_set_cursor(gtk_widget_get_window(MainWindow),MouseCursor);
}

void Set_Unbusy_Cursor (void)
{
    /* Back to standard cursor */
    gdk_window_set_cursor(gtk_widget_get_window(MainWindow),NULL);
    Destroy_Mouse_Cursor();
}



/*
 * Add easytag specific icons to GTK stock set
 */
#include "../pixmaps/add_folder.xpm"
#include "../pixmaps/album.xpm"
#include "../pixmaps/all_uppercase.xpm"
#include "../pixmaps/all_downcase.xpm"
#include "../pixmaps/artist.xpm"
#include "../pixmaps/artist_album.xpm"
//#include "../pixmaps/blackwhite.xpm"
#include "../pixmaps/first_letter_uppercase.xpm"
#include "../pixmaps/first_letter_uppercase_word.xpm"
#include "../pixmaps/forbidden.xpm"
#include "../pixmaps/grab.xpm"
#include "../pixmaps/invert_selection.xpm"
#include "../pixmaps/mask.xpm"
#include "../pixmaps/parent_folder.xpm"
#include "../pixmaps/read_only.xpm"
#include "../pixmaps/red_lines.xpm"
//#include "../pixmaps/sequence_track.xpm"
#include "../pixmaps/sound.xpm"
#include "../pixmaps/unselect_all.xpm"
void Init_Custom_Icons (void)
{
    Create_Xpm_Icon_Factory((const char**)artist_xpm,               "easytag-artist");
    Create_Xpm_Icon_Factory((const char**)album_xpm,                "easytag-album");
    Create_Xpm_Icon_Factory((const char**)invert_selection_xpm,     "easytag-invert-selection");
    Create_Xpm_Icon_Factory((const char**)unselect_all_xpm,         "easytag-unselect-all");
    Create_Xpm_Icon_Factory((const char**)grab_xpm,                 "easytag-grab");
    Create_Xpm_Icon_Factory((const char**)mask_xpm,                 "easytag-mask");
    //Create_Xpm_Icon_Factory((const char**)blackwhite_xpm,         "easytag-blackwhite");
    Create_Xpm_Icon_Factory((const char**)forbidden_xpm,            "easytag-forbidden");
    Create_Xpm_Icon_Factory((const char**)read_only_xpm,            "easytag-read-only");
    //Create_Xpm_Icon_Factory((const char**)sequence_track_xpm,     "easytag-sequence-track");
    Create_Xpm_Icon_Factory((const char**)red_lines_xpm,            "easytag-red-lines");
    Create_Xpm_Icon_Factory((const char**)artist_album_xpm,     "easytag-artist-album");
////    Create_Png_Icon_Factory("artist_album.png",                     "easytag-artist-album");
    Create_Xpm_Icon_Factory((const char**)parent_folder_xpm,        "easytag-parent-folder");
    Create_Xpm_Icon_Factory((const char**)add_folder_xpm,           "easytag-add-folder");
    Create_Xpm_Icon_Factory((const char**)sound_xpm,                "easytag-sound");
    Create_Xpm_Icon_Factory((const char**)all_uppercase_xpm,        "easytag-all-uppercase");
    Create_Xpm_Icon_Factory((const char**)all_downcase_xpm,         "easytag-all-downcase");
    Create_Xpm_Icon_Factory((const char**)first_letter_uppercase_xpm,       "easytag-first-letter-uppercase");
    Create_Xpm_Icon_Factory((const char**)first_letter_uppercase_word_xpm,  "easytag-first-letter-uppercase-word");
}


/*
 * Create an icon factory from the specified pixmap
 * Also add it to the GTK stock images
 */
void Create_Xpm_Icon_Factory (const char **xpm_data, const char *name_in_factory)
{
    GtkIconSet      *icon;
    GtkIconFactory  *factory;
    GdkPixbuf       *pixbuf;

    if (!*xpm_data || !name_in_factory)
        return;

    pixbuf = gdk_pixbuf_new_from_xpm_data(xpm_data);

    if (pixbuf)
    {
        icon = gtk_icon_set_new_from_pixbuf(pixbuf);
        g_object_unref(G_OBJECT(pixbuf));

        factory = gtk_icon_factory_new();
        gtk_icon_factory_add(factory, name_in_factory, icon);
        gtk_icon_set_unref(icon);
        gtk_icon_factory_add_default(factory);
    }
}

/*
 * Create an icon factory from the specified png file
 * Also add it to the GTK stock images
 */
/*
void Create_Png_Icon_Factory (const char *png_file, const char *name_in_factory)
{
    GdkPixbuf       *pixbuf;
    GtkIconSet      *icon;
    GtkIconFactory  *factory;
    gchar           *path;
    GError          *error = NULL;
    
    if (!*png_file || !name_in_factory)
        return;

    path = g_strconcat(PACKAGE_DATA_DIR,"/",png_file,NULL);
    pixbuf = gdk_pixbuf_new_from_file(path,&error);
    g_free(path);
    
    if (pixbuf)
    {
        icon = gtk_icon_set_new_from_pixbuf(pixbuf);
        g_object_unref(G_OBJECT(pixbuf));

        factory = gtk_icon_factory_new();
        gtk_icon_factory_add(factory, name_in_factory, icon);
        gtk_icon_set_unref(icon);
        gtk_icon_factory_add_default(factory);
    }else
    {
        Log_Print(LOG_ERROR,error->message);
        g_error_free(error);
    }
}
*/

/*
 * Return a widget with a pixmap
 * Note: for pixmap 'pixmap.xpm', pixmap_name is 'pixmap_xpm'
 */
GtkWidget *Create_Xpm_Image (const char **xpm_name)
{
    GdkPixbuf *pixbuf;
    GtkWidget *image;

    if (!*xpm_name)
        return NULL;

    pixbuf = gdk_pixbuf_new_from_xpm_data(xpm_name);
    image = gtk_image_new_from_pixbuf(GDK_PIXBUF(pixbuf));

    return image;
}



/*
 * Iter compare func: Sort alphabetically
 */
gint Combo_Alphabetic_Sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
    gchar *text1, *text1_folded;
    gchar *text2, *text2_folded;
    gint ret;

    gtk_tree_model_get(model, a, MISC_COMBO_TEXT, &text1, -1);
    gtk_tree_model_get(model, b, MISC_COMBO_TEXT, &text2, -1);

    if (text1 == text2)
    {
        g_free(text1);
        g_free(text2);
        return 0;
    }

    if (text1 == NULL)
    {
        g_free(text2);
        return -1;
    }

    if (text2 == NULL)
    {
        g_free(text1);
        return 1;
    }

    text1_folded = g_utf8_casefold(text1, -1);
    text2_folded = g_utf8_casefold(text2, -1);
    ret = g_utf8_collate(text1_folded, text2_folded);

    g_free(text1);
    g_free(text2);
    g_free(text1_folded);
    g_free(text2_folded);
    return ret;
}


/*************************
 * File selection window *
 *************************/
void File_Selection_Window_For_File (GtkWidget *entry)
{
    Open_File_Selection_Window(entry, _("Select file…"), GTK_FILE_CHOOSER_ACTION_OPEN);
}

void File_Selection_Window_For_Directory (GtkWidget *entry)
{
    Open_File_Selection_Window(entry, _("Select directory…"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
}

/*
 * Open the file selection window and saves the selected file path into entry
 */
static void Open_File_Selection_Window (GtkWidget *entry, gchar *title, GtkFileChooserAction action)
{
    const gchar *tmp;
    gchar *filename, *filename_utf8;
    GtkWidget *FileSelectionWindow;
    GtkWindow *parent_window = NULL;
    gint response;

    parent_window = (GtkWindow*) gtk_widget_get_toplevel(entry);
    if (!gtk_widget_is_toplevel(GTK_WIDGET(parent_window)))
    {
        g_warning("Could not get parent window\n");
        return;
    }

    FileSelectionWindow = gtk_file_chooser_dialog_new(title, parent_window, action,
                                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                      GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,
                                                      NULL);
    // Set initial directory
    tmp = gtk_entry_get_text(GTK_ENTRY(entry));
    if (tmp && *tmp)
    {
        if (!gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(FileSelectionWindow),tmp))
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(FileSelectionWindow),tmp);
    }

    response = gtk_dialog_run(GTK_DIALOG(FileSelectionWindow));
    if (response == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileSelectionWindow));
        filename_utf8 = filename_to_display(filename);
        gtk_entry_set_text(GTK_ENTRY(entry),filename_utf8);
		g_free(filename);
        g_free(filename_utf8);
		// Gives the focus to the entry (useful for the button on the main window)
		gtk_widget_grab_focus(GTK_WIDGET(entry));
    }
	
    gtk_widget_destroy(FileSelectionWindow);
}



/*
 * Run the audio player and load files of the current dir
 */
void Run_Audio_Player_Using_File_List (GList *etfilelist_init)
{
    gchar  **argv;
    gint     argv_index = 0;
    GList   *etfilelist;
    ET_File *etfile;
    gchar   *filename;
    gchar   *program_path;
#ifdef WIN32
    gchar              *argv_join;
    STARTUPINFO         siStartupInfo;
    PROCESS_INFORMATION piProcessInfo;
#else
    pid_t   pid;
    gchar **argv_user;
    gint    argv_user_number;
#endif

    // Exit if no program selected...
    if (!AUDIO_FILE_PLAYER || strlen(g_strstrip(AUDIO_FILE_PLAYER))<1)
    {
        GtkWidget *msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                      GTK_MESSAGE_WARNING,
                                                      GTK_BUTTONS_CLOSE,
                                                      "%s",
                                                      _("No audio player defined"));
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Audio Player Warning"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        
        return;
    }

    if ( !(program_path = Check_If_Executable_Exists(AUDIO_FILE_PLAYER)) )
    {
        gchar *msg = g_strdup_printf(_("The program '%s' cannot be found"),AUDIO_FILE_PLAYER);
        Log_Print(LOG_ERROR,msg);
        g_free(msg);
        return;
    }
    g_free(program_path);

    // The list of files to play
    etfilelist = etfilelist_init;

#ifdef WIN32

    // See documentation : http://c.developpez.com/faq/vc/?page=ProcessThread and http://www.answers.com/topic/createprocess
    ZeroMemory(&siStartupInfo, sizeof(siStartupInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);
    ZeroMemory(&piProcessInfo, sizeof(piProcessInfo));

    argv = g_new0(gchar *,g_list_length(etfilelist) + 2); // "+2" for 1rst arg 'foo' and last arg 'NULL'
    argv[argv_index++] = "foo";

    // Load files as arguments
    while (etfilelist)
    {
        etfile   = (ET_File *)etfilelist->data;
        filename = ((File_Name *)etfile->FileNameCur->data)->value;
        //filename_utf8 = ((File_Name *)etfile->FileNameCur->data)->value_utf8;
        // We must enclose filename between quotes, because of possible (probable!) spaces in filenames"
        argv[argv_index++] = g_strconcat("\"", filename, "\"", NULL);
        etfilelist = etfilelist->next;
    }
    argv[argv_index] = NULL; // Ends the list of arguments

    // Make a command line with all arguments (joins strings together to form one long string separated by a space)
    argv_join = g_strjoinv(" ", argv);

    if (CreateProcess(AUDIO_FILE_PLAYER,
                      argv_join,
                      NULL,
                      NULL,
                      FALSE,
                      CREATE_DEFAULT_ERROR_MODE,
                      NULL,
                      NULL,
                      &siStartupInfo,
                      &piProcessInfo) == FALSE)
    {
        Log_Print(LOG_ERROR,_("Cannot execute %s (error %d)\n"), AUDIO_FILE_PLAYER, GetLastError());
    }

    // Free allocated parameters (for each filename)
    for (argv_index = 1; argv[argv_index]; argv_index++)
        g_free(argv[argv_index]);

    g_free(argv_join);

#else

    argv_user = g_strsplit(AUDIO_FILE_PLAYER," ",0); // the string may contains arguments, space is the delimiter
    // Number of arguments into 'argv_user'
    for (argv_user_number=0;argv_user[argv_user_number];argv_user_number++);

    argv = g_new0(gchar *,argv_user_number + g_list_length(etfilelist) + 1); // +1 for NULL

    // Load 'user' arguments (program name and more...)
    while (argv_user[argv_index])
    {
        argv[argv_index] = argv_user[argv_index];
        argv_index++;
    }

    // Load files as arguments
    while (etfilelist)
    {
        etfile   = (ET_File *)etfilelist->data;
        filename = ((File_Name *)etfile->FileNameCur->data)->value;
        //filename_utf8 = ((File_Name *)etfile->FileNameCur->data)->value_utf8;
        argv[argv_index++] = filename;
        etfilelist = etfilelist->next;
    }
    argv[argv_index] = NULL; // Ends the list of arguments

    pid = fork();
    switch (pid)
    {
        case -1:
            Log_Print(LOG_ERROR,_("Cannot fork another process"));
            break;
        case 0:
        {
            if (execvp(argv[0],argv) == -1)
            {
                Log_Print(LOG_ERROR,_("Cannot execute %s (%s)"),argv[0],g_strerror(errno));
            }
            g_strfreev(argv_user);
            _exit(1);
            break;
        }
        default:
            break;
    }

#endif

    g_free(argv);
}

void Run_Audio_Player_Using_Directory (void)
{
    GList *etfilelist = g_list_first(ETCore->ETFileList);

    Run_Audio_Player_Using_File_List(etfilelist);
}

void Run_Audio_Player_Using_Selection (void)
{
    GList *etfilelist = NULL;
    GList *selfilelist = NULL;
    ET_File *etfile;
    GtkTreeSelection *selection;

    if (!BrowserList) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);
    while (selfilelist)
    {
        etfile = Browser_List_Get_ETFile_From_Path(selfilelist->data);
        etfilelist = g_list_append(etfilelist, etfile);

        if (!selfilelist->next) break;
        selfilelist = selfilelist->next;
    }

    g_list_foreach(selfilelist, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selfilelist);

    Run_Audio_Player_Using_File_List(etfilelist);

    g_list_free(etfilelist);
}

void Run_Audio_Player_Using_Browser_Artist_List (void)
{
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreeModel *artistListModel;
    GList *AlbumList, *etfilelist;
    GList *concatenated_list = NULL;

    if (!BrowserArtistList) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserArtistList));
    if (!gtk_tree_selection_get_selected(selection, &artistListModel, &iter))
        return;

    gtk_tree_model_get(artistListModel, &iter,
                       ARTIST_ALBUM_LIST_POINTER, &AlbumList,
                       -1);

    while (AlbumList)
    {
        etfilelist = g_list_copy((GList *)AlbumList->data);
        if (!concatenated_list)
            concatenated_list = etfilelist;
        else
            concatenated_list = g_list_concat(concatenated_list,etfilelist);
        AlbumList = AlbumList->next;
    }

    Run_Audio_Player_Using_File_List(concatenated_list);

    if (concatenated_list)
        g_list_free(concatenated_list);
}

void Run_Audio_Player_Using_Browser_Album_List (void)
{
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreeModel *albumListModel;
    GList *etfilelist;

    if (!BrowserAlbumList) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserAlbumList));
    if (!gtk_tree_selection_get_selected(selection, &albumListModel, &iter))
        return;

    gtk_tree_model_get(albumListModel, &iter,
                       ALBUM_ETFILE_LIST_POINTER, &etfilelist,
                       -1);

    Run_Audio_Player_Using_File_List(etfilelist);
}


/*
 * Check if the executable passed in parameter can be launched
 * Returns the full path of the file (must be freed if not used)
 */
gchar *Check_If_Executable_Exists (const gchar *program)
{
    gchar *program_tmp;
    gchar *tmp;

    if (!program)
        return NULL;

    program_tmp = g_strdup(program);
    g_strstrip(program_tmp);

#ifdef WIN32
    // Remove arguments if found, after '.exe'
    if ( (tmp=strstr(program_tmp,".exe")) )
        *(tmp + 4) = 0;
#else
    // Remove arguments if found
    if ( (tmp=strchr(program_tmp,' ')) )
        *tmp = 0;
#endif

    if (g_path_is_absolute(program_tmp))
    {
        if (access(program_tmp, X_OK) == 0)
        {
            return program_tmp;
        } else
        {
            g_free(program_tmp);
            return NULL;
        }
    } else
    {
        tmp = g_find_program_in_path(program_tmp);
        if (tmp)
        {
            g_free(program_tmp);
            return tmp;
        }else
        {
            g_free(program_tmp);
            return NULL;
        }
    }

}



/*
 * The returned string must be freed after used
 */
gchar *Convert_Size (gfloat size)
{
    gchar *data = NULL;
    /* Units Tab of file size (bytes,kilobytes,...) */
    gchar *Units_Tab[] = { N_("B"), N_("KB"), N_("MB"), N_("GB"), N_("TB")};
    gint i = 0;

    while ( (gint)size/1024 && i<(gint)(sizeof(Units_Tab)/sizeof(Units_Tab[0])-1) )
    {
        size = size/1024;
        i++;
    }
    return data = g_strdup_printf("%.1f %s",size,_(Units_Tab[i]));
}

/*
 * Same that before except that if value in MB, we display 3 numbers after the coma
 * The returned string must be freed after used
 */
gchar *Convert_Size_1 (gfloat size)
{
    gchar *data = NULL;
    /* Units Tab of file size (bytes,kilobytes,...) */
    gchar *Units_Tab[] = { N_("B"), N_("KB"), N_("MB"), N_("GB"), N_("TB")};
    guint i = 0;

    while ( (gint)size/1024 && i<(sizeof(Units_Tab)/sizeof(Units_Tab[0])-1) )
    {
        size = size/1024;
        i++;
    }
    if (i >= 2) // For big values : display 3 number afer the separator (coma or point)
        return data = g_strdup_printf("%.3f %s",size,_(Units_Tab[i]));
    else
        return data = g_strdup_printf("%.1f %s",size,_(Units_Tab[i]));
}

/*
 * Convert a series of seconds into a readable duration
 * Remember to free the string that is returned
 */
gchar *Convert_Duration (gulong duration)
{
    guint hour=0;
    guint minute=0;
    guint second=0;
    gchar *data = NULL;

    if (duration<=0)
        return g_strdup_printf("%d:%.2d",minute,second);

    hour   = duration/3600;
    minute = (duration%3600)/60;
    second = (duration%3600)%60;

    if (hour)
        data = g_strdup_printf("%d:%.2d:%.2d",hour,minute,second);
    else
        data = g_strdup_printf("%d:%.2d",minute,second);

    return data;
}

/*
 * Returns the size of a file in bytes
 */
gulong Get_File_Size(gchar *filename)
{
    struct stat statbuf;

    if (filename)
    {
        stat(filename,&statbuf);
        return statbuf.st_size;
    }else
    {
        return 0;
    }
}

/*
 * Delete spaces at the end and the beginning of the string
 */
void Strip_String (gchar *string)
{
    if (!string) return;
    string = g_strstrip(string);
}



/*******************************
 * Writting playlist functions *
 *******************************/
/*
 * The window to write playlists.
 */
void Open_Write_Playlist_Window (void)
{
    GtkWidget *Frame;
    GtkWidget *VBox;
    GtkWidget *vbox, *hbox;
    GtkWidget *ButtonBox;
    GtkWidget *Button;
    GtkWidget *Separator;
    GtkWidget *Icon;
    GtkWidget *MaskStatusIconBox, *MaskStatusIconBox1;

    if (WritePlaylistWindow != NULL)
    {
        gtk_window_present(GTK_WINDOW(WritePlaylistWindow));
        return;
    }

    WritePlaylistWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(WritePlaylistWindow),_("Generate a playlist"));
    gtk_window_set_transient_for(GTK_WINDOW(WritePlaylistWindow),GTK_WINDOW(MainWindow));
    g_signal_connect(G_OBJECT(WritePlaylistWindow),"destroy", G_CALLBACK(Destroy_Write_Playlist_Window),NULL);
    g_signal_connect(G_OBJECT(WritePlaylistWindow),"delete_event", G_CALLBACK(Destroy_Write_Playlist_Window),NULL);
    g_signal_connect(G_OBJECT(WritePlaylistWindow),"key_press_event", G_CALLBACK(Write_Playlist_Window_Key_Press),NULL);

    // Just center on mainwindow
    gtk_window_set_position(GTK_WINDOW(WritePlaylistWindow), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_default_size(GTK_WINDOW(WritePlaylistWindow),PLAYLIST_WINDOW_WIDTH,PLAYLIST_WINDOW_HEIGHT);

    Frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(WritePlaylistWindow),Frame);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),4);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 4);

    /* Playlist name */
    if (!PlayListNameMaskModel)
        PlayListNameMaskModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);
    else
        gtk_list_store_clear(PlayListNameMaskModel);

    Frame = gtk_frame_new(_("M3U Playlist Name"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,TRUE,TRUE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

    playlist_use_mask_name = gtk_radio_button_new_with_label(NULL, _("Use mask:"));
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),playlist_use_mask_name,FALSE,FALSE,0);
    PlayListNameMaskCombo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(PlayListNameMaskModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(PlayListNameMaskCombo),MISC_COMBO_TEXT);
    gtk_box_pack_start(GTK_BOX(hbox),PlayListNameMaskCombo,FALSE,FALSE,4);
    playlist_use_dir_name = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(playlist_use_mask_name),_("Use directory name"));
    gtk_box_pack_start(GTK_BOX(vbox),playlist_use_dir_name,FALSE,FALSE,0);
    // History list
    Load_Play_List_Name_List(PlayListNameMaskModel, MISC_COMBO_TEXT);
    Add_String_To_Combo_List(PlayListNameMaskModel, PLAYLIST_NAME);
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(PlayListNameMaskCombo))), PLAYLIST_NAME);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_use_mask_name),PLAYLIST_USE_MASK_NAME);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_use_dir_name),PLAYLIST_USE_DIR_NAME);

    // Mask status icon
    MaskStatusIconBox = Create_Pixmap_Icon_With_Event_Box("easytag-forbidden");
    gtk_box_pack_start(GTK_BOX(hbox),MaskStatusIconBox,FALSE,FALSE,0);
    gtk_widget_set_tooltip_text(MaskStatusIconBox,_("Invalid Scanner Mask"));
    // Signal connection to check if mask is correct into the mask entry
    g_signal_connect_swapped(G_OBJECT(gtk_bin_get_child(GTK_BIN(PlayListNameMaskCombo))),"changed",
        G_CALLBACK(Playlist_Check_Content_Mask),G_OBJECT(MaskStatusIconBox));

    // Button for Mask editor
    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock("easytag-mask", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Edit Masks"));
    // The masks will be edited into a tab of the preferences window. In the future...
    //g_signal_connect(G_OBJECT(Button),"clicked",(GtkSignalFunc)???,NULL);
    // FIX ME : edit the masks
    gtk_widget_set_sensitive(GTK_WIDGET(Button),FALSE);


    /* Playlist options */
    Frame = gtk_frame_new(_("Playlist Options"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,TRUE,TRUE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

    playlist_only_selected_files = gtk_check_button_new_with_label(_("Include only the selected files"));
    gtk_box_pack_start(GTK_BOX(vbox),playlist_only_selected_files,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_only_selected_files),PLAYLIST_ONLY_SELECTED_FILES);
    gtk_widget_set_tooltip_text(playlist_only_selected_files,_("If activated, only the selected files will be "
        "written in the playlist file. Else, all the files will be written."));

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox),Separator,FALSE,FALSE,0);

    playlist_full_path = gtk_radio_button_new_with_label(NULL,_("Use full path for files in playlist"));
    gtk_box_pack_start(GTK_BOX(vbox),playlist_full_path,FALSE,FALSE,0);
    playlist_relative_path = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(playlist_full_path),
        _("Use relative path for files in playlist"));
    gtk_box_pack_start(GTK_BOX(vbox),playlist_relative_path,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_full_path),PLAYLIST_FULL_PATH);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_relative_path),PLAYLIST_RELATIVE_PATH);

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox),Separator,FALSE,FALSE,0);

    // Create playlist in parent directory
    playlist_create_in_parent_dir = gtk_check_button_new_with_label(_("Create playlist in the parent directory"));
    gtk_box_pack_start(GTK_BOX(vbox),playlist_create_in_parent_dir,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_create_in_parent_dir),PLAYLIST_CREATE_IN_PARENT_DIR);
    gtk_widget_set_tooltip_text(playlist_create_in_parent_dir,_("If activated, the playlist will be created "
        "in the parent directory."));

    // DOS Separator
    playlist_use_dos_separator = gtk_check_button_new_with_label(_("Use DOS directory separator"));
#ifndef WIN32 // This has no sense under Win32, so we don't display it
    gtk_box_pack_start(GTK_BOX(vbox),playlist_use_dos_separator,FALSE,FALSE,0);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_use_dos_separator),PLAYLIST_USE_DOS_SEPARATOR);
    gtk_widget_set_tooltip_text(playlist_use_dos_separator,_("This option replaces the UNIX directory "
        "separator '/' into DOS separator '\\'."));

    /* Playlist content */
    if (!PlayListContentMaskModel)
        PlayListContentMaskModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);
    else
        gtk_list_store_clear(PlayListContentMaskModel);

    Frame = gtk_frame_new(_("Playlist Content"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,TRUE,TRUE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

    playlist_content_none = gtk_radio_button_new_with_label(NULL,_("Write only list of files"));
    gtk_box_pack_start(GTK_BOX(vbox),playlist_content_none,FALSE,FALSE,0);

    playlist_content_filename = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(playlist_content_none),_("Write info using filename"));
    gtk_box_pack_start(GTK_BOX(vbox),playlist_content_filename,FALSE,FALSE,0);

    playlist_content_mask = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(playlist_content_none), _("Write info using:"));
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),playlist_content_mask,FALSE,FALSE,0);
    // Set a label, a combobox and un editor button in the 3rd radio button
    PlayListContentMaskCombo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(PlayListContentMaskModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(PlayListContentMaskCombo),MISC_COMBO_TEXT);
    gtk_box_pack_start(GTK_BOX(hbox),PlayListContentMaskCombo,FALSE,FALSE,0);
    // History list
    Load_Playlist_Content_Mask_List(PlayListContentMaskModel, MISC_COMBO_TEXT);
    Add_String_To_Combo_List(PlayListContentMaskModel, PLAYLIST_CONTENT_MASK_VALUE);
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(PlayListContentMaskCombo))), PLAYLIST_CONTENT_MASK_VALUE);

    // Mask status icon
    MaskStatusIconBox1 = Create_Pixmap_Icon_With_Event_Box("easytag-forbidden");
    gtk_box_pack_start(GTK_BOX(hbox),MaskStatusIconBox1,FALSE,FALSE,0);
    gtk_widget_set_tooltip_text(MaskStatusIconBox1,_("Invalid Scanner Mask"));
    // Signal connection to check if mask is correct into the mask entry
    g_signal_connect_swapped(G_OBJECT(gtk_bin_get_child(GTK_BIN(PlayListContentMaskCombo))),"changed",
        G_CALLBACK(Playlist_Check_Content_Mask),G_OBJECT(MaskStatusIconBox1));

    // Button for Mask editor
    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock("easytag-mask", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Edit Masks"));
    // The masks will be edited into a tab of the preferences window. In the future...
    //g_signal_connect(G_OBJECT(Button),"clicked",(GtkSignalFunc)???,NULL);
    // FIX ME : edit the masks
    gtk_widget_set_sensitive(GTK_WIDGET(Button),FALSE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_content_none),    PLAYLIST_CONTENT_NONE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_content_filename),PLAYLIST_CONTENT_FILENAME);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_content_mask),    PLAYLIST_CONTENT_MASK);


    /* Separator line */
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    ButtonBox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),ButtonBox,FALSE,FALSE,0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(ButtonBox),GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(ButtonBox), 10);

    /* Button to Cancel */
    Button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    gtk_widget_grab_default(Button);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(Destroy_Write_Playlist_Window),NULL);

    /* Button to Write the playlist */
    Button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",G_CALLBACK(Playlist_Write_Button_Pressed),NULL);

    gtk_widget_show_all(WritePlaylistWindow);
    if (PLAYLIST_WINDOW_X > 0 && PLAYLIST_WINDOW_Y > 0)
        gtk_window_move(GTK_WINDOW(WritePlaylistWindow),PLAYLIST_WINDOW_X,PLAYLIST_WINDOW_Y);

    /* To initialize the mask status icon and visibility */
    g_signal_emit_by_name(G_OBJECT(gtk_bin_get_child(GTK_BIN(PlayListNameMaskCombo))),"changed");
    g_signal_emit_by_name(G_OBJECT(gtk_bin_get_child(GTK_BIN(PlayListContentMaskCombo))),"changed");
}

void Destroy_Write_Playlist_Window (void)
{
    if (WritePlaylistWindow)
    {
        Write_Playlist_Window_Apply_Changes();

        gtk_widget_destroy(WritePlaylistWindow);
        WritePlaylistWindow = NULL;
    }
}

/*
 * For the configuration file...
 */
void Write_Playlist_Window_Apply_Changes (void)
{
    if (WritePlaylistWindow)
    {
        gint x, y, width, height;
        GdkWindow *window;

        window = gtk_widget_get_window (WritePlaylistWindow);

        if ( window && gdk_window_is_visible(window) && gdk_window_get_state(window)!=GDK_WINDOW_STATE_MAXIMIZED )
        {
            // Position and Origin of the window
            gdk_window_get_root_origin(window,&x,&y);
            PLAYLIST_WINDOW_X = x;
            PLAYLIST_WINDOW_Y = y;
            width = gdk_window_get_width(window);
            height = gdk_window_get_height(window);
            PLAYLIST_WINDOW_WIDTH  = width;
            PLAYLIST_WINDOW_HEIGHT = height;
        }

        /* List of variables also set in the function 'Playlist_Write_Button_Pressed' */
        if (PLAYLIST_NAME) g_free(PLAYLIST_NAME);
        PLAYLIST_NAME                 = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(PlayListNameMaskCombo)))));
        PLAYLIST_USE_MASK_NAME        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_use_mask_name));
        PLAYLIST_USE_DIR_NAME         = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_use_dir_name));

        PLAYLIST_ONLY_SELECTED_FILES  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_only_selected_files));
        PLAYLIST_FULL_PATH            = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_full_path));
        PLAYLIST_RELATIVE_PATH        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_relative_path));
        PLAYLIST_CREATE_IN_PARENT_DIR = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_create_in_parent_dir));
        PLAYLIST_USE_DOS_SEPARATOR    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_use_dos_separator));

        PLAYLIST_CONTENT_NONE         = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_content_none));
        PLAYLIST_CONTENT_FILENAME     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_content_filename));
        PLAYLIST_CONTENT_MASK         = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_content_mask));
        
        if (PLAYLIST_CONTENT_MASK_VALUE) g_free(PLAYLIST_CONTENT_MASK_VALUE);
        PLAYLIST_CONTENT_MASK_VALUE   = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(PlayListContentMaskCombo)))));

        /* Save combobox history lists before exit */
        Save_Play_List_Name_List(PlayListNameMaskModel, MISC_COMBO_TEXT);
        Save_Playlist_Content_Mask_List(PlayListContentMaskModel, MISC_COMBO_TEXT);
    }
}

gboolean Write_Playlist_Window_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_KEY_Escape:
                Destroy_Write_Playlist_Window();
                break;
        }
    }
    return FALSE;
}

void Playlist_Write_Button_Pressed (void)
{
    gchar *playlist_name = NULL;
    gchar *playlist_path_utf8;      // Path
    gchar *playlist_basename_utf8;  // Filename
    gchar *playlist_name_utf8;      // Path + filename
    gchar *temp;
    FILE  *file;
    GtkWidget *msgdialog;
    gint response = 0;


    // Check if playlist name was filled
    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_use_mask_name))
    &&   g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(PlayListNameMaskCombo)))), -1)<=0 )
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(playlist_use_dir_name),TRUE);

    /* List of variables also set in the function 'Write_Playlist_Window_Apply_Changes' */
    /***if (PLAYLIST_NAME) g_free(PLAYLIST_NAME);
    PLAYLIST_NAME                 = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(PlayListNameMaskCombo)->child)));
    PLAYLIST_USE_MASK_NAME        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_use_mask_name));
    PLAYLIST_USE_DIR_NAME         = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_use_dir_name));

    PLAYLIST_ONLY_SELECTED_FILES  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_only_selected_files));
    PLAYLIST_FULL_PATH            = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_full_path));
    PLAYLIST_RELATIVE_PATH        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_relative_path));
    PLAYLIST_CREATE_IN_PARENT_DIR = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_create_in_parent_dir));
    PLAYLIST_USE_DOS_SEPARATOR    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_use_dos_separator));

    PLAYLIST_CONTENT_NONE         = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_content_none));
    PLAYLIST_CONTENT_FILENAME     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_content_filename));
    PLAYLIST_CONTENT_MASK         = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(playlist_content_mask));
    if (PLAYLIST_CONTENT_MASK_VALUE) g_free(PLAYLIST_CONTENT_MASK_VALUE);
    PLAYLIST_CONTENT_MASK_VALUE   = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(PlayListContentMaskCombo)->child)));***/
    Write_Playlist_Window_Apply_Changes();

    // Path of the playlist file (may be truncated later if PLAYLIST_CREATE_IN_PARENT_DIR is TRUE)
    playlist_path_utf8 = filename_to_display(Browser_Get_Current_Path());

    // Build the playlist file name
    if (PLAYLIST_USE_MASK_NAME)
    {

        if (!ETCore->ETFileList)
            return;

        Add_String_To_Combo_List(PlayListNameMaskModel, PLAYLIST_NAME);

        // Generate filename from tag of the current selected file (hummm FIX ME)
        temp = filename_from_display(PLAYLIST_NAME);
        playlist_basename_utf8 = Scan_Generate_New_Filename_From_Mask(ETCore->ETFileDisplayed,temp,FALSE);
        g_free(temp);

        // Replace Characters (with scanner)
        if (RFS_CONVERT_UNDERSCORE_AND_P20_INTO_SPACE)
        {
            Scan_Convert_Underscore_Into_Space(playlist_basename_utf8);
            Scan_Convert_P20_Into_Space(playlist_basename_utf8);
        }
        if (RFS_CONVERT_SPACE_INTO_UNDERSCORE)
        {
            Scan_Convert_Space_Into_Undescore(playlist_basename_utf8);
        }
        if (RFS_REMOVE_SPACES)
				 {
				    Scan_Remove_Spaces(playlist_basename_utf8);
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

    // Must be placed after "Build the playlist file name", as we can truncate the path!
    if (PLAYLIST_CREATE_IN_PARENT_DIR)
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
    playlist_basename_utf8 = g_path_get_basename(playlist_name_utf8);

    // Check if file exists
    if (CONFIRM_WRITE_PLAYLIST)
    {
        if ( (file=fopen(playlist_name,"r")) != NULL )
        {
            fclose(file);
            msgdialog = gtk_message_dialog_new(GTK_WINDOW(WritePlaylistWindow),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_NONE,
                                               _("Playlist file '%s' already exists"),
                                               playlist_basename_utf8);
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),"%s",_("Do you want to save the playlist, overwriting the existing file?"));
            gtk_dialog_add_buttons(GTK_DIALOG(msgdialog),GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_SAVE,GTK_RESPONSE_YES,NULL);
            gtk_window_set_title(GTK_WINDOW(msgdialog),_("Write Playlist"));

            response = gtk_dialog_run(GTK_DIALOG(msgdialog));
            gtk_widget_destroy(msgdialog);
        }
    }

    // Writing playlist if ok
    if (response == 0
    ||  response == GTK_RESPONSE_YES )
    {
        if ( Write_Playlist(playlist_name) == FALSE )
        {
            // Writing fails...
            msgdialog = gtk_message_dialog_new(GTK_WINDOW(WritePlaylistWindow),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_CLOSE,
                                               _("Cannot write playlist file '%s'"),
                                               playlist_name_utf8);
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),"%s",g_strerror(errno));
            gtk_window_set_title(GTK_WINDOW(msgdialog),_("Playlist File Error"));

            gtk_dialog_run(GTK_DIALOG(msgdialog));
            gtk_widget_destroy(msgdialog);
        }else
        {
            gchar *msg;
            msg = g_strdup_printf(_("Written playlist file '%s'"),playlist_name_utf8);
            /*msgbox = msg_box_new(_("Information…"),
                                   GTK_WINDOW(WritePlaylistWindow),
                                   NULL,
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								   msg,
								   GTK_STOCK_DIALOG_INFO,
                                   GTK_STOCK_OK, GTK_RESPONSE_OK,
                                   NULL);
            gtk_dialog_run(GTK_DIALOG(msgbox));
            gtk_widget_destroy(msgbox);*/
            Statusbar_Message(msg,TRUE);
            g_free(msg);
        }
    }
    g_free(playlist_name_utf8);
    g_free(playlist_basename_utf8);
    g_free(playlist_name);
}

gboolean Playlist_Check_Content_Mask (GtkWidget *widget_to_show_hide, GtkEntry *widget_source)
{
    gchar *tmp  = NULL;
    gchar *mask = NULL;


    if (!widget_to_show_hide || !widget_source)
        goto Bad_Mask;

    mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget_source)));
    if (!mask || strlen(mask)<1)
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
        gtk_widget_show(GTK_WIDGET(widget_to_show_hide));
        return FALSE;

    Good_Mask:
        g_free(mask);
        gtk_widget_hide(GTK_WIDGET(widget_to_show_hide));
        return TRUE;
}

/*
 * Function to replace UNIX ForwardSlash with a DOS BackSlash
 */
void Playlist_Convert_Forwardslash_Into_Backslash (gchar *string)
{
    gchar *tmp;

    while ((tmp=strchr(string,'/'))!=NULL)
        *tmp = '\\';
}


/*
 * Write a playlist
 *  - 'playlist_name' in file system encoding (not UTF-8)
 */
gboolean Write_Playlist (gchar *playlist_name)
{
    FILE  *file;
    ET_File *etfile;
    GList *etfilelist = NULL;
    gchar *filename;
    gchar *playlist_name_utf8 = filename_to_display(playlist_name);
    gchar *basedir;
    gchar *temp;
    gint   duration;

    if ((file = fopen(playlist_name,"wb")) == NULL)
    {
        Log_Print(LOG_ERROR,_("Error while opening file: '%s' (%s)."),playlist_name_utf8,g_strerror(errno));
        g_free(playlist_name_utf8);
        return FALSE;
    }

    /* 'base directory' where is located the playlist. Used also to write file with a
     * relative path for file located in this directory and sub-directories
     */
    basedir = g_path_get_dirname(playlist_name);

    // 1) First line of the file (if playlist content is not set to "write only list of files")
    if (!PLAYLIST_CONTENT_NONE)
    {
        fprintf(file,"#EXTM3U\r\n");
    }

    if (PLAYLIST_ONLY_SELECTED_FILES)
    {
        GList *selfilelist = NULL;
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));

        selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);
        while (selfilelist)
        {
            etfile = Browser_List_Get_ETFile_From_Path(selfilelist->data);
            etfilelist = g_list_append(etfilelist, etfile);

            if (!selfilelist->next) break;
            selfilelist = selfilelist->next;
        }

        g_list_foreach(selfilelist, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selfilelist);
    }else
    {
        etfilelist = g_list_first(ETCore->ETFileList);
    }
    while (etfilelist)
    {
        etfile   = (ET_File *)etfilelist->data;
        filename = ((File_Name *)etfile->FileNameCur->data)->value;
        duration = ((ET_File_Info *)etfile->ETFileInfo)->duration;

        if (PLAYLIST_RELATIVE_PATH)
        {
            // Keep only files in this directory and sub-dirs
            if ( strncmp(filename,basedir,strlen(basedir))==0 )
            {
                // 2) Write the header
                if (PLAYLIST_CONTENT_NONE)
                {
                    // No header written
                }else if (PLAYLIST_CONTENT_FILENAME)
                {
                    // Header uses only filename
                    temp = g_path_get_basename(filename);
                    fprintf(file,"#EXTINF:%d,%s\r\n",duration,temp); // Must be written in system encoding (not UTF-8)
                    g_free(temp);
                }else if (PLAYLIST_CONTENT_MASK)
                {
                    // Header uses generated filename from a mask
                    gchar *mask = filename_from_display(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(PlayListContentMaskCombo)))));
                    // Special case : we don't replace illegal characters and don't check if there is a directory separator in the mask.
                    gchar *filename_generated_utf8 = Scan_Generate_New_Filename_From_Mask(etfile,mask,TRUE);
                    gchar *filename_generated = filename_from_display(filename_generated_utf8);
                    fprintf(file,"#EXTINF:%d,%s\r\n",duration,filename_generated); // Must be written in system encoding (not UTF-8)
                    g_free(mask);
                    g_free(filename_generated_utf8);
                }

                // 3) Write the file path
                if (PLAYLIST_USE_DOS_SEPARATOR)
                {
                    gchar *filename_conv = g_strdup(filename+strlen(basedir)+1);
                    Playlist_Convert_Forwardslash_Into_Backslash(filename_conv);
                    fprintf(file,"%s\r\n",filename_conv); // Must be written in system encoding (not UTF-8)
                    g_free(filename_conv);
                }else
                {
                    fprintf(file,"%s\r\n",filename+strlen(basedir)+1); // Must be written in system encoding (not UTF-8)
                }
            }
        }else // PLAYLIST_FULL_PATH
        {
            // 2) Write the header
            if (PLAYLIST_CONTENT_NONE)
            {
                // No header written
            }else if (PLAYLIST_CONTENT_FILENAME)
            {
                // Header uses only filename
                temp = g_path_get_basename(filename);
                fprintf(file,"#EXTINF:%d,%s\r\n",duration,temp); // Must be written in system encoding (not UTF-8)
                g_free(temp);
            }else if (PLAYLIST_CONTENT_MASK)
            {
                // Header uses generated filename from a mask
                gchar *mask = filename_from_display(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(PlayListContentMaskCombo)))));
                gchar *filename_generated_utf8 = Scan_Generate_New_Filename_From_Mask(etfile,mask,TRUE);
                gchar *filename_generated = filename_from_display(filename_generated_utf8);
                fprintf(file,"#EXTINF:%d,%s\r\n",duration,filename_generated); // Must be written in system encoding (not UTF-8)
                g_free(mask);
                g_free(filename_generated_utf8);
            }

            // 3) Write the file path
            if (PLAYLIST_USE_DOS_SEPARATOR)
            {
                gchar *filename_conv = g_strdup(filename);
                Playlist_Convert_Forwardslash_Into_Backslash(filename_conv);
                fprintf(file,"%s\r\n",filename_conv); // Must be written in system encoding (not UTF-8)
                g_free(filename_conv);
            }else
            {
                fprintf(file,"%s\r\n",filename); // Must be written in system encoding (not UTF-8)
            }
        }
        etfilelist = etfilelist->next;
    }
    fclose(file);

    if (PLAYLIST_ONLY_SELECTED_FILES)
        g_list_free(etfilelist);
    g_free(playlist_name_utf8);
    g_free(basedir);
    return TRUE;
}




/*****************************
 * Searching files functions *
 *****************************/
/*
 * The window to search keywords in the list of files.
 */
void Open_Search_File_Window (void)
{
    GtkWidget *VBox;
    GtkWidget *Frame;
    GtkWidget *Table;
    GtkWidget *Label;
    GtkWidget *Button;
    GtkWidget *Separator;
    GtkWidget *ScrollWindow;
    GtkTreeViewColumn* column;
    GtkCellRenderer* renderer;
    gchar *SearchResultList_Titles[] = { N_("File Name"),
                                         N_("Title"),
                                         N_("Artist"),
                                         N_("Album Artist"),
                                         N_("Album"),
                                         N_("CD"),
                                         N_("Year"),
                                         N_("Track"),
                                         N_("Genre"),
                                         N_("Comment"),
                                         N_("Composer"),
                                         N_("Original Artist"),
                                         N_("Copyright"),
                                         N_("URL"),
                                         N_("Encoded By")
                                       };


    if (SearchFileWindow != NULL)
    {
        gtk_window_present(GTK_WINDOW(SearchFileWindow));
        return;
    }

    SearchFileWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(SearchFileWindow),_("Search a file"));
    g_signal_connect(G_OBJECT(SearchFileWindow),"destroy", G_CALLBACK(Destroy_Search_File_Window),NULL);
    g_signal_connect(G_OBJECT(SearchFileWindow),"delete_event", G_CALLBACK(Destroy_Search_File_Window),NULL);
    g_signal_connect(G_OBJECT(SearchFileWindow),"key_press_event", G_CALLBACK(Search_File_Window_Key_Press),NULL);
    gtk_window_set_default_size(GTK_WINDOW(SearchFileWindow),SEARCH_WINDOW_WIDTH,SEARCH_WINDOW_HEIGHT);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(SearchFileWindow),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 1);

    Frame = gtk_frame_new(NULL);
    //gtk_container_add(GTK_CONTAINER(SearchFileWindow),Frame);
    gtk_box_pack_start(GTK_BOX(VBox),Frame,TRUE,TRUE,0);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),2);

    Table = gtk_table_new(3,6,FALSE);
    gtk_container_add(GTK_CONTAINER(Frame),Table);
    gtk_container_set_border_width(GTK_CONTAINER(Table), 2);
    gtk_table_set_row_spacings(GTK_TABLE(Table),4);
    gtk_table_set_col_spacings(GTK_TABLE(Table),4);

    // Words to search
    if (!SearchStringModel)
        SearchStringModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);
    else
        gtk_list_store_clear(SearchStringModel);

    Label = gtk_label_new(_("Search:"));
    gtk_misc_set_alignment(GTK_MISC(Label),1.0,0.5);
    gtk_table_attach(GTK_TABLE(Table),Label,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
    SearchStringCombo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(SearchStringModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(SearchStringCombo),MISC_COMBO_TEXT);
    gtk_widget_set_size_request(GTK_WIDGET(SearchStringCombo),200,-1);
    gtk_table_attach(GTK_TABLE(Table),SearchStringCombo,1,5,0,1,GTK_EXPAND|GTK_FILL,GTK_FILL,0,0);
    // History List
    Load_Search_File_List(SearchStringModel, MISC_COMBO_TEXT);
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(SearchStringCombo))),"");
    gtk_widget_set_tooltip_text(GTK_WIDGET(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(SearchStringCombo)))),
        _("Type the word to search into files. Or type nothing to display all files."));

    // Set content of the clipboard if available
    gtk_editable_paste_clipboard(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(SearchStringCombo))));

    // Where...
    Label = gtk_label_new(_("In:"));
    gtk_misc_set_alignment(GTK_MISC(Label),1.0,0.5);
    gtk_table_attach(GTK_TABLE(Table),Label,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
    /* Translators: This option is for the previous 'in' option. For instance,
     * translate this as "Search" "In:" "the Filename". */
    SearchInFilename = gtk_check_button_new_with_label(_("the Filename"));
    /* Translators: This option is for the previous 'in' option. For instance,
     * translate this as "Search" "In:" "the Tag".
     * Note: label changed to "the Tag" (to be the only one) to fix a Hungarian
     * grammatical problem (which uses one word to say "in the tag" like here)
     */
    SearchInTag = gtk_check_button_new_with_label(_("the Tag"));
    gtk_table_attach(GTK_TABLE(Table),SearchInFilename,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),SearchInTag,2,3,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SearchInFilename),SEARCH_IN_FILENAME);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SearchInTag),SEARCH_IN_TAG);

    Separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_table_attach(GTK_TABLE(Table),Separator,3,4,1,2,GTK_FILL,GTK_FILL,4,0);

    // Property of the search
    SearchCaseSensitive = gtk_check_button_new_with_label(_("Case sensitive"));
    gtk_table_attach(GTK_TABLE(Table),SearchCaseSensitive,4,5,1,2,GTK_EXPAND|GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SearchCaseSensitive),SEARCH_CASE_SENSITIVE);

    // Results list
    ScrollWindow = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindow),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(ScrollWindow), -1, 130);
    gtk_table_attach(GTK_TABLE(Table),ScrollWindow,0,6,2,3,GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,0);

    SearchResultListModel = gtk_list_store_new(SEARCH_COLUMN_COUNT,
                                               G_TYPE_STRING, /* Filename */
                                               G_TYPE_STRING, /* Title */
                                               G_TYPE_STRING, /* Artist */
                                               G_TYPE_STRING, /* Album Artist */
											   G_TYPE_STRING, /* Album */
                                               G_TYPE_STRING, /* Disc Number */
                                               G_TYPE_STRING, /* Year */
                                               G_TYPE_STRING, /* Track + Track Total */
                                               G_TYPE_STRING, /* Genre */
                                               G_TYPE_STRING, /* Comment */
                                               G_TYPE_STRING, /* Composer */
                                               G_TYPE_STRING, /* Orig. Artist */
                                               G_TYPE_STRING, /* Copyright */
                                               G_TYPE_STRING, /* URL */
                                               G_TYPE_STRING, /* Encoded by */

                                               G_TYPE_INT, /* Font Weight for Filename */
                                               G_TYPE_INT, /* Font Weight for Title */
                                               G_TYPE_INT, /* Font Weight for Artist */
                                               G_TYPE_INT, /* Font Weight for Album Artist */
											   G_TYPE_INT, /* Font Weight for Album */
                                               G_TYPE_INT, /* Font Weight for Disc Number */
                                               G_TYPE_INT, /* Font Weight for Year */
                                               G_TYPE_INT, /* Font Weight for Track + Track Total */
                                               G_TYPE_INT, /* Font Weight for Genre */
                                               G_TYPE_INT, /* Font Weight for Comment */
                                               G_TYPE_INT, /* Font Weight for Composer */
                                               G_TYPE_INT, /* Font Weight for Orig. Artist */
                                               G_TYPE_INT, /* Font Weight for Copyright */
                                               G_TYPE_INT, /* Font Weight for URL */
                                               G_TYPE_INT, /* Font Weight for Encoded by */

                                               GDK_TYPE_COLOR, /* Color Weight for Filename */
                                               GDK_TYPE_COLOR, /* Color Weight for Title */
                                               GDK_TYPE_COLOR, /* Color Weight for Artist */
                                               GDK_TYPE_COLOR, /* Color Weight for Album Artist */
											   GDK_TYPE_COLOR, /* Color Weight for Album */
                                               GDK_TYPE_COLOR, /* Color Weight for Disc Number */
                                               GDK_TYPE_COLOR, /* Color Weight for Year */
                                               GDK_TYPE_COLOR, /* Color Weight for Track + Track Total */
                                               GDK_TYPE_COLOR, /* Color Weight for Genre */
                                               GDK_TYPE_COLOR, /* Color Weight for Comment */
                                               GDK_TYPE_COLOR, /* Color Weight for Composer */
                                               GDK_TYPE_COLOR, /* Color Weight for Orig. Artist */
                                               GDK_TYPE_COLOR, /* Color Weight for Copyright */
                                               GDK_TYPE_COLOR, /* Color Weight for URL */
                                               GDK_TYPE_COLOR, /* Color Weight for Encoded by */

                                               G_TYPE_POINTER);
    SearchResultList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(SearchResultListModel));

    renderer = gtk_cell_renderer_text_new(); /* Filename */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[0]), renderer,
                                                      "text",           SEARCH_RESULT_FILENAME,
                                                      "weight",         SEARCH_RESULT_FILENAME_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_FILENAME_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Title */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[1]), renderer,
                                                      "text",           SEARCH_RESULT_TITLE,
                                                      "weight",         SEARCH_RESULT_TITLE_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_TITLE_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Artist */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[2]), renderer,
                                                      "text",           SEARCH_RESULT_ARTIST,
                                                      "weight",         SEARCH_RESULT_ARTIST_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_ARTIST_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	renderer = gtk_cell_renderer_text_new(); /* Album Artist */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[3]), renderer,
                                                      "text",           SEARCH_RESULT_ALBUM_ARTIST,
                                                      "weight",         SEARCH_RESULT_ALBUM_ARTIST_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_ALBUM_ARTIST_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Album */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[4]), renderer,
                                                      "text",           SEARCH_RESULT_ALBUM,
                                                      "weight",         SEARCH_RESULT_ALBUM_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_ALBUM_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Disc Number */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[5]), renderer,
                                                      "text",           SEARCH_RESULT_DISC_NUMBER,
                                                      "weight",         SEARCH_RESULT_DISC_NUMBER_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_DISC_NUMBER_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Year */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[6]), renderer,
                                                      "text",           SEARCH_RESULT_YEAR,
                                                      "weight",         SEARCH_RESULT_YEAR_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_YEAR_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Track */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[7]), renderer,
                                                      "text",           SEARCH_RESULT_TRACK,
                                                      "weight",         SEARCH_RESULT_TRACK_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_TRACK_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Genre */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[8]), renderer,
                                                      "text",           SEARCH_RESULT_GENRE,
                                                      "weight",         SEARCH_RESULT_GENRE_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_GENRE_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Comment */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[9]), renderer,
                                                      "text",           SEARCH_RESULT_COMMENT,
                                                      "weight",         SEARCH_RESULT_COMMENT_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_COMMENT_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Composer */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[10]), renderer,
                                                      "text",           SEARCH_RESULT_COMPOSER,
                                                      "weight",         SEARCH_RESULT_COMPOSER_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_COMPOSER_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Orig. Artist */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[11]), renderer,
                                                      "text",           SEARCH_RESULT_ORIG_ARTIST,
                                                      "weight",         SEARCH_RESULT_ORIG_ARTIST_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_ORIG_ARTIST_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Copyright */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[12]), renderer,
                                                      "text",           SEARCH_RESULT_COPYRIGHT,
                                                      "weight",         SEARCH_RESULT_COPYRIGHT_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_COPYRIGHT_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* URL */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[13]), renderer,
                                                      "text",           SEARCH_RESULT_URL,
                                                      "weight",         SEARCH_RESULT_URL_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_URL_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new(); /* Encoded by */
    column = gtk_tree_view_column_new_with_attributes(_(SearchResultList_Titles[14]), renderer,
                                                      "text",           SEARCH_RESULT_ENCODED_BY,
                                                      "weight",         SEARCH_RESULT_ENCODED_BY_WEIGHT,
                                                      "foreground-gdk", SEARCH_RESULT_ENCODED_BY_FOREGROUND,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(SearchResultList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    gtk_container_add(GTK_CONTAINER(ScrollWindow),SearchResultList);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(SearchResultList)),
            GTK_SELECTION_MULTIPLE);
    //gtk_tree_view_columns_autosize(GTK_TREE_VIEW(SearchResultList)); // Doesn't seem to work...
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(SearchResultList), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(SearchResultList), FALSE);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(SearchResultList))),
            "changed", G_CALLBACK(Search_Result_List_Row_Selected), NULL);

    // Button to run the search
    Button = gtk_button_new_from_stock(GTK_STOCK_FIND);
    gtk_table_attach(GTK_TABLE(Table),Button,5,6,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_widget_set_can_default(Button,TRUE);
    gtk_widget_grab_default(Button);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(Search_File),NULL);
    g_signal_connect(G_OBJECT(gtk_bin_get_child(GTK_BIN(SearchStringCombo))),"activate", G_CALLBACK(Search_File),NULL);

    // Button to cancel
    Button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_table_attach(GTK_TABLE(Table),Button,5,6,1,2,GTK_FILL,GTK_FILL,0,0);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(Destroy_Search_File_Window),NULL);

    // Status bar
    SearchStatusBar = gtk_statusbar_new();
    //gtk_table_attach(GTK_TABLE(Table),SearchStatusBar,0,6,3,4,GTK_FILL,GTK_FILL,0,0);
    gtk_box_pack_start(GTK_BOX(VBox),SearchStatusBar,FALSE,TRUE,0);
    SearchStatusBarContext = gtk_statusbar_get_context_id(GTK_STATUSBAR(SearchStatusBar),"Messages");
    gtk_statusbar_push(GTK_STATUSBAR(SearchStatusBar),SearchStatusBarContext,_("Ready to search…"));

    gtk_widget_show_all(SearchFileWindow);

    if (SET_SEARCH_WINDOW_POSITION)
        gtk_window_move(GTK_WINDOW(SearchFileWindow), SEARCH_WINDOW_X, SEARCH_WINDOW_Y);
    //else
    //    gtk_window_set_position(GTK_WINDOW(SearchFileWindow), GTK_WIN_POS_CENTER_ON_PARENT); // Must use gtk_window_set_transient_for to work
}

void Destroy_Search_File_Window (void)
{
    if (SearchFileWindow)
    {
        /* Save combobox history lists before exit */
        Save_Search_File_List(SearchStringModel, MISC_COMBO_TEXT);

        Search_File_Window_Apply_Changes();

        gtk_widget_destroy(SearchFileWindow);
        SearchFileWindow = NULL;
    }
}

/*
 * For the configuration file...
 */
void Search_File_Window_Apply_Changes (void)
{
    if (SearchFileWindow)
    {
        gint x, y, width, height;
        GdkWindow *window;

        window = gtk_widget_get_window(SearchFileWindow);

        if ( window && gdk_window_is_visible(window) && gdk_window_get_state(window)!=GDK_WINDOW_STATE_MAXIMIZED )
        {
            // Position and Origin of the scanner window
            gdk_window_get_root_origin(window,&x,&y);
            SEARCH_WINDOW_X = x;
            SEARCH_WINDOW_Y = y;
            width = gdk_window_get_width(window);
            height = gdk_window_get_height(window);
            SEARCH_WINDOW_WIDTH  = width;
            SEARCH_WINDOW_HEIGHT = height;
        }

        SEARCH_IN_FILENAME    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SearchInFilename));
        SEARCH_IN_TAG         = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SearchInTag));
        SEARCH_CASE_SENSITIVE = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SearchCaseSensitive));
    }
}

gboolean Search_File_Window_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_KEY_Escape:
                Destroy_Search_File_Window();
                break;
        }
    }
    return FALSE;
}

/*
 * This function and the one below could do with improving
 * as we are looking up tag data twice (once when searching, once when adding to list)
 */
void Search_File (GtkWidget *search_button)
{
    const gchar *string_to_search = NULL;
    GList *etfilelist;
    ET_File *ETFile;
    gchar *msg;
    gchar *temp = NULL;
    gchar *title2, *artist2, *album_artist2, *album2, *disc_number2, *year2, *track2,
          *track_total2, *genre2, *comment2, *composer2, *orig_artist2,
          *copyright2, *url2, *encoded_by2, *string_to_search2;
    gint resultCount = 0;


    if (!SearchStringCombo || !SearchInFilename || !SearchInTag || !SearchResultList)
        return;

    string_to_search = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(SearchStringCombo))));
    if (!string_to_search)
        return;

    Add_String_To_Combo_List(SearchStringModel, string_to_search);

    gtk_widget_set_sensitive(GTK_WIDGET(search_button),FALSE);
    gtk_list_store_clear(SearchResultListModel);
    gtk_statusbar_push(GTK_STATUSBAR(SearchStatusBar),SearchStatusBarContext,"");

    etfilelist = g_list_first(ETCore->ETFileList);
    while (etfilelist)
    {
        ETFile = (ET_File *)etfilelist->data;

        // Search in the filename
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SearchInFilename)))
        {
            gchar *filename_utf8 = ((File_Name *)ETFile->FileNameNew->data)->value_utf8;
            gchar *basename_utf8;

            // To search without case sensivity
            if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SearchCaseSensitive)))
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
                Add_Row_To_Search_Result_List(ETFile, string_to_search2);
                etfilelist = etfilelist->next;
                g_free(basename_utf8);
                g_free(string_to_search2);
                continue;
            }
            g_free(basename_utf8);
            g_free(string_to_search2);
        }

        // Search in the tag
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SearchInTag)))
        {
            File_Tag *FileTag   = (File_Tag *)ETFile->FileTag->data;

            // To search with or without case sensivity
            if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SearchCaseSensitive)))
            {
                // To search without case sensivity...
                // Duplicate and convert the strings into UTF-8 in loxer case
                if (FileTag->title)       title2       = g_utf8_casefold(FileTag->title, -1);        else title2       = NULL;
                if (FileTag->artist)      artist2      = g_utf8_casefold(FileTag->artist, -1);       else artist2      = NULL;
                if (FileTag->album_artist) album_artist2 = g_utf8_casefold(FileTag->album_artist, -1); else album_artist2= NULL;
                if (FileTag->album)       album2       = g_utf8_casefold(FileTag->album, -1);        else album2       = NULL;
                if (FileTag->disc_number) disc_number2 = g_utf8_casefold(FileTag->disc_number, -1);  else disc_number2 = NULL;
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
                Add_Row_To_Search_Result_List(ETFile, string_to_search);
            }
            g_free(title2);
            g_free(artist2);
            g_free(album_artist2);
            g_free(album2);
            g_free(disc_number2);
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
        etfilelist = etfilelist->next;
    }

    gtk_widget_set_sensitive(GTK_WIDGET(search_button),TRUE);

    // Display the number of files in the statusbar
    resultCount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(SearchResultListModel), NULL);
    msg = g_strdup_printf(_("Found: %d file(s)"), resultCount);
    gtk_statusbar_push(GTK_STATUSBAR(SearchStatusBar),SearchStatusBarContext,msg);
    g_free(msg);

    // Disable result list if no row inserted
    if (resultCount > 0 )
    {
        gtk_widget_set_sensitive(GTK_WIDGET(SearchResultList), TRUE);
    } else
    {
        gtk_widget_set_sensitive(GTK_WIDGET(SearchResultList), FALSE);
    }
}

void Add_Row_To_Search_Result_List(ET_File *ETFile,const gchar *string_to_search)
{
    gchar *SearchResultList_Text[15]; // Because : 15 columns to display
    gint SearchResultList_Weight[15] = {PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_NORMAL,
                                        PANGO_WEIGHT_NORMAL};
    GdkColor *SearchResultList_Color[15] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                            NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                            NULL};
    gchar *track, *track_total;
    gboolean case_sensitive;
    gint column;
    GtkTreeIter iter;

    if (!ETFile || !string_to_search)
        return;

    case_sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SearchCaseSensitive));

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
    // Disc Number
    SearchResultList_Text[SEARCH_RESULT_DISC_NUMBER] = g_strdup(((File_Tag *)ETFile->FileTag->data)->disc_number);
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
                if (CHANGED_FILES_DISPLAYED_TO_BOLD)
                    SearchResultList_Weight[column] = PANGO_WEIGHT_BOLD;
                else
                    SearchResultList_Color[column] = &RED;
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
                if (CHANGED_FILES_DISPLAYED_TO_BOLD)
                    SearchResultList_Weight[column] = PANGO_WEIGHT_BOLD;
                else
                    SearchResultList_Color[column] = &RED;
            }

            g_free(list_text);
            g_free(string_to_search2);
        }
    }

    // Load the row in the list
    gtk_list_store_append(SearchResultListModel, &iter);
    gtk_list_store_set(SearchResultListModel, &iter,
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
 * Callback to select-row event
 * Select all results that are selected in the search result list also in the browser list
 */
void Search_Result_List_Row_Selected(GtkTreeSelection *selection, gpointer data)
{
    GList       *selectedRows;
    GList       *selectedRowsCopy;
    ET_File     *ETFile;
    GtkTreeIter  currentFile;
    gboolean     found;

    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);
    selectedRowsCopy = selectedRows;

    /* We might be called with no rows selected */
    if (g_list_length(selectedRows) == 0)
    {
        g_list_foreach(selectedRowsCopy, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selectedRowsCopy);
        return;
    }

    // Unselect files in the main list before re-selecting them...
    Browser_List_Unselect_All_Files();

    while (selectedRows)
    {
        found = gtk_tree_model_get_iter(GTK_TREE_MODEL(SearchResultListModel), &currentFile, (GtkTreePath*)selectedRows->data);
        if (found)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(SearchResultListModel), &currentFile, 
                               SEARCH_RESULT_POINTER, &ETFile, -1);
            // Select the files (but don't display them to increase speed)
            Browser_List_Select_File_By_Etfile(ETFile, TRUE);
            // Display only the last file (to increase speed)
            if (!selectedRows->next)
                Action_Select_Nth_File_By_Etfile(ETFile);
        }
        selectedRows = selectedRows->next;
    }

    g_list_foreach(selectedRowsCopy, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selectedRowsCopy);
}


/********************************************
 * Load Filenames from a TXT file functions *
 ********************************************/
/*
 * The window to load the filenames from a txt.
 */
void Open_Load_Filename_Window (void)
{
    GtkWidget *VBox, *hbox;
    GtkWidget *Frame;
    GtkWidget *Label;
    GtkWidget *ButtonBox;
    GtkWidget *Button;
    GtkWidget *Icon;
    GtkWidget *Entry;
    GtkWidget *ButtonLoad;
    GtkWidget *Separator;
    GtkWidget *ScrollWindow;
    GtkWidget *loadedvbox;
    GtkWidget *filelistvbox;
    GtkWidget *vboxpaned;
    gchar *path;
    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column;

    if (LoadFilenameWindow != NULL)
    {
        gtk_window_present(GTK_WINDOW(LoadFilenameWindow));
        return;
    }

    LoadFilenameWindow  = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(LoadFilenameWindow),_("Load the filenames from a TXT file"));
    gtk_window_set_transient_for(GTK_WINDOW(LoadFilenameWindow),GTK_WINDOW(MainWindow));
    g_signal_connect(G_OBJECT(LoadFilenameWindow),"destroy",G_CALLBACK(Destroy_Load_Filename_Window),NULL);
    g_signal_connect(G_OBJECT(LoadFilenameWindow),"delete_event", G_CALLBACK(Destroy_Load_Filename_Window),NULL);
    g_signal_connect(G_OBJECT(LoadFilenameWindow),"key_press_event", G_CALLBACK(Load_Filename_Window_Key_Press),NULL);

    // Just center on mainwindow
    gtk_window_set_position(GTK_WINDOW(LoadFilenameWindow), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_default_size(GTK_WINDOW(LoadFilenameWindow),LOAD_FILE_WINDOW_WIDTH,LOAD_FILE_WINDOW_HEIGHT);

    Frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(LoadFilenameWindow),Frame);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),2);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 2);

    // Hbox for file entry and browser/load buttons
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_box_pack_start(GTK_BOX(VBox),hbox,FALSE,TRUE,0);

    // File to load
    if (!FileToLoadModel)
        FileToLoadModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);
    else
        gtk_list_store_clear(FileToLoadModel);

    Label = gtk_label_new(_("File:"));
    gtk_misc_set_alignment(GTK_MISC(Label),1.0,0.5);
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);
    FileToLoadCombo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(FileToLoadModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(FileToLoadCombo),MISC_COMBO_TEXT);
    gtk_widget_set_size_request(GTK_WIDGET(FileToLoadCombo), 200, -1);
    gtk_box_pack_start(GTK_BOX(hbox),FileToLoadCombo,TRUE,TRUE,0);
    // History List
    Load_File_To_Load_List(FileToLoadModel, MISC_COMBO_TEXT);
    // Initial value
    if ((path=Browser_Get_Current_Path())!=NULL)
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(FileToLoadCombo))),path);
    // the 'changed' signal is attached below to enable/disable the button to load
    // Button 'browse'
    Button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked", G_CALLBACK(File_Selection_Window_For_File), G_OBJECT(gtk_bin_get_child(GTK_BIN(FileToLoadCombo))));
    // Button 'load'
    // the signal attached to this button, to load the file, is placed after the LoadFileContentList definition
    ButtonLoad = Create_Button_With_Icon_And_Label(GTK_STOCK_REVERT_TO_SAVED,_(" Load "));
    //ButtonLoad = gtk_button_new_with_label(_(" Load "));
    gtk_box_pack_start(GTK_BOX(hbox),ButtonLoad,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(gtk_bin_get_child(GTK_BIN(FileToLoadCombo))),"changed", G_CALLBACK(Button_Load_Set_Sensivity), G_OBJECT(ButtonLoad));

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    //
    // Vbox for loaded files
    //
    loadedvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    // Content of the loaded file
    ScrollWindow = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindow),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(ScrollWindow), 250, 200);
    gtk_box_pack_start(GTK_BOX(loadedvbox), ScrollWindow, TRUE, TRUE, 0);
    LoadFileContentListModel = gtk_list_store_new(LOAD_FILE_CONTENT_COUNT, G_TYPE_STRING);
    LoadFileContentList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(LoadFileContentListModel));
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Content of TXT file"),
                                                      renderer, "text", LOAD_FILE_CONTENT_TEXT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(LoadFileContentList), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(LoadFileContentList), TRUE);
    //gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(LoadFileContentList)),GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(LoadFileContentList),TRUE);
    gtk_container_add(GTK_CONTAINER(ScrollWindow),LoadFileContentList);

    // Signal to automatically load the file
    g_signal_connect_swapped(G_OBJECT(ButtonLoad),"clicked", G_CALLBACK(Load_File_Content), G_OBJECT(gtk_bin_get_child(GTK_BIN(FileToLoadCombo))));
    g_signal_connect(G_OBJECT(LoadFileContentList),"key-press-event", G_CALLBACK(Load_Filename_List_Key_Press),NULL);

    // Commands (like the popup menu)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_box_pack_start(GTK_BOX(loadedvbox),hbox,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Insert a blank line before the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Insert_Blank_Line), G_OBJECT(LoadFileContentList));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Delete the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Delete_Line), G_OBJECT(LoadFileContentList));
    
    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Delete all blank lines"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Delete_All_Blank_Lines), G_OBJECT(LoadFileContentList));
    
    Label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Move up the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Move_Up), G_OBJECT(LoadFileContentList));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Move down the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Move_Down), G_OBJECT(LoadFileContentList));

    Label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REFRESH,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Reload"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Reload), G_OBJECT(LoadFileContentList));
    
    gtk_widget_show_all(loadedvbox);

    
    //
    // Vbox for file list files
    //
    filelistvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    // List of current filenames
    ScrollWindow = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindow),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(ScrollWindow), 250, 200);
    gtk_box_pack_start(GTK_BOX(filelistvbox), ScrollWindow, TRUE, TRUE, 0);
    LoadFileNameListModel = gtk_list_store_new(LOAD_FILE_NAME_COUNT, G_TYPE_STRING,G_TYPE_POINTER);
    LoadFileNameList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(LoadFileNameListModel));
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("List of files"), 
                                                      renderer, "text", LOAD_FILE_NAME_TEXT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(LoadFileNameList), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(LoadFileNameList), TRUE);
    //gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(LoadFileNameList)),GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(LoadFileNameList),TRUE);
    g_signal_connect(G_OBJECT(LoadFileNameList),"key-press-event", G_CALLBACK(Load_Filename_List_Key_Press),NULL);
    gtk_container_add(GTK_CONTAINER(ScrollWindow),LoadFileNameList);

    // Signals to 'select' the same row into the other list (to show the corresponding filenames)
    g_signal_connect_swapped(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(LoadFileContentList))),"changed", G_CALLBACK(Load_Filename_Select_Row_In_Other_List), G_OBJECT(LoadFileNameList));
    g_signal_connect_swapped(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(LoadFileNameList))),"changed", G_CALLBACK(Load_Filename_Select_Row_In_Other_List), G_OBJECT(LoadFileContentList));

    // Commands (like the popup menu)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_box_pack_start(GTK_BOX(filelistvbox),hbox,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Insert a blank line before the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Insert_Blank_Line), G_OBJECT(LoadFileNameList));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Delete the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Delete_Line), G_OBJECT(LoadFileNameList));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Delete all blank lines"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Delete_All_Blank_Lines), G_OBJECT(LoadFileNameList));
    
    Label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Move up the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Move_Up), G_OBJECT(LoadFileNameList));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Move down the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Move_Down), G_OBJECT(LoadFileNameList));

    Label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REFRESH,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Reload"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Reload), G_OBJECT(LoadFileNameList));
    
    gtk_widget_show_all(filelistvbox);

    
    // Load the list of files in the list widget
    Load_File_List();

    vboxpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_pack1(GTK_PANED(vboxpaned),loadedvbox,  TRUE,FALSE);
    gtk_paned_pack2(GTK_PANED(vboxpaned),filelistvbox,TRUE,FALSE);
    gtk_box_pack_start(GTK_BOX(VBox),vboxpaned,TRUE,TRUE,0);

    // Create popup menus
    Create_Load_Filename_Popup_Menu(LoadFileContentList);
    Create_Load_Filename_Popup_Menu(LoadFileNameList);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_box_pack_start(GTK_BOX(VBox),hbox,FALSE,TRUE,0);

    Label = gtk_label_new(_("Selected line:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    // Entry to edit a line into the list
    Entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox),Entry,TRUE,TRUE,0);
    g_signal_connect(G_OBJECT(Entry),"changed",G_CALLBACK(Load_Filename_Update_Text_Line),G_OBJECT(LoadFileContentList));

    // Signal to load the line text in the editing entry
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(LoadFileContentList))),"changed", G_CALLBACK(Load_Filename_Edit_Text_Line), G_OBJECT(Entry));

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    LoadFileRunScanner = gtk_check_button_new_with_label(_("Run the current scanner for each file"));
    gtk_box_pack_start(GTK_BOX(VBox),LoadFileRunScanner,FALSE,TRUE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(LoadFileRunScanner),LOAD_FILE_RUN_SCANNER);
    gtk_widget_set_tooltip_text(LoadFileRunScanner,_("When activating this option, after loading the "
        "filenames, the current selected scanner will be ran (the scanner window must be opened)."));

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    ButtonBox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),ButtonBox,FALSE,FALSE,0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(ButtonBox),GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(ButtonBox), 10);

    // Button to cancel
    Button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    gtk_widget_grab_default(Button);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(Destroy_Load_Filename_Window),NULL);

    // Button to load filenames
    Button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(Load_Filename_Set_Filenames),NULL);


    // To initialize 'ButtonLoad' sensivity
    g_signal_emit_by_name(G_OBJECT(gtk_bin_get_child(GTK_BIN(FileToLoadCombo))),"changed");

    gtk_widget_show_all(LoadFilenameWindow);
    if (LOAD_FILE_WINDOW_X > 0 && LOAD_FILE_WINDOW_Y > 0)
        gtk_window_move(GTK_WINDOW(LoadFilenameWindow),LOAD_FILE_WINDOW_X,LOAD_FILE_WINDOW_Y);
}

void Destroy_Load_Filename_Window (void)
{
    if (LoadFilenameWindow)
    {
        /* Save combobox history lists before exit */
        Save_File_To_Load_List(FileToLoadModel, MISC_COMBO_TEXT);

        Load_Filename_Window_Apply_Changes();

        gtk_widget_destroy(LoadFilenameWindow);
        LoadFilenameWindow = NULL;
    }
}

/*
 * For the configuration file...
 */
void Load_Filename_Window_Apply_Changes (void)
{
    if (LoadFilenameWindow)
    {
        gint x, y, width, height;
        GdkWindow *window;

        window = gtk_widget_get_window (LoadFilenameWindow);

        if ( window && gdk_window_is_visible(window) && gdk_window_get_state(window)!=GDK_WINDOW_STATE_MAXIMIZED )
        {
            // Position and Origin of the window
            gdk_window_get_root_origin(window,&x,&y);
            LOAD_FILE_WINDOW_X = x;
            LOAD_FILE_WINDOW_Y = y;
            width = gdk_window_get_width(window);
            height = gdk_window_get_height(window);
            LOAD_FILE_WINDOW_WIDTH  = width;
            LOAD_FILE_WINDOW_HEIGHT = height;
        }

        LOAD_FILE_RUN_SCANNER = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(LoadFileRunScanner));
    }
}

gboolean Load_Filename_Window_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_KEY_Escape:
                Destroy_Load_Filename_Window();
                break;
        }
    }
    return FALSE;
}

/*
 * To enable/disable sensivity of the button 'Load'
 */
void Button_Load_Set_Sensivity (GtkWidget *button, GtkWidget *entry)
{
    struct stat statbuf;
    gchar *path;

    if (!entry || !button)
        return;

    path = filename_from_display(gtk_entry_get_text(GTK_ENTRY(entry)));
    if ( path && stat(path,&statbuf)==0 && S_ISREG(statbuf.st_mode))
        gtk_widget_set_sensitive(GTK_WIDGET(button),TRUE);
    else
        gtk_widget_set_sensitive(GTK_WIDGET(button),FALSE);

    g_free(path);
}

void Load_Filename_List_Key_Press (GtkWidget *treeview, GdkEvent *event)
{
    if (event && event->type == GDK_KEY_PRESS)
    {
        GdkEventKey *kevent = (GdkEventKey *)event;

        switch(kevent->keyval)
        {
            case GDK_KEY_Delete:
                Load_Filename_List_Delete_Line(treeview);
                break;
            case GDK_KEY_I:
            case GDK_KEY_i:
                Load_Filename_List_Insert_Blank_Line(treeview);
                break;
        }
    }
}

/*
 * Load content of the file into the LoadFileContentList list
 */
void Load_File_Content (GtkWidget *entry)
{
    FILE *file;
    gchar *filename;
    const gchar *filename_utf8;
    gchar buffer[MAX_STRING_LEN];
    gchar *text;
    gchar *valid;
    GtkTreeIter iter;

    if (!entry)
        return;

    // The file to read
    filename_utf8 = gtk_entry_get_text(GTK_ENTRY(entry)); // Don't free me!
    Add_String_To_Combo_List(FileToLoadModel, filename_utf8);
    filename = filename_from_display(filename_utf8);

    if ( (file=fopen(filename,"r"))==0 )
    {
        Log_Print(LOG_ERROR,_("Can't open file '%s' (%s)"),filename_utf8,g_strerror(errno));
        g_free(filename);
        return;
    }

    gtk_list_store_clear(LoadFileContentListModel);
    while (fgets(buffer, sizeof(buffer), file))
    {
        if (buffer[strlen(buffer)-1]=='\n')
            buffer[strlen(buffer)-1]='\0';
        if (buffer[strlen(buffer)-1]=='\r')
            buffer[strlen(buffer)-1]='\0';
        text = &buffer[0];

        /*if (g_utf8_validate(text, -1, NULL))
        {
            valid = g_strdup(buffer);
        } else
        {
            valid = convert_to_utf8(text);
        }*/
        valid = Try_To_Validate_Utf8_String(text);

        gtk_list_store_append(LoadFileContentListModel, &iter);
        gtk_list_store_set(LoadFileContentListModel, &iter,
                           LOAD_FILE_CONTENT_TEXT, valid,
                           -1);
        g_free(valid);
    }

    fclose(file);
    g_free(filename);
}

/*
 * Load the names of the current list of files
 */
void Load_File_List (void)
{
    GList *etfilelist;
    ET_File *etfile;
    gchar *filename_utf8;
    gchar *pos;
    GtkTreeIter iter;

    gtk_list_store_clear(LoadFileNameListModel);
    etfilelist = g_list_first(ETCore->ETFileList);
    while (etfilelist)
    {
        etfile   = (ET_File *)etfilelist->data;
        filename_utf8 = g_path_get_basename(((File_Name *)etfile->FileNameNew->data)->value_utf8);
        // Remove the extension ('filename' must be allocated to don't affect the initial value)
        if ((pos=strrchr(filename_utf8,'.'))!=NULL)
            *pos = 0;
        gtk_list_store_append(LoadFileNameListModel, &iter);
        gtk_list_store_set(LoadFileNameListModel, &iter,
                           LOAD_FILE_NAME_TEXT, filename_utf8,
                           LOAD_FILE_NAME_POINTER, etfilelist->data,
                           -1);
        g_free(filename_utf8);
        etfilelist = etfilelist->next;
    }
}
/*
 * To select the corresponding row in the other list
 */
void Load_Filename_Select_Row_In_Other_List(GtkWidget* treeview_target, gpointer origselection)
{
    GtkAdjustment *ct_adj, *ce_adj;
    GtkTreeSelection *selection_orig;
    GtkTreeSelection *selection_target;
    GtkTreeView *treeview_orig;
    GtkTreeModel *treemodel_orig;
    GtkTreeModel *treemodel_target;
    GtkTreeIter iter_orig;
    GtkTreeIter iter_target;
    GtkTreePath *path_orig;
    gint *indices_orig;
    gchar *stringiter;

    if (!treeview_target || !origselection)
        return;

    selection_orig = GTK_TREE_SELECTION(origselection);
    selection_target = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_target));
    treemodel_target = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_target));

    if (!gtk_tree_selection_get_selected(selection_orig, &treemodel_orig, &iter_orig))
        return; /* Might be called with no selection */

    treeview_orig = gtk_tree_selection_get_tree_view(selection_orig);
    path_orig = gtk_tree_model_get_path(treemodel_orig, &iter_orig);
    gtk_tree_selection_unselect_all(selection_target);

    // Synchronize the two lists
    ce_adj = gtk_scrollable_get_vadjustment(treeview_orig);
    ct_adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(treeview_target));

    if (gtk_adjustment_get_upper(GTK_ADJUSTMENT(ct_adj)) >= gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ct_adj))
    &&  gtk_adjustment_get_upper(GTK_ADJUSTMENT(ce_adj)) >= gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ce_adj)))
    {
        // Rules are displayed in the both clist
        if (gtk_adjustment_get_value(GTK_ADJUSTMENT(ce_adj)) <= gtk_adjustment_get_upper(GTK_ADJUSTMENT(ct_adj)) - gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ct_adj)))
        {
            gtk_adjustment_set_value(GTK_ADJUSTMENT(ct_adj),gtk_adjustment_get_value(GTK_ADJUSTMENT(ce_adj)));
        } else
        {
            gtk_adjustment_set_value(GTK_ADJUSTMENT(ct_adj),gtk_adjustment_get_upper(GTK_ADJUSTMENT(ct_adj)) - gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ct_adj)));
            indices_orig = gtk_tree_path_get_indices(path_orig);

            if (indices_orig[0] <= (gtk_tree_model_iter_n_children(treemodel_target, NULL) - 1))
                gtk_adjustment_set_value(GTK_ADJUSTMENT(ce_adj),gtk_adjustment_get_value(GTK_ADJUSTMENT(ct_adj)));

        }
    }else if (gtk_adjustment_get_upper(GTK_ADJUSTMENT(ct_adj)) < gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ct_adj))) // Target Clist rule not visible
    {
        indices_orig = gtk_tree_path_get_indices(path_orig);

        if (indices_orig[0] <= (gtk_tree_model_iter_n_children(treemodel_target, NULL) - 1))
            gtk_adjustment_set_value(GTK_ADJUSTMENT(ce_adj),gtk_adjustment_get_value(GTK_ADJUSTMENT(ct_adj)));
    }

    // Must block the select signal of the target to avoid looping
    g_signal_handlers_block_by_func(G_OBJECT(selection_target), G_CALLBACK(Load_Filename_Select_Row_In_Other_List), G_OBJECT(treeview_orig));

    stringiter = gtk_tree_model_get_string_from_iter(treemodel_orig, &iter_orig);
    if (gtk_tree_model_get_iter_from_string(treemodel_target, &iter_target, stringiter))
    {
        gtk_tree_selection_select_iter(selection_target, &iter_target);
    }

    g_free(stringiter);
    g_signal_handlers_unblock_by_func(G_OBJECT(selection_target), G_CALLBACK(Load_Filename_Select_Row_In_Other_List), G_OBJECT(treeview_orig));
}

/*
 * Set the new file name of each file.
 * Associate lines from LoadFileContentList with LoadFileNameList
 */
void Load_Filename_Set_Filenames (void)
{
    gint row;
    ET_File   *ETFile;
    File_Name *FileName;
    gchar *list_text;
    gint rowcount;
    gboolean found;

    GtkTreePath *currentPath = NULL;
    GtkTreeIter iter_name;
    GtkTreeIter iter_content;

    if ( !ETCore->ETFileList || !LoadFileContentList || !LoadFileNameList)
        return;

    /* Save current file */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    rowcount = MIN(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(LoadFileNameListModel), NULL),
                   gtk_tree_model_iter_n_children(GTK_TREE_MODEL(LoadFileContentListModel), NULL));

    for (row=0; row < rowcount; row++)
    {
        if (row == 0)
            currentPath = gtk_tree_path_new_first();
        else
            gtk_tree_path_next(currentPath);

        found = gtk_tree_model_get_iter(GTK_TREE_MODEL(LoadFileNameListModel), &iter_name, currentPath);
        if (found)
            gtk_tree_model_get(GTK_TREE_MODEL(LoadFileNameListModel), &iter_name, 
                               LOAD_FILE_NAME_POINTER, &ETFile, -1);

        found = gtk_tree_model_get_iter(GTK_TREE_MODEL(LoadFileContentListModel), &iter_content, currentPath);
        if (found)
            gtk_tree_model_get(GTK_TREE_MODEL(LoadFileContentListModel), &iter_content, 
                               LOAD_FILE_CONTENT_TEXT, &list_text, -1);

        if (ETFile && list_text && g_utf8_strlen(list_text, -1)>0)
        {
            gchar *list_text_tmp;
            gchar *filename_new_utf8;

            list_text_tmp = g_strdup(list_text);
            ET_File_Name_Convert_Character(list_text_tmp); // Replace invalid characters

            /* Build the filename with the path */
            filename_new_utf8 = ET_File_Name_Generate(ETFile,list_text_tmp);
            g_free(list_text_tmp);

            /* Set the new filename */
            // Create a new 'File_Name' item
            FileName = ET_File_Name_Item_New();
            // Save changes of the 'File_Name' item
            ET_Set_Filename_File_Name_Item(FileName,filename_new_utf8,NULL);
            ET_Manage_Changes_Of_File_Data(ETFile,FileName,NULL);

            g_free(filename_new_utf8);

            // Then run current scanner if asked...
            if (ScannerWindow && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(LoadFileRunScanner)) )
                Scan_Select_Mode_And_Run_Scanner(ETFile);
        }
        g_free(list_text);
    }

    gtk_tree_path_free(currentPath);

    Browser_List_Refresh_Whole_List();
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
}

/*
 * Create and attach a popup menu on the two clist of the LoadFileWindow
 */
static gboolean
Load_Filename_Popup_Menu_Handler (GtkMenu *menu, GdkEventButton *event)
{
    if (event && (event->type==GDK_BUTTON_PRESS) && (event->button==3))
    {
        gtk_menu_popup(menu,NULL,NULL,NULL,NULL,event->button,event->time);
        return TRUE;
    }
    return FALSE;
}

GtkWidget *Create_Load_Filename_Popup_Menu(GtkWidget *list)
{
    GtkWidget *BrowserPopupMenu;
    GtkWidget *Image;
    GtkWidget *MenuItem;


    BrowserPopupMenu = gtk_menu_new();
    g_signal_connect_swapped(G_OBJECT(list), "button_press_event", 
                             G_CALLBACK(Load_Filename_Popup_Menu_Handler), G_OBJECT(BrowserPopupMenu));
    
    MenuItem = gtk_image_menu_item_new_with_label(_("Insert a blank line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu), MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Insert_Blank_Line), G_OBJECT(list));

    MenuItem = gtk_image_menu_item_new_with_label(_("Delete this line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Delete_Line), G_OBJECT(list));

    MenuItem = gtk_image_menu_item_new_with_label(_("Delete all blank lines"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Delete_All_Blank_Lines),G_OBJECT(list));

    MenuItem = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);

    MenuItem = gtk_image_menu_item_new_with_label(_("Move up this line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_GO_UP,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Move_Up),G_OBJECT(list));

    MenuItem = gtk_image_menu_item_new_with_label(_("Move down this line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Move_Down),G_OBJECT(list));

    MenuItem = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);

    MenuItem = gtk_image_menu_item_new_with_label(_("Reload"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REFRESH,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Reload), G_OBJECT(list));

    gtk_widget_show_all(BrowserPopupMenu);
    return BrowserPopupMenu;
}

/*
 * Insert a blank line before the selected line in the treeview passed as parameter
 */
void Load_Filename_List_Insert_Blank_Line (GtkWidget *treeview)
{
    GtkTreeSelection *selection;
    GtkTreeIter selectedIter;
    GtkTreeIter *temp;
    GtkTreeModel *model;

    if (!treeview) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (gtk_tree_selection_get_selected(selection, &model, &selectedIter) != TRUE)
        return;

    temp = &selectedIter; /* Not used here, but it must be non-NULL to keep GTK+ happy! */
    gtk_list_store_insert_before(GTK_LIST_STORE(model), temp, &selectedIter);
}

/*
 * Delete all blank lines in the treeview passed as parameter
 */
void Load_Filename_List_Delete_All_Blank_Lines (GtkWidget *treeview)
{
    gchar *text = NULL;
    GtkTreeIter iter;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

    if (!gtk_tree_model_get_iter_first(model, &iter))
        return;

    while (TRUE)
    {
        gtk_tree_model_get(model, &iter, LOAD_FILE_NAME_TEXT, &text, -1);

        /* Check for blank entry */
        if (!text || g_utf8_strlen(text, -1) == 0)
        {
            g_free(text);

            if (!gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
                break;
            else
                continue;
        }
        g_free(text);

        if (!gtk_tree_model_iter_next(model, &iter))
            break;
    }
}

/*
 * Delete the selected line in the treeview passed as parameter
 */
void Load_Filename_List_Delete_Line (GtkWidget *treeview)
{
    GtkTreeSelection *selection;
    GtkTreeIter selectedIter, itercopy;
    GtkTreeModel *model;
    gboolean rowafter;

    if (!treeview) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (gtk_tree_selection_get_selected(selection, &model, &selectedIter) != TRUE)
    {
        return;
    }

    // If there is a line following the one about to be removed, select it for convenience
    itercopy = selectedIter;
    rowafter = gtk_tree_model_iter_next(model, &itercopy);

    // Remove the line to be deleted
    gtk_list_store_remove(GTK_LIST_STORE(model), &selectedIter);

    if (rowafter)
        gtk_tree_selection_select_iter(selection, &itercopy);
}

/*
 * Move up the selected line in the treeview passed as parameter
 */
void Load_Filename_List_Move_Up (GtkWidget *treeview)
{
    GtkTreeSelection *selection;
    GList *selectedRows;
    GList *selectedRowsCopy;
    GtkTreeIter currentFile;
    GtkTreeIter nextFile;
    GtkTreePath *currentPath;
    GtkTreeModel *treemodel;
    gboolean valid;

    if (!treeview) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (g_list_length(selectedRows) == 0)
    {
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
            // Find the entry above the node...
            if (gtk_tree_path_prev(currentPath))
            {
                // ...and if it exists, swap the two rows by iter
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
 * Move down the selected line in the treeview passed as parameter
 */
void Load_Filename_List_Move_Down (GtkWidget *treeview)
{
    GtkTreeSelection *selection;
    GList *selectedRows;
    GList *selectedRowsCopy;
    GtkTreeIter currentFile;
    GtkTreeIter nextFile;
    GtkTreePath *currentPath;
    GtkTreeModel *treemodel;
    gboolean valid;

    if (!treeview) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (g_list_length(selectedRows) == 0)
    {
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
            // Find the entry below the node and swap the two nodes by iter
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
 * Reload a list of choice
 * The list parameter refers to a GtkTreeView (LoadFileNameList or LoadFileContentList)
 */
void Load_Filename_List_Reload (GtkWidget *treeview)
{
    if (!treeview) return;

    if (GTK_TREE_VIEW(treeview) == GTK_TREE_VIEW(LoadFileContentList))
    {
        Load_File_Content(gtk_bin_get_child(GTK_BIN(FileToLoadCombo)));
        
    } else if (GTK_TREE_VIEW(treeview) == GTK_TREE_VIEW(LoadFileNameList))
    {
        Load_File_List();
    }
}

/*
 * Update the text of the selected line into the list, with the text entered into the entry
 */
void Load_Filename_Update_Text_Line(GtkWidget *entry, GtkWidget *list)
{
    GtkTreeIter SelectedRow;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    gboolean hasSelectedRows = FALSE;

    if (!list || !entry) return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
    hasSelectedRows = gtk_tree_selection_get_selected(selection, &model, &SelectedRow);
    if (hasSelectedRows)
    {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        gtk_list_store_set(GTK_LIST_STORE(model), &SelectedRow, LOAD_FILE_CONTENT_TEXT, text, -1);
    }
}

/*
 * Set the text of the selected line of the list into the entry
 */
void Load_Filename_Edit_Text_Line(GtkTreeSelection *selection, gpointer data)
{
    gchar *text;
    GtkTreeIter selectedIter;
    GtkEntry *entry = GTK_ENTRY(data);
    gulong handler;

    if (gtk_tree_selection_get_selected(selection, NULL, &selectedIter) != TRUE)
        return;

    gtk_tree_model_get(GTK_TREE_MODEL(LoadFileContentListModel), &selectedIter, LOAD_FILE_NAME_TEXT, &text, -1);

    handler = g_signal_handler_find(entry, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, Load_Filename_Update_Text_Line, NULL);
    g_signal_handler_block(entry, handler);
    if (text)
    {
        gtk_entry_set_text(entry, text);
        g_free(text);
    } else
        gtk_entry_set_text(entry, "");

    g_signal_handler_unblock(entry, handler);
}
