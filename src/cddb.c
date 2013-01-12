/* cddb.c - 2000/09/15 */
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

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef G_OS_WIN32
#include "win32/win32dep.h"
#else /* !G_OS_WIN32 */
#include <sys/socket.h>
/* Patch OpenBSD from Jim Geovedi. */
#include <netinet/in.h>
#include <arpa/inet.h>
/* End patch */
#include <netdb.h>
#endif /* !G_OS_WIN32 */
#include <errno.h>

#include "gtk2_compat.h"
#include "cddb.h"
#include "easytag.h"
#include "et_core.h"
#include "browser.h"
#include "base64.h"
#include "scan.h"
#include "log.h"
#include "misc.h"
#include "setting.h"
#include "id3_tag.h"
#include "setting.h"
#include "charset.h"

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
static char *cddb_genre_vs_id3_genre [][2] =
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


// File for result of the Cddb/Freedb request (on remote access)
static const gchar CDDB_RESULT_FILE[] = "cddb_result_file.tmp";


/****************
 * Declarations *
 ****************/
static GtkWidget *CddbNoteBook;
static GList *CddbAlbumList = NULL;

static GtkWidget *CddbSearchStringCombo = NULL;
static GtkListStore *CddbSearchStringModel = NULL;

static GtkWidget *CddbSearchStringInResultCombo;
static GtkListStore *CddbSearchStringInResultModel = NULL;

static GtkWidget *CddbAlbumListView = NULL;
static GtkListStore *CddbAlbumListModel = NULL;
static GtkWidget *CddbTrackListView = NULL;
static GtkListStore *CddbTrackListModel = NULL;
static GtkWidget *CddbApplyButton = NULL;
static GtkWidget *CddbSearchButton = NULL;
static GtkWidget *CddbSearchAutoButton = NULL;
static GtkWidget *CddbStatusBar;
static guint CddbStatusBarContext;

static GtkWidget *CddbStopSearchButton;
static GtkWidget *CddbStopSearchAutoButton;
static GtkWidget *CddbSearchStringInResultNextButton;
static GtkWidget *CddbSearchStringInResultPrevButton;
static GtkWidget *CddbDisplayRedLinesButton;
static GtkWidget *CddbSelectAllInResultButton;
static GtkWidget *CddbUnselectAllInResultButton;
static GtkWidget *CddbInvertSelectionInResultButton;

static gboolean CddbStopSearch = FALSE;


/**************
 * Prototypes *
 **************/
static gboolean Cddb_Destroy_Window (GtkWidget *widget, GdkEvent *event,
                                     gpointer data);
static gboolean Cddb_Window_Key_Press (GtkWidget *window, GdkEvent *event);
static void Cddb_Show_Album_Info (GtkTreeSelection *selection, gpointer data);

static gboolean Cddb_Free_Album_List (void);
static gboolean Cddb_Free_Track_Album_List (GList *track_list);

static gint Cddb_Open_Connection (const gchar *host, gint port);
static void Cddb_Close_Connection (gint socket_id);
static gint Cddb_Read_Line        (FILE **file, gchar **cddb_out);
static gint Cddb_Read_Http_Header (FILE **file, gchar **cddb_out);
static gint Cddb_Read_Cddb_Header (FILE **file, gchar **cddb_out);

static gint Cddb_Write_Result_To_File (gint socket_id,
                                       gulong *bytes_read_total);

static gboolean Cddb_Search_Album_List_From_String (void);
static gboolean Cddb_Search_Album_List_From_String_Freedb (void);
static gboolean Cddb_Search_Album_List_From_String_Gnudb (void);
static gboolean Cddb_Search_Album_From_Selected_Files (void);
static gboolean Cddb_Get_Album_Tracks_List_CB (GtkTreeSelection *selection,
                                               gpointer data);
static gboolean Cddb_Get_Album_Tracks_List (GtkTreeSelection *selection);

static void Cddb_Load_Album_List (gboolean only_red_lines);
static void Cddb_Load_Track_Album_List (GList *track_list);
static gboolean Cddb_Set_Track_Infos_To_File_List (void);
static void Cddb_Album_List_Set_Row_Appearance (GtkTreeIter *row);
static GdkPixbuf *Cddb_Get_Pixbuf_From_Server_Name (const gchar *server_name);

static void Cddb_Search_In_All_Fields_Check_Button_Toggled (void);
static void Cddb_Search_In_All_Categories_Check_Button_Toggled (void);
static void Cddb_Set_To_All_Fields_Check_Button_Toggled (void);
static void Cddb_Stop_Search (void);
static void Cddb_Notebook_Switch_Page (GtkNotebook *notebook, gpointer page,
                                       guint page_num, gpointer user_data);
static void Cddb_Search_String_In_Result (GtkWidget *entry, GtkButton *button);
static void Cddb_Display_Red_Lines_In_Result (void);

static void Cddb_Set_Apply_Button_Sensitivity (void);
static void Cddb_Set_Search_Button_Sensitivity (void);
static void Cddb_Use_Dlm_2_Check_Button_Toggled (void);
static void Cddb_Show_Categories_Button_Toggled (void);
static gchar *Cddb_Generate_Request_String_With_Fields_And_Categories_Options (void);
static const gchar *Cddb_Get_Id3_Genre_From_Cddb_Genre (const gchar *cddb_genre);

static void Cddb_Track_List_Row_Selected (GtkTreeSelection *selection,
                                          gpointer data);
static gboolean Cddb_Track_List_Button_Press (GtkTreeView *treeView,
                                              GdkEventButton *event);

static void Cddb_Track_List_Select_All (void);
static void Cddb_Track_List_Unselect_All (void);
static void Cddb_Track_List_Invert_Selection (void);

static gint Cddb_Track_List_Sort_Func (GtkTreeModel *model, GtkTreeIter *a,
                                       GtkTreeIter *b, gpointer data);

static gchar *Cddb_Format_Proxy_Authentification (void);



/*************
 * Functions *
 *************/
void Init_CddbWindow (void)
{
    CddbWindow = NULL;
}

/*
 * The window to connect to the cd data base.
 */

void Open_Cddb_Window (void)
{
    GtkWidget *MainVBox, *VBox, *vbox, *hbox, *notebookvbox;
    GtkWidget *Frame;
    GtkWidget *Table;
    GtkWidget *Label;
    GtkWidget *Button;
    GtkWidget *Separator;
    GtkWidget *ScrollWindow;
    GtkWidget *Icon;
    gchar *CddbAlbumList_Titles[] = { NULL, N_("Artist / Album"), N_("Category")}; // Note: don't set "" instead of NULL else this will cause problem with translation language
    gchar *CddbTrackList_Titles[] = { "#", N_("Track Name"), N_("Duration")};
    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column;
    GtkAllocation allocation = { 0,0,0,0 };

    if (CddbWindow != NULL)
    {
        gtk_window_present(GTK_WINDOW(CddbWindow));
        return;
    }
    CddbWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(CddbWindow),_("CD Database Search"));
    gtk_window_set_position(GTK_WINDOW(CddbWindow),GTK_WIN_POS_CENTER);

    // This part is needed to set correctly the position of handle panes
    gtk_window_set_default_size(GTK_WINDOW(CddbWindow),CDDB_WINDOW_WIDTH,CDDB_WINDOW_HEIGHT);

    g_signal_connect(G_OBJECT(CddbWindow),"delete_event", G_CALLBACK(Cddb_Destroy_Window),NULL);
    g_signal_connect(G_OBJECT(CddbWindow),"key_press_event", G_CALLBACK(Cddb_Window_Key_Press),NULL);

    MainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(CddbWindow),MainVBox);
    gtk_container_set_border_width(GTK_CONTAINER(MainVBox),1);

    Frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(MainVBox),Frame,TRUE,TRUE,0);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),2);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox),2);


     /*
      * Cddb NoteBook
      */
    CddbNoteBook = gtk_notebook_new();
    gtk_notebook_popup_enable(GTK_NOTEBOOK(CddbNoteBook));
    gtk_box_pack_start(GTK_BOX(VBox),CddbNoteBook,FALSE,FALSE,0);

    /*
     * 1 - Page for automatic search (generate the CDDBId from files)
     */
    Label = gtk_label_new(_("Automatic Search"));
    Frame = gtk_frame_new(NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(CddbNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),2);

    notebookvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(Frame),notebookvbox);
    gtk_container_set_border_width(GTK_CONTAINER(notebookvbox),2);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_box_pack_start(GTK_BOX(notebookvbox),hbox,FALSE,FALSE,0);

    Label = gtk_label_new(_("Request CD database:"));
    gtk_misc_set_alignment(GTK_MISC(Label),1.0,0.5);
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    // Button to generate CddbId and request string from the selected files
    CddbSearchAutoButton = gtk_button_new_from_stock(GTK_STOCK_FIND);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSearchAutoButton,FALSE,FALSE,0);
    gtk_widget_set_can_default(CddbSearchAutoButton,TRUE);
    gtk_widget_grab_default(CddbSearchAutoButton);
    g_signal_connect(G_OBJECT(CddbSearchAutoButton),"clicked",G_CALLBACK(Cddb_Search_Album_From_Selected_Files),NULL);
    gtk_widget_set_tooltip_text(CddbSearchAutoButton,_("Request automatically the "
        "CDDB database using the selected files (the order is important) to "
        "generate the CddbID"));

    // Button to stop the search
    CddbStopSearchAutoButton = Create_Button_With_Icon_And_Label(GTK_STOCK_STOP,NULL);
    gtk_box_pack_start(GTK_BOX(hbox),CddbStopSearchAutoButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(CddbStopSearchAutoButton),GTK_RELIEF_NONE);
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
    g_signal_connect(G_OBJECT(CddbStopSearchAutoButton), "clicked", G_CALLBACK(Cddb_Stop_Search), NULL);
    gtk_widget_set_tooltip_text(CddbStopSearchAutoButton,_("Stop the search…"));

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(hbox),Separator,FALSE,FALSE,0);

    // Check box to run the scanner
    CddbUseLocalAccess = gtk_check_button_new_with_label(_("Use local CDDB"));
    gtk_box_pack_start(GTK_BOX(hbox),CddbUseLocalAccess,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbUseLocalAccess),CDDB_USE_LOCAL_ACCESS);
    gtk_widget_set_tooltip_text(CddbUseLocalAccess,_("When activating this option, after loading the "
        "fields, the current selected scanner will be ran (the scanner window must be opened)."));

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(hbox),Separator,FALSE,FALSE,0);

    // Button to select all files in list
    Button = Create_Button_With_Icon_And_Label("easytag-select-all",NULL);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Select All Files"));
    g_signal_connect(G_OBJECT(Button),"clicked",G_CALLBACK(Action_Select_All_Files),NULL);

    // Button to invert selection of files in list
    Button = Create_Button_With_Icon_And_Label("easytag-invert-selection",NULL);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Invert Files Selection"));
    g_signal_connect(G_OBJECT(Button),"clicked",G_CALLBACK(Action_Invert_Files_Selection),NULL);

