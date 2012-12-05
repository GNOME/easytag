/* prefs.c - 2000/05/06 */
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
#include <errno.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gtk2_compat.h"
#include "prefs.h"
#include "setting.h"
#include "bar.h"
#include "misc.h"
#include "scan.h"
#include "easytag.h"
#include "browser.h"
#include "cddb.h"
#include "charset.h"

//#ifdef WIN32
//#   include "win32/win32dep.h"
//#endif


/**************
 * Prototypes *
 **************/
/* Options window */
static void OptionsWindow_Quit (void);
static void OptionsWindow_Save_Button (void);
static void OptionsWindow_Cancel_Button (void);
static gboolean OptionsWindow_Key_Press (GtkWidget *window, GdkEvent *event);
static gboolean Check_Config (void);

static void Set_Default_Comment_Check_Button_Toggled (void);
static void Number_Track_Formatted_Toggled (void);
static void Number_Track_Formatted_Spin_Button_Changed (GtkWidget *Label,
                                                        GtkWidget *SpinButton);
static void Change_Id3_Settings_Toggled (void);
static void Use_Non_Standard_Id3_Reading_Character_Set_Toggled (void);
static void Scanner_Convert_Check_Button_Toggled_1 (GtkWidget *object_rec,
                                                    GtkWidget *object_emi);
static void Cddb_Use_Proxy_Toggled (void);

static void DefaultPathToMp3_Combo_Add_String (void);
static void CddbLocalPath_Combo_Add_String (void);



/*************
 * Functions *
 *************/
void Init_OptionsWindow (void)
{
    OptionsWindow = NULL;
}

/*
 * The window for options
 */
void Open_OptionsWindow (void)
{
    GtkWidget *OptionsVBox;
    GtkWidget *ButtonBox;
    GtkWidget *Button;
    GtkWidget *Label;
    GtkWidget *Frame;
    GtkWidget *Table;
    GtkWidget *VBox, *vbox;
    GtkWidget *HBox, *hbox, *id3v1v2hbox;
    GtkWidget *Separator;
    GtkWidget *EventBox;
    gchar temp[MAX_STRING_LEN];
    gchar *path_utf8;
    gchar *program_path;

    /* Check if already opened */
    if (OptionsWindow)
    {
        gtk_window_present(GTK_WINDOW(OptionsWindow));
        return;
    }

    /* The window */
    OptionsWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    /* Config */
    gtk_window_set_position(GTK_WINDOW(OptionsWindow),GTK_WIN_POS_CENTER);
    gtk_window_set_transient_for(GTK_WINDOW(OptionsWindow),GTK_WINDOW(MainWindow));
    gtk_container_set_border_width(GTK_CONTAINER(OptionsWindow), 5);
    gtk_window_set_default_size(GTK_WINDOW(OptionsWindow),OPTIONS_WINDOW_WIDTH,OPTIONS_WINDOW_HEIGHT);
    /* Title */
    gtk_window_set_title(GTK_WINDOW(OptionsWindow),_("Preferences…"));

    /* Signals connection */
    g_signal_connect(G_OBJECT(OptionsWindow),"destroy", G_CALLBACK(OptionsWindow_Quit),NULL);
    g_signal_connect(G_OBJECT(OptionsWindow),"delete_event",G_CALLBACK(OptionsWindow_Quit),NULL);
    g_signal_connect(G_OBJECT(OptionsWindow),"key_press_event",
                     G_CALLBACK(OptionsWindow_Key_Press),NULL);

     /* Options */
     /* The vbox */
    OptionsVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_set_spacing (GTK_BOX(OptionsVBox),5);
    gtk_container_add(GTK_CONTAINER(OptionsWindow),OptionsVBox);

     /* Options NoteBook */
    OptionsNoteBook = gtk_notebook_new();
    gtk_notebook_popup_enable(GTK_NOTEBOOK(OptionsNoteBook));
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(OptionsNoteBook),TRUE);
    gtk_box_pack_start(GTK_BOX(OptionsVBox),OptionsNoteBook,TRUE,TRUE,0);



    /*
     * Browser
     */
    Label = gtk_label_new(_("Browser"));
    Frame = gtk_frame_new(_("Browser"));
    gtk_notebook_append_page(GTK_NOTEBOOK(OptionsNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame), 5);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 4);


    /* File Browser frame */
    Frame = gtk_frame_new(_("File Browser"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    /* Default directory */
    HBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(vbox),HBox,FALSE,FALSE,0);

    // Label
    Label = gtk_label_new(_("Default directory:"));
    gtk_box_pack_start(GTK_BOX(HBox),Label,FALSE,FALSE,0);

    // Combo
    if (DefaultPathModel != NULL)
        gtk_list_store_clear(DefaultPathModel);
    else
        DefaultPathModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);

    DefaultPathToMp3 = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(DefaultPathModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(DefaultPathToMp3), MISC_COMBO_TEXT);
    gtk_box_pack_start(GTK_BOX(HBox),DefaultPathToMp3,TRUE,TRUE,0);
    gtk_widget_set_size_request(DefaultPathToMp3, 400, -1);
    gtk_widget_set_tooltip_text(gtk_bin_get_child(GTK_BIN(DefaultPathToMp3)),_("Specify the directory where "
        "your files are located. This path will be loaded when EasyTAG starts without parameter."));
    g_signal_connect(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(DefaultPathToMp3)))),"activate",G_CALLBACK(DefaultPathToMp3_Combo_Add_String),NULL);
    //g_signal_connect(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(DefaultPathToMp3)))),"focus_out_event",G_CALLBACK(DefaultPathToMp3_Combo_Add_String),NULL);

    // History list
    Load_Default_Path_To_MP3_List(DefaultPathModel, MISC_COMBO_TEXT);
    // If default path hasn't been added already, add it now..
    path_utf8 = filename_to_display(DEFAULT_PATH_TO_MP3);
    Add_String_To_Combo_List(DefaultPathModel, path_utf8);
    if (path_utf8)
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(DefaultPathToMp3))), path_utf8);
    g_free(path_utf8);

    // Button browse
    Button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_box_pack_start(GTK_BOX(HBox),Button,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(File_Selection_Window_For_Directory),G_OBJECT(gtk_bin_get_child(GTK_BIN(DefaultPathToMp3))));

    /* Load directory on startup */
    LoadOnStartup = gtk_check_button_new_with_label(_("Load on startup the default directory or the directory passed as argument"));
    gtk_box_pack_start(GTK_BOX(vbox),LoadOnStartup,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(LoadOnStartup),LOAD_ON_STARTUP);
    gtk_widget_set_tooltip_text(LoadOnStartup,_("Automatically search files, when EasyTAG starts, "
        "into the default directory. Note that this path may be overridden by the parameter "
        "passed to easytag (easytag /path_to/mp3_files)."));

    /* Browse subdirectories */
    BrowseSubdir = gtk_check_button_new_with_label(_("Search subdirectories"));
    gtk_box_pack_start(GTK_BOX(vbox),BrowseSubdir,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(BrowseSubdir),BROWSE_SUBDIR);
    gtk_widget_set_tooltip_text(BrowseSubdir,_("Search subdirectories for files when reading "
        "a directory into the tree."));

    /* Open the node to show subdirectories */
    OpenSelectedBrowserNode = gtk_check_button_new_with_label(_("Show subdirectories when selecting "
        "a directory"));
    gtk_box_pack_start(GTK_BOX(vbox),OpenSelectedBrowserNode,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(OpenSelectedBrowserNode),OPEN_SELECTED_BROWSER_NODE);
    gtk_widget_set_tooltip_text(OpenSelectedBrowserNode,_("This expands the selected node into the file "
        "browser to display the sub-directories."));

    /* Browse hidden directories */
    BrowseHiddendir = gtk_check_button_new_with_label(_("Search hidden directories"));
#ifndef WIN32 /* Always true and not user modifiable on win32 */
    gtk_box_pack_start(GTK_BOX(vbox),BrowseHiddendir,FALSE,FALSE,0);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(BrowseHiddendir),BROWSE_HIDDEN_DIR);
    gtk_widget_set_tooltip_text(BrowseHiddendir,_("Search hidden directories for files "
        "(directories starting by a '.')."));



    /*
     * Misc
     */
    Label = gtk_label_new (_("Misc"));
    Frame = gtk_frame_new (_("Misc"));
    gtk_notebook_append_page (GTK_NOTEBOOK(OptionsNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame), 5);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 4);


    /* User interface */
    Frame = gtk_frame_new (_("User Interface"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    // Show header infos
    ShowHeaderInfos = gtk_check_button_new_with_label(_("Show header information of file"));
    gtk_box_pack_start(GTK_BOX(vbox),ShowHeaderInfos,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ShowHeaderInfos),SHOW_HEADER_INFO);
    gtk_widget_set_tooltip_text(ShowHeaderInfos,_("If activated, information about the file as "
        "the bitrate, the time, the size, will be displayed under the filename entry."));

    // Display color mode for changed files in list
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
    Label = gtk_label_new(_("Display changed files in list using:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    ChangedFilesDisplayedToRed = gtk_radio_button_new_with_label(NULL,_("Red color"));
    gtk_box_pack_start(GTK_BOX(hbox),ChangedFilesDisplayedToRed,FALSE,FALSE,4);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ChangedFilesDisplayedToRed),CHANGED_FILES_DISPLAYED_TO_RED);

    // Set "new" Gtk+-2.0ish black/bold style for changed items
    ChangedFilesDisplayedToBold = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(ChangedFilesDisplayedToRed)),_("Bold style"));
    gtk_box_pack_start(GTK_BOX(hbox),ChangedFilesDisplayedToBold,FALSE,FALSE,2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ChangedFilesDisplayedToBold),CHANGED_FILES_DISPLAYED_TO_BOLD);


    /* Sorting List Options */
    Frame = gtk_frame_new (_("Sorting List Options"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
    /* Sorting method */
    Label = gtk_label_new(_("Sort the file list by:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    EventBox = gtk_event_box_new();
    SortingFileCombo = gtk_combo_box_text_new();
    gtk_container_add(GTK_CONTAINER(EventBox),SortingFileCombo);
    gtk_box_pack_start(GTK_BOX(hbox),EventBox,FALSE,FALSE,2);
    gtk_widget_set_size_request(GTK_WIDGET(SortingFileCombo), 260, -1);
    gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(SortingFileCombo),2); // Two columns

    // Items of option menu
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Ascending file name"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Descending file name"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Ascending track number"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Descending track number"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Ascending creation date"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Descending creation date"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Ascending title"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Descending title"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Ascending artist"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Descending artist"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Ascending album"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Descending album"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Ascending year"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Descending year"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Ascending genre"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Descending genre"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Ascending comment"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SortingFileCombo), _("Descending comment"));

    gtk_combo_box_set_active(GTK_COMBO_BOX(SortingFileCombo), SORTING_FILE_MODE);
    gtk_widget_set_tooltip_text(EventBox,_("Select the type of file sorting "
        "when loading a directory."));

    SortingFileCaseSensitive = gtk_check_button_new_with_label(_("Case sensitive"));
#ifndef WIN32 /* Always true and not user modifiable on win32, as strncasecmp() doesn't work correctly with g_utf8_collate_key() */
    gtk_box_pack_start(GTK_BOX(hbox),SortingFileCaseSensitive,FALSE,FALSE,0);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SortingFileCaseSensitive),
        SORTING_FILE_CASE_SENSITIVE);
    gtk_widget_set_tooltip_text(SortingFileCaseSensitive,_("If activated, the "
        "sorting of the list will be dependent on the case."));

    /* Message Dialog Position */
    Frame = gtk_frame_new (_("Message Dialog Position"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    Table = gtk_table_new(2,2,FALSE);
    gtk_container_add(GTK_CONTAINER(Frame),Table);
    //gtk_table_set_row_spacings(GTK_TABLE(Table),2);
    gtk_table_set_col_spacings(GTK_TABLE(Table),4);


    MessageBoxPositionNone = gtk_radio_button_new_with_label(NULL,_("No particular position"));
    gtk_table_attach(GTK_TABLE(Table),MessageBoxPositionNone,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(MessageBoxPositionNone),
        MESSAGE_BOX_POSITION_NONE);
    gtk_widget_set_tooltip_text(MessageBoxPositionNone,_("Let the Window Manager "
        "to place the windows."));

    MessageBoxPositionCenterOnParent = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(MessageBoxPositionNone)),
        _("Center of the main window"));
    gtk_table_attach(GTK_TABLE(Table),MessageBoxPositionCenterOnParent,1,2,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(MessageBoxPositionCenterOnParent),
        MESSAGE_BOX_POSITION_CENTER_ON_PARENT);
    gtk_widget_set_tooltip_text(MessageBoxPositionCenterOnParent,_("Windows should "
        "be placed in the center of the main window."));

    MessageBoxPositionCenter = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(MessageBoxPositionNone)),
        _("Center of the screen"));
    gtk_table_attach(GTK_TABLE(Table),MessageBoxPositionCenter,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(MessageBoxPositionCenter),
        MESSAGE_BOX_POSITION_CENTER);
    gtk_widget_set_tooltip_text(MessageBoxPositionCenter,_("Windows should be placed "
        "in the center of the screen."));

    MessageBoxPositionMouse = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(MessageBoxPositionNone)),
        _("Mouse position"));
    gtk_table_attach(GTK_TABLE(Table),MessageBoxPositionMouse,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(MessageBoxPositionMouse),
        MESSAGE_BOX_POSITION_MOUSE);
    gtk_widget_set_tooltip_text(MessageBoxPositionMouse,_("Windows should be placed "
        "at the current mouse position."));

    /* File Player */
    Frame = gtk_frame_new (_("File Audio Player"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);

    // Player name with params
    if (FilePlayerModel == NULL)
        FilePlayerModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);
    else
        gtk_list_store_clear(FilePlayerModel);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
    Label = gtk_label_new (_("Player to run:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);
    FilePlayerCombo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(FilePlayerModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(FilePlayerCombo),MISC_COMBO_TEXT);
    gtk_widget_set_size_request(GTK_WIDGET(FilePlayerCombo), 300, -1);
    gtk_box_pack_start(GTK_BOX(hbox),FilePlayerCombo,FALSE,FALSE,0);
    gtk_widget_set_tooltip_text(gtk_bin_get_child(GTK_BIN(FilePlayerCombo)),_("Enter the program used to "
        "play the files. Some arguments can be passed for the program (as 'xmms -p') before "
        "to receive files as other arguments."));
    // History List
    Load_Audio_File_Player_List(FilePlayerModel, MISC_COMBO_TEXT);
    Add_String_To_Combo_List(FilePlayerModel, AUDIO_FILE_PLAYER);
    // Don't load the parameter if XMMS not found, else user can't save the preference
    if ( (program_path=Check_If_Executable_Exists(AUDIO_FILE_PLAYER)))
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(FilePlayerCombo))), AUDIO_FILE_PLAYER);
    g_free(program_path);

    // Button browse
    Button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
        G_CALLBACK(File_Selection_Window_For_File), G_OBJECT(gtk_bin_get_child(GTK_BIN(FilePlayerCombo))));

    /* Log options */
    Frame = gtk_frame_new (_("Log Options"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    // Show / hide log view
    ShowLogView = gtk_check_button_new_with_label(_("Show log view in main window"));
    gtk_box_pack_start(GTK_BOX(vbox),ShowLogView,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ShowLogView),SHOW_LOG_VIEW);
    gtk_widget_set_tooltip_text(ShowLogView,_("If activated, the log view would be "
                                            "visible in the main window."));
   
    // Max number of lines
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
    Label = gtk_label_new (_("Max number of lines:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);
    
    LogMaxLinesSpinButton = gtk_spin_button_new_with_range(10.0,1500.0,10.0);
    gtk_box_pack_start(GTK_BOX(hbox),LogMaxLinesSpinButton,FALSE,FALSE,0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(LogMaxLinesSpinButton),(gfloat)LOG_MAX_LINES);
    /* g_signal_connect(G_OBJECT(NumberTrackFormated),"toggled",G_CALLBACK(Number_Track_Formatted_Toggled),NULL);
     * g_signal_emit_by_name(G_OBJECT(NumberTrackFormated),"toggled");
       gtk_tooltips_set_tip(Tips,GTK_BIN(FilePlayerCombo)->child,_("Enter the program used to "
        "play the files. Some arguments can be passed for the program (as 'xmms -p') before "
        "to receive files as other arguments."),NULL);
*/



    /*
     * File Settings
     */
    Label = gtk_label_new (_("File Settings"));
    Frame = gtk_frame_new (_("File Settings"));
    gtk_notebook_append_page (GTK_NOTEBOOK(OptionsNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),5);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox),4);


    /* File (name) Options */
    Frame = gtk_frame_new (_("File Options"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    ReplaceIllegalCharactersInFilename = gtk_check_button_new_with_label(_("Replace illegal characters in filename (for Windows and CD-Rom)"));
    gtk_box_pack_start(GTK_BOX(vbox),ReplaceIllegalCharactersInFilename,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ReplaceIllegalCharactersInFilename),REPLACE_ILLEGAL_CHARACTERS_IN_FILENAME);
    gtk_widget_set_tooltip_text(ReplaceIllegalCharactersInFilename,_("Convert illegal characters for "
        "FAT32/16 and ISO9660 + Joliet filesystems ('\\', ':', ';', '*', '?', '\"', '<', '>', '|') "
        "of the filename to avoid problem when renaming the file. This is useful when renaming the "
        "file from the tag with the scanner."));

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
    /* Extension case (lower/upper?) */
    Label = gtk_label_new(_("Convert filename extension to:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    FilenameExtensionLowerCase = gtk_radio_button_new_with_label(NULL,_("Lower Case"));
    gtk_box_pack_start(GTK_BOX(hbox),FilenameExtensionLowerCase,FALSE,FALSE,2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FilenameExtensionLowerCase),FILENAME_EXTENSION_LOWER_CASE);
    gtk_widget_set_tooltip_text(FilenameExtensionLowerCase,_("For example, the extension will be converted to '.mp3'"));

    FilenameExtensionUpperCase = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FilenameExtensionLowerCase)),_("Upper Case"));
    gtk_box_pack_start(GTK_BOX(hbox),FilenameExtensionUpperCase,FALSE,FALSE,2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FilenameExtensionUpperCase),FILENAME_EXTENSION_UPPER_CASE);
    gtk_widget_set_tooltip_text(FilenameExtensionUpperCase,_("For example, the extension will be converted to '.MP3'"));

    FilenameExtensionNoChange = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FilenameExtensionLowerCase)),_("No Change"));
    gtk_box_pack_start(GTK_BOX(hbox),FilenameExtensionNoChange,FALSE,FALSE,2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FilenameExtensionNoChange),FILENAME_EXTENSION_NO_CHANGE);
    gtk_widget_set_tooltip_text(FilenameExtensionNoChange,_("The extension will not be converted"));

    /* Preserve modification time */
    PreserveModificationTime = gtk_check_button_new_with_label(_("Preserve modification time of the file"));
    gtk_box_pack_start(GTK_BOX(vbox),PreserveModificationTime,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreserveModificationTime),PRESERVE_MODIFICATION_TIME);
    gtk_widget_set_tooltip_text(PreserveModificationTime,_("Preserve the modification time "
        "(in file properties) when saving the file."));

    /* Change directory modification time */
    UpdateParentDirectoryModificationTime = gtk_check_button_new_with_label(_("Update modification time "
        "of the parent directory of the file (recommended when using Amarok)"));
    gtk_box_pack_start(GTK_BOX(vbox),UpdateParentDirectoryModificationTime,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(UpdateParentDirectoryModificationTime),UPDATE_PARENT_DIRECTORY_MODIFICATION_TIME);
    gtk_widget_set_tooltip_text(UpdateParentDirectoryModificationTime,_("The modification time "
        "of the parent directory of the file will be updated when saving tag the file. At the "
        "present time it is automatically done only when renaming a file.\nThis feature is "
        "interesting when using applications like Amarok. For performance reasons, they refresh "
        "file information by detecting changes of the parent directory."));


    /* Character Set for File Name */
    Frame = gtk_frame_new (_("Character Set for File Name"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    /****hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);***/

    Table = gtk_table_new(4,2,FALSE);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    //gtk_table_set_row_spacings(GTK_TABLE(Table),2);
    gtk_table_set_col_spacings(GTK_TABLE(Table),2);

    /* Rules for character set */
    Label = gtk_label_new(_("Rules to apply if some characters can't be converted to "
        "the system character encoding when writing filename:"));
    gtk_table_attach(GTK_TABLE(Table),Label,0,2,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);

    Label = gtk_label_new("    ");
    gtk_table_attach(GTK_TABLE(Table),Label,0,1,1,2,GTK_FILL,GTK_FILL,0,0);

    FilenameCharacterSetOther = gtk_radio_button_new_with_label(NULL,_("Try another "
        "character encoding"));
    gtk_table_attach(GTK_TABLE(Table),FilenameCharacterSetOther,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FilenameCharacterSetOther),FILENAME_CHARACTER_SET_OTHER);
    gtk_widget_set_tooltip_text(FilenameCharacterSetOther,_("With this option, it will "
        "try the conversion to the encoding associated to your locale (for example: "
        "ISO-8859-1 for 'fr', KOI8-R for 'ru', ISO-8859-2 for 'ro'). If it fails, it "
        "will try the character encoding ISO-8859-1."));

    FilenameCharacterSetApproximate = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FilenameCharacterSetOther)),
        _("Force using the system character encoding and activate the transliteration"));
    gtk_table_attach(GTK_TABLE(Table),FilenameCharacterSetApproximate,1,2,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FilenameCharacterSetApproximate),FILENAME_CHARACTER_SET_APPROXIMATE);
    gtk_widget_set_tooltip_text(FilenameCharacterSetApproximate,_("With this option, when "
        "a character cannot be represented in the target character set, it can be "
        "approximated through one or several similarly looking characters."));

    FilenameCharacterSetDiscard = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FilenameCharacterSetOther)),
        _("Force using the system character encoding and silently discard some characters"));
    gtk_table_attach(GTK_TABLE(Table),FilenameCharacterSetDiscard,1,2,3,4,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FilenameCharacterSetDiscard),FILENAME_CHARACTER_SET_DISCARD);
    gtk_widget_set_tooltip_text(FilenameCharacterSetDiscard,_("With this option, when "
        "a characters cannot be represented in the target character set, it will "
        "be silently discarded."));



    /*
     * Tag Settings
     */
    Label = gtk_label_new (_("Tag Settings"));
    Frame = gtk_frame_new (_("Tag Settings"));
    gtk_notebook_append_page (GTK_NOTEBOOK(OptionsNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),5);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox),4);

    /* Tag Options */
    Frame = gtk_frame_new (_("Tag Options"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    DateAutoCompletion = gtk_check_button_new_with_label(_("Auto completion of date if not complete"));
    gtk_box_pack_start(GTK_BOX(vbox),DateAutoCompletion,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(DateAutoCompletion),DATE_AUTO_COMPLETION);
    gtk_widget_set_tooltip_text(DateAutoCompletion,_("Try to complete the year field if you enter "
        "only the last numerals of the date (for instance, if the current year is 2005: "
        "5 => 2005, 4 => 2004, 6 => 1996, 95 => 1995…)."));

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

    NumberTrackFormated = gtk_check_button_new_with_label(_("Write the track field with the following number of digits:"));
    gtk_box_pack_start(GTK_BOX(hbox),NumberTrackFormated,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NumberTrackFormated),NUMBER_TRACK_FORMATED);
    gtk_widget_set_tooltip_text(NumberTrackFormated,_("If activated, the track field is written using "
        "the number '0' as padding to obtain a number with 'n' digits (for example, with two digits: '05', "
        "'09', '10'…). Else it keeps the 'raw' track value."));

    NumberTrackFormatedSpinButton = gtk_spin_button_new_with_range(2.0,6.0,1.0);
    gtk_box_pack_start(GTK_BOX(hbox),NumberTrackFormatedSpinButton,FALSE,FALSE,0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(NumberTrackFormatedSpinButton),(gfloat)NUMBER_TRACK_FORMATED_SPIN_BUTTON);
    g_signal_connect(G_OBJECT(NumberTrackFormated),"toggled",G_CALLBACK(Number_Track_Formatted_Toggled),NULL);
    g_signal_emit_by_name(G_OBJECT(NumberTrackFormated),"toggled");

    Label = gtk_label_new(""); // Label to show the example
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,4);
    g_signal_connect_swapped(G_OBJECT(NumberTrackFormatedSpinButton),"changed",G_CALLBACK(Number_Track_Formatted_Spin_Button_Changed),G_OBJECT(Label));
    g_signal_emit_by_name(G_OBJECT(NumberTrackFormatedSpinButton),"changed",NULL);

    OggTagWriteXmmsComment = gtk_check_button_new_with_label(_("Ogg Vorbis Files: write also the comment in the XMMS format"));
    gtk_box_pack_start(GTK_BOX(vbox),OggTagWriteXmmsComment,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(OggTagWriteXmmsComment),OGG_TAG_WRITE_XMMS_COMMENT);
    gtk_widget_set_tooltip_text(OggTagWriteXmmsComment,_("XMMS doesn't make use of the right way to "
        "identify a comment in Ogg Vorbis files as other apps do. In fact, this field is usually labeled "
        "with 'comment=', whereas XMMS uses only `='. Please, uncheck this option if you don't want "
        "other apps to complain about an unknown field. Comments won't be shown in XMMS, though."));

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox),Separator,FALSE,FALSE,0);

    /* Tag field focus */
    Table = gtk_table_new(2,3,FALSE);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    //gtk_table_set_row_spacings(GTK_TABLE(Table),2);
    gtk_table_set_col_spacings(GTK_TABLE(Table),2);

    Label = gtk_label_new(_("Tag field focus when switching files in list with "
        "shortcuts Page Up/Page Down:"));
    gtk_table_attach(GTK_TABLE(Table),Label,0,2,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);

    Label = gtk_label_new("    ");
    gtk_table_attach(GTK_TABLE(Table),Label,0,1,1,2,GTK_FILL,GTK_FILL,0,0);

    SetFocusToSameTagField = gtk_radio_button_new_with_label(NULL,
        _("Keep focus to the same tag field"));
    gtk_table_attach(GTK_TABLE(Table),SetFocusToSameTagField,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
    //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SetFocusToSameTagField),SET_FOCUS_TO_SAME_TAG_FIELD);

    SetFocusToFirstTagField = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(SetFocusToSameTagField)),
        _("Return focus to the first tag field (i.e. 'Title' field)"));
    gtk_table_attach(GTK_TABLE(Table),SetFocusToFirstTagField,1,2,2,3,GTK_FILL,GTK_FILL,0,0);
    //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SetFocusToFirstTagField),SET_FOCUS_TO_FIRST_TAG_FIELD);
    
    /* Tag Splitting */
    Frame = gtk_frame_new (_("Tag Splitting"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);

    Table = gtk_table_new(5,2,FALSE);
    gtk_container_add(GTK_CONTAINER(Frame),Table);
    gtk_table_set_col_spacings(GTK_TABLE(Table),2);
    
    Label = gtk_label_new(_("For Vorbis tags, selected fields will be split at dashes and saved as separate tags"));
    gtk_table_attach(GTK_TABLE(Table),Label,0,2,0,1,GTK_FILL,GTK_FILL,0,1);
    gtk_misc_set_alignment(GTK_MISC(Label),0,0.5);

    VorbisSplitFieldTitle = gtk_check_button_new_with_label(_("Title"));
    VorbisSplitFieldArtist = gtk_check_button_new_with_label(_("Artist"));
    VorbisSplitFieldAlbum = gtk_check_button_new_with_label(_("Album"));
    VorbisSplitFieldGenre = gtk_check_button_new_with_label(_("Genre"));
    VorbisSplitFieldComment = gtk_check_button_new_with_label(_("Comment"));
    VorbisSplitFieldComposer = gtk_check_button_new_with_label(_("Composer"));
    VorbisSplitFieldOrigArtist = gtk_check_button_new_with_label(_("Original artist"));

    gtk_table_attach(GTK_TABLE(Table),VorbisSplitFieldTitle,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),VorbisSplitFieldArtist,0,1,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),VorbisSplitFieldAlbum,0,1,3,4,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),VorbisSplitFieldGenre,0,1,4,5,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),VorbisSplitFieldComment,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),VorbisSplitFieldComposer,1,2,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(Table),VorbisSplitFieldOrigArtist,1,2,3,4,GTK_FILL,GTK_FILL,0,0);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(VorbisSplitFieldTitle), VORBIS_SPLIT_FIELD_TITLE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(VorbisSplitFieldArtist), VORBIS_SPLIT_FIELD_ARTIST);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(VorbisSplitFieldAlbum), VORBIS_SPLIT_FIELD_ALBUM);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(VorbisSplitFieldGenre), VORBIS_SPLIT_FIELD_GENRE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(VorbisSplitFieldComment), VORBIS_SPLIT_FIELD_COMMENT);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(VorbisSplitFieldComposer), VORBIS_SPLIT_FIELD_COMPOSER);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(VorbisSplitFieldOrigArtist), VORBIS_SPLIT_FIELD_ORIG_ARTIST);

    /*
     * ID3 Tag Settings
     */
    Label = gtk_label_new (_("ID3 Tag Settings"));
    Frame = gtk_frame_new (_("ID3 Tag Settings"));