/*    // Button to sort by ascending filename
    Button = Create_Button_With_Icon_And_Label(GTK_STOCK_SORT_ASCENDING,NULL);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_tooltips_set_tip(Tips,Button,_("Sort list ascending by filename"),NULL);
    g_signal_connect(G_OBJECT(Button),"clicked",G_CALLBACK(ET_Sort_Displayed_File_List_And_Update_UI),GINT_TO_POINTER(SORTING_BY_ASCENDING_FILENAME));

    // Button to sort by ascending track number
    Button = Create_Button_With_Icon_And_Label(GTK_STOCK_SORT_ASCENDING,NULL);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_tooltips_set_tip(Tips,Button,_("Sort list ascending by track number"),NULL);
    g_signal_connect(G_OBJECT(Button),"clicked",G_CALLBACK(ET_Sort_Displayed_File_List_And_Update_UI),GINT_TO_POINTER(SORTING_BY_ASCENDING_TRACK_NUMBER));
*/
    // Button to quit
    Button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_box_pack_end(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    gtk_widget_set_can_default(Button,TRUE);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(Cddb_Destroy_Window),NULL);


    /*
     * 2 - Page for manual search
     */
    Label = gtk_label_new(_("Manual Search"));
    Frame = gtk_frame_new(NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(CddbNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),2);

    notebookvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(Frame),notebookvbox);
    gtk_container_set_border_width(GTK_CONTAINER(notebookvbox),2);

    /*
     * Words to search
     */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_box_pack_start(GTK_BOX(notebookvbox),hbox,FALSE,FALSE,0);

    Label = gtk_label_new(_("Words:"));
    gtk_misc_set_alignment(GTK_MISC(Label),1.0,0.5);
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    if(CddbSearchStringModel == NULL)
        CddbSearchStringModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);
    else
        gtk_list_store_clear(CddbSearchStringModel);

    CddbSearchStringCombo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(CddbSearchStringModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(CddbSearchStringCombo),MISC_COMBO_TEXT);
    gtk_widget_set_size_request(GTK_WIDGET(CddbSearchStringCombo),220,-1);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSearchStringCombo,FALSE,TRUE,0);
    gtk_widget_set_tooltip_text(GTK_WIDGET(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringCombo)))),_("Enter the words to "
        "search (separated by a space or '+')"));
    // History List
    Load_Cddb_Search_String_List(CddbSearchStringModel, MISC_COMBO_TEXT);

    g_signal_connect(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringCombo)))),"activate",
        G_CALLBACK(Cddb_Search_Album_List_From_String),NULL);
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringCombo))),"");

    // Set content of the clipboard if available
    gtk_editable_paste_clipboard(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(CddbSearchStringCombo))));

    // Button to run the search
    CddbSearchButton = gtk_button_new_from_stock(GTK_STOCK_FIND);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSearchButton,FALSE,FALSE,0);
    gtk_widget_set_can_default(CddbSearchButton,TRUE);
    gtk_widget_grab_default(CddbSearchButton);
    g_signal_connect(G_OBJECT(CddbSearchButton),"clicked",
        G_CALLBACK(Cddb_Search_Album_List_From_String),NULL);
    g_signal_connect(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringCombo)))),"changed",
        G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);

    // Button to stop the search
    CddbStopSearchButton = Create_Button_With_Icon_And_Label(GTK_STOCK_STOP,NULL);
    gtk_box_pack_start(GTK_BOX(hbox),CddbStopSearchButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(CddbStopSearchButton),GTK_RELIEF_NONE);
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
    g_signal_connect(G_OBJECT(CddbStopSearchButton), "clicked", G_CALLBACK(Cddb_Stop_Search), NULL);
    gtk_widget_set_tooltip_text(CddbStopSearchButton,_("Stop the search…"));

    // Button to quit
    Button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_box_pack_end(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    gtk_widget_set_can_default(Button,TRUE);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(Cddb_Destroy_Window),NULL);


    /*
     * Search options
     */
    Frame = gtk_frame_new(_("Search In:"));
    gtk_box_pack_start(GTK_BOX(notebookvbox),Frame,FALSE,TRUE,0);

    Table = gtk_table_new(7,4,FALSE);
    gtk_container_add(GTK_CONTAINER(Frame),Table);
    gtk_table_set_row_spacings(GTK_TABLE(Table),1);
    gtk_table_set_col_spacings(GTK_TABLE(Table),1);

    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "All fields". */
    CddbSearchInAllFields      = gtk_check_button_new_with_label(_("All Fields"));
    Separator                  = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Artist". */
    CddbSearchInArtistField    = gtk_check_button_new_with_label(_("Artist"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Album". */
    CddbSearchInTitleField     = gtk_check_button_new_with_label(_("Album"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Track Name". */
    CddbSearchInTrackNameField = gtk_check_button_new_with_label(_("Track Name"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Other". */
    CddbSearchInOtherField     = gtk_check_button_new_with_label(_("Other"));
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInAllFields,     0,1,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),Separator,                 1,2,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInArtistField,   2,3,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInTitleField,    3,4,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInTrackNameField,4,5,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInOtherField,    5,6,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInAllFields), CDDB_SEARCH_IN_ALL_FIELDS);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInArtistField), CDDB_SEARCH_IN_ARTIST_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInTitleField), CDDB_SEARCH_IN_TITLE_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInTrackNameField), CDDB_SEARCH_IN_TRACK_NAME_FIELD);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInOtherField), CDDB_SEARCH_IN_OTHER_FIELD);
    g_signal_connect(G_OBJECT(CddbSearchInAllFields), "toggled", G_CALLBACK(Cddb_Search_In_All_Fields_Check_Button_Toggled),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInAllFields), "toggled", G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInArtistField), "toggled", G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInTitleField), "toggled", G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInTrackNameField), "toggled", G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInOtherField), "toggled", G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);

    CddbSeparatorH = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_table_attach(GTK_TABLE(Table),CddbSeparatorH,0,7,1,2,GTK_FILL,GTK_FILL,0,0);

    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "All Categories". */
    CddbSearchInAllCategories      = gtk_check_button_new_with_label(_("All Categories"));
    CddbSeparatorV                 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Blues". */
    CddbSearchInBluesCategory      = gtk_check_button_new_with_label(_("Blues"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Classical". */
    CddbSearchInClassicalCategory  = gtk_check_button_new_with_label(_("Classical"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Country". */
    CddbSearchInCountryCategory    = gtk_check_button_new_with_label(_("Country"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Folk". */
    CddbSearchInFolkCategory       = gtk_check_button_new_with_label(_("Folk"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Jazz". */
    CddbSearchInJazzCategory       = gtk_check_button_new_with_label(_("Jazz"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Misc". */
    CddbSearchInMiscCategory       = gtk_check_button_new_with_label(_("Misc."));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "New age". */
    CddbSearchInNewageCategory     = gtk_check_button_new_with_label(_("New Age"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Reggae". */
    CddbSearchInReggaeCategory     = gtk_check_button_new_with_label(_("Reggae"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Rock". */
    CddbSearchInRockCategory       = gtk_check_button_new_with_label(_("Rock"));
    /* Translators: This option is for the previous 'search in' option. For
     * instance, translate this as "Search in:" "Soundtrack". */
    CddbSearchInSoundtrackCategory = gtk_check_button_new_with_label(_("Soundtrack"));
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInAllCategories,     0,1,2,4,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSeparatorV,                1,2,2,4,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInBluesCategory,     2,3,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInClassicalCategory, 3,4,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInCountryCategory,   4,5,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInFolkCategory,      5,6,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInJazzCategory,      6,7,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInMiscCategory,      2,3,3,4,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInNewageCategory,    3,4,3,4,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInReggaeCategory,    4,5,3,4,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInRockCategory,      5,6,3,4,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),CddbSearchInSoundtrackCategory,6,7,3,4,GTK_FILL,GTK_FILL,0,0);
    gtk_label_set_line_wrap(GTK_LABEL(gtk_bin_get_child(GTK_BIN(CddbSearchInAllCategories))),TRUE); // Wrap label of the check button.
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories),     CDDB_SEARCH_IN_ALL_CATEGORIES);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInBluesCategory),     CDDB_SEARCH_IN_BLUES_CATEGORY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInClassicalCategory), CDDB_SEARCH_IN_CLASSICAL_CATEGORY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInCountryCategory),   CDDB_SEARCH_IN_COUNTRY_CATEGORY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInFolkCategory),      CDDB_SEARCH_IN_FOLK_CATEGORY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInJazzCategory),      CDDB_SEARCH_IN_JAZZ_CATEGORY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInMiscCategory),      CDDB_SEARCH_IN_MISC_CATEGORY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInNewageCategory),    CDDB_SEARCH_IN_NEWAGE_CATEGORY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInReggaeCategory),    CDDB_SEARCH_IN_REGGAE_CATEGORY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInRockCategory),      CDDB_SEARCH_IN_ROCK_CATEGORY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSearchInSoundtrackCategory),CDDB_SEARCH_IN_SOUNDTRACK_CATEGORY);
    g_signal_connect(G_OBJECT(CddbSearchInAllCategories),     "toggled",G_CALLBACK(Cddb_Search_In_All_Categories_Check_Button_Toggled),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInAllCategories),     "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInBluesCategory),     "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInClassicalCategory), "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInCountryCategory),   "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInFolkCategory),      "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInJazzCategory),      "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInMiscCategory),      "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInNewageCategory),    "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInReggaeCategory),    "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInRockCategory),      "toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSearchInSoundtrackCategory),"toggled",G_CALLBACK(Cddb_Set_Search_Button_Sensitivity),NULL);
    gtk_widget_set_tooltip_text(CddbSearchInRockCategory,_("included: funk, soul, rap, pop, industrial, metal, etc."));
    gtk_widget_set_tooltip_text(CddbSearchInSoundtrackCategory,_("movies, shows"));
    gtk_widget_set_tooltip_text(CddbSearchInMiscCategory,_("others that do not fit in the above categories"));

    // Button to display/hide the categories
    CddbShowCategoriesButton = gtk_toggle_button_new_with_label(_("Categories"));
    gtk_table_attach(GTK_TABLE(Table),CddbShowCategoriesButton,6,7,0,1,GTK_FILL,GTK_FILL,4,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbShowCategoriesButton),CDDB_SHOW_CATEGORIES);
    g_signal_connect(G_OBJECT(CddbShowCategoriesButton),"toggled", G_CALLBACK(Cddb_Show_Categories_Button_Toggled),NULL);

    /*
     * Results command
     */
    Frame = gtk_frame_new(_("Results:"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,TRUE,0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_container_set_border_width(GTK_CONTAINER(hbox),2);
    gtk_container_add(GTK_CONTAINER(Frame),hbox);

    Label = gtk_label_new(_("Search:"));
    gtk_misc_set_alignment(GTK_MISC(Label),1.0,0.5);
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    if(CddbSearchStringInResultModel == NULL)
        CddbSearchStringInResultModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);
    else
        gtk_list_store_clear(CddbSearchStringInResultModel);

    CddbSearchStringInResultCombo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(CddbSearchStringInResultModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(CddbSearchStringInResultCombo),MISC_COMBO_TEXT);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSearchStringInResultCombo,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringInResultCombo)))),"activate",
                             G_CALLBACK(Cddb_Search_String_In_Result), G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringInResultCombo)))));
    gtk_widget_set_tooltip_text(GTK_WIDGET(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringInResultCombo)))),_("Enter the words to "
        "search in the list below"));

    // History List
    Load_Cddb_Search_String_In_Result_List(CddbSearchStringInResultModel, MISC_COMBO_TEXT);

    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringInResultCombo))),"");

    CddbSearchStringInResultNextButton = Create_Button_With_Icon_And_Label(GTK_STOCK_GO_DOWN,NULL);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSearchStringInResultNextButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(CddbSearchStringInResultNextButton),GTK_RELIEF_NONE);
    g_signal_connect_swapped(G_OBJECT(CddbSearchStringInResultNextButton),"clicked", G_CALLBACK(Cddb_Search_String_In_Result), G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringInResultCombo)))));
    gtk_widget_set_tooltip_text(CddbSearchStringInResultNextButton,_("Search Next"));

    CddbSearchStringInResultPrevButton = Create_Button_With_Icon_And_Label(GTK_STOCK_GO_UP,NULL);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSearchStringInResultPrevButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(CddbSearchStringInResultPrevButton),GTK_RELIEF_NONE);
    g_signal_connect_swapped(G_OBJECT(CddbSearchStringInResultPrevButton),"clicked", G_CALLBACK(Cddb_Search_String_In_Result), G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringInResultCombo)))));
    gtk_widget_set_tooltip_text(CddbSearchStringInResultPrevButton,_("Search Previous"));

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(hbox),Separator,FALSE,FALSE,2);

    CddbDisplayRedLinesButton = gtk_toggle_button_new();
    Icon = gtk_image_new_from_stock("easytag-red-lines", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(CddbDisplayRedLinesButton),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),CddbDisplayRedLinesButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(CddbDisplayRedLinesButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(CddbDisplayRedLinesButton,_("Show only red lines (or show all lines) in the 'Artist / Album' list"));
    g_signal_connect(G_OBJECT(CddbDisplayRedLinesButton),"toggled",G_CALLBACK(Cddb_Display_Red_Lines_In_Result),NULL);

    CddbUnselectAllInResultButton = Create_Button_With_Icon_And_Label("easytag-unselect-all",NULL);
    gtk_box_pack_end(GTK_BOX(hbox),CddbUnselectAllInResultButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(CddbUnselectAllInResultButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(CddbUnselectAllInResultButton,_("Unselect all lines"));
    g_signal_connect(G_OBJECT(CddbUnselectAllInResultButton),"clicked",G_CALLBACK(Cddb_Track_List_Unselect_All),NULL);

    CddbInvertSelectionInResultButton = Create_Button_With_Icon_And_Label("easytag-invert-selection",NULL);
    gtk_box_pack_end(GTK_BOX(hbox),CddbInvertSelectionInResultButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(CddbInvertSelectionInResultButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(CddbInvertSelectionInResultButton,_("Invert lines selection"));
    g_signal_connect(G_OBJECT(CddbInvertSelectionInResultButton),"clicked",G_CALLBACK(Cddb_Track_List_Invert_Selection),NULL);

    CddbSelectAllInResultButton = Create_Button_With_Icon_And_Label("easytag-select-all",NULL);
    gtk_box_pack_end(GTK_BOX(hbox),CddbSelectAllInResultButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(CddbSelectAllInResultButton),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(CddbSelectAllInResultButton,_("Select all lines"));
    g_signal_connect(G_OBJECT(CddbSelectAllInResultButton),"clicked",G_CALLBACK(Cddb_Track_List_Select_All),NULL);

    /*
     * Result of search
     */
    CddbWindowHPaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),CddbWindowHPaned,TRUE,TRUE,0);
    gtk_paned_set_position(GTK_PANED(CddbWindowHPaned),CDDB_PANE_HANDLE_POSITION);

    // List of albums
    ScrollWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindow),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(ScrollWindow),-1,100);
    gtk_paned_pack1(GTK_PANED(CddbWindowHPaned),ScrollWindow,TRUE,FALSE);

    CddbAlbumListModel = gtk_list_store_new(CDDB_ALBUM_LIST_COUNT,
                                            GDK_TYPE_PIXBUF,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING,
                                            G_TYPE_POINTER,
                                            PANGO_TYPE_STYLE,
                                            G_TYPE_INT,
                                            GDK_TYPE_COLOR);
    CddbAlbumListView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(CddbAlbumListModel));

    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes(_(CddbAlbumList_Titles[0]), renderer,
                                                      "pixbuf",         CDDB_ALBUM_LIST_PIXBUF,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(CddbAlbumListView), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_(CddbAlbumList_Titles[1]), renderer,
                                                      "text",           CDDB_ALBUM_LIST_ALBUM,
                                                      "weight",         CDDB_ALBUM_LIST_FONT_WEIGHT,
                                                      "style",          CDDB_ALBUM_LIST_FONT_STYLE,
                                                      "foreground-gdk", CDDB_ALBUM_LIST_FOREGROUND_COLOR,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(CddbAlbumListView), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_(CddbAlbumList_Titles[2]), renderer,
                                                      "text",           CDDB_ALBUM_LIST_CATEGORY,
                                                      "weight",         CDDB_ALBUM_LIST_FONT_WEIGHT,
                                                      "style",          CDDB_ALBUM_LIST_FONT_STYLE,
                                                      "foreground-gdk", CDDB_ALBUM_LIST_FOREGROUND_COLOR,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(CddbAlbumListView), column);
    //gtk_tree_view_columns_autosize(GTK_TREE_VIEW(CddbAlbumListView));

    gtk_container_add(GTK_CONTAINER(ScrollWindow), CddbAlbumListView);

    gtk_tree_view_set_cursor(GTK_TREE_VIEW(CddbAlbumListView), gtk_tree_path_new_first(), NULL, FALSE);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbAlbumListView))),
            "changed", G_CALLBACK(Cddb_Show_Album_Info), NULL);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbAlbumListView))),
            "changed", G_CALLBACK(Cddb_Get_Album_Tracks_List_CB), NULL);

    // List of tracks
    ScrollWindow = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindow),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);


    gtk_widget_set_size_request(GTK_WIDGET(ScrollWindow), -1, 100);
    gtk_paned_pack2(GTK_PANED(CddbWindowHPaned), ScrollWindow, TRUE, FALSE);

    CddbTrackListModel = gtk_list_store_new(CDDB_TRACK_LIST_COUNT,
                                            G_TYPE_UINT,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING,
                                            G_TYPE_POINTER,
                                            G_TYPE_POINTER);
    CddbTrackListView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(CddbTrackListModel));
    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL); // Align to the right
    column = gtk_tree_view_column_new_with_attributes(_(CddbTrackList_Titles[0]), renderer,
                                                      "text", CDDB_TRACK_LIST_NUMBER, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(CddbTrackListView), column);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(CddbTrackListModel), SORT_LIST_NUMBER,
                                    Cddb_Track_List_Sort_Func, GINT_TO_POINTER(SORT_LIST_NUMBER), NULL);
    gtk_tree_view_column_set_sort_column_id(column, SORT_LIST_NUMBER);
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_(CddbTrackList_Titles[1]), renderer,
                                                      "text", CDDB_TRACK_LIST_NAME, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(CddbTrackListView), column);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(CddbTrackListModel), SORT_LIST_NAME,
                                    Cddb_Track_List_Sort_Func, GINT_TO_POINTER(SORT_LIST_NAME), NULL);
    gtk_tree_view_column_set_sort_column_id(column, SORT_LIST_NAME);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL); // Align to the right
    column = gtk_tree_view_column_new_with_attributes(_(CddbTrackList_Titles[2]), renderer,
                                                      "text", CDDB_TRACK_LIST_TIME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(CddbTrackListView), column);

    //gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(CddbTrackListModel), SORT_LIST_NUMBER, GTK_SORT_ASCENDING);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(CddbTrackListView), TRUE);

    gtk_container_add(GTK_CONTAINER(ScrollWindow),CddbTrackListView);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbTrackListView)),
                                GTK_SELECTION_MULTIPLE);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbTrackListView))),
                     "changed", G_CALLBACK(Cddb_Track_List_Row_Selected), NULL);
    g_signal_connect(G_OBJECT(CddbTrackListView),"button_press_event", G_CALLBACK(Cddb_Track_List_Button_Press),NULL);
    gtk_widget_set_tooltip_text(CddbTrackListView, _("Select lines to 'apply' to "
        "your files list. All lines will be processed if no line is selected.\n"
        "You can also reorder lines in this list before using 'apply' button."));

    /*
     * Apply results to fields...
     */
    Frame = gtk_frame_new(_("Set Into:"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,TRUE,0);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox),2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);

    CddbSetToAllFields  = gtk_check_button_new_with_label(_("All"));
    Separator           = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    CddbSetToFileName   = gtk_check_button_new_with_label(_("File Name"));
    CddbSetToTitle      = gtk_check_button_new_with_label(_("Title"));
    CddbSetToArtist     = gtk_check_button_new_with_label(_("Artist"));
    CddbSetToAlbum      = gtk_check_button_new_with_label(_("Album"));
    CddbSetToYear       = gtk_check_button_new_with_label(_("Year"));
    CddbSetToTrack      = gtk_check_button_new_with_label(_("Track #"));
    CddbSetToTrackTotal = gtk_check_button_new_with_label(_("# Tracks"));
    CddbSetToGenre      = gtk_check_button_new_with_label(_("Genre"));
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSetToAllFields, FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),Separator,          FALSE,FALSE,2);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSetToFileName,  FALSE,FALSE,2);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSetToTitle,     FALSE,FALSE,2);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSetToArtist,    FALSE,FALSE,2);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSetToAlbum,     FALSE,FALSE,2);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSetToYear,      FALSE,FALSE,2);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSetToTrack,     FALSE,FALSE,2);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSetToTrackTotal,FALSE,FALSE,2);
    gtk_box_pack_start(GTK_BOX(hbox),CddbSetToGenre,     FALSE,FALSE,2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields), CDDB_SET_TO_ALL_FIELDS);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSetToTitle),     CDDB_SET_TO_TITLE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSetToArtist),    CDDB_SET_TO_ARTIST);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSetToAlbum),     CDDB_SET_TO_ALBUM);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSetToYear),      CDDB_SET_TO_YEAR);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSetToTrack),     CDDB_SET_TO_TRACK);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSetToTrackTotal),CDDB_SET_TO_TRACK_TOTAL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSetToGenre),     CDDB_SET_TO_GENRE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbSetToFileName),  CDDB_SET_TO_FILE_NAME);
    g_signal_connect(G_OBJECT(CddbSetToAllFields), "toggled",G_CALLBACK(Cddb_Set_To_All_Fields_Check_Button_Toggled),NULL);
    g_signal_connect(G_OBJECT(CddbSetToAllFields), "toggled",G_CALLBACK(Cddb_Set_Apply_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSetToTitle),     "toggled",G_CALLBACK(Cddb_Set_Apply_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSetToArtist),    "toggled",G_CALLBACK(Cddb_Set_Apply_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSetToAlbum),     "toggled",G_CALLBACK(Cddb_Set_Apply_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSetToYear),      "toggled",G_CALLBACK(Cddb_Set_Apply_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSetToTrack),     "toggled",G_CALLBACK(Cddb_Set_Apply_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSetToTrackTotal),"toggled",G_CALLBACK(Cddb_Set_Apply_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSetToGenre),     "toggled",G_CALLBACK(Cddb_Set_Apply_Button_Sensitivity),NULL);
    g_signal_connect(G_OBJECT(CddbSetToFileName),  "toggled",G_CALLBACK(Cddb_Set_Apply_Button_Sensitivity),NULL);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

    // Check box to run the scanner
    CddbRunScanner = gtk_check_button_new_with_label(_("Run the current scanner for each file"));
    gtk_box_pack_start(GTK_BOX(hbox),CddbRunScanner,FALSE,TRUE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbRunScanner),CDDB_RUN_SCANNER);
    gtk_widget_set_tooltip_text(CddbRunScanner,_("When activating this option, after loading the "
        "fields, the current selected scanner will be ran (the scanner window must be opened)."));

    // Check box to use DLM (also used in the preferences window)
    CddbUseDLM2 = gtk_check_button_new_with_label(_("Match lines with the Levenshtein algorithm"));
    gtk_box_pack_start(GTK_BOX(hbox),CddbUseDLM2,FALSE,FALSE,2);
    // Doesn't activate it by default because if the new user don't pay attention to it,
    // it will not understand why the cddb results aren't loaded correctly...
    //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbUseDLM2),CDDB_USE_DLM);
    gtk_widget_set_tooltip_text(CddbUseDLM2,_("When activating this option, the "
        "Levenshtein algorithm (DLM: Damerau-Levenshtein Metric) will be used "
        "to match the CDDB title against every file name in the current folder, "
        "and to select the best match. This will be used when selecting the "
        "corresponding audio file, or applying CDDB results, instead of using "
        "directly the position order."));
    g_signal_connect(G_OBJECT(CddbUseDLM2),"toggled",G_CALLBACK(Cddb_Use_Dlm_2_Check_Button_Toggled),NULL);

    // Button to apply
    CddbApplyButton = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    gtk_box_pack_end(GTK_BOX(hbox),CddbApplyButton,FALSE,FALSE,2);
    g_signal_connect(G_OBJECT(CddbApplyButton),"clicked", G_CALLBACK(Cddb_Set_Track_Infos_To_File_List),NULL);
    gtk_widget_set_tooltip_text(CddbApplyButton,_("Load the selected lines or all lines (if no line selected)."));

    /*
     * Status bar
     */
    CddbStatusBar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(MainVBox),CddbStatusBar,FALSE,TRUE,0);
    gtk_widget_set_size_request(CddbStatusBar, 300, -1);
    CddbStatusBarContext = gtk_statusbar_get_context_id(GTK_STATUSBAR(CddbStatusBar),"Messages");
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,_("Ready to search…"));


    g_signal_emit_by_name(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringCombo)))),"changed");
    g_signal_emit_by_name(G_OBJECT(CddbSearchInAllFields),"toggled");
    g_signal_emit_by_name(G_OBJECT(CddbSearchInAllCategories),"toggled");
    g_signal_emit_by_name(G_OBJECT(CddbSetToAllFields),"toggled");
    CddbStopSearch = FALSE;

    gtk_widget_show_all(CddbWindow);
    if (SET_CDDB_WINDOW_POSITION
    && CDDB_WINDOW_X > 0 && CDDB_WINDOW_Y > 0)
    {
        gtk_window_move(GTK_WINDOW(CddbWindow),CDDB_WINDOW_X,CDDB_WINDOW_Y);
    }
    // Force resize window
    gtk_widget_get_allocation(GTK_WIDGET(CddbSearchInAllCategories), &allocation);
    gtk_widget_set_size_request(GTK_WIDGET(CddbSearchInAllFields), allocation.width, -1);
    g_signal_emit_by_name(G_OBJECT(CddbShowCategoriesButton),"toggled");

    g_signal_connect(G_OBJECT(CddbNoteBook),"switch-page",G_CALLBACK(Cddb_Notebook_Switch_Page),NULL);
    //g_signal_emit_by_name(G_OBJECT(CddbNoteBook),"switch-page"); // Cause crash... => the 2 following lines to fix
    gtk_notebook_set_current_page(GTK_NOTEBOOK(CddbNoteBook),1);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(CddbNoteBook),0);
}