#ifdef ENABLE_MP3
    gtk_notebook_append_page (GTK_NOTEBOOK(OptionsNoteBook),Frame,Label);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(Frame),5);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox),4);


    /* Tag Rules frame */
    Frame = gtk_frame_new (_("ID3 Tag Rules"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox),2);

    Table = gtk_table_new(3,2,FALSE);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    gtk_table_set_row_spacings(GTK_TABLE(Table),2);
    gtk_table_set_col_spacings(GTK_TABLE(Table),2);

    /* Write ID3 tags in FLAC files */
    WriteId3TagsInFlacFiles = gtk_check_button_new_with_label(_("Write ID3 tags in FLAC files (in addition to FLAC tag)"));
    gtk_table_attach(GTK_TABLE(Table),WriteId3TagsInFlacFiles,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(WriteId3TagsInFlacFiles),WRITE_ID3_TAGS_IN_FLAC_FILE);
    gtk_widget_set_tooltip_text(WriteId3TagsInFlacFiles,_("If activated, ID3 tags will be "
        "also added in the FLAC file (according the two rules above, plus the FLAC tag). "
        "Else ID3 tags will be stripped."));

    /* Strip tag when fields (managed by EasyTAG) are empty */
    StripTagWhenEmptyFields = gtk_check_button_new_with_label(_("Strip tags if all fields are set to blank"));
    gtk_table_attach(GTK_TABLE(Table),StripTagWhenEmptyFields,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(StripTagWhenEmptyFields),STRIP_TAG_WHEN_EMPTY_FIELDS);
    gtk_widget_set_tooltip_text(StripTagWhenEmptyFields,_("As ID3v2 tags may contain other data than "
        "Title, Artist, Album, Year, Track, Genre or Comment (as an attached picture, lyrics…), "
        "this option allows you to strip the whole tag when these seven standard data fields have "
        "been set to blank."));

    /* Convert old ID3v2 tag version */
    ConvertOldId3v2TagVersion = gtk_check_button_new_with_label(_("Automatically convert old ID3v2 tag versions"));
    gtk_table_attach(GTK_TABLE(Table),ConvertOldId3v2TagVersion,0,1,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ConvertOldId3v2TagVersion),CONVERT_OLD_ID3V2_TAG_VERSION);
    gtk_widget_set_tooltip_text(ConvertOldId3v2TagVersion,_("If activated, an old ID3v2 tag version (as "
        "ID3v2.2) will be updated to the ID3v2.3 version."));

    /* Use CRC32 */
    FileWritingId3v2UseCrc32 = gtk_check_button_new_with_label(_("Use CRC32"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v2UseCrc32,1,2,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v2UseCrc32),FILE_WRITING_ID3V2_USE_CRC32);
    gtk_widget_set_tooltip_text(FileWritingId3v2UseCrc32,_("Set CRC32 in the ID3v2 tags"));

    /* Use Compression */
    FileWritingId3v2UseCompression = gtk_check_button_new_with_label(_("Use Compression"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v2UseCompression,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v2UseCompression),FILE_WRITING_ID3V2_USE_COMPRESSION);
    gtk_widget_set_tooltip_text(FileWritingId3v2UseCompression,_("Set Compression in the ID3v2 tags"));
	
    /* Write Genre in text */
    FileWritingId3v2TextOnlyGenre = gtk_check_button_new_with_label(_("Write Genre in text only"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v2TextOnlyGenre,1,2,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v2TextOnlyGenre),FILE_WRITING_ID3V2_TEXT_ONLY_GENRE);
    gtk_widget_set_tooltip_text(FileWritingId3v2TextOnlyGenre,_("Don't use ID3v1 number references in genre tag. Enable this if you see numbers as genre in your music player."));	

    /* Character Set for writing ID3 tag */
    Frame = gtk_frame_new (_("Character Set for writing ID3 tags"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    id3v1v2hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,3);
    gtk_container_add(GTK_CONTAINER(Frame),id3v1v2hbox);
    gtk_container_set_border_width(GTK_CONTAINER(id3v1v2hbox), 2);

    // ID3v2 tags
    Frame = gtk_frame_new (_("ID3v2 tags"));
    gtk_box_pack_start(GTK_BOX(id3v1v2hbox),Frame,FALSE,FALSE,2);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox),2);

    Table = gtk_table_new(8,6,FALSE);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    gtk_table_set_row_spacings(GTK_TABLE(Table),2);
    gtk_table_set_col_spacings(GTK_TABLE(Table),2);

    /* Write ID3v2 tag */
    FileWritingId3v2WriteTag = gtk_check_button_new_with_label(_("Write ID3v2 tag"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v2WriteTag,0,5,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v2WriteTag),FILE_WRITING_ID3V2_WRITE_TAG);
    gtk_widget_set_tooltip_text(FileWritingId3v2WriteTag,_("If activated, an ID3v2.4 tag will be added or "
        "updated at the beginning of the MP3 files. Else it will be stripped."));
    g_signal_connect_after(G_OBJECT(FileWritingId3v2WriteTag),"toggled",
        G_CALLBACK(Change_Id3_Settings_Toggled),NULL);

#ifdef ENABLE_ID3LIB
    /* ID3v2 tag version */
    LabelId3v2Version = gtk_label_new(_("Version:"));
    gtk_table_attach(GTK_TABLE(Table),LabelId3v2Version,0,2,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(LabelId3v2Version),0,0.5);

    EventBox = gtk_event_box_new();
    FileWritingId3v2VersionCombo = gtk_combo_box_text_new();
    gtk_container_add(GTK_CONTAINER(EventBox),FileWritingId3v2VersionCombo);
    gtk_widget_set_tooltip_text(EventBox,_("Select the ID3v2 tag version to write:\n"
        " - ID3v2.3 is written using id3lib,\n"
        " - ID3v2.4 is written using libid3tag (recommended)."));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(FileWritingId3v2VersionCombo), "ID3v2.4");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(FileWritingId3v2VersionCombo), "ID3v2.3");
    gtk_combo_box_set_active(GTK_COMBO_BOX(FileWritingId3v2VersionCombo),
        FILE_WRITING_ID3V2_VERSION_4 ? 0 : 1);
    gtk_table_attach(GTK_TABLE(Table),EventBox,2,4,1,2,GTK_FILL,GTK_FILL,0,0);
    g_signal_connect_after(G_OBJECT(FileWritingId3v2VersionCombo),"changed",
        G_CALLBACK(Change_Id3_Settings_Toggled),NULL);