static gboolean
Cddb_Destroy_Window (GtkWidget *widget, GdkEvent *event, gpointer data)
{

    CddbStopSearch = TRUE;
    if (CddbWindow)
    {
        Cddb_Window_Apply_Changes();

        // FIX ME : This causes problem with memory !!
        Cddb_Free_Album_List();

        gtk_widget_destroy(CddbWindow);
        CddbWindow            = NULL;
        CddbAlbumList         = NULL;
        CddbSearchStringCombo = NULL;
        CddbSearchStringModel = NULL;
        CddbAlbumListView     = NULL;
        CddbAlbumListModel    = NULL;
        CddbTrackListView     = NULL;
        CddbTrackListModel    = NULL;
        CddbApplyButton       = NULL;
        CddbSearchButton      = NULL;
        CddbSearchAutoButton  = NULL;
    }
    return FALSE;
}

/*
 * For the configuration file...
 */
void Cddb_Window_Apply_Changes (void)
{
    if (CddbWindow)
    {
        gint x, y, width, height;
        GdkWindow *window;

        window = gtk_widget_get_window(CddbWindow);

        if ( window && gdk_window_is_visible(window) && gdk_window_get_state(window)!=GDK_WINDOW_STATE_MAXIMIZED )
        {
            // Position and Origin of the window
            gdk_window_get_root_origin(window,&x,&y);
            CDDB_WINDOW_X = x;
            CDDB_WINDOW_Y = y;
            width = gdk_window_get_width(window);
            height = gdk_window_get_height(window);
            CDDB_WINDOW_WIDTH  = width;
            CDDB_WINDOW_HEIGHT = height;

            // Handle panes position
            CDDB_PANE_HANDLE_POSITION = gtk_paned_get_position(GTK_PANED(CddbWindowHPaned));
        }

        CDDB_SEARCH_IN_ALL_FIELDS       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllFields));
        CDDB_SEARCH_IN_ARTIST_FIELD     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInArtistField));
        CDDB_SEARCH_IN_TITLE_FIELD      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInTitleField));
        CDDB_SEARCH_IN_TRACK_NAME_FIELD = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInTrackNameField));
        CDDB_SEARCH_IN_OTHER_FIELD      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInOtherField));
        CDDB_SHOW_CATEGORIES            = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbShowCategoriesButton));

        CDDB_SEARCH_IN_ALL_CATEGORIES      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories));
        CDDB_SEARCH_IN_BLUES_CATEGORY      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInBluesCategory));
        CDDB_SEARCH_IN_CLASSICAL_CATEGORY  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInClassicalCategory));
        CDDB_SEARCH_IN_COUNTRY_CATEGORY    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInCountryCategory));
        CDDB_SEARCH_IN_FOLK_CATEGORY       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInFolkCategory));
        CDDB_SEARCH_IN_JAZZ_CATEGORY       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInJazzCategory));
        CDDB_SEARCH_IN_MISC_CATEGORY       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInMiscCategory));
        CDDB_SEARCH_IN_NEWAGE_CATEGORY     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInNewageCategory));
        CDDB_SEARCH_IN_REGGAE_CATEGORY     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInReggaeCategory));
        CDDB_SEARCH_IN_ROCK_CATEGORY       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInRockCategory));
        CDDB_SEARCH_IN_SOUNDTRACK_CATEGORY = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInSoundtrackCategory));

        CDDB_SET_TO_ALL_FIELDS  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields));
        CDDB_SET_TO_TITLE       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToTitle));
        CDDB_SET_TO_ARTIST      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToArtist));
        CDDB_SET_TO_ALBUM       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAlbum));
        CDDB_SET_TO_YEAR        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToYear));
        CDDB_SET_TO_TRACK       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToTrack));
        CDDB_SET_TO_TRACK_TOTAL = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToTrackTotal));
        CDDB_SET_TO_GENRE       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToGenre));
        CDDB_SET_TO_FILE_NAME   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToFileName));

        CDDB_RUN_SCANNER        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbRunScanner));
        CDDB_USE_DLM            = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbUseDLM2));
        CDDB_USE_LOCAL_ACCESS   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbUseLocalAccess));

        // Save combobox history lists before exit
        Save_Cddb_Search_String_List(CddbSearchStringModel, MISC_COMBO_TEXT);
        Save_Cddb_Search_String_In_Result_List(CddbSearchStringInResultModel, MISC_COMBO_TEXT);
    }
}


static gboolean
Cddb_Window_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_KEY_Escape:
                Cddb_Destroy_Window(window, event, NULL);
                break;
        }
    }
    return FALSE;
}


static void
Cddb_Search_In_All_Fields_Check_Button_Toggled (void)
{
    if (CddbSearchInAllFields)
    {
        gtk_widget_set_sensitive(CddbSearchInArtistField,   !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllFields)));
        gtk_widget_set_sensitive(CddbSearchInTitleField,    !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllFields)));
        gtk_widget_set_sensitive(CddbSearchInTrackNameField,!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllFields)));
        gtk_widget_set_sensitive(CddbSearchInOtherField,    !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllFields)));
    }
}

static void
Cddb_Show_Categories_Button_Toggled (void)
{
    if (CddbShowCategoriesButton)
    {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbShowCategoriesButton)))
        {
            gtk_widget_show(CddbSeparatorH);
            gtk_widget_show(CddbSearchInAllCategories);
            gtk_widget_show(CddbSeparatorV);
            gtk_widget_show(CddbSearchInBluesCategory);
            gtk_widget_show(CddbSearchInClassicalCategory);
            gtk_widget_show(CddbSearchInCountryCategory);
            gtk_widget_show(CddbSearchInFolkCategory);
            gtk_widget_show(CddbSearchInJazzCategory);
            gtk_widget_show(CddbSearchInMiscCategory);
            gtk_widget_show(CddbSearchInNewageCategory);
            gtk_widget_show(CddbSearchInReggaeCategory);
            gtk_widget_show(CddbSearchInRockCategory);
            gtk_widget_show(CddbSearchInSoundtrackCategory);
        }else
        {
            gtk_widget_hide(CddbSeparatorH);
            gtk_widget_hide(CddbSearchInAllCategories);
            gtk_widget_hide(CddbSeparatorV);
            gtk_widget_hide(CddbSearchInBluesCategory);
            gtk_widget_hide(CddbSearchInClassicalCategory);
            gtk_widget_hide(CddbSearchInCountryCategory);
            gtk_widget_hide(CddbSearchInFolkCategory);
            gtk_widget_hide(CddbSearchInJazzCategory);
            gtk_widget_hide(CddbSearchInMiscCategory);
            gtk_widget_hide(CddbSearchInNewageCategory);
            gtk_widget_hide(CddbSearchInReggaeCategory);
            gtk_widget_hide(CddbSearchInRockCategory);
            gtk_widget_hide(CddbSearchInSoundtrackCategory);
        }
        // Force the window to be redrawed
        gtk_widget_queue_resize(CddbWindow);
    }
}

static void
Cddb_Search_In_All_Categories_Check_Button_Toggled (void)
{
    if (CddbSearchInAllCategories)
    {
        gtk_widget_set_sensitive(CddbSearchInBluesCategory,     !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
        gtk_widget_set_sensitive(CddbSearchInClassicalCategory, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
        gtk_widget_set_sensitive(CddbSearchInCountryCategory,   !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
        gtk_widget_set_sensitive(CddbSearchInFolkCategory,      !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
        gtk_widget_set_sensitive(CddbSearchInJazzCategory,      !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
        gtk_widget_set_sensitive(CddbSearchInMiscCategory,      !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
        gtk_widget_set_sensitive(CddbSearchInNewageCategory,    !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
        gtk_widget_set_sensitive(CddbSearchInReggaeCategory,    !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
        gtk_widget_set_sensitive(CddbSearchInRockCategory,      !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
        gtk_widget_set_sensitive(CddbSearchInSoundtrackCategory,!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories)));
    }
}

static void
Cddb_Set_To_All_Fields_Check_Button_Toggled (void)
{
    if (CddbSetToAllFields)
    {
        gtk_widget_set_sensitive(CddbSetToTitle,     !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields)));
        gtk_widget_set_sensitive(CddbSetToArtist,    !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields)));
        gtk_widget_set_sensitive(CddbSetToAlbum,     !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields)));
        gtk_widget_set_sensitive(CddbSetToYear,      !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields)));
        gtk_widget_set_sensitive(CddbSetToTrack,     !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields)));
        gtk_widget_set_sensitive(CddbSetToTrackTotal,!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields)));
        gtk_widget_set_sensitive(CddbSetToGenre,     !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields)));
        gtk_widget_set_sensitive(CddbSetToFileName,  !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields)));
    }
}

static void
Cddb_Set_Apply_Button_Sensitivity (void)
{
    gboolean cddbsettoallfields, cddbsettotitle, cddbsettoartist, cddbsettoalbum,
             cddbsettoyear, cddbsettotrack, cddbsettotracktotal, cddbsettogenre, cddbsettofilename;

    // Tag fields
    cddbsettoallfields  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields));
    cddbsettotitle      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToTitle));
    cddbsettoartist     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToArtist));
    cddbsettoalbum      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAlbum));
    cddbsettoyear       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToYear));
    cddbsettotrack      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToTrack));
    cddbsettotracktotal = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToTrackTotal));
    cddbsettogenre      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToGenre));
    cddbsettofilename   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToFileName));
    if ( CddbApplyButton && gtk_tree_model_iter_n_children(GTK_TREE_MODEL(CddbTrackListModel), NULL) > 0
    && (cddbsettoallfields || cddbsettotitle || cddbsettoartist     || cddbsettoalbum || cddbsettoyear
        || cddbsettotrack  || cddbsettotracktotal || cddbsettogenre || cddbsettofilename) )
    {
        gtk_widget_set_sensitive(GTK_WIDGET(CddbApplyButton),TRUE);
    } else
    {
        gtk_widget_set_sensitive(GTK_WIDGET(CddbApplyButton),FALSE);
    }
}

static void
Cddb_Use_Dlm_2_Check_Button_Toggled (void)
{
    if (CddbUseDLM2)
    {
        CDDB_USE_DLM = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbUseDLM2));
    }
}

static void
Cddb_Set_Search_Button_Sensitivity (void)
{
    gboolean cddbinallfields, cddbinartistfield, cddbintitlefield, cddbintracknamefield, cddbinotherfield;
    gboolean cddbinallcategories, cddbinbluescategory, cddbinclassicalcategory, cddbincountrycategory,
             cddbinfolkcategory, cddbinjazzcategory, cddbinmisccategory, cddbinnewagecategory,
             cddbinreggaecategory, cddbinrockcategory, cddbinsoundtrackcategory;

    // Fields
    cddbinallfields      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllFields));
    cddbinartistfield    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInArtistField));
    cddbintitlefield     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInTitleField));
    cddbintracknamefield = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInTrackNameField));
    cddbinotherfield     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInOtherField));
    // Categories
    cddbinallcategories      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories));
    cddbinbluescategory      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInBluesCategory));
    cddbinclassicalcategory  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInClassicalCategory));
    cddbincountrycategory    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInCountryCategory));
    cddbinfolkcategory       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInFolkCategory));
    cddbinjazzcategory       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInJazzCategory));
    cddbinmisccategory       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInMiscCategory));
    cddbinnewagecategory     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInNewageCategory));
    cddbinreggaecategory     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInReggaeCategory));
    cddbinrockcategory       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInRockCategory));
    cddbinsoundtrackcategory = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInSoundtrackCategory));

    if ( CddbSearchButton && CddbSearchStringCombo && g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringCombo)))), -1) > 0
    && (cddbinallfields     || cddbinartistfield   || cddbintitlefield        || cddbintracknamefield || cddbinotherfield)
    && (cddbinallcategories || cddbinbluescategory || cddbinclassicalcategory || cddbincountrycategory
        || cddbinfolkcategory   || cddbinjazzcategory || cddbinmisccategory || cddbinnewagecategory
        || cddbinreggaecategory || cddbinrockcategory || cddbinsoundtrackcategory) )
    {
        gtk_widget_set_sensitive(GTK_WIDGET(CddbSearchButton),TRUE);
    } else
    {
        gtk_widget_set_sensitive(GTK_WIDGET(CddbSearchButton),FALSE);
    }
}

static void
Cddb_Stop_Search (void)
{
    CddbStopSearch = TRUE;
}

static void
Cddb_Notebook_Switch_Page (GtkNotebook *notebook, gpointer page,
                           guint page_num, gpointer user_data)
{
    gint page_total;
    guint page_tmp;

    // For size reasons, we display children of the current tab, and hide those
    // of others tabs => better display of notebook
    page_total = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
    for (page_tmp = 0; page_tmp < page_total; page_tmp++)
    {
        GtkWidget *frame = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_tmp); // Child of the page
        if (frame)
        {
            GtkWidget *box = gtk_bin_get_child(GTK_BIN(frame));
            if (box)
            {
                if (page_tmp == page_num)
                {
                    // Display children of page_tmp
                    gtk_widget_show(GTK_WIDGET(box));
                }else
                {
                    // Hide children of page_tmp
                    gtk_widget_hide(GTK_WIDGET(box));
                }
            }
        }
    }
}


/*
 * Searches the Cddb Album List for specific terms
 * (this is not search the remote CDDB database...)
 */