#endif


    /* Charset */
    LabelId3v2Charset = gtk_label_new(_("Charset:"));
    gtk_table_attach(GTK_TABLE(Table),LabelId3v2Charset,0,5,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(LabelId3v2Charset),0,0.5);

    Label = gtk_label_new("        ");
    gtk_table_attach(GTK_TABLE(Table),Label,0,1,3,4,GTK_FILL,GTK_FILL,0,0);

    // Unicode
    FileWritingId3v2UseUnicodeCharacterSet = gtk_radio_button_new_with_label(NULL, _("Unicode "));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v2UseUnicodeCharacterSet),
        FILE_WRITING_ID3V2_USE_UNICODE_CHARACTER_SET);
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v2UseUnicodeCharacterSet,1,2,3,4,GTK_FILL,GTK_FILL,0,0);

    EventBox = gtk_event_box_new();
    FileWritingId3v2UnicodeCharacterSetCombo = gtk_combo_box_text_new();
    gtk_container_add(GTK_CONTAINER(EventBox),FileWritingId3v2UnicodeCharacterSetCombo);
    gtk_widget_set_tooltip_text(EventBox,_("Unicode type to use"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(FileWritingId3v2UnicodeCharacterSetCombo), "UTF-8");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(FileWritingId3v2UnicodeCharacterSetCombo), "UTF-16");
    if ( FILE_WRITING_ID3V2_UNICODE_CHARACTER_SET == NULL )
        gtk_combo_box_set_active(GTK_COMBO_BOX(FileWritingId3v2UnicodeCharacterSetCombo), 0); // Set UTF-8 by default
    else
        gtk_combo_box_set_active(GTK_COMBO_BOX(FileWritingId3v2UnicodeCharacterSetCombo),
            strcmp(FILE_WRITING_ID3V2_UNICODE_CHARACTER_SET, "UTF-8") ? 1 : 0);
    gtk_table_attach(GTK_TABLE(Table),EventBox,2,4,3,4,GTK_FILL,GTK_FILL,0,0);
    g_signal_connect_after(G_OBJECT(FileWritingId3v2UseUnicodeCharacterSet),"toggled",
        G_CALLBACK(Change_Id3_Settings_Toggled),NULL);

    // Non-unicode
    FileWritingId3v2UseNoUnicodeCharacterSet = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FileWritingId3v2UseUnicodeCharacterSet)),
        _("Other"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v2UseNoUnicodeCharacterSet,1,2,4,5,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v2UseNoUnicodeCharacterSet),
        !FILE_WRITING_ID3V2_USE_UNICODE_CHARACTER_SET);

    EventBox = gtk_event_box_new();
    FileWritingId3v2NoUnicodeCharacterSetCombo = gtk_combo_box_text_new();
    gtk_container_add(GTK_CONTAINER(EventBox),FileWritingId3v2NoUnicodeCharacterSetCombo);
    gtk_widget_set_tooltip_text(EventBox,_("Character set used to write the tag "
        "data in the file."));

    Charset_Populate_Combobox(GTK_COMBO_BOX(FileWritingId3v2NoUnicodeCharacterSetCombo), 
        FILE_WRITING_ID3V2_NO_UNICODE_CHARACTER_SET);
    gtk_table_attach(GTK_TABLE(Table),EventBox,2,5,4,5,GTK_FILL,GTK_FILL,0,0);
    g_signal_connect_after(G_OBJECT(FileWritingId3v2UseNoUnicodeCharacterSet),"toggled",
        G_CALLBACK(Change_Id3_Settings_Toggled),NULL);
    
    // ID3v2 Additional iconv() options
    LabelAdditionalId3v2IconvOptions = gtk_label_new(_("Additional settings for iconv():"));
    gtk_table_attach(GTK_TABLE(Table),LabelAdditionalId3v2IconvOptions,2,5,5,6,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(LabelAdditionalId3v2IconvOptions),0,0.5);

    FileWritingId3v2IconvOptionsNo = gtk_radio_button_new_with_label(NULL,
        _("No"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v2IconvOptionsNo,2,3,6,7,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v2IconvOptionsNo),FILE_WRITING_ID3V2_ICONV_OPTIONS_NO);
    gtk_widget_set_tooltip_text(FileWritingId3v2IconvOptionsNo,_("With this option, when "
        "a character cannot be represented in the target character set, it isn't changed. "
        "But note that an error message will be displayed for information."));
    FileWritingId3v2IconvOptionsTranslit = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FileWritingId3v2IconvOptionsNo)),
        _("//TRANSLIT"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v2IconvOptionsTranslit,3,4,6,7,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v2IconvOptionsTranslit),FILE_WRITING_ID3V2_ICONV_OPTIONS_TRANSLIT);
    gtk_widget_set_tooltip_text(FileWritingId3v2IconvOptionsTranslit,_("With this option, when "
        "a character cannot be represented in the target character set, it can be "
        "approximated through one or several similarly looking characters."));

    FileWritingId3v2IconvOptionsIgnore = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FileWritingId3v2IconvOptionsNo)),
        _("//IGNORE"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v2IconvOptionsIgnore,4,5,6,7,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v2IconvOptionsIgnore),FILE_WRITING_ID3V2_ICONV_OPTIONS_IGNORE);
    gtk_widget_set_tooltip_text(FileWritingId3v2IconvOptionsIgnore,_("With this option, when "
        "a characters cannot be represented in the target character set, it will "
        "be silently discarded."));

    // ID3v1 tags
    Frame = gtk_frame_new (_("ID3v1 tags"));
    gtk_box_pack_start(GTK_BOX(id3v1v2hbox),Frame,FALSE,FALSE,2);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox),2);

    Table = gtk_table_new(6,5,FALSE);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    gtk_table_set_row_spacings(GTK_TABLE(Table),2);
    gtk_table_set_col_spacings(GTK_TABLE(Table),2);


    /* Write ID3v1 tag */
    FileWritingId3v1WriteTag = gtk_check_button_new_with_label(_("Write ID3v1.x tag"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v1WriteTag,0,4,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v1WriteTag),FILE_WRITING_ID3V1_WRITE_TAG);
    gtk_widget_set_tooltip_text(FileWritingId3v1WriteTag,_("If activated, an ID3v1 tag will be added or "
        "updated at the end of the MP3 files. Else it will be stripped."));
    g_signal_connect_after(G_OBJECT(FileWritingId3v1WriteTag),"toggled",
        G_CALLBACK(Change_Id3_Settings_Toggled),NULL);

    /* Id3V1 writing character set */
    LabelId3v1Charset = gtk_label_new(_("Charset:"));
    gtk_table_attach(GTK_TABLE(Table),LabelId3v1Charset,0,4,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(LabelId3v1Charset),0,0.5);

    Label = gtk_label_new("        ");
    gtk_table_attach(GTK_TABLE(Table),Label,0,1,2,3,GTK_FILL,GTK_FILL,0,0);

    EventBox = gtk_event_box_new();
    FileWritingId3v1CharacterSetCombo = gtk_combo_box_text_new();
    gtk_container_add(GTK_CONTAINER(EventBox),FileWritingId3v1CharacterSetCombo);
    gtk_table_attach(GTK_TABLE(Table),EventBox,1,4,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_widget_set_tooltip_text(EventBox,_("Character set used to write ID3v1 tag data "
        "in the file."));
    Charset_Populate_Combobox(GTK_COMBO_BOX(FileWritingId3v1CharacterSetCombo), FILE_WRITING_ID3V1_CHARACTER_SET);

    /* ID3V1 Additional iconv() options*/
    LabelAdditionalId3v1IconvOptions = gtk_label_new(_("Additional settings for iconv():"));
    gtk_table_attach(GTK_TABLE(Table),LabelAdditionalId3v1IconvOptions,1,4,3,4,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(LabelAdditionalId3v1IconvOptions),0,0.5);

    FileWritingId3v1IconvOptionsNo = gtk_radio_button_new_with_label(NULL,
        _("No"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v1IconvOptionsNo,1,2,4,5,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v1IconvOptionsNo),FILE_WRITING_ID3V1_ICONV_OPTIONS_NO);
    gtk_widget_set_tooltip_text(FileWritingId3v1IconvOptionsNo,_("With this option, when "
        "a character cannot be represented in the target character set, it isn't changed. "
        "But note that an error message will be displayed for information."));
    FileWritingId3v1IconvOptionsTranslit = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FileWritingId3v1IconvOptionsNo)),
        _("//TRANSLIT"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v1IconvOptionsTranslit,2,3,4,5,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v1IconvOptionsTranslit),FILE_WRITING_ID3V1_ICONV_OPTIONS_TRANSLIT);
    gtk_widget_set_tooltip_text(FileWritingId3v1IconvOptionsTranslit,_("With this option, when "
        "a character cannot be represented in the target character set, it can be "
        "approximated through one or several similarly looking characters."));

    FileWritingId3v1IconvOptionsIgnore = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FileWritingId3v1IconvOptionsNo)),
        _("//IGNORE"));
    gtk_table_attach(GTK_TABLE(Table),FileWritingId3v1IconvOptionsIgnore,3,4,4,5,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileWritingId3v1IconvOptionsIgnore),FILE_WRITING_ID3V1_ICONV_OPTIONS_IGNORE);
    gtk_widget_set_tooltip_text(FileWritingId3v1IconvOptionsIgnore,_("With this option, when "
        "a characters cannot be represented in the target character set, it will "
        "be silently discarded."));

    /* Character Set for reading tag */
    Frame = gtk_frame_new (_("Character Set for reading ID3 tags"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    Table = gtk_table_new(4,2,FALSE);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    //gtk_container_set_border_width(GTK_CONTAINER(Table), 2);
    //gtk_table_set_row_spacings(GTK_TABLE(Table),2);
    gtk_table_set_col_spacings(GTK_TABLE(Table),2);

    // "File Reading Charset" Check Button + Combo
    UseNonStandardId3ReadingCharacterSet = gtk_check_button_new_with_label(_(
        "Non-standard:"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(UseNonStandardId3ReadingCharacterSet),
        USE_NON_STANDARD_ID3_READING_CHARACTER_SET);
    gtk_table_attach(GTK_TABLE(Table),UseNonStandardId3ReadingCharacterSet,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_widget_set_tooltip_text(UseNonStandardId3ReadingCharacterSet,
        _("This character set will be used when reading the tag data, to convert "
        "each string found in an ISO-8859-1 field in the tag (for ID3v2 or/and ID3v1 tag).\n"
        "\n"
        "For example:\n"
        "  - In previous versions of EasyTAG, you could save UTF-8 strings in an ISO-8859-1 "
        "field. This is not correct. To convert these tags to Unicode: activate this option "
        "and select UTF-8. You must also activate above the option 'Try to save tags to "
        "ISO-8859-1. If it isn't possible then use UNICODE (recommended)' or 'Always save "
        "tags to UNICODE character set'.\n"
        "  - If Unicode was not used, Russian people can select the character set "
        "'Windows-1251' to load tags written under Windows. And 'KOI8-R' to load tags "
        "written under Unix systems."));

    EventBox = gtk_event_box_new();
    FileReadingId3v1v2CharacterSetCombo = gtk_combo_box_text_new();
    gtk_container_add(GTK_CONTAINER(EventBox),FileReadingId3v1v2CharacterSetCombo);
    gtk_table_attach(GTK_TABLE(Table),EventBox,2,3,0,1,GTK_FILL,GTK_FILL,0,0);

    gtk_widget_set_tooltip_text(EventBox,_("Character set used to read tag data "
        "in the file."));

    Charset_Populate_Combobox(GTK_COMBO_BOX(FileReadingId3v1v2CharacterSetCombo), 
        FILE_READING_ID3V1V2_CHARACTER_SET);
    g_signal_connect_after(G_OBJECT(UseNonStandardId3ReadingCharacterSet),"toggled",
                           G_CALLBACK(Use_Non_Standard_Id3_Reading_Character_Set_Toggled),NULL);

    Use_Non_Standard_Id3_Reading_Character_Set_Toggled();
    Change_Id3_Settings_Toggled();


    /*
     * Scanner
     */
    Label = gtk_label_new (_("Scanner"));
    Frame = gtk_frame_new (_("Scanner"));
    gtk_notebook_append_page (GTK_NOTEBOOK(OptionsNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame), 5);

    /* Save the number of the page. Asked in Scanner window */
    OptionsNoteBook_Scanner_Page_Num = gtk_notebook_page_num(GTK_NOTEBOOK(OptionsNoteBook),Frame);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 4);

    /* Character conversion for the 'Fill Tag' scanner (=> FTS...) */
    Frame = gtk_frame_new (_("Fill Tag Scanner - Character Conversion"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    FTSConvertUnderscoreAndP20IntoSpace = gtk_check_button_new_with_label(_("Convert underscore "
        "character '_' and string '%20' to space ' '"));
    FTSConvertSpaceIntoUnderscore = gtk_check_button_new_with_label(_("Convert space ' ' to underscore '_'"));
    gtk_box_pack_start(GTK_BOX(vbox),FTSConvertUnderscoreAndP20IntoSpace,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),FTSConvertSpaceIntoUnderscore,      FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(FTSConvertUnderscoreAndP20IntoSpace),"toggled",
        G_CALLBACK(Scanner_Convert_Check_Button_Toggled_1),G_OBJECT(FTSConvertSpaceIntoUnderscore));
    g_signal_connect_swapped(G_OBJECT(FTSConvertSpaceIntoUnderscore),"toggled",
        G_CALLBACK(Scanner_Convert_Check_Button_Toggled_1),G_OBJECT(FTSConvertUnderscoreAndP20IntoSpace));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FTSConvertUnderscoreAndP20IntoSpace),
        FTS_CONVERT_UNDERSCORE_AND_P20_INTO_SPACE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FTSConvertSpaceIntoUnderscore),
        FTS_CONVERT_SPACE_INTO_UNDERSCORE);
    gtk_widget_set_tooltip_text(FTSConvertUnderscoreAndP20IntoSpace,_("If activated, this conversion "
        "will be used when applying a mask from the scanner for tags."));
    gtk_widget_set_tooltip_text(FTSConvertSpaceIntoUnderscore,_("If activated, this conversion "
        "will be used when applying a mask from the scanner for tags."));

    /* Character conversion for the 'Rename File' scanner (=> RFS...) */
    Frame = gtk_frame_new (_("Rename File Scanner - Character Conversion"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox),2);
    RFSConvertUnderscoreAndP20IntoSpace = gtk_radio_button_new_with_label(NULL, _("Convert underscore " "character '_' and string '%20' to space ' '"));
    RFSConvertSpaceIntoUnderscore = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(RFSConvertUnderscoreAndP20IntoSpace), _("Convert space ' ' to underscore '_'"));
		RFSRemoveSpaces = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(RFSConvertUnderscoreAndP20IntoSpace), _("Remove spaces"));
    gtk_box_pack_start(GTK_BOX(vbox),RFSConvertUnderscoreAndP20IntoSpace,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),RFSConvertSpaceIntoUnderscore,      FALSE,FALSE,0);
		 gtk_box_pack_start(GTK_BOX(vbox),RFSRemoveSpaces,                    FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RFSConvertUnderscoreAndP20IntoSpace),
        RFS_CONVERT_UNDERSCORE_AND_P20_INTO_SPACE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RFSConvertSpaceIntoUnderscore),
        RFS_CONVERT_SPACE_INTO_UNDERSCORE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RFSRemoveSpaces),
        RFS_REMOVE_SPACES);
    gtk_widget_set_tooltip_text(RFSConvertUnderscoreAndP20IntoSpace,_("If activated, this conversion "
        "will be used when applying a mask from the scanner for filenames."));
    gtk_widget_set_tooltip_text(RFSConvertSpaceIntoUnderscore,_("If activated, this conversion "
        "will be used when applying a mask from the scanner for filenames."));
    gtk_widget_set_tooltip_text(RFSRemoveSpaces,_("If activated, this conversion "        "will be used when applying a mask from the scanner for filenames."));

    /* Character conversion for the 'Process Fields' scanner (=> PFS...) */
    Frame = gtk_frame_new (_("Process Fields Scanner - Character Conversion"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox),2);

    // Don't convert some words like to, feat. first letter uppercase.
    PFSDontUpperSomeWords = gtk_check_button_new_with_label(_("Don't uppercase "
        "first letter of words for some prepositions and articles."));
    gtk_box_pack_start(GTK_BOX(vbox),PFSDontUpperSomeWords, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PFSDontUpperSomeWords), PFS_DONT_UPPER_SOME_WORDS);
    gtk_widget_set_tooltip_text(PFSDontUpperSomeWords, _("Don't convert first "
        "letter of the words like prepositions, articles and words like feat., "
        "when using the scanner 'First letter uppercase of each word' (for "
        "example, you will obtain 'Text in an Entry' instead of 'Text In An Entry')."));

    /* Properties of the scanner window */
    Frame = gtk_frame_new (_("Scanner Window"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    OpenScannerWindowOnStartup = gtk_check_button_new_with_label(_("Open the Scanner Window on startup"));
    gtk_box_pack_start(GTK_BOX(vbox),OpenScannerWindowOnStartup,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(OpenScannerWindowOnStartup),OPEN_SCANNER_WINDOW_ON_STARTUP);
    gtk_widget_set_tooltip_text(OpenScannerWindowOnStartup,_("Activate this option to open automatically "
        "the scanner window when EasyTAG starts."));

    ScannerWindowOnTop = gtk_check_button_new_with_label(_("Scanner window always on top"));
    gtk_box_pack_start(GTK_BOX(vbox),ScannerWindowOnTop,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ScannerWindowOnTop),SCANNER_WINDOW_ON_TOP);
    gtk_widget_set_tooltip_text(ScannerWindowOnTop,_("If activated, the window which contains the masks "
                        "will stay always over the main window."));


    /* Other options */
    Frame = gtk_frame_new (_("Fields"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    // Overwrite text into tag fields
    OverwriteTagField = gtk_check_button_new_with_label(_("Overwrite fields when scanning tag"));
    gtk_box_pack_start(GTK_BOX(vbox),OverwriteTagField,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(OverwriteTagField),OVERWRITE_TAG_FIELD);
    gtk_widget_set_tooltip_text(OverwriteTagField,_("If activated, the scanner will replace existing text "
        "in fields by the new one. If deactivated, only blank fields of the tag will be filled."));

    // Set a default comment text or CRC-32 checksum
    if (!DefaultCommentModel)
        DefaultCommentModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);
    else
        gtk_list_store_clear(DefaultCommentModel);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    SetDefaultComment = gtk_check_button_new_with_label(_("Set this text as default comment:"));
    gtk_box_pack_start(GTK_BOX(hbox),SetDefaultComment,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SetDefaultComment),SET_DEFAULT_COMMENT);
    gtk_widget_set_tooltip_text(SetDefaultComment,_("Activate this option if you want to put the "
        "following string into the comment field when using the 'Fill Tag' scanner."));
    DefaultComment = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(DefaultCommentModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(DefaultComment),MISC_COMBO_TEXT);
    gtk_box_pack_start(GTK_BOX(hbox),DefaultComment,FALSE,FALSE,0);
    gtk_widget_set_size_request(GTK_WIDGET(DefaultComment), 250, -1);
    g_signal_connect(G_OBJECT(SetDefaultComment),"toggled",
            G_CALLBACK(Set_Default_Comment_Check_Button_Toggled),NULL);
    if (DEFAULT_COMMENT==NULL)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SetDefaultComment),FALSE);
    Set_Default_Comment_Check_Button_Toggled();
    /* History list */
    Load_Default_Tag_Comment_Text_List(DefaultCommentModel, MISC_COMBO_TEXT);
    Add_String_To_Combo_List(DefaultCommentModel, DEFAULT_COMMENT);
    if (DEFAULT_COMMENT)
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(DefaultComment))), DEFAULT_COMMENT);

    // CRC32 comment
    Crc32Comment = gtk_check_button_new_with_label(_("Use CRC32 as the default "
        "comment (for files with ID3 tags only)."));
    gtk_box_pack_start(GTK_BOX(vbox),Crc32Comment,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Crc32Comment),SET_CRC32_COMMENT);
    gtk_widget_set_tooltip_text(Crc32Comment,_("Calculates the CRC-32 value of the file "
        "and writes it into the comment field when using the 'Fill Tag' scanner."));
    g_signal_connect_swapped(G_OBJECT(SetDefaultComment), "toggled",
        G_CALLBACK(Scanner_Convert_Check_Button_Toggled_1),G_OBJECT(Crc32Comment));
    g_signal_connect_swapped(G_OBJECT(Crc32Comment), "toggled",
        G_CALLBACK(Scanner_Convert_Check_Button_Toggled_1),G_OBJECT(SetDefaultComment));


    /*
     * CDDB
     */
    Label = gtk_label_new (_("CD Data Base"));
    Frame = gtk_frame_new (_("CD Data Base"));
    gtk_notebook_append_page (GTK_NOTEBOOK(OptionsNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame), 5);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 4);

    // CDDB Server Settings (Automatic Search)
    Frame = gtk_frame_new (_("Server Settings for Automatic Search"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE, 0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

    // 1rst automatic search server
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_container_add(GTK_CONTAINER(vbox),hbox);
    Label = gtk_label_new(_("Name:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerNameAutomaticSearch = gtk_combo_box_text_new_with_entry();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerNameAutomaticSearch,FALSE,FALSE,0);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "freedb.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "www.gnudb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "at.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "au.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "ca.freedb.org");
    //gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "ca2.freedb.org");
    //gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "de.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "es.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "fi.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "ru.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "uk.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "us.freedb.org");
    if (CDDB_SERVER_NAME_AUTOMATIC_SEARCH)
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbServerNameAutomaticSearch))),CDDB_SERVER_NAME_AUTOMATIC_SEARCH);

    Label = gtk_label_new (_("Port:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerPortAutomaticSearch = gtk_entry_new();
    gtk_widget_set_size_request(GTK_WIDGET(CddbServerPortAutomaticSearch), 45, -1);
    gtk_entry_set_max_length(GTK_ENTRY(CddbServerPortAutomaticSearch),5);
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerPortAutomaticSearch,FALSE,FALSE,0);
    sprintf(temp,"%i",CDDB_SERVER_PORT_AUTOMATIC_SEARCH);
    gtk_entry_set_text(GTK_ENTRY(CddbServerPortAutomaticSearch),temp);
    g_signal_connect(G_OBJECT(CddbServerPortAutomaticSearch),"insert_text",G_CALLBACK(Insert_Only_Digit),NULL);

    Label = gtk_label_new (_("CGI Path:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerCgiPathAutomaticSearch = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerCgiPathAutomaticSearch,FALSE,FALSE,0);
    if (CDDB_SERVER_CGI_PATH_AUTOMATIC_SEARCH)
        gtk_entry_set_text(GTK_ENTRY(CddbServerCgiPathAutomaticSearch),CDDB_SERVER_CGI_PATH_AUTOMATIC_SEARCH);

    // 2sd automatic search server
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_container_add(GTK_CONTAINER(vbox),hbox);
    Label = gtk_label_new(_("Name:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerNameAutomaticSearch2 = gtk_combo_box_text_new_with_entry();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerNameAutomaticSearch2,FALSE,FALSE,0);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch2), "freedb.musicbrainz.org");
    if (CDDB_SERVER_NAME_AUTOMATIC_SEARCH2)
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbServerNameAutomaticSearch2))),CDDB_SERVER_NAME_AUTOMATIC_SEARCH2);

    Label = gtk_label_new (_("Port:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerPortAutomaticSearch2 = gtk_entry_new();
    gtk_widget_set_size_request(GTK_WIDGET(CddbServerPortAutomaticSearch2), 45, -1);
    gtk_entry_set_max_length(GTK_ENTRY(CddbServerPortAutomaticSearch2),5);
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerPortAutomaticSearch2,FALSE,FALSE,0);
    sprintf(temp,"%i",CDDB_SERVER_PORT_AUTOMATIC_SEARCH2);
    gtk_entry_set_text(GTK_ENTRY(CddbServerPortAutomaticSearch2),temp);
    g_signal_connect(G_OBJECT(CddbServerPortAutomaticSearch2),"insert_text",G_CALLBACK(Insert_Only_Digit),NULL);

    Label = gtk_label_new (_("CGI Path:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerCgiPathAutomaticSearch2 = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerCgiPathAutomaticSearch2,FALSE,FALSE,0);
    if (CDDB_SERVER_CGI_PATH_AUTOMATIC_SEARCH2)
        gtk_entry_set_text(GTK_ENTRY(CddbServerCgiPathAutomaticSearch2) ,CDDB_SERVER_CGI_PATH_AUTOMATIC_SEARCH2);

    // CDDB Server Settings (Manual Search)
    Frame = gtk_frame_new (_("Server Settings for Manual Search"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_container_add(GTK_CONTAINER(vbox),hbox);
    Label = gtk_label_new(_("Name:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerNameManualSearch = gtk_combo_box_text_new_with_entry();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerNameManualSearch,FALSE,FALSE,0);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameManualSearch), "www.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameManualSearch), "www.gnudb.org");
    if (CDDB_SERVER_NAME_MANUAL_SEARCH)
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbServerNameManualSearch))),CDDB_SERVER_NAME_MANUAL_SEARCH);

    Label = gtk_label_new (_("Port:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerPortManualSearch = gtk_entry_new();
    gtk_widget_set_size_request(GTK_WIDGET(CddbServerPortManualSearch), 45, -1);
    gtk_entry_set_max_length(GTK_ENTRY(CddbServerPortManualSearch),5);
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerPortManualSearch,FALSE,FALSE,0);
    sprintf(temp,"%i",CDDB_SERVER_PORT_MANUAL_SEARCH);
    gtk_entry_set_text(GTK_ENTRY(CddbServerPortManualSearch),temp);
    g_signal_connect(G_OBJECT(CddbServerPortManualSearch),"insert_text",G_CALLBACK(Insert_Only_Digit),NULL);

    Label = gtk_label_new (_("CGI Path:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerCgiPathManualSearch = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerCgiPathManualSearch,FALSE,FALSE,0);
    if (CDDB_SERVER_CGI_PATH_MANUAL_SEARCH)
        gtk_entry_set_text(GTK_ENTRY(CddbServerCgiPathManualSearch) ,CDDB_SERVER_CGI_PATH_MANUAL_SEARCH);

    // Local access for CDDB (Automatic Search)
    Frame = gtk_frame_new (_("Local CD Data Base"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_container_add(GTK_CONTAINER(vbox),hbox);
    Label = gtk_label_new(_("Path:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);

    if (CddbLocalPathModel != NULL)
        gtk_list_store_clear(CddbLocalPathModel);
    else
        CddbLocalPathModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);

    CddbLocalPath = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(CddbLocalPathModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(CddbLocalPath),MISC_COMBO_TEXT);
    gtk_box_pack_start(GTK_BOX(hbox),CddbLocalPath,FALSE,FALSE,0);
    gtk_widget_set_size_request(GTK_WIDGET(CddbLocalPath), 450, -1);
    gtk_widget_set_tooltip_text(gtk_bin_get_child(GTK_BIN(CddbLocalPath)),_("Specify the directory "
        "where are located the local CD data base. The local CD data base contains the eleven following "
        "directories 'blues', 'classical', 'country', 'data', 'folk', 'jazz', 'newage', 'reggae', "
        "'rock', 'soundtrack' and 'misc'."));
    g_signal_connect(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbLocalPath)))),"activate",G_CALLBACK(CddbLocalPath_Combo_Add_String),NULL);
    //g_signal_connect(G_OBJECT(GTK_ENTRY(GTK_BIN(CddbLocalPath)->child)),"focus_out_event",G_CALLBACK(CddbLocalPath_Combo_Add_String),NULL);

    // History list
    Load_Cddb_Local_Path_List(CddbLocalPathModel, MISC_COMBO_TEXT);

    // If default path hasn't been added already, add it now..
    if (CDDB_LOCAL_PATH)
    {
        path_utf8 = filename_to_display(CDDB_LOCAL_PATH);
        Add_String_To_Combo_List(CddbLocalPathModel, path_utf8);
        if (path_utf8)
            gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbLocalPath))), path_utf8);
        g_free(path_utf8);
    }

    Button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(File_Selection_Window_For_Directory),G_OBJECT(gtk_bin_get_child(GTK_BIN(CddbLocalPath))));

    // CDDB Proxy Settings
    Frame = gtk_frame_new (_("Proxy Settings"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);

    Table = gtk_table_new(3,5,FALSE);
    gtk_container_add(GTK_CONTAINER(Frame),Table);
    gtk_table_set_row_spacings(GTK_TABLE(Table),2);
    gtk_table_set_col_spacings(GTK_TABLE(Table),4);
    gtk_container_set_border_width(GTK_CONTAINER(Table), 2);

    CddbUseProxy = gtk_check_button_new_with_label(_("Use a proxy"));
    gtk_table_attach(GTK_TABLE(Table),CddbUseProxy,0,5,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbUseProxy),CDDB_USE_PROXY);
    gtk_widget_set_tooltip_text(CddbUseProxy,_("Set active the settings of the proxy server."));

    Label = gtk_label_new("     ");
    gtk_table_attach(GTK_TABLE(Table),Label,0,1,1,2,GTK_FILL,GTK_FILL,0,0);

    Label = gtk_label_new(_("Host Name:"));
    gtk_table_attach(GTK_TABLE(Table),Label,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(Label),1,0.5);
    CddbProxyName = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),CddbProxyName,2,3,1,2,GTK_FILL,GTK_FILL,0,0);
    if (CDDB_PROXY_NAME)
        gtk_entry_set_text(GTK_ENTRY(CddbProxyName),CDDB_PROXY_NAME);
    gtk_widget_set_tooltip_text(CddbProxyName,_("Name of the proxy server."));
    Label = gtk_label_new (_("Port:"));
    gtk_table_attach(GTK_TABLE(Table),Label,3,4,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(Label),1,0.5);
    CddbProxyPort = gtk_entry_new();
    gtk_widget_set_size_request(GTK_WIDGET(CddbProxyPort), 45, -1);
    gtk_entry_set_max_length(GTK_ENTRY(CddbProxyPort),5);
    gtk_table_attach(GTK_TABLE(Table),CddbProxyPort,4,5,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_widget_set_tooltip_text(CddbProxyPort,_("Port of the proxy server."));
    sprintf(temp,"%i",CDDB_PROXY_PORT);
    gtk_entry_set_text(GTK_ENTRY(CddbProxyPort),temp);
    g_signal_connect(G_OBJECT(CddbProxyPort),"insert_text",G_CALLBACK(Insert_Only_Digit),NULL);
    g_signal_connect(G_OBJECT(CddbUseProxy),"toggled",G_CALLBACK(Cddb_Use_Proxy_Toggled),NULL);
    Label = gtk_label_new(_("User Name:"));
    gtk_table_attach(GTK_TABLE(Table),Label,1,2,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(Label),1,0.5);
    CddbProxyUserName = gtk_entry_new();
    if (CDDB_PROXY_USER_NAME)
        gtk_entry_set_text(GTK_ENTRY(CddbProxyUserName),CDDB_PROXY_USER_NAME);
    gtk_table_attach(GTK_TABLE(Table),CddbProxyUserName,2,3,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_widget_set_tooltip_text(CddbProxyUserName,_("Name of user for the the proxy server."));
    Label = gtk_label_new(_("User Password:"));
    gtk_table_attach(GTK_TABLE(Table),Label,3,4,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_alignment(GTK_MISC(Label),1,0.5);
    CddbProxyUserPassword = gtk_entry_new();
    if (CDDB_PROXY_USER_PASSWORD)
        gtk_entry_set_text(GTK_ENTRY(CddbProxyUserPassword),CDDB_PROXY_USER_PASSWORD);
    gtk_table_attach(GTK_TABLE(Table),CddbProxyUserPassword,4,5,2,3,GTK_FILL,GTK_FILL,0,0);
    gtk_entry_set_visibility(GTK_ENTRY(CddbProxyUserPassword),FALSE);
    gtk_widget_set_tooltip_text(CddbProxyUserPassword,_("Password of user for the the proxy server."));
    Cddb_Use_Proxy_Toggled();


    // Track Name list (CDDB results)
    Frame = gtk_frame_new (_("Track Name List"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

    CddbFollowFile = gtk_check_button_new_with_label(_("Select corresponding audio "
        "file (according position or DLM if activated below)"));
    gtk_box_pack_start(GTK_BOX(vbox),CddbFollowFile,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbFollowFile),CDDB_FOLLOW_FILE);
    gtk_widget_set_tooltip_text(CddbFollowFile,_("If activated, when selecting a "
        "line in the list of tracks name, the corresponding audio file in the "
        "main list will be also selected."));

    // Check box to use DLM (also used in the cddb window)
    CddbUseDLM = gtk_check_button_new_with_label(_("Use the Levenshtein algorithm "
        "(DLM) to match lines (using title) with audio files (using filename)"));
    gtk_box_pack_start(GTK_BOX(vbox),CddbUseDLM,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CddbUseDLM),CDDB_USE_DLM);
    gtk_widget_set_tooltip_text(CddbUseDLM,_("When activating this option, the "
        "Levenshtein algorithm (DLM: Damerau-Levenshtein Metric) will be used "
        "to match the CDDB title against every file name in the current folder, "
        "and to select the best match. This will be used when selecting the "
        "corresponding audio file, or applying CDDB results, instead of using "
        "directly the position order."));


    /*
     * Confirmation
     */
    Label = gtk_label_new (_("Confirmation"));
    Frame = gtk_frame_new (_("Confirmation"));
    gtk_notebook_append_page (GTK_NOTEBOOK(OptionsNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame), 5);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 2);

    ConfirmBeforeExit = gtk_check_button_new_with_label(_("Confirm exit from program"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmBeforeExit,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ConfirmBeforeExit),CONFIRM_BEFORE_EXIT);
    gtk_widget_set_tooltip_text(ConfirmBeforeExit,_("If activated, opens a dialog box to ask "
        "confirmation before exiting the program."));

    ConfirmWriteTag = gtk_check_button_new_with_label(_("Confirm writing of file tag"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmWriteTag,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ConfirmWriteTag),CONFIRM_WRITE_TAG);

    ConfirmRenameFile = gtk_check_button_new_with_label(_("Confirm renaming of file"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmRenameFile,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ConfirmRenameFile),CONFIRM_RENAME_FILE);

    ConfirmDeleteFile = gtk_check_button_new_with_label(_("Confirm deleting of file"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmDeleteFile,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ConfirmDeleteFile),CONFIRM_DELETE_FILE);

    ConfirmWritePlayList = gtk_check_button_new_with_label(_("Confirm writing of playlist"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmWritePlayList,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ConfirmWritePlayList),CONFIRM_WRITE_PLAYLIST);

    ConfirmWhenUnsavedFiles = gtk_check_button_new_with_label(_("Confirm changing directory when there are unsaved changes"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmWhenUnsavedFiles,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ConfirmWhenUnsavedFiles),CONFIRM_WHEN_UNSAVED_FILES);


    /*
     * Buttons box of Option Window
     */
    ButtonBox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(OptionsVBox), ButtonBox, FALSE, FALSE, 4);

    gtk_button_box_set_layout (GTK_BUTTON_BOX (ButtonBox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing (GTK_BOX(ButtonBox), 15);


    /* Apply Button */
    Button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    gtk_widget_set_tooltip_text(Button,_("Apply changes (but don't save) and close this window"));


    /* Cancel Button */
    Button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_container_add(GTK_CONTAINER(ButtonBox), Button);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(OptionsWindow_Cancel_Button),NULL);
    gtk_widget_set_can_default(Button,TRUE);
    gtk_widget_grab_default(Button);
    gtk_widget_set_tooltip_text(Button,_("Close this window without saving"));


    /* Save Button */
    Button = gtk_button_new_from_stock(GTK_STOCK_OK);
    gtk_container_add(GTK_CONTAINER(ButtonBox), Button);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(OptionsWindow_Save_Button),NULL);
    gtk_widget_set_can_default(Button,TRUE);
    gtk_widget_set_tooltip_text(Button,_("Save changes and close this window"));

    /* Show all in the options window */
    gtk_widget_show_all(OptionsWindow);

    /* Load the default page */
    gtk_notebook_set_current_page(GTK_NOTEBOOK(OptionsNoteBook), OPTIONS_NOTEBOOK_PAGE);
}