static void
Cddb_Search_String_In_Result (GtkWidget *entry, GtkButton *button)
{
    gchar *string;
    gchar  buffer[256];
    gchar *pbuffer;
    gchar *text;
    gchar *temp;
    gint   i;
    gint  rowcount;
    GtkTreeSelection* treeSelection;
    GtkTreeIter iter;
    GtkTreePath *rowpath;
    gboolean result;
    gboolean itemselected = FALSE;
    GtkTreeIter itercopy = iter;

    if (!CddbWindow || !CddbAlbumListView)
        return;

    if (!entry || !button)
        return;

    string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    if (!string || strlen(string)==0)
        return;
    temp = g_utf8_strdown(string, -1);
    g_free(string);
    string = temp;

    Add_String_To_Combo_List(CddbSearchStringInResultModel, string);

    /* Get the currently selected row into &iter and set itemselected to reflect this */
    treeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbAlbumListView));
    if (gtk_tree_selection_get_selected(treeSelection, NULL, &iter) == TRUE)
        itemselected = TRUE;

    rowcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(CddbAlbumListModel), NULL);

    if (button != GTK_BUTTON(CddbSearchStringInResultPrevButton)) /* Next result button has been clicked */
    {
        /* Search in the album list (from top to bottom) */
        if (itemselected == TRUE)
        {
            gtk_tree_selection_unselect_iter(treeSelection, &iter);
            result = gtk_tree_model_iter_next(GTK_TREE_MODEL(CddbAlbumListModel), &iter);
        } else
        {
            result = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(CddbAlbumListModel), &iter);
        }

        itercopy = iter;

        /* If list entries follow the previously selected item, loop through them looking for a match */
        if(result == TRUE)
        {
            do /* Search following results */
            {
                gtk_tree_model_get(GTK_TREE_MODEL(CddbAlbumListModel), &iter, CDDB_ALBUM_LIST_ALBUM, &text, -1);
                g_utf8_strncpy(buffer, text, 256);

                temp = g_utf8_strdown(buffer, -1);
                pbuffer = temp;

                if (pbuffer && strstr(pbuffer, string) != NULL)
                {
                    gtk_tree_selection_select_iter(treeSelection, &iter);
                    rowpath = gtk_tree_model_get_path(GTK_TREE_MODEL(CddbAlbumListModel), &iter);
                    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(CddbAlbumListView), rowpath, NULL, FALSE, 0, 0);
                    gtk_tree_path_free(rowpath);
                    g_free(text);
                    g_free(temp);
                    g_free(string);
                    return;
                }
                g_free(temp);
                g_free(text);
            } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(CddbAlbumListModel), &iter));
        }

        for (i = 0; i < rowcount; i++)
        {
            gboolean found;

            if (i == 0)
                found = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(CddbAlbumListModel), &itercopy);
            else
                found = gtk_tree_model_iter_next(GTK_TREE_MODEL(CddbAlbumListModel), &itercopy);

            if (found)
            {
                gtk_tree_model_get(GTK_TREE_MODEL(CddbAlbumListModel), &itercopy, CDDB_ALBUM_LIST_ALBUM, &text, -1);
                g_utf8_strncpy(buffer, text, 256);

                temp = g_utf8_strdown(buffer, -1);
                pbuffer = temp;

                if (pbuffer && strstr(pbuffer,string) != NULL)
                {
                    gtk_tree_selection_select_iter(treeSelection, &itercopy);
                    rowpath = gtk_tree_model_get_path(GTK_TREE_MODEL(CddbAlbumListModel), &itercopy);
                    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(CddbAlbumListView), rowpath, NULL, FALSE, 0, 0);
                    gtk_tree_path_free(rowpath);
                    g_free(text);
                    g_free(temp);
                    g_free(string);
                    return;
                }
                g_free(temp);
                g_free(text);
            }
        }
    } else
    {
        /* Previous result button */

        /* Search in the album list (from bottom/selected-item to top) */
        if (itemselected == TRUE)
        {
            rowpath = gtk_tree_model_get_path(GTK_TREE_MODEL(CddbAlbumListModel), &iter);
            gtk_tree_path_prev(rowpath);
        } else
        {
            rowpath = gtk_tree_path_new_from_indices(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(CddbAlbumListModel), NULL) - 1, -1);
        }

        do
        {
            gboolean found;

            found = gtk_tree_model_get_iter(GTK_TREE_MODEL(CddbAlbumListModel), &iter, rowpath);
            if (found)
            {
                gtk_tree_model_get(GTK_TREE_MODEL(CddbAlbumListModel), &iter, CDDB_ALBUM_LIST_ALBUM, &text, -1);
                g_utf8_strncpy(buffer,text,256);
                temp = g_utf8_strdown(buffer, -1);
                pbuffer = temp;

                if (pbuffer && strstr(pbuffer,string) != NULL)
                {
                    gtk_tree_selection_select_iter(treeSelection, &iter);
                    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(CddbAlbumListView), rowpath, NULL, FALSE, 0, 0);
                    gtk_tree_path_free(rowpath);
                    g_free(text);
                    g_free(temp);
                    g_free(string);
                    return;
                }
                g_free(temp);
                g_free(text);
            }
        } while(gtk_tree_path_prev(rowpath));
        gtk_tree_path_free(rowpath);
    }
    g_free(string);
}


/*
 * Show collected infos of the album in the status bar
 */
static void
Cddb_Show_Album_Info (GtkTreeSelection *selection, gpointer data)
{
    CddbAlbum *cddbalbum = NULL;
    gchar *msg, *duration_str;
    GtkTreeIter row;

    if (!CddbWindow)
        return;

    if (gtk_tree_selection_get_selected(selection, NULL, &row))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(CddbAlbumListModel), &row, CDDB_ALBUM_LIST_DATA, &cddbalbum, -1);
    }
    if (!cddbalbum)
        return;

    duration_str = Convert_Duration((gulong)cddbalbum->duration);
    msg = g_strdup_printf(_("Album: '%s', "
                            "artist: '%s', "
                            "length: '%s', "
                            "year: '%s', "
                            "genre: '%s', "
                            "ID: '%s'"),
                            cddbalbum->album ? cddbalbum->album : "",
                            cddbalbum->artist ? cddbalbum->artist : "",
                            duration_str,
                            cddbalbum->year ? cddbalbum->year : "",
                            cddbalbum->genre ? cddbalbum->genre : "",
                            cddbalbum->id ? cddbalbum->id : "");
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar), CddbStatusBarContext, msg);
    g_free(msg);
    g_free(duration_str);
}


/*
 * Select the corresponding file into the main file list
 */
static void
Cddb_Track_List_Row_Selected (GtkTreeSelection *selection, gpointer data)
{
    GList       *selectedRows;
    GtkTreeIter  currentFile;
    gchar       *text_path;
    ET_File    **etfile;

    // Exit if we don't have to select files in the main list
    if (!CDDB_FOLLOW_FILE)
        return;

    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    // We might be called with no rows selected
    if (g_list_length(selectedRows) == 0)
    {
        g_list_foreach(selectedRows, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selectedRows);
        return;
    }

    // Unselect files in the main list before re-selecting them...
    Browser_List_Unselect_All_Files();

    while (selectedRows)
    {
        gboolean found;

        found = gtk_tree_model_get_iter(GTK_TREE_MODEL(CddbTrackListModel), &currentFile, (GtkTreePath*)selectedRows->data);
        if (found)
        {
            if (CDDB_USE_DLM)
            {
                gtk_tree_model_get(GTK_TREE_MODEL(CddbTrackListModel), &currentFile,
                                   CDDB_TRACK_LIST_NAME, &text_path,
                                   CDDB_TRACK_LIST_ETFILE, &etfile, -1);
                *etfile = Browser_List_Select_File_By_DLM(text_path, TRUE);
            } else
            {
                text_path = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(CddbTrackListModel), &currentFile);
                Browser_List_Select_File_By_Iter_String(text_path, TRUE);
            }
            g_free(text_path);
        }

        if (!selectedRows->next) break;
        selectedRows = selectedRows->next;
    }

    g_list_foreach(selectedRows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selectedRows);
}

/*
 * Unselect all rows in the track list
 */
static void
Cddb_Track_List_Unselect_All ()
{
    GtkTreeSelection *selection;

    g_return_if_fail (CddbTrackListView != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbTrackListView));
    if (selection)
    {
        gtk_tree_selection_unselect_all(selection);
    }
}

/*
 * Select all rows in the track list
 */
static void
Cddb_Track_List_Select_All ()
{
    GtkTreeSelection *selection;

    g_return_if_fail (CddbTrackListView != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbTrackListView));
    if (selection)
    {
        gtk_tree_selection_select_all(selection);
    }
}

/*
 * Invert the selection of every row in the track list
 */
static void
Cddb_Track_List_Invert_Selection ()
{
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gboolean valid;

    g_return_if_fail (CddbTrackListView != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbTrackListView));

    if (selection)
    {
        /* Must block the select signal to avoid selecting all files (one by one) in the main list */
        g_signal_handlers_block_by_func(G_OBJECT(selection), G_CALLBACK(Cddb_Track_List_Row_Selected), NULL);

        valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(CddbTrackListModel), &iter);
        while (valid)
        {
            if (gtk_tree_selection_iter_is_selected(selection, &iter))
            {
                gtk_tree_selection_unselect_iter(selection, &iter);
            } else
            {
                gtk_tree_selection_select_iter(selection, &iter);
            }
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(CddbTrackListModel), &iter);
        }
        g_signal_handlers_unblock_by_func(G_OBJECT(selection), G_CALLBACK(Cddb_Track_List_Row_Selected), NULL);
        g_signal_emit_by_name(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbTrackListView))), "changed");
    }
}

static gboolean
Cddb_Track_List_Button_Press (GtkTreeView *treeView, GdkEventButton *event)
{
    if (!event)
        return FALSE;

    if (event->type==GDK_2BUTTON_PRESS && event->button==1)
    {
        /* Double left mouse click */
        Cddb_Track_List_Select_All();
    }
    return FALSE;
}


/*
 * To run an "automatic search" from a popup menu with the sélected files
 */
void Cddb_Popup_Menu_Search_Selected_File (void)
{
    Open_Cddb_Window();
    Cddb_Search_Album_From_Selected_Files();
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
            ret = SORTING_FILE_CASE_SENSITIVE ? strcmp(text1cp,text2cp) : strcasecmp(text1cp,text2cp);

            g_free(text1);
            g_free(text2);
            g_free(text1cp);
            g_free(text2cp);
            break;
    }

    return ret;
}

/*
 * Open a connection to "server_name" and retun the socket_id
 * On error, returns 0.
 *
 * Some help on : http://shoe.bocks.com/net/
 *                http://www.zone-h.org/files/4/socket.txt
 */
static gint
Cddb_Open_Connection (const gchar *host, gint port)
{
    gint               socket_id = 0;
    struct hostent    *hostent;
    struct sockaddr_in sockaddr;
    gint               optval = 1;
    gchar             *msg;


    if (!CddbWindow)
        return 0;

    if (!host || port <= 0)
        return 0;

    msg = g_strdup_printf(_("Resolving host '%s'…"),host);
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
    g_free(msg);
    while (gtk_events_pending())
        gtk_main_iteration();

    if ( (hostent=gethostbyname(host)) == NULL )
    {
        msg = g_strdup_printf(_("Can't resolve host '%s' (%s)."),host,g_strerror(errno));
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
        Log_Print(LOG_ERROR,"%s",msg);
        g_free(msg);
        return 0;
    }

    memset((void *)&sockaddr,0,sizeof(sockaddr)); // Initialize with zero
    memcpy(&sockaddr.sin_addr.s_addr,*(hostent->h_addr_list),sizeof(sockaddr.sin_addr.s_addr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port   = htons(port);

    // Create socket
    if( (socket_id = socket(AF_INET,SOCK_STREAM,0)) < 0 )
    {
        msg = g_strdup_printf(_("Cannot create a new socket (%s)"),g_strerror(errno));
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
        Log_Print(LOG_ERROR,"%s",msg);
        g_free(msg);
        return 0;
    }

    // FIX ME : must catch SIGPIPE?
    if ( setsockopt(socket_id,SOL_SOCKET,SO_KEEPALIVE,(gchar *)&optval,sizeof(optval)) < 0 )
    {
        Log_Print(LOG_ERROR,"Cannot set options on the newly-created socket");
    }

    // Open connection to the server
    msg = g_strdup_printf(_("Connecting to host '%s', port '%d'…"),host,port);
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
    g_free(msg);
    while (gtk_events_pending())
        gtk_main_iteration();
    if ( connect(socket_id,(struct sockaddr *)&sockaddr,sizeof(struct sockaddr_in)) < 0 )
    {
        msg = g_strdup_printf(_("Cannot connect to host '%s' (%s)"),host,g_strerror(errno));
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
        Log_Print(LOG_ERROR,"%s",msg);
        g_free(msg);
        return 0;
    }
    msg = g_strdup_printf(_("Connected to host '%s'"),host);
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
    g_free(msg);
    while (gtk_events_pending())
        gtk_main_iteration();

    return socket_id;
}


/*
 * Close the connection correcponding to the socket_id
 */
static void
Cddb_Close_Connection (gint socket_id)
{
#ifndef G_OS_WIN32
    shutdown(socket_id,SHUT_RDWR);
#endif /* !G_OS_WIN32 */
    close(socket_id);

    if (!CddbWindow)
        return;

    socket_id = 0;
    CddbStopSearch = FALSE;
}


/*
 * Read the result of the request and write it into a file.
 * And return the number of bytes read.
 *  - bytes_read=0 => no more data.
 *  - bytes_read_total : use to add bytes of severals pages... must be initialized before
 *
 * Server answser is formated like this :
 *
 * HTTP/1.1 200 OK\r\n                              }
 * Server: Apache/1.3.19 (Unix) PHP/4.0.4pl1\r\n    } "Header"
 * Connection: close\r\n                            }
 * \r\n
 * <html>\n                                         }
 * [...]                                            } "Body"
 */
static gint
Cddb_Write_Result_To_File (gint socket_id, gulong *bytes_read_total)
{
    gchar *file_path = NULL;
    FILE  *file;

    /* Cache directory was already created by Log_Print(). */
    file_path = g_build_filename (g_get_user_cache_dir (), PACKAGE_TARNAME,
                                  CDDB_RESULT_FILE, NULL);

    if ((file = fopen (file_path, "w+")) != NULL)
    {
        gchar cddb_out[MAX_STRING_LEN+1];
        gint  bytes_read = 0;

        while ( CddbWindow && !CddbStopSearch
        // Read data
        && (bytes_read = recv(socket_id,(void *)&cddb_out,MAX_STRING_LEN,0)) > 0 )
        {
            gchar *size_str;
            gchar *msg;


            // Write to file
            cddb_out[bytes_read] = 0;
            fwrite(&cddb_out,bytes_read,1,file);

            *bytes_read_total += bytes_read;

            //g_print("\nLine : %lu : %s\n",bytes_read,cddb_out);

            // Display message
            size_str = Convert_Size_1(*bytes_read_total);
            msg = g_strdup_printf(_("Receiving data (%s)…"),size_str);
            gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
            g_free(msg);
            g_free(size_str);
            while (gtk_events_pending())
                gtk_main_iteration();
        }

        fclose(file);

        if (bytes_read < 0)
        {
            Log_Print (LOG_ERROR, _("Error when reading CDDB response (%s)"),
	               g_strerror(errno));
            return -1; // Error!
        }

    } else
    {
        Log_Print (LOG_ERROR, _("Cannot create file '%s' (%s)"), file_path,
	           g_strerror(errno));
    }
    g_free(file_path);

    return 0;
}


/*
 * Read one line (of the connection) into cddb_out.
 * return  : -1 on error
 *            0 if no more line to read (EOF)
 *            1 if more lines to read
 *
 * Server answser is formated like this :
 *
 * HTTP/1.1 200 OK\r\n                              }
 * Server: Apache/1.3.19 (Unix) PHP/4.0.4pl1\r\n    } "Header"
 * Connection: close\r\n                            }
 * \r\n
 * <html>\n                                         }
 * [...]                                            } "Body"
 */
static gint
Cddb_Read_Line (FILE **file, gchar **cddb_out)
{
    gchar  buffer[MAX_STRING_LEN];
    gchar *result;
    size_t l;

    if (*file == NULL)
    {
        // Open the file for reading the first time
        gchar *file_path;

        file_path = g_build_filename (g_get_user_cache_dir (), PACKAGE_TARNAME,
                                      CDDB_RESULT_FILE, NULL);

        if ((*file = fopen (file_path, "r")) == 0)
        {
            Log_Print (LOG_ERROR, _("Cannot open file '%s' (%s)"), file_path,
                       g_strerror(errno));
            g_free (file_path);
            return -1; // Error!
        }
        g_free (file_path);
    }

    result = fgets(buffer,sizeof(buffer),*file);
    if (result != NULL)
    {
	l = strlen(buffer);
        if (l > 0 && buffer[l-1] == '\n')
            buffer[l-1] = '\0';

	// Many '\r' chars may be present
        while ((l = strlen(buffer)) > 0 && buffer[l-1] == '\r')
            buffer[l-1] = '\0';

        *cddb_out = g_strdup(buffer);
    }else
    {
        // On error, or EOF
        fclose(*file);
        *file = NULL;

        //*cddb_out = NULL;
        *cddb_out = g_strdup(""); // To avoid a crash

        return 0;
    }

    //g_print("Line read: %s\n",*cddb_out);
    return 1;
}


/*
 * Read HTTP header data : from "HTTP/1.1 200 OK" to the blank line
 */
static gint
Cddb_Read_Http_Header (FILE **file, gchar **cddb_out)
{

    // The 'file' is opened (if no error) in this function
    if ( Cddb_Read_Line(file,cddb_out) < 0 )
        return -1; // Error!

    // First line must be : "HTTP/1.1 200 OK"
    if ( !*cddb_out || strncmp("HTTP",*cddb_out,4)!=0 || strstr(*cddb_out,"200 OK")==NULL )
        return -1;

    // Read until end of the http header up to the next blank line
    while ( Cddb_Read_Line(file,cddb_out) > 0 && *cddb_out && strlen(*cddb_out) > 0 )
    {
    }

    //g_print("Http Header : %s\n",*cddb_out);
    return 1;
}

/*
 * Read CDDB header data when requesting a file (cmd=cddb+read+<album genre>+<discid>)
 * Must be read after the HTTP header :
 *
 *      HTTP/1.1 200 OK
 *      Date: Sun, 26 Nov 2006 22:37:13 GMT
 *      Server: Apache/2.0.54 (Debian GNU/Linux) mod_python/3.1.3 Python/2.3.5 PHP/4.3.10-16 proxy_html/2.4 mod_perl/1.999.21 Perl/v5.8.4
 *      Expires: Sun Nov 26 23:37:14 2006
 *      Content-Length: 1013
 *      Connection: close
 *      Content-Type: text/plain; charset=UTF-8
 *
 *      210 newage 710ed208 CD database entry follows (until terminating `.')
 *
 * Cddb Header is the line like this :
 *      210 newage 710ed208 CD database entry follows (until terminating `.')
 */
static gint
Cddb_Read_Cddb_Header (FILE **file, gchar **cddb_out)
{
    if ( Cddb_Read_Line(file,cddb_out) < 0 )
        return -1; // Error!

    // Some requests receive some strange data (arbitrary : less than 10 chars.)
    // at the beginning (2 or 3 characters)... So we read one line more...
    if ( !*cddb_out || strlen(*cddb_out) < 10 )
        if ( Cddb_Read_Line(file,cddb_out) < 0 )
            return -1; // Error!

    //g_print("Cddb Header : %s\n",*cddb_out);

    // Read the line
    // 200 - exact match
    // 210 - multiple exact matches
    // 211 - inexact match
    if ( *cddb_out == NULL
    || (strncmp(*cddb_out,"200",3)!=0
    &&  strncmp(*cddb_out,"210",3)!=0
    &&  strncmp(*cddb_out,"211",3)!=0) )
        return -1;

    return 1;
}



/*
 * Free the CddbAlbumList
 */
static gboolean
Cddb_Free_Album_List (void)
{
    g_return_val_if_fail (CddbAlbumList != NULL, FALSE);

    CddbAlbumList = g_list_last(CddbAlbumList);
    while (CddbAlbumList)
    {
        CddbAlbum *cddbalbum = CddbAlbumList->data;

        if (cddbalbum)
        {
            g_free(cddbalbum->server_name);
            g_object_unref(cddbalbum->bitmap);

            g_free(cddbalbum->artist_album);
            g_free(cddbalbum->category);
            g_free(cddbalbum->id);
            Cddb_Free_Track_Album_List(cddbalbum->track_list);

            g_free(cddbalbum->artist);
            g_free(cddbalbum->album);
            g_free(cddbalbum->genre);
            g_free(cddbalbum->year);

            g_free(cddbalbum);
            cddbalbum = NULL;
        }
        if (!CddbAlbumList->prev) break;
        CddbAlbumList = CddbAlbumList->prev;
    }

    g_list_free(CddbAlbumList);
    CddbAlbumList = NULL;
    return TRUE;
}

static gboolean
Cddb_Free_Track_Album_List (GList *track_list)
{
    GList *CddbTrackAlbumList;

    g_return_val_if_fail (track_list != NULL, FALSE);

    CddbTrackAlbumList = g_list_last(track_list);
    while (CddbTrackAlbumList)
    {
        CddbTrackAlbum *cddbtrackalbum = CddbTrackAlbumList->data;
        if (cddbtrackalbum)
        {
            g_free(cddbtrackalbum->track_name);
            g_free(cddbtrackalbum);
            cddbtrackalbum = NULL;
        }
        if (!CddbTrackAlbumList->prev) break;
        CddbTrackAlbumList = CddbTrackAlbumList->prev;
    }
    g_list_free(CddbTrackAlbumList);
    CddbTrackAlbumList = NULL;
    return TRUE;
}



/*
 * Load the CddbAlbumList into the corresponding List
 */
static void
Cddb_Load_Album_List (gboolean only_red_lines)
{
    if (CddbWindow && CddbAlbumList && CddbAlbumListView)
    {
        GtkTreeIter iter;
        GList *cddbalbumlist;

        GtkTreeSelection *selection;
        GList            *selectedRows = NULL;
        GtkTreeIter       currentIter;
        CddbAlbum        *cddbalbumSelected = NULL;

        // Memorize the current selected item
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbAlbumListView));
        selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);
        if (selectedRows)
        {
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(CddbAlbumListModel), &currentIter, (GtkTreePath*)selectedRows->data))
                gtk_tree_model_get(GTK_TREE_MODEL(CddbAlbumListModel), &currentIter,
                                   CDDB_ALBUM_LIST_DATA, &cddbalbumSelected, -1);
        }

        // Remove lines
        gtk_list_store_clear(CddbAlbumListModel);

        // Reload list following parameter 'only_red_lines'
        cddbalbumlist = g_list_first(CddbAlbumList);
        while (cddbalbumlist)
        {
            CddbAlbum *cddbalbum = cddbalbumlist->data;

            if ( (only_red_lines && cddbalbum->track_list) || !only_red_lines)
            {
                // Load the row in the list
                gtk_list_store_append(CddbAlbumListModel, &iter);
                gtk_list_store_set(CddbAlbumListModel, &iter,
                                   CDDB_ALBUM_LIST_PIXBUF,   cddbalbum->bitmap,
                                   CDDB_ALBUM_LIST_ALBUM,    cddbalbum->artist_album,
                                   CDDB_ALBUM_LIST_CATEGORY, cddbalbum->category,
                                   CDDB_ALBUM_LIST_DATA,     cddbalbum, -1);

                Cddb_Album_List_Set_Row_Appearance(&iter);

                // Select this item if it is the saved one...
                if (cddbalbum == cddbalbumSelected)
                    gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbAlbumListView)), &iter);
            }
            cddbalbumlist = cddbalbumlist->next;
        }
    }
}


/*
 * Load the CddbTrackList into the corresponding List
 */
static void
Cddb_Load_Track_Album_List (GList *track_list)
{
    GtkTreeIter iter;

    if (CddbWindow && track_list && CddbTrackListView)
    {
        GList *tracklist = g_list_first(track_list);

        // Must block the select signal of the target to avoid looping
        gtk_list_store_clear(CddbTrackListModel);
        while (tracklist)
        {
            gchar *row_text[1];
            CddbTrackAlbum *cddbtrackalbum = tracklist->data;
            ET_File **etfile;
            etfile = g_malloc0(sizeof(ET_File *));

            row_text[0] = Convert_Duration((gulong)cddbtrackalbum->duration);

            // Load the row in the list
            gtk_list_store_append(CddbTrackListModel, &iter);
            gtk_list_store_set(CddbTrackListModel, &iter,
                               CDDB_TRACK_LIST_NUMBER, cddbtrackalbum->track_number,
                               CDDB_TRACK_LIST_NAME,   cddbtrackalbum->track_name,
                               CDDB_TRACK_LIST_TIME,   row_text[0],
                               CDDB_TRACK_LIST_DATA,   cddbtrackalbum,
                               CDDB_TRACK_LIST_ETFILE, etfile,
                               -1);

            tracklist = tracklist->next;
            g_free(row_text[0]);
        }
        Cddb_Set_Apply_Button_Sensitivity();
    }
}

/*
 * Fields          : artist, title, track, rest
 * CDDB Categories : blues, classical, country, data, folk, jazz, misc, newage, reggae, rock, soundtrack
 */
static gchar *
Cddb_Generate_Request_String_With_Fields_And_Categories_Options (void)
{
    gchar string[256];
    gboolean cddbinallfields, cddbinartistfield, cddbintitlefield, cddbintracknamefield, cddbinotherfield;
    gboolean cddbinallcategories, cddbinbluescategory, cddbinclassicalcategory, cddbincountrycategory,
             cddbinfolkcategory, cddbinjazzcategory, cddbinmisccategory, cddbinnewagecategory,
             cddbinreggaecategory, cddbinrockcategory, cddbinsoundtrackcategory;

    // Init
    string[0] = 0;

    // Fields
    cddbinallfields      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllFields));
    cddbinartistfield    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInArtistField));
    cddbintitlefield     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInTitleField));
    cddbintracknamefield = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInTrackNameField));
    cddbinotherfield     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInOtherField));

    if (cddbinallfields)      strncat(string,"&allfields=YES",14);
    else                      strncat(string,"&allfields=NO",13);

    if (cddbinartistfield)    strncat(string,"&fields=artist",14);
    if (cddbintitlefield)     strncat(string,"&fields=title",13);
    if (cddbintracknamefield) strncat(string,"&fields=track",13);
    if (cddbinotherfield)     strncat(string,"&fields=rest",12);


    // Categories (warning : there is one other CDDB catogories not used here ("data"))
    cddbinallcategories      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInAllCategories));
    cddbinbluescategory      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInBluesCategory));
    cddbinclassicalcategory  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInClassicalCategory));
    cddbincountrycategory    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInCountryCategory));
    cddbinfolkcategory       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInFolkCategory));
    cddbinjazzcategory       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInJazzCategory));
    cddbinmisccategory       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInMiscCategory));
    cddbinnewagecategory     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInNewageCategory));
    cddbinreggaecategory     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInReggaeCategory));
    cddbinrockcategory       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInRockCategory));
    cddbinsoundtrackcategory = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSearchInSoundtrackCategory));

    strncat(string,"&allcats=NO",11);
    if (cddbinallcategories)
    {
        // All categories except "data"
        strncat(string,"&cats=blues&cats=classical&cats=country&cats=folk&cats=jazz"
                       "&cats=misc&cats=newage&cats=reggae&cats=rock&cats=soundtrack",119);
    }else
    {
        if (cddbinbluescategory)      strncat(string,"&cats=blues",11);
        if (cddbinclassicalcategory)  strncat(string,"&cats=classical",15);
        if (cddbincountrycategory)    strncat(string,"&cats=country",13);
        if (cddbinfolkcategory)       strncat(string,"&cats=folk",10);
        if (cddbinjazzcategory)       strncat(string,"&cats=jazz",10);
        if (cddbinmisccategory)       strncat(string,"&cats=misc",10);
        if (cddbinnewagecategory)     strncat(string,"&cats=newage",12);
        if (cddbinreggaecategory)     strncat(string,"&cats=reggae",12);
        if (cddbinrockcategory)       strncat(string,"&cats=rock",10);
        if (cddbinsoundtrackcategory) strncat(string,"&cats=soundtrack",16);
    }

    return g_strdup(string);
}


/*
 * Select the function to use according the server adress for the manual search
 *      - freedb.freedb.org
 *      - gnudb.gnudb.org
 */
static gboolean
Cddb_Search_Album_List_From_String (void)
{
    if ( strstr(CDDB_SERVER_NAME_MANUAL_SEARCH,"gnudb") != NULL )
		// Use of gnudb
        return Cddb_Search_Album_List_From_String_Gnudb();
    else
		// Use of freedb
        return Cddb_Search_Album_List_From_String_Freedb();
}


/*
 * Site FREEDB.ORG - Manual Search
 * Send request (using the HTML search page in freedb.org site) to the CD database
 * to get the list of albums matching to a string.
 */
static gboolean
Cddb_Search_Album_List_From_String_Freedb (void)
{
    gint   socket_id;
    gchar *string = NULL;
    gchar *tmp, *tmp1;
    gchar *cddb_in;         // For the request to send
    gchar *cddb_out = NULL; // Answer received
    gchar *cddb_out_tmp;
    gchar *msg;
    gchar *proxy_auth = NULL;
    gchar *cddb_server_name;
    gint   cddb_server_port;
    gchar *cddb_server_cgi_path;

    gchar *ptr_cat, *cat_str, *id_str, *art_alb_str;
    gchar *art_alb_tmp = NULL;
    gboolean use_art_alb = FALSE;
    gchar *end_str;
    gchar *html_end_str;
    gchar  buffer[MAX_STRING_LEN+1];
    gint   bytes_written;
    gulong bytes_read_total = 0;
    FILE  *file = NULL;
    gboolean web_search_disabled = FALSE;

    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,"");

    /* Get words to search... */
    string = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringCombo)))));
    if (!string || g_utf8_strlen(string, -1) <= 0)
        return FALSE;

    /* Format the string of words */
    Strip_String(string);
    /* Remove the duplicated spaces */
    while ((tmp=strstr(string,"  "))!=NULL) // Search 2 spaces
    {
        tmp1 = tmp + 1;
        while (*tmp1)
            *(tmp++) = *(tmp1++);
        *tmp = '\0';
    }

    Add_String_To_Combo_List(CddbSearchStringModel, string);

    /* Convert spaces to '+' */
    while ( (tmp=strchr(string,' '))!=NULL )
        *tmp = '+';

    cddb_server_name     = g_strdup(CDDB_SERVER_NAME_MANUAL_SEARCH);    //"www.freedb.org");
    cddb_server_port     = CDDB_SERVER_PORT_MANUAL_SEARCH;              //80;
    cddb_server_cgi_path = g_strdup(CDDB_SERVER_CGI_PATH_MANUAL_SEARCH);//"/~cddb/cddb.cgi");

    /* Connection to the server */
    if ( (socket_id=Cddb_Open_Connection(CDDB_USE_PROXY?CDDB_PROXY_NAME:cddb_server_name,
                                         CDDB_USE_PROXY?CDDB_PROXY_PORT:cddb_server_port)) <= 0 )
    {
        g_free(string);
        g_free(cddb_server_name);
	g_free(cddb_server_cgi_path);
        return FALSE;
    }

    /* Build request */
    //cddb_in = g_strdup_printf("GET http://www.freedb.org/freedb_search.php?" // In this case, problem with squid cache...
    cddb_in = g_strdup_printf("GET %s%s/freedb_search.php?"
                              "words=%s"
                              "%s"
                              "&grouping=none"
                              " HTTP/1.1\r\n"
                              "Host: %s:%d\r\n"
                              "User-Agent: %s %s\r\n"
                              "%s"
                              "Connection: close\r\n"
                              "\r\n",
                              CDDB_USE_PROXY?"http://":"", CDDB_USE_PROXY?cddb_server_name:"",  // Needed when using proxy
                              string,
                              (tmp=Cddb_Generate_Request_String_With_Fields_And_Categories_Options()),
                              cddb_server_name,cddb_server_port,
                              PACKAGE_NAME, PACKAGE_VERSION,
                              (proxy_auth=Cddb_Format_Proxy_Authentification())
                              );

    g_free(string);
    g_free(tmp);
    g_free(proxy_auth);
    //g_print("Request Cddb_Search_Album_List_From_String_Freedb : '%s'\n", cddb_in);

    // Send the request
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,_("Sending request…"));
    while (gtk_events_pending()) gtk_main_iteration();
    if ( (bytes_written=send(socket_id,cddb_in,strlen(cddb_in)+1,0)) < 0)
    {
        Log_Print(LOG_ERROR,_("Cannot send the request (%s)"),g_strerror(errno));
        Cddb_Close_Connection(socket_id);
        g_free(cddb_in);
        g_free(string);
        g_free(cddb_server_name);
        g_free(cddb_server_cgi_path);
        return FALSE;
    }
    g_free(cddb_in);


    // Delete previous album list
    gtk_list_store_clear(CddbAlbumListModel);
    gtk_list_store_clear(CddbTrackListModel);
    Cddb_Free_Album_List();
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),TRUE);


    /*
     * Read the answer
     */
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,_("Receiving data…"));
    while (gtk_events_pending())
        gtk_main_iteration();

    // Write result in a file
    if (Cddb_Write_Result_To_File(socket_id,&bytes_read_total) < 0)
    {
        msg = g_strdup(_("The server returned a bad response"));
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
        Log_Print(LOG_ERROR,"%s",msg);
        g_free(msg);
        g_free(cddb_server_name);
        g_free(cddb_server_cgi_path);
        gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
        return FALSE;
    }

    // Parse server answer : Check returned code in the first line
    if (Cddb_Read_Http_Header(&file,&cddb_out) <= 0 || !cddb_out) // Order is important!
    {
        msg = g_strdup_printf(_("The server returned a bad response: %s"),cddb_out);
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
        Log_Print(LOG_ERROR,"%s",msg);
        g_free(msg);
        g_free(cddb_out);
        g_free(cddb_server_name);
        g_free(cddb_server_cgi_path);
        gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
        if (file)
            fclose(file);
        return FALSE;
    }
    g_free(cddb_out);

    // Read other lines, and get list of matching albums
    // Composition of a line :
    //  - freedb.org
    // <a href="http://www.freedb.org/freedb_search_fmt.php?cat=rock&id=8c0f0a0b">Bob Dylan / MTV Unplugged</a><br>
    cat_str      = g_strdup("http://www.freedb.org/freedb_search_fmt.php?cat=");
    id_str       = g_strdup("&id=");
    art_alb_str  = g_strdup("\">");
    end_str      = g_strdup("</a>"); //"</a><br>");
    html_end_str = g_strdup("</body>"); // To avoid the cddb lookups to hang
    while ( CddbWindow && !CddbStopSearch
    && Cddb_Read_Line(&file,&cddb_out) > 0 )
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

            cddbalbum = g_malloc0(sizeof(CddbAlbum));


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

            CddbAlbumList = g_list_append(CddbAlbumList,cddbalbum);
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

    // Close file opened for reading lines
    if (file)
    {
        fclose(file);
        file = NULL;
    }

    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);

    // Close connection
    Cddb_Close_Connection(socket_id);

    if (web_search_disabled)
        msg = g_strdup_printf(_("Sorry, the web-based search is currently not available"));
    else
        msg = g_strdup_printf(ngettext("Found one matching album","Found %d matching albums",g_list_length(CddbAlbumList)),g_list_length(CddbAlbumList));
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
    g_free(msg);

    // Initialize the button
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbDisplayRedLinesButton),FALSE);

    // Load the albums found in the list
    Cddb_Load_Album_List(FALSE);

    return TRUE;
}


/*
 * Site GNUDB.ORG - Manual Search
 * Send request (using the HTML search page in freedb.org site) to the CD database
 * to get the list of albums matching to a string.
 */