static void Set_Default_Comment_Check_Button_Toggled (void)
{
    gtk_widget_set_sensitive(DefaultComment,gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SetDefaultComment)));
}

void Number_Track_Formatted_Toggled (void)
{
    gtk_widget_set_sensitive(NumberTrackFormatedSpinButton,gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(NumberTrackFormated)));
    // To update the example...
    g_signal_emit_by_name(G_OBJECT(NumberTrackFormatedSpinButton),"changed",NULL);
}

static void
Number_Track_Formatted_Spin_Button_Changed (GtkWidget *Label,
                                            GtkWidget *SpinButton)
{
    gchar *tmp;
    gint val;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(NumberTrackFormated)))
        val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(SpinButton));
    else
        val = 1;

    // For translators : be aware to NOT translate '%.*d' in this string
    tmp = g_strdup_printf(_("(Example: %.*d_-_Track_name_1.mp3)"),val,1);

    gtk_label_set_text(GTK_LABEL(Label),tmp);
    g_free(tmp);
}

static void
Use_Non_Standard_Id3_Reading_Character_Set_Toggled (void)
{
    gtk_widget_set_sensitive(FileReadingId3v1v2CharacterSetCombo,
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(UseNonStandardId3ReadingCharacterSet)));
}

static void
Change_Id3_Settings_Toggled (void)
{
    int active;

    if ( !FileWritingId3v2UseUnicodeCharacterSet 
    ||   !FileWritingId3v2UseNoUnicodeCharacterSet
    ||   !FileWritingId3v2WriteTag
#ifdef ENABLE_ID3LIB
    ||   !FileWritingId3v2VersionCombo
    ||   !LabelId3v2Version
#endif
    ||   !FileWritingId3v1WriteTag
    ||   !LabelId3v2Charset
    ||   !FileWritingId3v2UseUnicodeCharacterSet
    ||   !FileWritingId3v2UseNoUnicodeCharacterSet
    ||   !FileWritingId3v2UnicodeCharacterSetCombo
    ||   !FileWritingId3v2NoUnicodeCharacterSetCombo
    ||   !LabelAdditionalId3v2IconvOptions
    ||   !FileWritingId3v2IconvOptionsNo
    ||   !FileWritingId3v2IconvOptionsTranslit
    ||   !FileWritingId3v2IconvOptionsIgnore
    ||   !FileWritingId3v2UseCrc32
    ||   !FileWritingId3v2UseCompression
    ||   !FileWritingId3v2TextOnlyGenre
    ||   !ConvertOldId3v2TagVersion
    ||   !LabelId3v1Charset
    ||   !FileWritingId3v1CharacterSetCombo
    ||   !LabelAdditionalId3v1IconvOptions
    ||   !FileWritingId3v1IconvOptionsNo
    ||   !FileWritingId3v1IconvOptionsTranslit
    ||   !FileWritingId3v1IconvOptionsIgnore
       )
        return;

    active = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FileWritingId3v2UseUnicodeCharacterSet)) != 0);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FileWritingId3v2WriteTag)))
    {
        gtk_widget_set_sensitive(LabelId3v2Charset, TRUE);

#ifdef ENABLE_ID3LIB
        gtk_widget_set_sensitive(LabelId3v2Version, TRUE);
        gtk_widget_set_sensitive(FileWritingId3v2VersionCombo, TRUE);
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(FileWritingId3v2VersionCombo)) == 1)
        {
            // When "ID3v2.3" is selected
            gtk_combo_box_set_active(GTK_COMBO_BOX(FileWritingId3v2UnicodeCharacterSetCombo), 1);
            gtk_widget_set_sensitive(FileWritingId3v2UnicodeCharacterSetCombo, FALSE);
        }else
        {
            // When "ID3v2.4" is selected
            gtk_combo_box_set_active(GTK_COMBO_BOX(FileWritingId3v2UnicodeCharacterSetCombo), 0); // Set "UTF-8" as default value for this version of tag
            gtk_widget_set_sensitive(FileWritingId3v2UnicodeCharacterSetCombo, active);
        }