static gboolean
Cddb_Search_Album_List_From_String_Gnudb (void)
{
    gint   socket_id;
    gchar *string = NULL;
    gchar *tmp, *tmp1;
    gchar *cddb_in;         // For the request to send
    gchar *cddb_out = NULL; // Answer received
    gchar *cddb_out_tmp;
    gchar *msg;
    gchar *proxy_auth = NULL;
    gchar *cddb_server_name;
    gint   cddb_server_port;
    gchar *cddb_server_cgi_path;

    gchar *ptr_cat, *cat_str, *art_alb_str;
    gchar *end_str;
    gchar *ptr_sraf, *sraf_str, *sraf_end_str;
    gchar *html_end_str;
    gchar  buffer[MAX_STRING_LEN+1];
    gint   bytes_written;
    gulong bytes_read_total = 0;
    FILE  *file;
    gint   num_albums = 0;
    gint   total_num_albums = 0;

    gchar *next_page = NULL;
    gint   next_page_cpt = 0;
    gboolean next_page_found;


    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,"");

    /* Get words to search... */
    string = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbSearchStringCombo)))));
    if (!string || g_utf8_strlen(string, -1) <= 0)
        return FALSE;

    /* Format the string of words */
    Strip_String(string);
    /* Remove the duplicated spaces */
    while ((tmp=strstr(string,"  "))!=NULL) // Search 2 spaces
    {
        tmp1 = tmp + 1;
        while (*tmp1)
            *(tmp++) = *(tmp1++);
        *tmp = '\0';
    }

    Add_String_To_Combo_List(CddbSearchStringModel, string);

    /* Convert spaces to '+' */
    while ( (tmp=strchr(string,' '))!=NULL )
        *tmp = '+';


    // Delete previous album list
    gtk_list_store_clear(CddbAlbumListModel);
    gtk_list_store_clear(CddbTrackListModel);
    Cddb_Free_Album_List();
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),TRUE);


    // Do a loop to load all the pages of results
    do
    {
        cddb_server_name     = g_strdup(CDDB_SERVER_NAME_MANUAL_SEARCH);    //"www.gnudb.org");
        cddb_server_port     = CDDB_SERVER_PORT_MANUAL_SEARCH;              //80;
        cddb_server_cgi_path = g_strdup(CDDB_SERVER_CGI_PATH_MANUAL_SEARCH);//"/~cddb/cddb.cgi");

        /* Connection to the server */
        if ( (socket_id=Cddb_Open_Connection(CDDB_USE_PROXY?CDDB_PROXY_NAME:cddb_server_name,
                                             CDDB_USE_PROXY?CDDB_PROXY_PORT:cddb_server_port)) <= 0 )
        {
            g_free(string);
            g_free(cddb_server_name);
            g_free(cddb_server_cgi_path);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
            return FALSE;
        }


        /* Build request */
        cddb_in = g_strdup_printf("GET %s%s/search/"
                                  "%s"
                                  "?page=%d"
                                  " HTTP/1.1\r\n"
                                  "Host: %s:%d\r\n"
                                  "User-Agent: %s %s\r\n"
                                  "%s"
                                  "Connection: close\r\n"
                                  "\r\n",
                                  CDDB_USE_PROXY?"http://":"", CDDB_USE_PROXY?cddb_server_name:"",  // Needed when using proxy
                                  string,
                                  next_page_cpt,
                                  cddb_server_name,cddb_server_port,
                                  PACKAGE_NAME, PACKAGE_VERSION,
                                  (proxy_auth=Cddb_Format_Proxy_Authentification())
                                  );
        next_page_found = FALSE;
        g_free(proxy_auth);
        //g_print("Request Cddb_Search_Album_List_From_String_Gnudb : '%s'\n", cddb_in);

        // Send the request
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,_("Sending request…"));
        while (gtk_events_pending()) gtk_main_iteration();
        if ( (bytes_written=send(socket_id,cddb_in,strlen(cddb_in)+1,0)) < 0)
        {
            Log_Print(LOG_ERROR,_("Cannot send the request (%s)"),g_strerror(errno));
            Cddb_Close_Connection(socket_id);
            g_free(cddb_in);
            g_free(string);
            g_free(cddb_server_name);
            g_free(cddb_server_cgi_path);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
            return FALSE;
        }
        g_free(cddb_in);


        /*
         * Read the answer
         */
        if (total_num_albums != 0)
            msg = g_strdup_printf(_("Receiving data of page %d (album %d/%d)…"),next_page_cpt,num_albums,total_num_albums);
        else
            msg = g_strdup_printf(_("Receiving data of page %d…"),next_page_cpt);

        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
        g_free(msg);
        while (gtk_events_pending())
            gtk_main_iteration();

        // Write result in a file
        if (Cddb_Write_Result_To_File(socket_id,&bytes_read_total) < 0)
        {
            msg = g_strdup(_("The server returned a bad response"));
            gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
            Log_Print(LOG_ERROR,"%s",msg);
            g_free(msg);
            g_free(string);
            g_free(cddb_server_name);
            g_free(cddb_server_cgi_path);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
            return FALSE;
        }

        // Parse server answer : Check returned code in the first line
        file = NULL;
        if (Cddb_Read_Http_Header(&file,&cddb_out) <= 0 || !cddb_out) // Order is important!
        {
            msg = g_strdup_printf(_("The server returned a bad response: %s"),cddb_out);
            gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
            Log_Print(LOG_ERROR,"%s",msg);
            g_free(msg);
            g_free(cddb_out);
            g_free(string);
            g_free(cddb_server_name);
            g_free(cddb_server_cgi_path);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
            if (file)
                fclose(file);
            return FALSE;
        }
        g_free(cddb_out);

        // The next page if exists will contains this url :
        g_free(next_page);
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

        while ( CddbWindow && !CddbStopSearch
        && Cddb_Read_Line(&file,&cddb_out) > 0 )
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

                cddbalbum = g_malloc0(sizeof(CddbAlbum));

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

                CddbAlbumList = g_list_append(CddbAlbumList,cddbalbum);
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
                    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
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

        // Close file opened for reading lines
        if (file)
        {
            fclose(file);
            file = NULL;
        }

        // Close connection
        Cddb_Close_Connection(socket_id);

    } while (next_page_found);
    g_free(string);


    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);

    msg = g_strdup_printf(ngettext("Found one matching album","Found %d matching albums",num_albums),num_albums);
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
    g_free(msg);

    // Initialize the button
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbDisplayRedLinesButton),FALSE);

    // Load the albums found in the list
    Cddb_Load_Album_List(FALSE);

    return TRUE;
}


/*
 * Send cddb query using the CddbId generated from the selected files to get the
 * list of albums matching with this cddbid.
 */
static gboolean
Cddb_Search_Album_From_Selected_Files (void)
{
    gint   socket_id;
    gint   bytes_written;
    gulong bytes_read_total = 0;
    FILE  *file = NULL;

    gchar *cddb_in;               /* For the request to send */
    gchar *cddb_out = NULL;       /* Answer received */
    gchar *cddb_out_tmp;
    gchar *msg;
    gchar *proxy_auth;
    gchar *cddb_server_name;
    gint   cddb_server_port;
    gchar *cddb_server_cgi_path;
    gint   server_try = 0;
    gchar *tmp, *valid;
    gchar *query_string;
    gchar *cddb_discid;
    gchar *cddb_end_str;

    guint total_frames = 150;   /* First offset is (almost) always 150 */
    guint disc_length  = 2;     /* and 2s elapsed before first track */

    GtkTreeSelection *file_selection = NULL;
    guint file_selectedcount = 0;
    GtkTreeIter  currentIter;
    guint total_id;
    guint num_tracks;

    gpointer iterptr;

    GtkListStore *fileListModel;
    GtkTreeIter *fileIter;
    GList *file_iterlist = NULL;

    g_return_val_if_fail (BrowserList != NULL, FALSE);

    // Number of selected files
    fileListModel = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(BrowserList)));
    file_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    file_selectedcount = gtk_tree_selection_count_selected_rows(file_selection);

    // Create the list 'file_iterlist' of selected files (no selected files => all files selected)
    if (file_selectedcount > 0)
    {
        GList* file_selectedrows = gtk_tree_selection_get_selected_rows(file_selection, NULL);

        while (file_selectedrows)
        {
            iterptr = g_malloc0(sizeof(GtkTreeIter));
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel),
                                    (GtkTreeIter*) iterptr,
                                    (GtkTreePath*) file_selectedrows->data))
                file_iterlist = g_list_append(file_iterlist, iterptr);

            if (!file_selectedrows->next) break;
            file_selectedrows = file_selectedrows->next;
        }
        g_list_foreach(file_selectedrows, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(file_selectedrows);

    } else /* No rows selected, use the whole list */
    {
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(fileListModel), &currentIter);

        do
        {
            iterptr = g_memdup(&currentIter, sizeof(GtkTreeIter));
            file_iterlist = g_list_append(file_iterlist, iterptr);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(fileListModel), &currentIter));

        file_selectedcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(fileListModel), NULL);
    }

    if (file_selectedcount == 0)
    {
        msg = g_strdup_printf(_("No file selected"));
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
        g_free(msg);
        return TRUE;
    }else if (file_selectedcount > 99)
    {
        // The CD redbook standard defines the maximum number of tracks as 99, any
        // queries with more than 99 tracks will never return a result.
        msg = g_strdup_printf(_("More than 99 files selected. Cannot send request"));
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
        g_free(msg);
        return FALSE;
    }else
    {
        msg = g_strdup_printf(ngettext("One file selected","%d files selected",file_selectedcount),file_selectedcount);
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
        g_free(msg);
    }

    // Generate query string and compute discid from the list 'file_iterlist'
    total_id = 0;
    num_tracks = file_selectedcount;
    query_string = g_strdup("");
    while (file_iterlist)
    {
        ET_File *etfile;
        gulong secs = 0;

        fileIter = (GtkTreeIter *)file_iterlist->data;
        etfile = Browser_List_Get_ETFile_From_Iter(fileIter);

        tmp = query_string;
        if (strlen(query_string)>0)
            query_string = g_strdup_printf("%s+%d", query_string, total_frames);
        else
            query_string = g_strdup_printf("%d", total_frames);
        g_free(tmp);

        secs = etfile->ETFileInfo->duration;
        total_frames += secs * 75;
        disc_length  += secs;
        while (secs > 0)
        {
            total_id = total_id + (secs % 10);
            secs = secs / 10;
        }
        if (!file_iterlist->next) break;
        file_iterlist = file_iterlist->next;
    }
    g_list_foreach(file_iterlist, (GFunc)g_free, NULL);
    g_list_free(file_iterlist);

    // Compute CddbId
    cddb_discid = g_strdup_printf("%08x",(guint)(((total_id % 0xFF) << 24) |
                                         (disc_length << 8) | num_tracks));


    // Delete previous album list
    gtk_list_store_clear(CddbAlbumListModel);
    gtk_list_store_clear(CddbTrackListModel);
    Cddb_Free_Album_List();
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),TRUE);


    CDDB_USE_LOCAL_ACCESS = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbUseLocalAccess));
    if (CDDB_USE_LOCAL_ACCESS) // Remote or Local acces?
    {
        /*
         * Local cddb acces
         */
        static const gchar *CddbDir[] = // Or use cddb_genre_vs_id3_genre[][2]?
        {
            "blues", "classical", "country", "data",   "folk",
            "jazz",  "misc",      "newage",  "reggae", "rock",
            "soundtrack"
        };
        static const gint CddbDirSize = sizeof(CddbDir)/sizeof(CddbDir[0]) - 1 ;
        guint i;

        // We check if the file corresponding to the discid exists in each directory
        for (i=0; i<=CddbDirSize; i++)
        {
            gchar *file_path;

            if (!CDDB_LOCAL_PATH || strlen(CDDB_LOCAL_PATH)==0)
            {
                GtkWidget *msgdialog;

                msgdialog = gtk_message_dialog_new(GTK_WINDOW(CddbWindow),
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "%s",
                                                   _("The path for 'Local CD Database' was not defined"));
                /* Translators: 'it' in this sentence refers to the local CD
                 * database path. */
                gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog), "%s", _("Enter it in the preferences window before using this search."));
                gtk_window_set_title(GTK_WINDOW(msgdialog),_("Local CD search…"));

                gtk_dialog_run(GTK_DIALOG(msgdialog));
                gtk_widget_destroy(msgdialog);
                break;
            }
            file_path = g_strconcat(CDDB_LOCAL_PATH,
                                    CDDB_LOCAL_PATH[strlen(CDDB_LOCAL_PATH)-1]!=G_DIR_SEPARATOR ? G_DIR_SEPARATOR_S : "",
                                    CddbDir[i],"/",cddb_discid,NULL);

            if ( (file=fopen(file_path,"r"))!=0 )
            {
                // File found
                CddbAlbum *cddbalbum;
                gint rc = 0;

                cddbalbum = g_malloc0(sizeof(CddbAlbum));

                // Parameters of the server used (Local acces => specific!)
                cddbalbum->server_name     = NULL;                // No server name
                cddbalbum->server_port     = 0;                   // No server port
                cddbalbum->server_cgi_path = g_strdup(file_path); // File name
                cddbalbum->bitmap          = Cddb_Get_Pixbuf_From_Server_Name(file_path);

                // Get album category
                cddbalbum->category = Try_To_Validate_Utf8_String(CddbDir[i]);

                // Get album ID
                cddbalbum->id = Try_To_Validate_Utf8_String(cddb_discid);

                while ( CddbWindow && !CddbStopSearch
                && (rc = Cddb_Read_Line(&file,&cddb_out)) > 0 )
                {
                    if (!cddb_out) // Empty line?
                        continue;
                    //g_print("%s\n",cddb_out);

                    // Get Album and Artist names
                    if ( strncmp(cddb_out,"DTITLE=",7)==0 )
                    {
                        // Note : disc title too long take severals lines. For example :
                        // DTITLE=Marilyn Manson / The Nobodies (2005 Against All Gods Mix - Korea Tour L
                        // DTITLE=imited Edition)
                        if (!cddbalbum->artist_album)
                        {
                            // It is the first time we find DTITLE...

                            // Artist and album
                            cddbalbum->artist_album = Try_To_Validate_Utf8_String(cddb_out+7); // '7' to skip 'DTITLE='
                        }else
                        {
                            // It is at least the second time we find DTITLE
                            // So we suppose that only the album was truncated

                            // Album
                            valid = Try_To_Validate_Utf8_String(cddb_out+7); // '7' to skip 'DTITLE='
                            tmp = cddbalbum->artist_album; // To free...
                            cddbalbum->artist_album = g_strconcat(cddbalbum->artist_album,valid,NULL);
                            g_free(tmp);

                            // Don't need to read more data to read in the file
                            break;
                        }
                    }

                    g_free(cddb_out);
                }

                CddbAlbumList = g_list_append(CddbAlbumList,cddbalbum);

                // Need to close it, if not done in Cddb_Read_Line
                if (file)
                    fclose(file);
                file = NULL;
            }
            g_free(file_path);

        }


    }else
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
            server_try++;
            if (server_try == 1)
            {
                // 1rst try
                cddb_server_name     = g_strdup(CDDB_SERVER_NAME_AUTOMATIC_SEARCH);
                cddb_server_port     = CDDB_SERVER_PORT_AUTOMATIC_SEARCH;
                cddb_server_cgi_path = g_strdup(CDDB_SERVER_CGI_PATH_AUTOMATIC_SEARCH);
            }else
            {
                // 2sd try
                cddb_server_name     = g_strdup(CDDB_SERVER_NAME_AUTOMATIC_SEARCH2);
                cddb_server_port     = CDDB_SERVER_PORT_AUTOMATIC_SEARCH2;
                cddb_server_cgi_path = g_strdup(CDDB_SERVER_CGI_PATH_AUTOMATIC_SEARCH2);
            }

            // Check values
            if (!cddb_server_name || strcmp(cddb_server_name,"")==0)
                continue;

            // Connection to the server
            if ( (socket_id=Cddb_Open_Connection(CDDB_USE_PROXY?CDDB_PROXY_NAME:cddb_server_name,
                                                 CDDB_USE_PROXY?CDDB_PROXY_PORT:cddb_server_port)) <= 0 )
            {
                g_free(cddb_in);
                g_free(cddb_server_name);
                g_free(cddb_server_cgi_path);
                return FALSE;
            }

            // CDDB Request (ex: GET /~cddb/cddb.cgi?cmd=cddb+query+0800ac01+1++150+172&hello=noname+localhost+EasyTAG+0.31&proto=1 HTTP/1.1\r\nHost: freedb.freedb.org:80\r\nConnection: close)
            // Without proxy : "GET /~cddb/cddb.cgi?…" but doesn't work with a proxy.
            // With proxy    : "GET http://freedb.freedb.org/~cddb/cddb.cgi?…"
            // proto=1 => ISO-8859-1 - proto=6 => UTF-8
            cddb_in = g_strdup_printf("GET %s%s%s?cmd=cddb+query+"
                                      "%s+"
                                      "%d+%s+"
                                      "%d"
                                      "&hello=noname+localhost+%s+%s"
                                      "&proto=6 HTTP/1.1\r\n"
                                      "Host: %s:%d\r\n"
                                      "%s"
                                      "Connection: close\r\n\r\n",
                                      CDDB_USE_PROXY?"http://":"",CDDB_USE_PROXY?cddb_server_name:"", cddb_server_cgi_path,
                                      cddb_discid,
                                      num_tracks, query_string,
                                      disc_length,
                                      PACKAGE_NAME, PACKAGE_VERSION,
                                      cddb_server_name,cddb_server_port,
                                      (proxy_auth=Cddb_Format_Proxy_Authentification())
                                      );
            g_free(proxy_auth);
            //g_print("Request Cddb_Search_Album_From_Selected_Files : '%s'\n", cddb_in);

            msg = g_strdup_printf(_("Sending request (CddbId: %s, #tracks: %d, Disc length: %d)…"),
                                cddb_discid,num_tracks,disc_length);
            gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
            g_free(msg);

            while (gtk_events_pending())
                gtk_main_iteration();

            if ( (bytes_written=send(socket_id,cddb_in,strlen(cddb_in)+1,0)) < 0)
            {
                Log_Print(LOG_ERROR,_("Cannot send the request (%s)"),g_strerror(errno));
                Cddb_Close_Connection(socket_id);
                g_free(cddb_in);
                g_free(cddb_server_name);
                g_free(cddb_server_cgi_path);
                return FALSE;
            }
            g_free(cddb_in);


            /*
             * Read the answer
             */
            gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,_("Receiving data…"));
            while (gtk_events_pending())
                gtk_main_iteration();

            // Write result in a file
            if (Cddb_Write_Result_To_File(socket_id,&bytes_read_total) < 0)
            {
                msg = g_strdup(_("The server returned a bad response"));
                gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
                Log_Print(LOG_ERROR,"%s",msg);
                g_free(msg);
                g_free(cddb_server_name);
                g_free(cddb_server_cgi_path);
                gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
                gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
                return FALSE;
            }

            // Parse server answer : Check returned code in the first line
            file = NULL;
            if (Cddb_Read_Http_Header(&file,&cddb_out) <= 0 || !cddb_out) // Order is important!
            {
                msg = g_strdup_printf(_("The server returned a bad response: %s"),cddb_out);
                gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
                Log_Print(LOG_ERROR,"%s",msg);
                g_free(msg);
                g_free(cddb_out);
                g_free(cddb_server_name);
                g_free(cddb_server_cgi_path);
                gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
                gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
                if (file)
                    fclose(file);
                return FALSE;
            }
            g_free(cddb_out);

            cddb_end_str = g_strdup(".");

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
            while ( CddbWindow && !CddbStopSearch
            && Cddb_Read_Line(&file,&cddb_out) > 0 )
            {
                cddb_out_tmp = cddb_out;
                //g_print("%s\n",cddb_out);

                // To avoid the cddb lookups to hang (Patch from Paul Giordano)
                /* It appears that on some systems that cddb lookups continue to attempt
                 * to get data from the socket even though the other system has completed
                 * sending. The fix adds one check to the loops to see if the actual
                 * end of data is in the last block read. In this case, the last line
                 * will be a single '.'
                 */
                if ( cddb_out_tmp && strlen(cddb_out_tmp)<=3 && strstr(cddb_out_tmp,cddb_end_str)!=NULL )
                    break;

                // Compatibility for the MusicBrainz CddbGateway
                if ( cddb_out_tmp && strlen(cddb_out_tmp)>3
                &&  (strncmp(cddb_out_tmp,"200",3)==0
                ||   strncmp(cddb_out_tmp,"210",3)==0
                ||   strncmp(cddb_out_tmp,"211",3)==0) )
                    cddb_out_tmp = cddb_out_tmp + 4;

                // Reading of lines with albums (skiping return code lines :
                // "211 Found inexact matches, list follows (until terminating `.')" )
                if (cddb_out != NULL && strstr(cddb_out_tmp,"/") != NULL)
                {
                    gchar* ptr;
                    CddbAlbum *cddbalbum;

                    cddbalbum = g_malloc0(sizeof(CddbAlbum));

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

                    CddbAlbumList = g_list_append(CddbAlbumList,cddbalbum);
                }

                g_free(cddb_out);
            }
            g_free(cddb_end_str);
            g_free(cddb_server_name);
            g_free(cddb_server_cgi_path);

            // Close file opened for reading lines
            if (file)
            {
                fclose(file);
                file = NULL;
            }

            // Close connection
            Cddb_Close_Connection(socket_id);
        }

    }

    msg = g_strdup_printf(ngettext("DiscID '%s' gave one matching album","DiscID '%s' gave %d matching albums",g_list_length(CddbAlbumList)),cddb_discid,g_list_length(CddbAlbumList));
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
    g_free(msg);

    g_free(cddb_discid);
    g_free(query_string);

    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);

    // Initialize the button
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbDisplayRedLinesButton), FALSE);

    // Load the albums found in the list
    Cddb_Load_Album_List(FALSE);

    return TRUE;
}