#else 
        gtk_widget_set_sensitive(FileWritingId3v2UnicodeCharacterSetCombo, active);
#endif
        gtk_widget_set_sensitive(FileWritingId3v2UseUnicodeCharacterSet, TRUE);
        gtk_widget_set_sensitive(FileWritingId3v2UseNoUnicodeCharacterSet, TRUE);
        gtk_widget_set_sensitive(FileWritingId3v2NoUnicodeCharacterSetCombo, !active);
        gtk_widget_set_sensitive(LabelAdditionalId3v2IconvOptions, !active);
        gtk_widget_set_sensitive(FileWritingId3v2IconvOptionsNo, !active);
        gtk_widget_set_sensitive(FileWritingId3v2IconvOptionsTranslit, !active);
        gtk_widget_set_sensitive(FileWritingId3v2IconvOptionsIgnore, !active);
        gtk_widget_set_sensitive(FileWritingId3v2UseCrc32, TRUE);
        gtk_widget_set_sensitive(FileWritingId3v2UseCompression, TRUE);
        gtk_widget_set_sensitive(FileWritingId3v2TextOnlyGenre, TRUE);
        gtk_widget_set_sensitive(ConvertOldId3v2TagVersion, TRUE);

    }else
    {
        gtk_widget_set_sensitive(LabelId3v2Charset, FALSE);
#ifdef ENABLE_ID3LIB
        gtk_widget_set_sensitive(LabelId3v2Version, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2VersionCombo, FALSE);
#endif
        gtk_widget_set_sensitive(FileWritingId3v2UseUnicodeCharacterSet, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2UseNoUnicodeCharacterSet, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2UnicodeCharacterSetCombo, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2NoUnicodeCharacterSetCombo, FALSE);
        gtk_widget_set_sensitive(LabelAdditionalId3v2IconvOptions, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2IconvOptionsNo, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2IconvOptionsTranslit, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2IconvOptionsIgnore, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2UseCrc32, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2UseCompression, FALSE);
        gtk_widget_set_sensitive(FileWritingId3v2TextOnlyGenre, FALSE);
        gtk_widget_set_sensitive(ConvertOldId3v2TagVersion, 0);
    }

    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FileWritingId3v1WriteTag));

    gtk_widget_set_sensitive(LabelId3v1Charset, active);
    gtk_widget_set_sensitive(FileWritingId3v1CharacterSetCombo, active);
    gtk_widget_set_sensitive(LabelAdditionalId3v1IconvOptions, active);
    gtk_widget_set_sensitive(FileWritingId3v1IconvOptionsNo, active);
    gtk_widget_set_sensitive(FileWritingId3v1IconvOptionsTranslit, active);
    gtk_widget_set_sensitive(FileWritingId3v1IconvOptionsIgnore, active);
}

static void
Cddb_Use_Proxy_Toggled (void)
{
    gboolean active;

    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(CddbUseProxy));
    gtk_widget_set_sensitive(CddbProxyName,active);
    gtk_widget_set_sensitive(CddbProxyPort,active);
    gtk_widget_set_sensitive(CddbProxyUserName,active);
    gtk_widget_set_sensitive(CddbProxyUserPassword,active);
}

/* Callback from Open_OptionsWindow */
static gboolean
OptionsWindow_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_KEY_Escape:
            {
                OptionsWindow_Quit();
                break;
            }
        }
    }
    return FALSE;
}

/* Callback from Open_OptionsWindow */
static void
OptionsWindow_Save_Button (void)
{
    if (!Check_Config()) return;

#ifndef WIN32
    /* FIXME : make gtk crash on win32 */
    Add_String_To_Combo_List(DefaultPathModel,      gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(DefaultPathToMp3)))));
    Add_String_To_Combo_List(FilePlayerModel,       gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(FilePlayerCombo)))));
    Add_String_To_Combo_List(DefaultCommentModel,   gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(DefaultComment)))));
    Add_String_To_Combo_List(CddbLocalPathModel,    gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbLocalPath)))));
#endif

    Save_Changes_Of_Preferences_Window();

    OptionsWindow_Quit();
    Statusbar_Message(_("Configuration saved"),TRUE);
}

/* Callback from Open_OptionsWindow */
static void
OptionsWindow_Cancel_Button (void)
{
    OptionsWindow_Quit();
    Statusbar_Message(_("Configuration unchanged"),TRUE);
}

/* Callback from Open_OptionsWindow */
static void
OptionsWindow_Quit (void)
{
    if (OptionsWindow)
    {
        OptionsWindow_Apply_Changes();

        /* Now quit */
        gtk_widget_destroy(OptionsWindow);
        OptionsWindow = NULL;
        gtk_widget_set_sensitive(MainWindow, TRUE);
    }
}

/*
 * For the configuration file...
 */
void OptionsWindow_Apply_Changes (void)
{
    if (OptionsWindow)
    {
        //gint x, y;
        gint width, height;
        GdkWindow *window;

        window = gtk_widget_get_window(OptionsWindow);

        if ( window && gdk_window_is_visible(window) && gdk_window_get_state(window)!=GDK_WINDOW_STATE_MAXIMIZED )
        {
            // Position and Origin of the preferences window
            //gdk_window_get_root_origin(OptionsWindow->window,&x,&y);
            //OPTIONS_WINDOW_X = x;
            //OPTIONS_WINDOW_Y = y;
            width = gdk_window_get_width(window);
            height = gdk_window_get_height(window);
            OPTIONS_WINDOW_WIDTH  = width;
            OPTIONS_WINDOW_HEIGHT = height;
        }

        /* Get the last visible notebook page */
        OPTIONS_NOTEBOOK_PAGE = gtk_notebook_get_current_page(GTK_NOTEBOOK(OptionsNoteBook));

        /* Save combobox history lists before exit */
        Save_Default_Path_To_MP3_List     (DefaultPathModel, MISC_COMBO_TEXT);
        Save_Default_Tag_Comment_Text_List(DefaultCommentModel, MISC_COMBO_TEXT);
        Save_Audio_File_Player_List       (FilePlayerModel, MISC_COMBO_TEXT);
        Save_Cddb_Local_Path_List         (CddbLocalPathModel, MISC_COMBO_TEXT);
    }
}



/*
 * Check_Config: Check if config information are correct
 * dsd: Check this... going from utf8 to raw is dodgy stuff
 *
 * Problem noted : if a character is escaped (like : 'C\351line DION') in
 *                 gtk_file_chooser it will converted to UTF-8. So after, there
 *                 is a problem to convert it in the right system encoding to be
 *                 passed to stat(), and it can find the directory.
 * exemple :
 *  - initial file on system                        : C\351line DION - D'eux (1995)
 *  - converted to UTF-8 (path_utf8)                : Céline DION - D'eux (1995)
 *  - try to convert to system encoding (path_real) : ?????
 */
static gboolean
Check_DefaultPathToMp3 (void)
{
    gchar *path_utf8;
    gchar *path_real;
    struct stat stbuf;

    path_utf8 = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(DefaultPathToMp3)))));
    if (!path_utf8 || g_utf8_strlen(path_utf8, -1) < 1)
    {
        g_free(path_utf8);
        return TRUE;
    }

    path_real = filename_from_display(path_utf8);

#ifdef WIN32
    /* On win32 : stat("c:\path\to\dir") succeed, while stat("c:\path\to\dir\") fails */
    ET_Win32_Path_Remove_Trailing_Backslash(path_real);
#endif

    if ( stat(path_real,&stbuf)==0 && S_ISDIR(stbuf.st_mode) )
    {
        g_free(path_real);
        g_free(path_utf8);
        return TRUE; /* Path is good */
    }else
    {
        GtkWidget *msgdialog = gtk_message_dialog_new(GTK_WINDOW(OptionsWindow),
                                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_CLOSE,
                                                      "%s",
                                                      _("The selected path for 'Default path to files' is invalid"));
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),_("Path: '%s'\nError: %s"),path_utf8,g_strerror(errno));
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Invalid Path Error"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        g_free(path_real);
        g_free(path_utf8);
        return FALSE;
    }
}

/*
 * The character set conversion is used for ID3 tag. UTF-8 is used to display.
 *  - reading_character is converted to UTF-8
 *  - writing_character is converted from UTF-8
 */
/*****************
gint Check_CharacterSetTranslation (void)
{
    gchar *temp;
    gchar *reading_character;
    gchar *writing_character;

    temp = Get_Active_Combo_Box_Item(GTK_COMBO_BOX(FileReadingCharacterSetCombo));
    reading_character = Charset_Get_Name_From_Title(temp);
    g_free(temp);

    temp = Get_Active_Combo_Box_Item(GTK_COMBO_BOX(FileWritingCharacterSetCombo));
    writing_character = Charset_Get_Name_From_Title(temp);
    g_free(temp);

    // Check conversion when reading file
    if ( GTK_TOGGLE_BUTTON(UseNonStandardId3ReadingCharacterSet)->active
    && (test_conversion_charset(reading_character,"UTF-8")!=TRUE) )
    {
        gchar *msg = g_strdup_printf(_("The character set translation from '%s'\n"
                                       "to '%s' is not supported"),reading_character,"UTF-8");
        GtkWidget *msgbox = msg_box_new(_("Error"),
                                        GTK_WINDOW(OptionsWindow),
                                        NULL,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        msg,
                                        GTK_STOCK_DIALOG_ERROR,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);
        gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);
        g_free(msg);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(UseNonStandardId3ReadingCharacterSet),FALSE);
        return 0;
    }
    // Check conversion when writing file
    if ( GTK_TOGGLE_BUTTON(UseNonStandardId3WritingCharacterSet)->active
    && (test_conversion_charset("UTF-8",writing_character)!=TRUE) )
    {
        gchar *msg = g_strdup_printf(_("The character set translation from '%s'\n"
                                       "to '%s' is not supported"),"UTF-8",writing_character);
        GtkWidget *msgbox = msg_box_new(_("Error"),
                                        GTK_WINDOW(OptionsWindow),
                                        NULL,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        msg,
                                        GTK_STOCK_DIALOG_ERROR,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);
        gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);
        g_free(msg);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(UseNonStandardId3WritingCharacterSet),FALSE);
        return 0;
    }

    return 1;
}
*************/

static gboolean
Check_DefaultComment (void)
{
    const gchar *file;

    file = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(DefaultComment))));
    if (!file || g_utf8_strlen(file, -1) < 1)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SetDefaultComment),FALSE);

    return TRUE;    /* A blank entry is ignored */
}

/*
 * Check if player binary is found
 */
static gboolean
Check_FilePlayerCombo (void)
{
    gchar *program_path = NULL;
    gchar *program_path_validated = NULL;

#ifdef WIN32
    return TRUE; /* FIXME see Check_If_Executable_Exists */
    /* Note : Check_If_Executable_Exists crashes when player is 'winamp.exe' with g_find_program_in_path */
#endif

    // The program typed
    program_path = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(FilePlayerCombo)))));
    g_strstrip(program_path);
    // The program file validated
    if (program_path && strlen(program_path)>0)
        program_path_validated = Check_If_Executable_Exists(program_path);

    if ( program_path && strlen(program_path)>0 && !program_path_validated ) // A file is typed but it is invalid!
    {
        GtkWidget *msgdialog = gtk_message_dialog_new(GTK_WINDOW(OptionsWindow),
                                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_CLOSE,
                                                      _("The audio file player '%s' cannot be found"),
                                                      program_path);
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Audio Player Error"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);

        g_free(program_path);
        return FALSE;
    } else
    {
        g_free(program_path);
        g_free(program_path_validated);
        return TRUE;
    }
}

static gboolean
Check_Config (void)
{
    if (Check_DefaultPathToMp3 ()
        && Check_DefaultComment ()
        && Check_FilePlayerCombo ())
        return TRUE; /* No problem detected */
    else
        return FALSE; /* Oops! */
}



/*
 * Manage Check buttons into Scanner tab: conversion group
 * This reproduces "something" like the behaviour of radio buttons with check buttons
 */
static void
Scanner_Convert_Check_Button_Toggled_1 (GtkWidget *object_rec,
                                        GtkWidget *object_emi)
{
    if (!object_rec || !object_emi) return;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object_emi)) == TRUE)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(object_rec),!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object_emi)));
}

static void
DefaultPathToMp3_Combo_Add_String (void)
{
    const gchar *path;

    path = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(DefaultPathToMp3))));
    Add_String_To_Combo_List(GTK_LIST_STORE(DefaultPathModel), path);
}

static void
CddbLocalPath_Combo_Add_String (void)
{
    const gchar *path;

    path = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(CddbLocalPath))));
    Add_String_To_Combo_List(GTK_LIST_STORE(CddbLocalPath), path);
}