/*
 * Callback when selecting a row in the Album List.
 * We get the list of tracks of the selected album
 */
static gboolean
Cddb_Get_Album_Tracks_List_CB (GtkTreeSelection *selection, gpointer data)
{
    gint i;
    gint i_max = 5;

    /* As may be not opened the first time (The server returned a wrong answer!)
     * me try to reconnect severals times */
    for (i = 1; i <= i_max; i++)
    {
        if ( Cddb_Get_Album_Tracks_List(selection) == TRUE )
        {
            break;
        }
    }
    if (i <= i_max)
    {
        return TRUE;
    } else
    {
        return FALSE;
    }
}

/*
 * Look up a specific album in freedb, and save to a CddbAlbum structure
 */
static gboolean
Cddb_Get_Album_Tracks_List (GtkTreeSelection* selection)
{
    gint       socket_id = 0;
    CddbAlbum *cddbalbum = NULL;
    GList     *TrackOffsetList = NULL;
    gchar     *cddb_in, *cddb_out = NULL;
    gchar     *cddb_end_str, *msg, *copy, *valid;
    gchar     *proxy_auth;
    gchar     *cddb_server_name;
    gint       cddb_server_port;
    gchar     *cddb_server_cgi_path;
    gint       bytes_written;
    gulong     bytes_read_total = 0;
    FILE      *file = NULL;
    gboolean   read_track_offset = FALSE;
    GtkTreeIter row;

    if (!CddbWindow)
        return FALSE;

    gtk_list_store_clear(CddbTrackListModel);
    Cddb_Set_Apply_Button_Sensitivity();
    if (gtk_tree_selection_get_selected(selection, NULL, &row))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(CddbAlbumListModel), &row, CDDB_ALBUM_LIST_DATA, &cddbalbum, -1);
    }
    if (!cddbalbum)
        return FALSE;

    // We have already the track list
    if (cddbalbum->track_list != NULL)
    {
        Cddb_Load_Track_Album_List(cddbalbum->track_list);
        return TRUE;
    }

    // Parameters of the server used
    cddb_server_name     = cddbalbum->server_name;
    cddb_server_port     = cddbalbum->server_port;
    cddb_server_cgi_path = cddbalbum->server_cgi_path;

    if (!cddb_server_name)
    {
        // Local access
        if ( (file=fopen(cddb_server_cgi_path,"r"))==0 )
        {
            Log_Print(LOG_ERROR,_("Can't load file: '%s' (%s)."),cddb_server_cgi_path,g_strerror(errno));
            return FALSE;
        }

    }else
    {
        // Remote access

        // Connection to the server
        if ( (socket_id=Cddb_Open_Connection(CDDB_USE_PROXY?CDDB_PROXY_NAME:cddb_server_name,
                                             CDDB_USE_PROXY?CDDB_PROXY_PORT:cddb_server_port)) <= 0 )
            return FALSE;

		if ( strstr(cddb_server_name,"gnudb") != NULL )
		{
			// For gnudb
			// New version of gnudb doesn't use a cddb request, but a http request
		    cddb_in = g_strdup_printf("GET %s%s/gnudb/"
		                              "%s/%s"
		                              " HTTP/1.1\r\n"
		                              "Host: %s:%d\r\n"
		                              "User-Agent: %s %s\r\n"
		                              "%s"
		                              "Connection: close\r\n"
		                              "\r\n",
		                              CDDB_USE_PROXY?"http://":"", CDDB_USE_PROXY?cddb_server_name:"",  // Needed when using proxy
		                              cddbalbum->category,cddbalbum->id,
		                              cddb_server_name,cddb_server_port,
		                              PACKAGE_NAME, PACKAGE_VERSION,
		                              (proxy_auth=Cddb_Format_Proxy_Authentification())
		                              );
		}else
		{
		    // CDDB Request (ex: GET /~cddb/cddb.cgi?cmd=cddb+read+jazz+0200a401&hello=noname+localhost+EasyTAG+0.31&proto=1 HTTP/1.1\r\nHost: freedb.freedb.org:80\r\nConnection: close)
		    // Without proxy : "GET /~cddb/cddb.cgi?…" but doesn't work with a proxy.
		    // With proxy    : "GET http://freedb.freedb.org/~cddb/cddb.cgi?…"
		    cddb_in = g_strdup_printf("GET %s%s%s?cmd=cddb+read+"
		                              "%s+%s"
		                              "&hello=noname+localhost+%s+%s"
		                              "&proto=6 HTTP/1.1\r\n"
		                              "Host: %s:%d\r\n"
		                              "%s"
		                              "Connection: close\r\n\r\n",
		                              CDDB_USE_PROXY?"http://":"",CDDB_USE_PROXY?cddb_server_name:"", cddb_server_cgi_path,
		                              cddbalbum->category,cddbalbum->id,
		                              PACKAGE_NAME, PACKAGE_VERSION,
		                              cddb_server_name,cddb_server_port,
		                              (proxy_auth=Cddb_Format_Proxy_Authentification())
		                              );
		}

		
		g_free(proxy_auth);
        //g_print("Request Cddb_Get_Album_Tracks_List : '%s'\n", cddb_in);

        // Send the request
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,_("Sending request…"));
        while (gtk_events_pending()) gtk_main_iteration();
        if ( (bytes_written=send(socket_id,cddb_in,strlen(cddb_in)+1,0)) < 0)
        {
            Log_Print(LOG_ERROR,_("Cannot send the request (%s)"),g_strerror(errno));
            Cddb_Close_Connection(socket_id);
            g_free(cddb_in);
            return FALSE;
        }
        g_free(cddb_in);


        // Read the answer
        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,_("Receiving data…"));
        while (gtk_events_pending())
            gtk_main_iteration();

        // Write result in a file
        if (Cddb_Write_Result_To_File(socket_id,&bytes_read_total) < 0)
        {
            msg = g_strdup(_("The server returned a bad response"));
            gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
            Log_Print(LOG_ERROR,"%s",msg);
            g_free(msg);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchButton),FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(CddbStopSearchAutoButton),FALSE);
            return FALSE;
        }


        // Parse server answer : Check HTTP Header (freedb or gnudb) and CDDB Header (freedb only)
        file = NULL;
		if ( strstr(cddb_server_name,"gnudb") != NULL )
		{
			// For gnudb (don't check CDDB header)
			if ( Cddb_Read_Http_Header(&file,&cddb_out) <= 0 )
		    {
		        gchar *msg = g_strdup_printf(_("The server returned a bad response: %s"),cddb_out);
		        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
		        Log_Print(LOG_ERROR,"%s",msg);
		        g_free(msg);
		        g_free(cddb_out);
		        if (file)
		            fclose(file);
		        return FALSE;
		    }
		}else
		{
			// For freedb
			if ( Cddb_Read_Http_Header(&file,&cddb_out) <= 0
		      || Cddb_Read_Cddb_Header(&file,&cddb_out) <= 0 )
		    {
		        gchar *msg = g_strdup_printf(_("The server returned a bad response: %s"),cddb_out);
		        gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,msg);
		        Log_Print(LOG_ERROR,"%s",msg);
		        g_free(msg);
		        g_free(cddb_out);
		        if (file)
		            fclose(file);
		        return FALSE;
		    }
		}
        g_free(cddb_out);

    }
    cddb_end_str = g_strdup(".");

    while ( CddbWindow && !CddbStopSearch
    && Cddb_Read_Line(&file,&cddb_out) > 0 )
    {
        if (!cddb_out) // Empty line?
            continue;
        //g_print("%s\n",cddb_out);

        // To avoid the cddb lookups to hang (Patch from Paul Giordano)
        /* It appears that on some systems that cddb lookups continue to attempt
         * to get data from the socket even though the other system has completed
         * sending. The fix adds one check to the loops to see if the actual
         * end of data is in the last block read. In this case, the last line
         * will be a single '.'
         */
        if (strlen(cddb_out)<=3 && strstr(cddb_out,cddb_end_str)!=NULL)
            break;

        if ( strstr(cddb_out,"Track frame offsets")!=NULL ) // We read the Track frame offset
        {
            read_track_offset = TRUE; // The next reads are for the tracks offset
            continue;

        }else if (read_track_offset) // We are reading a track offset? (generates TrackOffsetList)
        {
            if ( strtoul(cddb_out+1,NULL,10)>0 )
            {
                CddbTrackFrameOffset *cddbtrackframeoffset = g_malloc0(sizeof(CddbTrackFrameOffset));
                cddbtrackframeoffset->offset = strtoul(cddb_out+1,NULL,10);
                TrackOffsetList = g_list_append(TrackOffsetList,cddbtrackframeoffset);
            }else
            {
                read_track_offset = FALSE; // No more track offset
            }
            continue;

        }else if ( strstr(cddb_out,"Disc length: ")!=NULL ) // Length of album (in second)
        {
            cddbalbum->duration = atoi(strchr(cddb_out,':')+1);
            if (TrackOffsetList) // As it must be the last item, do nothing if no previous data
            {
                CddbTrackFrameOffset *cddbtrackframeoffset = g_malloc0(sizeof(CddbTrackFrameOffset));
                cddbtrackframeoffset->offset = cddbalbum->duration * 75; // It's the last offset
                TrackOffsetList = g_list_append(TrackOffsetList,cddbtrackframeoffset);
            }
            continue;

        }else if ( strncmp(cddb_out,"DTITLE=",7)==0 ) // "Artist / Album" names
        {
            // Note : disc title too long take severals lines. For example :
            // DTITLE=Marilyn Manson / The Nobodies (2005 Against All Gods Mix - Korea Tour L
            // DTITLE=imited Edition)
            if (!cddbalbum->album)
            {
                // It is the first time we find DTITLE...

                gchar *alb_ptr = strstr(cddb_out," / ");
                // Album
                if (alb_ptr && alb_ptr+3)
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
            continue;

        }else if ( strncmp(cddb_out,"DYEAR=",6)==0 ) // Year
        {
            valid = Try_To_Validate_Utf8_String(cddb_out+6); // '6' to skip 'DYEAR='
            if (g_utf8_strlen(valid, -1))
                cddbalbum->year = valid;
            continue;

        }else if ( strncmp(cddb_out,"DGENRE=",7)==0 ) // Genre
        {
            valid = Try_To_Validate_Utf8_String(cddb_out+7); // '7' to skip 'DGENRE='
            if (g_utf8_strlen(valid, -1))
                cddbalbum->genre = valid;
            continue;

        }else if ( strncmp(cddb_out,"TTITLE",6)==0 ) // Track title (for exemple : TTITLE10=xxxx)
        {
            CddbTrackAlbum *cddbtrackalbum_last = NULL;

            CddbTrackAlbum *cddbtrackalbum = g_malloc0(sizeof(CddbTrackAlbum));
            cddbtrackalbum->cddbalbum = cddbalbum; // To find the CddbAlbum father quickly

            // Here is a fix when TTITLExx doesn't contain an "=", we skip the line
            if ( (copy = g_utf8_strchr(cddb_out,-1,'=')) != NULL )
            {
                cddbtrackalbum->track_name = Try_To_Validate_Utf8_String(copy+1);
            }else
            {
                continue;
            }

            *g_utf8_strchr(cddb_out,-1,'=') = 0;
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

                // Frees useless allocated data previously
                g_free(cddbtrackalbum->track_name);
                g_free(cddbtrackalbum);
            }else
            {
                if (TrackOffsetList && TrackOffsetList->next)
                {
                    cddbtrackalbum->duration = ( ((CddbTrackFrameOffset *)TrackOffsetList->next->data)->offset - ((CddbTrackFrameOffset *)TrackOffsetList->data)->offset ) / 75; // Calculate time in seconds
                    TrackOffsetList = TrackOffsetList->next;
                }
                cddbalbum->track_list = g_list_append(cddbalbum->track_list,cddbtrackalbum);
            }
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
            continue;
        }

        g_free(cddb_out);
    }
    g_free(cddb_end_str);

    // Close file opened for reading lines
    if (file)
    {
        fclose(file);
        file = NULL;
    }

    if (cddb_server_name)
    {
        // Remote access

        /* Close connection */
        Cddb_Close_Connection(socket_id);
    }

    /* Set color of the selected row (without reloading the whole list) */
    Cddb_Album_List_Set_Row_Appearance(&row);

    /* Load the track list of the album */
    gtk_statusbar_push(GTK_STATUSBAR(CddbStatusBar),CddbStatusBarContext,_("Loading album track list…"));
    while (gtk_events_pending()) gtk_main_iteration();
    Cddb_Load_Track_Album_List(cddbalbum->track_list);

    Cddb_Show_Album_Info(gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbAlbumListView)),NULL);

    // Frees 'TrackOffsetList'
    TrackOffsetList = g_list_last(TrackOffsetList);
    while (TrackOffsetList)
    {
        g_free(TrackOffsetList->data);
        if (!TrackOffsetList->prev) break;
        TrackOffsetList = TrackOffsetList->prev;
    }
    g_list_free(TrackOffsetList);
    TrackOffsetList = NULL;
    return TRUE;
}

/*
 * Set the row apperance depending if we have cached info or not
 * Bold/Red = Info are already loaded, but not displayed
 * Italic/Light Red = Duplicate CDDB entry
 */
static void
Cddb_Album_List_Set_Row_Appearance (GtkTreeIter *row)
{
    CddbAlbum *cddbalbum = NULL;

    gtk_tree_model_get(GTK_TREE_MODEL(CddbAlbumListModel), row,
                       CDDB_ALBUM_LIST_DATA, &cddbalbum, -1);

    if (cddbalbum->track_list != NULL)
    {
        if (CHANGED_FILES_DISPLAYED_TO_BOLD)
        {
            gtk_list_store_set(CddbAlbumListModel, row,
                               CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                               CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_BOLD,
                               CDDB_ALBUM_LIST_FOREGROUND_COLOR, NULL,-1);
        } else
        {
            if (cddbalbum->other_version == TRUE)
            {
                gtk_list_store_set(CddbAlbumListModel, row,
                                   CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                                   CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                                   CDDB_ALBUM_LIST_FOREGROUND_COLOR, &LIGHT_RED, -1);
            } else
            {
                gtk_list_store_set(CddbAlbumListModel, row,
                                   CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                                   CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                                   CDDB_ALBUM_LIST_FOREGROUND_COLOR, &RED, -1);
            }
        }
    } else
    {
        if (cddbalbum->other_version == TRUE)
        {
            if (CHANGED_FILES_DISPLAYED_TO_BOLD)
            {
                gtk_list_store_set(CddbAlbumListModel, row,
                                   CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_ITALIC,
                                   CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                                   CDDB_ALBUM_LIST_FOREGROUND_COLOR, NULL,-1);
            } else
            {
                gtk_list_store_set(CddbAlbumListModel, row,
                                   CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                                   CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                                   CDDB_ALBUM_LIST_FOREGROUND_COLOR, &GREY, -1);
            }
        } else
        {
            gtk_list_store_set(CddbAlbumListModel, row,
                               CDDB_ALBUM_LIST_FONT_STYLE,       PANGO_STYLE_NORMAL,
                               CDDB_ALBUM_LIST_FONT_WEIGHT,      PANGO_WEIGHT_NORMAL,
                               CDDB_ALBUM_LIST_FOREGROUND_COLOR, NULL, -1);
        }
    }
}


/*
 * Set CDDB data (from tracks list) into tags of the main file list
 */
static gboolean
Cddb_Set_Track_Infos_To_File_List (void)
{
    guint row;
    guint list_length;
    guint rows_to_loop = 0;
    guint selectedcount;
    guint file_selectedcount;
    guint counter = 0;
    GList *file_iterlist = NULL;
    GList *file_selectedrows;
    GList *selectedrows = NULL;
    gchar buffer[256];
    gboolean CddbTrackList_Line_Selected;
    gboolean cddbsettoallfields, cddbsettotitle,      cddbsettoartist, cddbsettoalbum, cddbsettoyear,
             cddbsettotrack,     cddbsettotracktotal, cddbsettogenre,  cddbsettofilename;
    CddbTrackAlbum *cddbtrackalbum = NULL;
    GtkTreeSelection *selection = NULL;
    GtkTreeSelection *file_selection = NULL;
    GtkListStore *fileListModel;
    GtkTreePath *currentPath = NULL;
    GtkTreeIter  currentIter;
    GtkTreeIter *fileIter;
    gpointer iterptr;

    if (!CddbWindow || !BrowserList || !ETCore->ETFileDisplayedList)
        return FALSE;

    // Save the current displayed data
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    cddbsettoallfields  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAllFields));
    cddbsettotitle      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToTitle));
    cddbsettoartist     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToArtist));
    cddbsettoalbum      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToAlbum));
    cddbsettoyear       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToYear));
    cddbsettotrack      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToTrack));
    cddbsettotracktotal = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToTrackTotal));
    cddbsettogenre      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToGenre));
    cddbsettofilename   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbSetToFileName));

    fileListModel = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(BrowserList)));
    list_length = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(CddbTrackListModel), NULL);

    // Take the selected files in the cddb track list, else the full list
    // Note : Just used to calculate "cddb_track_list_length" because
    // "GPOINTER_TO_INT(cddb_track_list->data)" doesn't return the number of the
    // line when "cddb_track_list = g_list_first(GTK_CLIST(CddbTrackCList)->row_list)"
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(CddbTrackListView));
    selectedcount = gtk_tree_selection_count_selected_rows(selection);

    /* Check if at least one line was selected. No line selected is equal to all lines selected. */
    CddbTrackList_Line_Selected = FALSE;

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

    file_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    file_selectedcount = gtk_tree_selection_count_selected_rows(file_selection);

    if (file_selectedcount > 0)
    {
        /* Rows are selected in the file list, apply tags to them only */
        file_selectedrows = gtk_tree_selection_get_selected_rows(file_selection, NULL);

        while (file_selectedrows)
        {
            counter++;
            iterptr = g_malloc0(sizeof(GtkTreeIter));
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel),
                                   (GtkTreeIter *)iterptr,
                                   (GtkTreePath *)file_selectedrows->data))
                file_iterlist = g_list_append(file_iterlist, iterptr);

            if(!file_selectedrows->next || counter == rows_to_loop) break;
            file_selectedrows = file_selectedrows->next;
        }

        /* Free the useless bit */
        g_list_foreach(file_selectedrows, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(file_selectedrows);

    } else /* No rows selected, use the first x items in the list */
    {
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(fileListModel), &currentIter);

        do
        {
            counter++;
            iterptr = g_memdup(&currentIter, sizeof(GtkTreeIter));
            file_iterlist = g_list_append(file_iterlist, iterptr);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(fileListModel), &currentIter));

        file_selectedcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(fileListModel), NULL);
    }

    if (file_selectedcount != rows_to_loop)
    {
        GtkWidget *msgdialog;
        gint response;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(CddbWindow),
                                           GTK_DIALOG_MODAL  | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_QUESTION,
                                           GTK_BUTTONS_NONE,
                                           "%s",
                                           _("The number of CDDB results does not match the number of selected files"));
        gtk_dialog_add_buttons(GTK_DIALOG(msgdialog),GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_APPLY,GTK_RESPONSE_APPLY, NULL);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),"%s","Do you want to continue?");
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Write Tag from CDDB…"));
        response = gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);

        if (response != GTK_RESPONSE_APPLY)
        {
            g_list_foreach(file_iterlist, (GFunc)g_free, NULL);
            g_list_free(file_iterlist);
            //gdk_window_raise(CddbWindow->window);
            return FALSE;
        }
    }

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
                selectedrows = g_list_first(selectedrows);
                currentPath = (GtkTreePath *)selectedrows->data;
            } else
            {
                selectedrows = g_list_next(selectedrows);
                currentPath = (GtkTreePath *)selectedrows->data;
            }
        }

        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(CddbTrackListModel), &currentIter, currentPath))
            gtk_tree_model_get(GTK_TREE_MODEL(CddbTrackListModel), &currentIter, CDDB_TRACK_LIST_DATA, &cddbtrackalbum, -1);

        // Set values in the ETFile
        if (CDDB_USE_DLM)
        {
            // RQ : this part is ~ equal to code for '!CDDB_USE_DLM', but uses '*etfile' instead of 'etfile'
            ET_File **etfile = NULL;
            File_Name *FileName = NULL;
            File_Tag *FileTag = NULL;

            gtk_tree_model_get(GTK_TREE_MODEL(CddbTrackListModel), &currentIter,
                               CDDB_TRACK_LIST_ETFILE, &etfile, -1);

            /*
             * Tag fields
             */
            if (cddbsettoallfields || cddbsettotitle      || cddbsettotitle
            ||  cddbsettoartist    || cddbsettoalbum      || cddbsettoyear
            ||  cddbsettotrack     || cddbsettotracktotal || cddbsettogenre)
            {
                // Allocation of a new FileTag
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(*etfile,FileTag);

                if (cddbsettoallfields || cddbsettotitle)
                    ET_Set_Field_File_Tag_Item(&FileTag->title,cddbtrackalbum->track_name);

                if ( (cddbsettoallfields || cddbsettoartist) && cddbtrackalbum->cddbalbum->artist)
                    ET_Set_Field_File_Tag_Item(&FileTag->artist,cddbtrackalbum->cddbalbum->artist);

                if ( (cddbsettoallfields || cddbsettoalbum) && cddbtrackalbum->cddbalbum->album)
                    ET_Set_Field_File_Tag_Item(&FileTag->album, cddbtrackalbum->cddbalbum->album);

                if ( (cddbsettoallfields || cddbsettoyear) && cddbtrackalbum->cddbalbum->year)
                    ET_Set_Field_File_Tag_Item(&FileTag->year,  cddbtrackalbum->cddbalbum->year);

                if (cddbsettoallfields || cddbsettotrack)
                {
                    if (NUMBER_TRACK_FORMATED) snprintf(buffer,sizeof(buffer),"%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,cddbtrackalbum->track_number);
                    else                       snprintf(buffer,sizeof(buffer),"%d",  cddbtrackalbum->track_number);
                    ET_Set_Field_File_Tag_Item(&FileTag->track,buffer);
                }

                if (cddbsettoallfields || cddbsettotracktotal)
                {
                    if (NUMBER_TRACK_FORMATED) snprintf(buffer,sizeof(buffer),"%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,list_length);
                    else                       snprintf(buffer,sizeof(buffer),"%d",  list_length);
                    ET_Set_Field_File_Tag_Item(&FileTag->track_total,buffer);
                }

                if ( (cddbsettoallfields || cddbsettogenre) && (cddbtrackalbum->cddbalbum->genre || cddbtrackalbum->cddbalbum->category) )
                {
                    if (cddbtrackalbum->cddbalbum->genre && g_utf8_strlen(cddbtrackalbum->cddbalbum->genre, -1)>0)
                        ET_Set_Field_File_Tag_Item(&FileTag->genre,Cddb_Get_Id3_Genre_From_Cddb_Genre(cddbtrackalbum->cddbalbum->genre));
                    else
                        ET_Set_Field_File_Tag_Item(&FileTag->genre,Cddb_Get_Id3_Genre_From_Cddb_Genre(cddbtrackalbum->cddbalbum->category));
                }
            }

            /*
             * Filename field
             */
            if ( (cddbsettoallfields || cddbsettofilename) )
            {
                gchar *filename_generated_utf8;
                gchar *filename_new_utf8;

                // Allocation of a new FileName
                FileName = ET_File_Name_Item_New();

                // Build the filename with the path
                if (NUMBER_TRACK_FORMATED) snprintf(buffer,sizeof(buffer),"%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,cddbtrackalbum->track_number);
                else                       snprintf(buffer,sizeof(buffer),"%d",  cddbtrackalbum->track_number);

                filename_generated_utf8 = g_strconcat(buffer," - ",cddbtrackalbum->track_name,NULL);
                ET_File_Name_Convert_Character(filename_generated_utf8); // Replace invalid characters
                filename_new_utf8 = ET_File_Name_Generate(*etfile,filename_generated_utf8);

                ET_Set_Filename_File_Name_Item(FileName,filename_new_utf8,NULL);

                g_free(filename_generated_utf8);
                g_free(filename_new_utf8);
            }

            ET_Manage_Changes_Of_File_Data(*etfile,FileName,FileTag);

            // Then run current scanner if asked...
            if (ScannerWindow && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbRunScanner)) )
                Scan_Select_Mode_And_Run_Scanner(*etfile);

        } else if (cddbtrackalbum && file_iterlist && file_iterlist->data)
        {
            ET_File   *etfile;
            File_Name *FileName = NULL;
            File_Tag  *FileTag  = NULL;

            fileIter = (GtkTreeIter*) file_iterlist->data;
            etfile = Browser_List_Get_ETFile_From_Iter(fileIter);

            /*
             * Tag fields
             */
            if (cddbsettoallfields || cddbsettotitle      || cddbsettotitle
            ||  cddbsettoartist    || cddbsettoalbum      || cddbsettoyear
            ||  cddbsettotrack     || cddbsettotracktotal || cddbsettogenre)
            {
                // Allocation of a new FileTag
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(etfile,FileTag);

                if (cddbsettoallfields || cddbsettotitle)
                    ET_Set_Field_File_Tag_Item(&FileTag->title,cddbtrackalbum->track_name);

                if ( (cddbsettoallfields || cddbsettoartist) && cddbtrackalbum->cddbalbum->artist)
                    ET_Set_Field_File_Tag_Item(&FileTag->artist,cddbtrackalbum->cddbalbum->artist);

                if ( (cddbsettoallfields || cddbsettoalbum) && cddbtrackalbum->cddbalbum->album)
                    ET_Set_Field_File_Tag_Item(&FileTag->album, cddbtrackalbum->cddbalbum->album);

                if ( (cddbsettoallfields || cddbsettoyear) && cddbtrackalbum->cddbalbum->year)
                    ET_Set_Field_File_Tag_Item(&FileTag->year,  cddbtrackalbum->cddbalbum->year);

                if (cddbsettoallfields || cddbsettotrack)
                {
                    if (NUMBER_TRACK_FORMATED) snprintf(buffer,sizeof(buffer),"%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,cddbtrackalbum->track_number);
                    else                       snprintf(buffer,sizeof(buffer),"%d",  cddbtrackalbum->track_number);
                    ET_Set_Field_File_Tag_Item(&FileTag->track,buffer);
                }

                if (cddbsettoallfields || cddbsettotracktotal)
                {
                    if (NUMBER_TRACK_FORMATED) snprintf(buffer,sizeof(buffer),"%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,list_length);
                    else                       snprintf(buffer,sizeof(buffer),"%d",  list_length);
                    ET_Set_Field_File_Tag_Item(&FileTag->track_total,buffer);
                }

                if ( (cddbsettoallfields || cddbsettogenre) && (cddbtrackalbum->cddbalbum->genre || cddbtrackalbum->cddbalbum->category) )
                {
                    if (cddbtrackalbum->cddbalbum->genre && g_utf8_strlen(cddbtrackalbum->cddbalbum->genre, -1)>0)
                        ET_Set_Field_File_Tag_Item(&FileTag->genre,Cddb_Get_Id3_Genre_From_Cddb_Genre(cddbtrackalbum->cddbalbum->genre));
                    else
                        ET_Set_Field_File_Tag_Item(&FileTag->genre,Cddb_Get_Id3_Genre_From_Cddb_Genre(cddbtrackalbum->cddbalbum->category));
                }
            }

            /*
             * Filename field
             */
            if ( (cddbsettoallfields || cddbsettofilename) )
            {
                gchar *filename_generated_utf8;
                gchar *filename_new_utf8;

                // Allocation of a new FileName
                FileName = ET_File_Name_Item_New();

                // Build the filename with the path
                if (NUMBER_TRACK_FORMATED) snprintf(buffer,sizeof(buffer),"%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,cddbtrackalbum->track_number);
                else                       snprintf(buffer,sizeof(buffer),"%d",  cddbtrackalbum->track_number);

                filename_generated_utf8 = g_strconcat(buffer," - ",cddbtrackalbum->track_name,NULL);
                ET_File_Name_Convert_Character(filename_generated_utf8); // Replace invalid characters
                filename_new_utf8 = ET_File_Name_Generate(etfile,filename_generated_utf8);

                ET_Set_Filename_File_Name_Item(FileName,filename_new_utf8,NULL);

                g_free(filename_generated_utf8);
                g_free(filename_new_utf8);
            }

            ET_Manage_Changes_Of_File_Data(etfile,FileName,FileTag);

            // Then run current scanner if asked...
            if (ScannerWindow && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbRunScanner)) )
                Scan_Select_Mode_And_Run_Scanner(etfile);
        }

        if(!file_iterlist->next) break;
        file_iterlist = file_iterlist->next;
    }

    g_list_foreach(file_iterlist, (GFunc)g_free, NULL);
    g_list_free(file_iterlist);

    Browser_List_Refresh_Whole_List();
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);

    return TRUE;
}


static void
Cddb_Display_Red_Lines_In_Result (void)
{
    g_return_if_fail (CddbDisplayRedLinesButton != NULL);

    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbDisplayRedLinesButton)) )
    {
        // Show only red lines
        Cddb_Load_Album_List(TRUE);
    }else
    {
        // Show all lines
        Cddb_Load_Album_List(FALSE);
    }
}


/*
 * Returns the corresponding ID3 genre (the name, not the value)
 */
static const gchar *
Cddb_Get_Id3_Genre_From_Cddb_Genre (const gchar *cddb_genre)
{
    guint i;

    g_return_val_if_fail (cddb_genre != NULL, "");

    for (i=0; i<=CDDB_GENRE_MAX; i++)
        if (strcasecmp(cddb_genre,cddb_genre_vs_id3_genre[i][0])==0)
            return cddb_genre_vs_id3_genre[i][1];
    return cddb_genre;
}

/* Pixmaps */
#include "../pixmaps/freedb.xpm"
#include "../pixmaps/gnudb.xpm"
#include "../pixmaps/musicbrainz.xpm"
//#include "../pixmaps/closed_folder.xpm"

/*
 * Returns the pixmap to display following the server name
 */
static GdkPixbuf *
Cddb_Get_Pixbuf_From_Server_Name (const gchar *server_name)
{
    if (!server_name)
        return NULL;
    else if (strstr(server_name,"freedb.org"))
        return gdk_pixbuf_new_from_xpm_data(freedb_xpm);
    else if (strstr(server_name,"gnudb.org"))
        return gdk_pixbuf_new_from_xpm_data(gnudb_xpm);
    else if (strstr(server_name,"musicbrainz.org"))
        return gdk_pixbuf_new_from_xpm_data(musicbrainz_xpm);
    else if (strstr(server_name,"/"))
        //return gdk_pixbuf_new_from_xpm_data(closed_folder_xpm);
        return NULL;
    else
        return NULL;
}


/*
 * Function taken from gFTP.
 * The standard to Base64 encoding can be found in RFC2045
 */
/*
char *base64_encode (char *str)
{
    char *newstr, *newpos, *fillpos, *pos;
    unsigned char table[64], encode[3];
    int i, num;

    // Build table
    for (i = 0; i < 26; i++)
    {
        table[i] = 'A' + i;
        table[i + 26] = 'a' + i;
    }

    for (i = 0; i < 10; i++)
        table[i + 52] = '0' + i;

    table[62] = '+';
    table[63] = '/';


    num = strlen (str) / 3;
    if (strlen (str) % 3 > 0)
        num++;
    newstr = g_malloc (num * 4 + 1);
    newstr[num * 4] = '\0';
    newpos = newstr;

    pos = str;
    while (*pos != '\0')
    {
        memset (encode, 0, sizeof (encode));
        for (i = 0; i < 3 && *pos != '\0'; i++)
            encode[i] = *pos++;

        fillpos = newpos;
        *newpos++ = table[encode[0] >> 2];
        *newpos++ = table[(encode[0] & 3)   << 4 | encode[1] >> 4];
        *newpos++ = table[(encode[1] & 0xF) << 2 | encode[2] >> 6];
        *newpos++ = table[encode[2] & 0x3F];
        while (i < 3)
            fillpos[++i] = '=';
    }
    return (newstr);
}
*/

static gchar *
Cddb_Format_Proxy_Authentification (void)
{
    gchar *ret;

    if (CDDB_USE_PROXY &&  CDDB_PROXY_USER_NAME != NULL && *CDDB_PROXY_USER_NAME != '\0')
    {

        gchar *tempstr;
        gchar *str_encoded;

        tempstr = g_strconcat(CDDB_PROXY_USER_NAME, ":", CDDB_PROXY_USER_PASSWORD, NULL);
        //str_encoded = base64_encode(tempstr);
        base64_encode (tempstr, strlen(tempstr), &str_encoded);

        ret = g_strdup_printf("Proxy-authorization: Basic %s\r\n", str_encoded);
        g_free (str_encoded);
    }else
    {
        ret = g_strdup("");
    }
    return ret;
}
