/* easytag.c - 2000/04/28 */
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


#include <config.h> // For definition of ENABLE_OGG
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n-lib.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#ifdef ENABLE_MP3
#   include <id3tag.h>
#endif
#if defined ENABLE_MP3 && defined ENABLE_ID3LIB
#   include <id3.h>
#endif
#include <sys/types.h>
#include <utime.h>

#include "easytag.h"
#include "browser.h"
#include "log.h"
#include "misc.h"
#include "bar.h"
#include "prefs.h"
#include "setting.h"
#include "scan.h"
#include "mpeg_header.h"
#include "id3_tag.h"
#include "ogg_tag.h"
#include "msgbox.h"
#include "et_core.h"
#include "cddb.h"
#include "picture.h"
#include "charset.h"

#ifdef WIN32
#   include "win32/win32dep.h"
#endif

#include "../pixmaps/EasyTAG_icon.xpm"


/****************
 * Declarations *
 ****************/
guint idle_handler_id;
guint progressbar_index;  /* An integer to set the value of progress bar into the recurse fonction */

GtkWidget *QuitRecursionWindow = NULL;

/* Used to force to hide the msgbox when saving tag */
gboolean SF_HideMsgbox_Write_Tag;
/* To remember which button was pressed when saving tag */
gint     SF_ButtonPressed_Write_Tag;
/* Used to force to hide the msgbox when renaming file */
gboolean SF_HideMsgbox_Rename_File;
/* To remember which button was pressed when renaming file */
gint     SF_ButtonPressed_Rename_File;
/* Used to force to hide the msgbox when deleting file */
gboolean SF_HideMsgbox_Delete_File;
/* To remember which button was pressed when deleting file */
gint     SF_ButtonPressed_Delete_File;

#ifdef ENABLE_FLAC
    #include <FLAC/metadata.h>

    /* Patch from Josh Coalson
     * FLAC 1.1.3 has FLAC_API_VERSION_CURRENT == 8 *
     * by LEGACY_FLAC we mean pre-FLAC 1.1.3; in FLAC 1.1.3 the FLAC__FileDecoder was merged into the FLAC__StreamDecoder */
    #if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT < 8
    #define LEGACY_FLAC // For FLAC version < 1.1.3
    #else
    #undef LEGACY_FLAC
    #endif
#endif


/**************
 * Prototypes *
 **************/
void     Handle_Crash     (gint signal_id);
gchar   *signal_to_string (gint signal);

GtkWidget *Create_Browser_Area (void);
GtkWidget *Create_File_Area    (void);
GtkWidget *Create_Tag_Area     (void);

void Menu_Mini_Button_Clicked (GtkEntry *entry);
void Mini_Button_Clicked      (GObject *object);
void Disable_Command_Buttons (void);
void Clear_Tag_Entry_Fields  (void);
void Clear_File_Entry_Field  (void);
void Clear_Header_Fields     (void);

gint Make_Dir         (const gchar *dirname_old, const gchar *dirname_new);
gint Remove_Dir       (const gchar *dirname_old, const gchar *dirname_new);
gboolean Write_File_Tag   (ET_File *ETFile, gboolean hide_msgbox);
gboolean Rename_File      (ET_File *ETFile, gboolean hide_msgbox);
gint Save_File        (ET_File *ETFile, gboolean multiple_files, gboolean force_saving_files);
gint Delete_File      (ET_File *ETFile, gboolean multiple_files);
gint Save_All_Files_With_Answer        (gboolean force_saving_files);
gint Save_Selected_Files_With_Answer   (gboolean force_saving_files);
gint Save_List_Of_Files                (GList *etfilelist, gboolean force_saving_files);
gint Delete_Selected_Files_With_Answer (void);
gint Copy_File (gchar const *fileold, gchar const *filenew);

void Display_Usage (void);

void Init_Load_Default_Dir (void);
void EasyTAG_Exit (void);
void Quit_MainWindow_Ok_Button (void);

GList *Read_Directory_Recursively (GList *file_list, gchar *path, gint recurse);
void Open_Quit_Recursion_Function_Window    (void);
void Destroy_Quit_Recursion_Function_Window (void);
void Quit_Recursion_Function_Button_Pressed (void);
void Quit_Recursion_Window_Key_Press (GtkWidget *window, GdkEvent *event);



/********
 * Main *
 ********/
#ifdef WIN32
int easytag_main (struct HINSTANCE__ *hInstance, int argc, char *argv[]) /* entry point of DLL */
#else
int main (int argc, char *argv[])
#endif
{
    GtkWidget *MainVBox;
    GtkWidget *HBox, *VBox;
    gboolean created_settings;
    struct stat statbuf;
    //GError *error = NULL;
    //GdkPixbuf *pixbuf;
    GdkPixmap *pixmap;
    GdkBitmap *mask;


#ifdef WIN32
    weasytag_init();
    //ET_Win32_Init(hInstance);
#else
    /* Signal handling to display a message(SIGSEGV, ...) */
    signal(SIGBUS,Handle_Crash);
    signal(SIGFPE,Handle_Crash);
    signal(SIGSEGV,Handle_Crash);
    // Must handle this signal to avoid zombie of applications executed (ex: xmms)
    signal(SIGCHLD,SIG_IGN); // Fix me! : can't run nautilus 1.0.6 with "Browse Directory With"
#endif

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
    /* Initialize i18n support */
    //gtk_set_locale();
#endif
    Charset_Insert_Locales_Init();

    /* Initialize GTK */
    gtk_init(&argc, &argv);

    /* Get home variable */
#ifdef WIN32
	HOME_VARIABLE = (gchar *)weasytag_data_dir();
#else
	HOME_VARIABLE = g_get_home_dir();
    //HOME_VARIABLE = (gchar *)g_getenv("HOME");
#endif

    INIT_DIRECTORY = NULL;

    /* Starting messages */
    Log_Print(LOG_OK,_("Starting EasyTAG %s (PId: %d) ..."),VERSION,getpid());
#ifdef ENABLE_MP3
    Log_Print(LOG_OK,_("Currently using libid3tag version %s ..."), ID3_VERSION);
#endif
#if defined ENABLE_MP3 && defined ENABLE_ID3LIB
    Log_Print(LOG_OK,_("Currently using id3lib version %d.%d.%d ..."),ID3LIB_MAJOR_VERSION,
                                                               ID3LIB_MINOR_VERSION,
                                                               ID3LIB_PATCH_VERSION);
#endif

#ifdef WIN32
    if (g_getenv("EASYTAGLANG"))
        Log_Print(LOG_OK,_("Variable EASYTAGLANG defined. Setting locale : '%s'"),g_getenv("EASYTAGLANG"));
    else
        Log_Print(LOG_OK,_("Setting locale : '%s'"),g_getenv("LANG"));
#endif

    if (get_locale())
        Log_Print(LOG_OK,_("Currently using locale '%s' (and eventually '%s')..."),
                get_locale(),get_encoding_from_locale(get_locale()));


    /* Create all config files */
    created_settings = Setting_Create_Files();
    /* Load Config */
    Init_Config_Variables();
    Read_Config();
    /* Display_Config(); // <- for debugging */

    /* Check given arguments */
    if (argc>1)
    {
        if ( (strcmp(argv[1],"--version")==0) || (strcmp(argv[1],"-v")==0) ) // Query version
        {
            g_print(_("%s %s by %s (compiled %s, %s)\n"),APPNAME,VERSION,AUTHOR,__TIME__,__DATE__);
            g_print(_("E-mail: %s"),EMAIL"\n");
            g_print(_("Web Page: %s"),WEBPAGE"\n");
            exit (0);
        }else if ( (strcmp(argv[1],"--help")==0) || (strcmp(argv[1],"-h")==0) ) // Query help
        {
            Display_Usage();
        }else
        {
            gchar *path2check = NULL, *path2check_tmp = NULL;
            gint resultstat;
            gchar **pathsplit;
            gint ps_index = 0;

            // Check if relative or absolute path
            if (g_path_is_absolute(argv[1])) // Passed an absolute path
            {
                path2check = g_strdup(argv[1]);
            }else                            // Passed a relative path
            {
                gchar *curdir = g_get_current_dir();
                path2check = g_strconcat(g_get_current_dir(),G_DIR_SEPARATOR_S,argv[1],NULL);
                g_free(curdir);
            }

#ifdef WIN32
            ET_Win32_Path_Replace_Slashes(path2check);
#endif
            // Check if contains hidden directories
            pathsplit = g_strsplit(path2check,G_DIR_SEPARATOR_S,0);
            g_free(path2check);
            path2check = NULL;

            // Browse the list to build again the path
            //FIX ME : Should manage directory ".." in path
            while (pathsplit[ps_index])
            {
                // Activate hidden directories in browser if path contains a dir like ".hidden_dir"
                if ( (g_ascii_strcasecmp (pathsplit[ps_index],"..")   != 0)
                &&   (g_ascii_strncasecmp(pathsplit[ps_index],".", 1) == 0)
                &&   (strlen(pathsplit[ps_index]) > 1) )
                    BROWSE_HIDDEN_DIR = 1; // If user saves the config for this session, this value will be saved to 1

                if (pathsplit[ps_index]
                && g_ascii_strcasecmp(pathsplit[ps_index],".") != 0
                && g_ascii_strcasecmp(pathsplit[ps_index],"")  != 0)
                {
                    if (path2check)
                    {
                        path2check_tmp = g_strconcat(path2check,G_DIR_SEPARATOR_S,pathsplit[ps_index],NULL);
                    }else
                    {
#ifdef WIN32
                        // Build a path starting with the drive letter
                        path2check_tmp = g_strdup(pathsplit[ps_index]);
#else
                        path2check_tmp = g_strconcat(G_DIR_SEPARATOR_S,pathsplit[ps_index],NULL);
#endif
                    }

                    path2check = g_strdup(path2check_tmp);
                    g_free(path2check_tmp);
                }
                ps_index++;
            }

            // Check if file or directory
            resultstat = stat(path2check,&statbuf);
            if (resultstat==0 && S_ISDIR(statbuf.st_mode))       // Directory
            {
                INIT_DIRECTORY = g_strdup(path2check);
            }else if (resultstat==0 && S_ISREG(statbuf.st_mode)) // File
            {
                // When passing a file, we load only the directory
                INIT_DIRECTORY = g_path_get_dirname(path2check);
            }else
            {
                g_print(_("Unknown parameter or path '%s'\n"),argv[1]);
                Display_Usage();
            }
            g_free(path2check);
        }
    }


    /* Initialization */
    ET_Core_Create();
    Main_Stop_Button_Pressed = 0;
    Init_Custom_Icons();
    Init_Mouse_Cursor();
    Init_OptionsWindow();
    Init_ScannerWindow();
    Init_CddbWindow();
    BrowserEntryModel    = NULL;
    TrackEntryComboModel = NULL;
    GenreComboModel      = NULL;

    /* The main window */
    MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(MainWindow),APPNAME" "VERSION);
    // This part is needed to set correctly the position of handle panes
    gtk_window_set_default_size(GTK_WINDOW(MainWindow),MAIN_WINDOW_WIDTH,MAIN_WINDOW_HEIGHT);

    g_signal_connect(G_OBJECT(MainWindow),"delete_event",G_CALLBACK(Quit_MainWindow),NULL);
    g_signal_connect(G_OBJECT(MainWindow),"destroy",G_CALLBACK(Quit_MainWindow),NULL);

    /* Minimised window icon */
    gtk_widget_realize(MainWindow);

    pixmap = gdk_pixmap_create_from_xpm_d(MainWindow->window,&mask,NULL,EasyTAG_icon_xpm);
    gdk_window_set_icon(MainWindow->window,(GdkWindow *)NULL,pixmap,mask);
    /*pixbuf = gdk_pixbuf_new_from_file(PACKAGE_DATA_DIR"/EasyTAG_icon.png",&error);
    if (pixbuf)
    {
        gtk_window_set_icon(GTK_WINDOW(MainWindow),pixbuf);
        g_object_unref(G_OBJECT(pixbuf));
    }else
    {
        Log_Print(LOG_ERROR,error->message);
        g_error_free(error);
    }*/

    /* MainVBox for Menu bar + Tool bar + "Browser Area & FileArea & TagArea" + Log Area + "Status bar & Progress bar" */
    MainVBox = gtk_vbox_new(FALSE,0);
    gtk_container_add (GTK_CONTAINER(MainWindow),MainVBox);
    gtk_widget_show(MainVBox);

    /* Menu bar and tool bar */
    Create_UI(&MenuArea, &ToolArea);
    gtk_box_pack_start(GTK_BOX(MainVBox),MenuArea,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(MainVBox),ToolArea,FALSE,FALSE,0);


    /* The two panes: BrowserArea on the left, FileArea+TagArea on the right */
    MainWindowHPaned = gtk_hpaned_new();
    //gtk_box_pack_start(GTK_BOX(MainVBox),MainWindowHPaned,TRUE,TRUE,0);
    gtk_paned_set_position(GTK_PANED(MainWindowHPaned),PANE_HANDLE_POSITION1);
    gtk_widget_show(MainWindowHPaned);

    /* Browser (Tree + File list + Entry) */
    BrowseArea = Create_Browser_Area();
    gtk_paned_pack1(GTK_PANED(MainWindowHPaned),BrowseArea,TRUE,TRUE);

    /* Vertical box for FileArea + TagArea */
    VBox = gtk_vbox_new(FALSE,0);
    gtk_paned_pack2(GTK_PANED(MainWindowHPaned),VBox,FALSE,FALSE);
    gtk_widget_show(VBox);

    /* File */
    FileArea = Create_File_Area();
    gtk_box_pack_start(GTK_BOX(VBox),FileArea,FALSE,FALSE,0);

    /* Tag */
    TagArea = Create_Tag_Area();
    gtk_box_pack_start(GTK_BOX(VBox),TagArea,FALSE,FALSE,0);

    /* Vertical pane for Browser Area + FileArea + TagArea */
    MainWindowVPaned = gtk_vpaned_new();
    gtk_box_pack_start(GTK_BOX(MainVBox),MainWindowVPaned,TRUE,TRUE,0);
    gtk_paned_pack1(GTK_PANED(MainWindowVPaned),MainWindowHPaned,TRUE,FALSE);
    gtk_paned_set_position(GTK_PANED(MainWindowVPaned),PANE_HANDLE_POSITION4);
    gtk_widget_show(MainWindowVPaned);


    /* Log */
    LogArea = Create_Log_Area();
    gtk_paned_pack2(GTK_PANED(MainWindowVPaned),LogArea,FALSE,TRUE);

    /* Horizontal box for Status bar + Progress bar */
    HBox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(MainVBox),HBox,FALSE,FALSE,0);
    gtk_widget_show(HBox);

    /* Status bar */
    StatusArea = Create_Status_Bar();
    gtk_box_pack_start(GTK_BOX(HBox),StatusArea,TRUE,TRUE,0);

    /* Progress bar */
    ProgressArea = Create_Progress_Bar();
    gtk_box_pack_end(GTK_BOX(HBox),ProgressArea,FALSE,FALSE,0);

    gtk_widget_show(MainWindow);

    if (SET_MAIN_WINDOW_POSITION)
        gdk_window_move(MainWindow->window, MAIN_WINDOW_X, MAIN_WINDOW_Y);

    /* Load the default dir when the UI is created and displayed
     * to the screen and open also the scanner window */
    idle_handler_id = g_idle_add((GtkFunction)Init_Load_Default_Dir,NULL);

    /* Enter the event loop */
    gtk_main ();
    return 0;
}


GtkWidget *Create_Browser_Area (void)
{
    GtkWidget *Frame;
    GtkWidget *Tree;

    Frame = gtk_frame_new(_("Browser"));
    gtk_container_set_border_width(GTK_CONTAINER(Frame), 2);

    Tree = Create_Browser_Items(MainWindow);
    gtk_container_add(GTK_CONTAINER(Frame),Tree);

    /* Don't load init dir here because Tag area hasn't been yet created!.
     * It will be load at the end of the main function */
    //Browser_Tree_Select_Dir(DEFAULT_PATH_TO_MP3);

    gtk_widget_show(Frame);
    return Frame;
}


GtkWidget *Create_File_Area (void)
{
    GtkWidget *VBox, *HBox;
    GtkWidget *Separator;
    GtkTooltips *Tips;


    FileFrame = gtk_frame_new(_("File"));
    gtk_container_set_border_width(GTK_CONTAINER(FileFrame),2);

    VBox = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(FileFrame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox),2);

    /* Tips */
    Tips = gtk_tooltips_new();

    /* HBox for FileEntry and IconBox */
    HBox = gtk_hbox_new(FALSE,2);
    gtk_box_pack_start(GTK_BOX(VBox),HBox,TRUE,TRUE,0);

    /* File index (position in list + list length) */
    FileIndex = gtk_label_new("0/0:");
    gtk_box_pack_start(GTK_BOX(HBox),FileIndex,FALSE,FALSE,0);

    /* File name */
    FileEntry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(FileEntry), TRUE);
    gtk_box_pack_start(GTK_BOX(HBox),FileEntry,TRUE,TRUE,2);

    /* Access status icon */
    ReadOnlyStatusIconBox = Create_Pixmap_Icon_With_Event_Box("easytag-read-only");
    gtk_box_pack_start(GTK_BOX(HBox),ReadOnlyStatusIconBox,FALSE,FALSE,0);
    gtk_tooltips_set_tip(Tips,ReadOnlyStatusIconBox,_("Read Only File"),NULL);
    BrokenStatusIconBox = Create_Pixmap_Icon_With_Event_Box(GTK_STOCK_MISSING_IMAGE);
    gtk_box_pack_start(GTK_BOX(HBox),BrokenStatusIconBox,FALSE,FALSE,0);
    gtk_tooltips_set_tip(Tips,BrokenStatusIconBox,_("File Link Broken"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(FileEntry));


    /*
     *  File Infos
     */
    HeaderInfosTable = gtk_table_new(3,5,FALSE);
    gtk_container_add(GTK_CONTAINER(VBox),HeaderInfosTable);
    gtk_container_set_border_width(GTK_CONTAINER(HeaderInfosTable),2);
    gtk_table_set_row_spacings(GTK_TABLE(HeaderInfosTable),1);
    gtk_table_set_col_spacings(GTK_TABLE(HeaderInfosTable),2);

    VersionLabel = gtk_label_new(_("MPEG"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),VersionLabel,0,1,0,1);
    VersionValueLabel = gtk_label_new(_("?, Layer ?"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),VersionValueLabel,1,2,0,1);
    gtk_misc_set_alignment(GTK_MISC(VersionLabel),1,0.5);
    gtk_misc_set_alignment(GTK_MISC(VersionValueLabel),0,0.5);

    BitrateLabel = gtk_label_new(_("Bitrate:"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),BitrateLabel,0,1,1,2);
    BitrateValueLabel = gtk_label_new(_("? kb/s"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),BitrateValueLabel,1,2,1,2);
    gtk_misc_set_alignment(GTK_MISC(BitrateLabel),1,0.5);
    gtk_misc_set_alignment(GTK_MISC(BitrateValueLabel),0,0.5);

    SampleRateLabel = gtk_label_new(_("Freq:"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),SampleRateLabel,0,1,2,3);
    SampleRateValueLabel = gtk_label_new(_("? Hz"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),SampleRateValueLabel,1,2,2,3);
    gtk_misc_set_alignment(GTK_MISC(SampleRateLabel),1,0.5);
    gtk_misc_set_alignment(GTK_MISC(SampleRateValueLabel),0,0.5);

    Separator = gtk_vseparator_new();
    gtk_table_attach(GTK_TABLE(HeaderInfosTable),Separator,2,3,0,4,GTK_FILL,GTK_FILL,0,0);

    ModeLabel = gtk_label_new(_("Mode:"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),ModeLabel,3,4,0,1);
    ModeValueLabel = gtk_label_new(_("?"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),ModeValueLabel,4,5,0,1);
    gtk_misc_set_alignment(GTK_MISC(ModeLabel),1,0.5);
    gtk_misc_set_alignment(GTK_MISC(ModeValueLabel),0,0.5);

    SizeLabel = gtk_label_new(_("Size:"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),SizeLabel,3,4,1,2);
    SizeValueLabel = gtk_label_new(_("? kb"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),SizeValueLabel,4,5,1,2);
    gtk_misc_set_alignment(GTK_MISC(SizeLabel),1,0.5);
    gtk_misc_set_alignment(GTK_MISC(SizeValueLabel),0,0.5);

    DurationLabel = gtk_label_new(_("Time:"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),DurationLabel,3,4,2,3);
    DurationValueLabel = gtk_label_new(_("?"));
    gtk_table_attach_defaults(GTK_TABLE(HeaderInfosTable),DurationValueLabel,4,5,2,3);
    gtk_misc_set_alignment(GTK_MISC(DurationLabel),1,0.5);
    gtk_misc_set_alignment(GTK_MISC(DurationValueLabel),0,0.5);

    gtk_widget_show(FileFrame);
    gtk_widget_show(VBox);
    gtk_widget_show(HBox);
    gtk_widget_show(FileIndex);
    gtk_widget_show(FileEntry);
    if (SHOW_HEADER_INFO)
        gtk_widget_show_all(HeaderInfosTable);
    return FileFrame;
}

#include "../pixmaps/sequence_track.xpm"
GtkWidget *Create_Tag_Area (void)
{
    GtkWidget *Separator;
    GtkWidget *Frame;
    GtkWidget *Table;
    GtkWidget *Label;
    GtkWidget *Icon;
    GtkWidget *VBox;
    GtkWidget *hbox;
    GList *focusable_widgets_list = NULL;
    //GtkWidget *ScrollWindow;
    //GtkTextBuffer *TextBuffer;
    GtkEntryCompletion *completion;
    GtkTooltips *Tips;
    gint MButtonSize = 13;
    gint TablePadding = 2;

    // For Picture
    static const GtkTargetEntry drops[] = { { "text/uri-list", 0, TARGET_URI_LIST } };
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;


    /* Tips */
    Tips = gtk_tooltips_new();

    /* Main Frame */
    TagFrame = gtk_frame_new(_("Tag"));
    gtk_container_set_border_width(GTK_CONTAINER(TagFrame),2);

    /* Box for the notebook (only for setting a border) */
    VBox = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(TagFrame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox),2);

    /*
     * Note book
     */
    TagNoteBook = gtk_notebook_new();
    gtk_notebook_popup_enable(GTK_NOTEBOOK(TagNoteBook));
    //gtk_container_add(GTK_CONTAINER(TagFrame),TagNoteBook);
    gtk_box_pack_start(GTK_BOX(VBox),TagNoteBook,TRUE,TRUE,0);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(TagNoteBook),GTK_POS_TOP);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(TagNoteBook),FALSE);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(TagNoteBook));

    /*
     * 1 - Page for common tag fields
     */
    Label = gtk_label_new(_("Common"));
    Frame = gtk_frame_new(NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(TagNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame), 0);
    gtk_frame_set_shadow_type(GTK_FRAME(Frame),GTK_SHADOW_NONE); // Hide the Frame


    Table = gtk_table_new(11,11,FALSE);
    gtk_container_add(GTK_CONTAINER(Frame),Table);
    gtk_container_set_border_width(GTK_CONTAINER(Table),2);

    /* Title */
    TitleLabel = gtk_label_new(_("Title:"));
    gtk_table_attach(GTK_TABLE(Table),TitleLabel,0,1,0,1,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(TitleLabel),1,0.5);

    TitleEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),TitleEntry,1,10,0,1,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);
    gtk_widget_set_size_request(TitleEntry, 150, -1);

    TitleMButton = gtk_button_new();
    gtk_widget_set_size_request(TitleMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),TitleMButton,10,11,0,1,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(TitleMButton),"clicked", G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,TitleMButton,_("Tag selected files with this title"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(TitleEntry));
    g_object_set_data(G_OBJECT(TitleEntry),"MButtonName",TitleMButton);


    /* Artist */
    ArtistLabel = gtk_label_new(_("Artist:"));
    gtk_table_attach(GTK_TABLE(Table),ArtistLabel,0,1,1,2,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(ArtistLabel),1,0.5);

    ArtistEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),ArtistEntry,1,10,1,2,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);

    ArtistMButton = gtk_button_new();
    gtk_widget_set_size_request(ArtistMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),ArtistMButton,10,11,1,2,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(ArtistMButton),"clicked", G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,ArtistMButton,_("Tag selected files with this artist"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(ArtistEntry));
    g_object_set_data(G_OBJECT(ArtistEntry),"MButtonName",ArtistMButton);

    /* Album */
    AlbumLabel = gtk_label_new(_("Album:"));
    gtk_table_attach(GTK_TABLE(Table),AlbumLabel,0,1,2,3,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(AlbumLabel),1,0.5);

    AlbumEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),AlbumEntry,1,7,2,3,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);

    AlbumMButton = gtk_button_new();
    //gtk_widget_set_size_request(AlbumMButton, 10, 10);
    gtk_widget_set_size_request(AlbumMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),AlbumMButton,7,8,2,3,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(AlbumMButton),"clicked",G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,AlbumMButton,_("Tag selected files with this album name"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(AlbumEntry));
    g_object_set_data(G_OBJECT(AlbumEntry),"MButtonName",AlbumMButton);

    /* Disc Number */
    DiscNumberLabel = gtk_label_new(_("CD:"));
    gtk_table_attach(GTK_TABLE(Table),DiscNumberLabel,8,9,2,3,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(DiscNumberLabel),1,0.5);

    DiscNumberEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),DiscNumberEntry,9,10,2,3,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);
    gtk_widget_set_size_request(DiscNumberEntry,30,-1);
    // FIX ME should allow to type only something like : 1/3
    //g_signal_connect(G_OBJECT(GTK_ENTRY(DiscNumberEntry)),"insert_text",G_CALLBACK(Insert_Only_Digit),NULL);

    DiscNumberMButton = gtk_button_new();
    //gtk_widget_set_size_request(DiscNumberMButton, 10, 10);
    gtk_widget_set_size_request(DiscNumberMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),DiscNumberMButton,10,11,2,3,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(DiscNumberMButton),"clicked",G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,DiscNumberMButton,_("Tag selected files with this disc number"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(DiscNumberEntry));
    g_object_set_data(G_OBJECT(DiscNumberEntry),"MButtonName",DiscNumberMButton);

    /* Year */
    YearLabel = gtk_label_new(_("Year:"));
    gtk_table_attach(GTK_TABLE(Table),YearLabel,0,1,3,4,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(YearLabel),1,0.5);

    YearEntry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(YearEntry), 4);
    gtk_table_attach(GTK_TABLE(Table),YearEntry,1,2,3,4,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);
    gtk_widget_set_size_request(YearEntry,37,-1);
    g_signal_connect(G_OBJECT(YearEntry),"insert_text",G_CALLBACK(Insert_Only_Digit),NULL);
    g_signal_connect(G_OBJECT(YearEntry),"activate",G_CALLBACK(Parse_Date),NULL);
    g_signal_connect(G_OBJECT(YearEntry),"focus-out-event",G_CALLBACK(Parse_Date),NULL);

    YearMButton = gtk_button_new();
    gtk_widget_set_size_request(YearMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),YearMButton,2,3,3,4,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(YearMButton),"clicked",G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,YearMButton,_("Tag selected files with this year"),NULL);

    /* Small vertical separator */
    Separator = gtk_vseparator_new();
    gtk_table_attach(GTK_TABLE(Table),Separator,3,4,3,4,GTK_FILL,GTK_FILL,TablePadding,TablePadding);


    /* Track and Track total */
    TrackMButtonSequence = gtk_button_new();
    gtk_widget_set_size_request(TrackMButtonSequence,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),TrackMButtonSequence,4,5,3,4,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(TrackMButtonSequence),"clicked",G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,TrackMButtonSequence,_("Number selected tracks sequentially. "
                                                     "Starts at 01 in each subdirectory."), NULL);
    // Pixmap into TrackMButtonSequence button
    //Icon = gtk_image_new_from_stock("easytag-sequence-track", GTK_ICON_SIZE_BUTTON); // FIX ME : it doesn't display the good size of the '#'
    Icon = Create_Xpm_Image((const char **)sequence_track_xpm);
    gtk_container_add(GTK_CONTAINER(TrackMButtonSequence),Icon);
    GTK_WIDGET_UNSET_FLAGS(TrackMButtonSequence,GTK_CAN_DEFAULT); // To have enought space to display the icon
    GTK_WIDGET_UNSET_FLAGS(TrackMButtonSequence,GTK_CAN_FOCUS);   // To have enought space to display the icon

    TrackLabel = gtk_label_new(_("Track #:"));
    gtk_table_attach(GTK_TABLE(Table),TrackLabel,5,6,3,4,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(TrackLabel),1,0.5);

    if (TrackEntryComboModel != NULL)
        gtk_list_store_clear(TrackEntryComboModel);
    else
        TrackEntryComboModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);

    TrackEntryCombo = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(TrackEntryComboModel), MISC_COMBO_TEXT);
    gtk_table_attach(GTK_TABLE(Table),TrackEntryCombo,6,7,3,4,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);
    gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(TrackEntryCombo),3); // Three columns to display track numbers list

    gtk_widget_set_size_request(TrackEntryCombo,50,-1);
    g_signal_connect(G_OBJECT(GTK_ENTRY(GTK_BIN(TrackEntryCombo)->child)),"insert_text",
        G_CALLBACK(Insert_Only_Digit),NULL);

    Label = gtk_label_new("/");
    gtk_table_attach(GTK_TABLE(Table),Label,7,8,3,4,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(Label),0.5,0.5);

    TrackMButtonNbrFiles = gtk_button_new();
    gtk_widget_set_size_request(TrackMButtonNbrFiles,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),TrackMButtonNbrFiles,8,9,3,4,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(TrackMButtonNbrFiles),"clicked",G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,TrackMButtonNbrFiles,_("Set the number of files, in the same directory of the displayed file, to the selected tracks."), NULL);
    // Pixmap into TrackMButtonNbrFiles button
    //Icon = gtk_image_new_from_stock("easytag-sequence-track", GTK_ICON_SIZE_BUTTON);
    Icon = Create_Xpm_Image((const char **)sequence_track_xpm);
    gtk_container_add(GTK_CONTAINER(TrackMButtonNbrFiles),Icon);
    GTK_WIDGET_UNSET_FLAGS(TrackMButtonNbrFiles,GTK_CAN_DEFAULT); // To have enought space to display the icon
    GTK_WIDGET_UNSET_FLAGS(TrackMButtonNbrFiles,GTK_CAN_FOCUS);   // To have enought space to display the icon

    TrackTotalEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),TrackTotalEntry,9,10,3,4,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);
    gtk_widget_set_size_request(TrackTotalEntry,30,-1);
    g_signal_connect(G_OBJECT(GTK_ENTRY(TrackTotalEntry)),"insert_text",
        G_CALLBACK(Insert_Only_Digit),NULL);

    TrackMButton = gtk_button_new();
    gtk_widget_set_size_request(TrackMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),TrackMButton,10,11,3,4,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(TrackMButton),"clicked",G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,TrackMButton,_("Tag selected files with this number of tracks"),NULL);

    g_object_set_data(G_OBJECT(GTK_ENTRY(GTK_BIN(TrackEntryCombo)->child)),"MButtonName",TrackMButton);
    g_object_set_data(G_OBJECT(TrackTotalEntry),"MButtonName",TrackMButton);

    /* Genre */
    GenreLabel = gtk_label_new(_("Genre:"));
    gtk_table_attach(GTK_TABLE(Table),GenreLabel,0,1,4,5,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(GenreLabel),1,0.5);

    if (GenreComboModel != NULL)
        gtk_list_store_clear(GenreComboModel);
    else
        GenreComboModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);
    GenreCombo = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(GenreComboModel), MISC_COMBO_TEXT);
    completion = gtk_entry_completion_new();
    gtk_entry_set_completion(GTK_ENTRY(GTK_BIN(GenreCombo)->child), completion);
    g_object_unref(completion);
    gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(GenreComboModel));
    gtk_entry_completion_set_text_column(completion, 0);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(GenreComboModel), MISC_COMBO_TEXT, Combo_Alphabetic_Sort, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(GenreComboModel), MISC_COMBO_TEXT, GTK_SORT_ASCENDING);
    gtk_table_attach(GTK_TABLE(Table),GenreCombo,1,10,4,5,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);
    Load_Genres_List_To_UI();
    gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(GenreCombo),2); // Two columns to display genres list

    GenreMButton = gtk_button_new();
    gtk_widget_set_size_request(GenreMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),GenreMButton,10,11,4,5,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(GenreMButton),"clicked",G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,GenreMButton,_("Tag selected files with this genre"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(GTK_BIN(GenreCombo)->child));
    g_object_set_data(G_OBJECT(GTK_BIN(GenreCombo)->child),"MButtonName",GenreMButton);

    /* Comment */
    CommentLabel = gtk_label_new(_("Comment:"));
    gtk_table_attach(GTK_TABLE(Table),CommentLabel,0,1,5,6,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(CommentLabel),1,0.5);

    CommentEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),CommentEntry,1,10,5,6,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);

	// Use of a text view instead of an entry...
    /******ScrollWindow = gtk_scrolled_window_new(NULL,NULL);
    gtk_table_attach(GTK_TABLE(Table),ScrollWindow,1,10,5,6,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindow), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(ScrollWindow), GTK_SHADOW_IN);
    gtk_widget_set_size_request(ScrollWindow,-1,52); // Display ~3 lines...
    TextBuffer = gtk_text_buffer_new(NULL);
    CommentView = gtk_text_view_new_with_buffer(TextBuffer);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(CommentView), GTK_WRAP_WORD); // To not display the horizontal scrollbar
    gtk_container_add(GTK_CONTAINER(ScrollWindow),CommentView);
    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(CommentView));
    *******/

    CommentMButton = gtk_button_new();
    gtk_widget_set_size_request(CommentMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),CommentMButton,10,11,5,6,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(CommentMButton),"clicked",G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,CommentMButton,_("Tag selected files with this comment"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(CommentEntry));
    g_object_set_data(G_OBJECT(CommentEntry),"MButtonName",CommentMButton);
    //Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(CommentView));
    //g_object_set_data(G_OBJECT(CommentView),"MButtonName",CommentMButton);


    /* Composer (name of the composers) */
    ComposerLabel = gtk_label_new(_("Composer:"));
    gtk_table_attach(GTK_TABLE(Table),ComposerLabel,0,1,6,7,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(ComposerLabel),1,0.5);

    ComposerEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),ComposerEntry,1,10,6,7,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);

    ComposerMButton = gtk_button_new();
    gtk_widget_set_size_request(ComposerMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),ComposerMButton,10,11,6,7,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(ComposerMButton),"clicked", G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,ComposerMButton,_("Tag selected files with this composer"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(ComposerEntry));
    g_object_set_data(G_OBJECT(ComposerEntry),"MButtonName",ComposerMButton);


    /* Original Artist / Performer */
    OrigArtistLabel = gtk_label_new(_("Orig. Artist:"));
    gtk_table_attach(GTK_TABLE(Table),OrigArtistLabel,0,1,7,8,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(OrigArtistLabel),1,0.5);

    OrigArtistEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),OrigArtistEntry,1,10,7,8,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);

    OrigArtistMButton = gtk_button_new();
    gtk_widget_set_size_request(OrigArtistMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),OrigArtistMButton,10,11,7,8,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(OrigArtistMButton),"clicked", G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,OrigArtistMButton,_("Tag selected files with this original artist"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(OrigArtistEntry));
    g_object_set_data(G_OBJECT(OrigArtistEntry),"MButtonName",OrigArtistMButton);


    /* Copyright */
    CopyrightLabel = gtk_label_new(_("Copyright:"));
    gtk_table_attach(GTK_TABLE(Table),CopyrightLabel,0,1,8,9,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(CopyrightLabel),1,0.5);

    CopyrightEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),CopyrightEntry,1,10,8,9,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);

    CopyrightMButton = gtk_button_new();
    gtk_widget_set_size_request(CopyrightMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),CopyrightMButton,10,11,8,9,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(CopyrightMButton),"clicked", G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,CopyrightMButton,_("Tag selected files with this copyright"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(CopyrightEntry));
    g_object_set_data(G_OBJECT(CopyrightEntry),"MButtonName",CopyrightMButton);


    /* URL */
    URLLabel = gtk_label_new(_("URL:"));
    gtk_table_attach(GTK_TABLE(Table),URLLabel,0,1,9,10,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(URLLabel),1,0.5);

    URLEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),URLEntry,1,10,9,10,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);

    URLMButton = gtk_button_new();
    gtk_widget_set_size_request(URLMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),URLMButton,10,11,9,10,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(URLMButton),"clicked", G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,URLMButton,_("Tag selected files with this URL"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(URLEntry));
    g_object_set_data(G_OBJECT(URLEntry),"MButtonName",URLMButton);


    /* Encoded by */
    EncodedByLabel = gtk_label_new(_("Encoded by:"));
    gtk_table_attach(GTK_TABLE(Table),EncodedByLabel,0,1,10,11,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(EncodedByLabel),1,0.5);

    EncodedByEntry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(Table),EncodedByEntry,1,10,10,11,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);

    EncodedByMButton = gtk_button_new();
    gtk_widget_set_size_request(EncodedByMButton,MButtonSize,MButtonSize);
    gtk_table_attach(GTK_TABLE(Table),EncodedByMButton,10,11,10,11,0,0,TablePadding,TablePadding);
    g_signal_connect(G_OBJECT(EncodedByMButton),"clicked", G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,EncodedByMButton,_("Tag selected files with this encoder name"),NULL);

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(EncodedByEntry));
    g_object_set_data(G_OBJECT(EncodedByEntry),"MButtonName",EncodedByMButton);


    // Managing of entries when pressing the Enter key
    g_signal_connect_swapped(G_OBJECT(TitleEntry),      "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(ArtistEntry));
    g_signal_connect_swapped(G_OBJECT(ArtistEntry),     "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(AlbumEntry));
    g_signal_connect_swapped(G_OBJECT(AlbumEntry),      "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(DiscNumberEntry));
    g_signal_connect_swapped(G_OBJECT(DiscNumberEntry), "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(YearEntry));
    g_signal_connect_swapped(G_OBJECT(YearEntry),       "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(GTK_ENTRY(GTK_BIN(TrackEntryCombo)->child)));
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(GTK_BIN(TrackEntryCombo)->child)),"activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(TrackTotalEntry));
    g_signal_connect_swapped(G_OBJECT(TrackTotalEntry), "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(GTK_BIN(GenreCombo)->child));
    g_signal_connect_swapped(G_OBJECT(GTK_BIN(GenreCombo)->child),"activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(CommentEntry));
    g_signal_connect_swapped(G_OBJECT(CommentEntry),    "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(ComposerEntry));
    g_signal_connect_swapped(G_OBJECT(ComposerEntry),   "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(OrigArtistEntry));
    g_signal_connect_swapped(G_OBJECT(OrigArtistEntry), "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(CopyrightEntry));
    g_signal_connect_swapped(G_OBJECT(CopyrightEntry),  "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(URLEntry));
    g_signal_connect_swapped(G_OBJECT(URLEntry),        "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(EncodedByEntry));
    g_signal_connect_swapped(G_OBJECT(EncodedByEntry),  "activate",G_CALLBACK(gtk_widget_grab_focus),G_OBJECT(TitleEntry));

    // Set focus chain
    focusable_widgets_list = g_list_append(focusable_widgets_list,TitleEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,TitleMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,ArtistEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,ArtistMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,AlbumEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,AlbumMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,DiscNumberEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,DiscNumberMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,YearEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,YearMButton);
    //focusable_widgets_list = g_list_append(focusable_widgets_list,TrackMButtonSequence); // Doesn't work as focus disabled for this widget to have enought space to display icon
    focusable_widgets_list = g_list_append(focusable_widgets_list,TrackEntryCombo);
    //focusable_widgets_list = g_list_append(focusable_widgets_list,TrackMButtonNbrFiles);
    focusable_widgets_list = g_list_append(focusable_widgets_list,TrackTotalEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,TrackMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,GenreCombo);
    focusable_widgets_list = g_list_append(focusable_widgets_list,GenreMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,CommentEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,CommentMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,ComposerEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,ComposerMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,OrigArtistEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,OrigArtistMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,CopyrightEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,CopyrightMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,URLEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,URLMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,EncodedByEntry);
    focusable_widgets_list = g_list_append(focusable_widgets_list,EncodedByMButton);
    focusable_widgets_list = g_list_append(focusable_widgets_list,TitleEntry); // To loop to the beginning
    gtk_container_set_focus_chain(GTK_CONTAINER(Table),focusable_widgets_list);



    /*
     * 2 - Page for extra tag fields
     */
    Label = gtk_label_new(_("Pictures")); // As there is only the picture field... - also used in ET_Display_File_Tag_To_UI
    Frame = gtk_frame_new(NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(TagNoteBook),Frame,Label);
    gtk_container_set_border_width(GTK_CONTAINER(Frame), 0);
    gtk_frame_set_shadow_type(GTK_FRAME(Frame),GTK_SHADOW_NONE); // Hide the Frame

    Table = gtk_table_new(3,5,FALSE);
    gtk_container_add(GTK_CONTAINER(Frame),Table);
    gtk_container_set_border_width(GTK_CONTAINER(Table),2);

    /* Picture */
    PictureLabel = gtk_label_new(_("Pictures:"));
    gtk_table_attach(GTK_TABLE(Table),PictureLabel,0,1,0,1,GTK_FILL,GTK_FILL,TablePadding,TablePadding);
    gtk_misc_set_alignment(GTK_MISC(PictureLabel),1,0.5);

    // Scroll window for PictureEntryView
    PictureScrollWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(PictureScrollWindow),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_table_attach(GTK_TABLE(Table), PictureScrollWindow, 1,4,0,1,
                     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,TablePadding,TablePadding);

    PictureEntryModel = gtk_list_store_new(PICTURE_COLUMN_COUNT,
                                           GDK_TYPE_PIXBUF,
                                           G_TYPE_STRING,
                                           G_TYPE_POINTER);
    PictureEntryView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(PictureEntryModel));
    //gtk_tree_view_set_reorderable(GTK_TREE_VIEW(PictureEntryView),TRUE);
    gtk_container_add(GTK_CONTAINER(PictureScrollWindow), PictureEntryView);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(PictureEntryView), FALSE);
    gtk_widget_set_size_request(PictureEntryView, -1, 200);
    gtk_tooltips_set_tip(Tips,PictureEntryView,_("You can use drag and drop to add picture."),NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(PictureEntryView));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "pixbuf", PICTURE_COLUMN_PIC,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(PictureEntryView), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text", PICTURE_COLUMN_TEXT,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(PictureEntryView), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    // Activate Drag'n'Drop for the PictureEntryView (and PictureAddButton)
    gtk_drag_dest_set(GTK_WIDGET(PictureEntryView),
                      GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                      drops, sizeof(drops) / sizeof(GtkTargetEntry),
                      GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(PictureEntryView),"drag_data_received", G_CALLBACK(Tag_Area_Picture_Drag_Data), 0);
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(Picture_Selection_Changed_cb), NULL);
    g_signal_connect(G_OBJECT(PictureEntryView), "button_press_event",G_CALLBACK(Picture_Entry_View_Button_Pressed),NULL);
    g_signal_connect(G_OBJECT(PictureEntryView),"key_press_event", G_CALLBACK(Picture_Entry_View_Key_Pressed),NULL);

    // The mini button for picture
    PictureMButton = gtk_button_new();
    gtk_table_attach(GTK_TABLE(Table),PictureMButton,4,5,0,1,0,0,TablePadding,TablePadding);
    gtk_widget_set_usize(PictureMButton,MButtonSize,MButtonSize);
    g_signal_connect(G_OBJECT(PictureMButton),"clicked",G_CALLBACK(Mini_Button_Clicked),NULL);
    gtk_tooltips_set_tip(Tips,PictureMButton,_("Tag selected files with these pictures"),NULL);

    // Picture action buttons
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_table_attach(GTK_TABLE(Table),hbox,1,4,1,2,GTK_FILL,GTK_FILL,TablePadding,TablePadding);

    PictureAddButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(PictureAddButton),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),PictureAddButton,FALSE,FALSE,0);
    gtk_tooltips_set_tip(Tips,PictureAddButton,_("Add pictures to the tag (drag and drop is also available)."),NULL);

    PictureClearButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(PictureClearButton),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),PictureClearButton,FALSE,FALSE,0);
    gtk_tooltips_set_tip(Tips,PictureClearButton,_("Remove selected pictures, else all pictures."),NULL);

    Label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    PictureSaveButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(PictureSaveButton),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),PictureSaveButton,FALSE,FALSE,0);
    gtk_widget_set_sensitive(GTK_WIDGET(PictureSaveButton), FALSE);
    gtk_tooltips_set_tip(Tips,PictureSaveButton,_("Save the selected pictures on the hard disk."),NULL);

    PicturePropertiesButton = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(PicturePropertiesButton),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),PicturePropertiesButton,FALSE,FALSE,0);
    gtk_widget_set_sensitive(GTK_WIDGET(PicturePropertiesButton), FALSE);
    gtk_tooltips_set_tip(Tips,PicturePropertiesButton,_("Set properties of the selected pictures."),NULL);

    g_signal_connect(G_OBJECT(PictureClearButton),      "clicked", G_CALLBACK(Picture_Clear_Button_Clicked), NULL);
    g_signal_connect(G_OBJECT(PictureAddButton),        "clicked", G_CALLBACK(Picture_Add_Button_Clicked), NULL);
    g_signal_connect(G_OBJECT(PictureSaveButton),       "clicked", G_CALLBACK(Picture_Save_Button_Clicked), NULL);
    g_signal_connect(G_OBJECT(PicturePropertiesButton), "clicked", G_CALLBACK(Picture_Properties_Button_Clicked), NULL);

    // Activate Drag'n'Drop for the PictureAddButton (and PictureEntryView)
    gtk_drag_dest_set(GTK_WIDGET(PictureAddButton),
                    GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                    drops, sizeof(drops) / sizeof(GtkTargetEntry),
                    GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(PictureAddButton),"drag_data_received", G_CALLBACK(Tag_Area_Picture_Drag_Data), 0);


    //Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(PictureEntryView));
    gtk_object_set_data(GTK_OBJECT(PictureEntryView),"MButtonName",PictureMButton);

    gtk_widget_show_all(TagFrame);
    return TagFrame;
}



/*
 * Actions when mini buttons are pressed: apply the field to all others files
 */
void Menu_Mini_Button_Clicked (GtkEntry *entry)
{
    if ( g_object_get_data(G_OBJECT(entry),"MButtonName") )
        Mini_Button_Clicked((GObject *)g_object_get_data(G_OBJECT(entry),"MButtonName"));
}
void Mini_Button_Clicked (GObject *object)
{
    GList *etfilelist = NULL;
    GList *selection_filelist = NULL;
    gchar *string_to_set = NULL;
    gchar *string_to_set1 = NULL;
    gchar *msg = NULL;
    ET_File *etfile;
    File_Tag *FileTag;
    GtkTreeSelection *selection;


    if (!ETCore->ETFileDisplayedList || !BrowserList) return;

    // Save the current displayed data
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    // Warning : 'selection_filelist' is not a list of 'ETFile' items!
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    selection_filelist = gtk_tree_selection_get_selected_rows(selection, NULL);
    // Create an 'ETFile' list from 'selection_filelist'
    while (selection_filelist)
    {
        etfile = Browser_List_Get_ETFile_From_Path(selection_filelist->data);
        etfilelist = g_list_append(etfilelist,etfile);

        if (!selection_filelist->next) break;
        selection_filelist = g_list_next(selection_filelist);
    }
    g_list_foreach(selection_filelist, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selection_filelist);


    if (object==G_OBJECT(TitleMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(TitleEntry),0,-1); // The string to apply to all other files
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->title,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with title '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed title from selected files."));
    }
    else if (object==G_OBJECT(ArtistMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(ArtistEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->artist,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with artist '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed artist from selected files."));
    }
    else if (object==G_OBJECT(AlbumMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(AlbumEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->album,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with album '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed album name from selected files."));
    }
    else if (object==G_OBJECT(DiscNumberMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(DiscNumberEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->disc_number,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with disc number '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed disc number from selected files."));
    }
    else if (object==G_OBJECT(YearMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(YearEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->year,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with year '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed year from selected files."));
    }
    else if (object==G_OBJECT(TrackMButton))
    {
        /* Used of Track and Total Track values */
        string_to_set = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(TrackEntryCombo)->child)));
        string_to_set1 = gtk_editable_get_chars(GTK_EDITABLE(TrackTotalEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);

            // We apply the TrackEntry field to all others files only if it is to delete
            // the field (string=""). Else we don't overwrite the track number
            if (!string_to_set || g_utf8_strlen(string_to_set, -1) == 0)
                ET_Set_Field_File_Tag_Item(&FileTag->track,string_to_set);
            ET_Set_Field_File_Tag_Item(&FileTag->track_total,string_to_set1);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }

        if ( string_to_set && g_utf8_strlen(string_to_set, -1) > 0 ) //&& atoi(string_to_set)>0 )
        {
            if ( string_to_set1 != NULL && g_utf8_strlen(string_to_set1, -1)>0 ) //&& atoi(string_to_set1)>0 )
            {
                msg = g_strdup_printf(_("Selected files tagged with track like 'xx/%s'."),string_to_set1);
            }else
            {
                msg = g_strdup_printf(_("Selected files tagged with track like 'xx'."));
            }
        }else
        {
            msg = g_strdup(_("Removed track number from selected files."));
        }
    }
    else if (object==G_OBJECT(TrackMButtonSequence))
    {
        /* This part doesn't set the same track number to all files, but sequence the tracks.
         * So we must browse the whole 'etfilelistfull' to get position of each selected file.
         * Note : 'etfilelistfull' and 'etfilelist' must be sorted in the same order */
        GList *etfilelistfull = NULL;
        gchar *path = NULL;
        gchar *path1 = NULL;
        gint i = 0;

        /* FIX ME!: see to fill also the Total Track (it's a good idea?) */
        etfilelistfull = g_list_first(ETCore->ETFileList);

        // Sort 'etfilelistfull' and 'etfilelist' in the same order
        etfilelist     = ET_Sort_File_List(etfilelist,SORTING_FILE_MODE);
        etfilelistfull = ET_Sort_File_List(etfilelistfull,SORTING_FILE_MODE);

        while (etfilelist && etfilelistfull)
        {
            // To get the path of the file
            File_Name *FileNameCur = (File_Name *)((ET_File *)etfilelistfull->data)->FileNameCur->data;
            // The ETFile in the selected file list
            etfile = etfilelist->data;

            // Restart counter when entering a new directory
            g_free(path1);
            path1 = g_path_get_dirname(FileNameCur->value);
            if ( path && path1 && strcmp(path,path1)!=0 )
                i = 0;
            if (NUMBER_TRACK_FORMATED)
                string_to_set = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,++i);
            else
                string_to_set = g_strdup_printf("%d",++i);

            // The file is in the selection?
            if ( (ET_File *)etfilelistfull->data == etfile )
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(etfile,FileTag);
                ET_Set_Field_File_Tag_Item(&FileTag->track,string_to_set);
                ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

                if (!etfilelist->next) break;
                etfilelist = g_list_next(etfilelist);
            }

            g_free(string_to_set);
            g_free(path);
            path = g_strdup(path1);

            etfilelistfull = g_list_next(etfilelistfull);
        }
        g_free(path);
        g_free(path1);
        //msg = g_strdup_printf(_("All %d tracks numbered sequentially."), ETCore->ETFileSelectionList_Length);
        msg = g_strdup_printf(_("Selected tracks numbered sequentially."));
    }
    else if (object==G_OBJECT(TrackMButtonNbrFiles))
    {
        /* Used of Track and Total Track values */
        while (etfilelist)
        {
            gchar *path_utf8, *filename_utf8;

            etfile        = (ET_File *)etfilelist->data;
            filename_utf8 = ((File_Name *)etfile->FileNameNew->data)->value_utf8;
            path_utf8     = g_path_get_dirname(filename_utf8);
            if (NUMBER_TRACK_FORMATED)
                string_to_set = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,ET_Get_Number_Of_Files_In_Directory(path_utf8));
            else
                string_to_set = g_strdup_printf("%d",ET_Get_Number_Of_Files_In_Directory(path_utf8));
            g_free(path_utf8);
            if (!string_to_set1)
                string_to_set1 = g_strdup(string_to_set); // Just for the message below...

            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->track_total,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }

        if ( string_to_set1 != NULL && g_utf8_strlen(string_to_set1, -1)>0 ) //&& atoi(string_to_set1)>0 )
        {
            msg = g_strdup_printf(_("Selected files tagged with track like 'xx/%s'."),string_to_set1);
        }else
        {
            msg = g_strdup(_("Removed track number from selected files."));
        }
    }
    else if (object==G_OBJECT(GenreMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(GTK_BIN(GenreCombo)->child),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->genre,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with genre '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed genre from selected files."));
    }
    else if (object==G_OBJECT(CommentMButton))
    {
        //GtkTextBuffer *textbuffer;
        //GtkTextIter    start_iter;
        //GtkTextIter    end_iter;
        //textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(CommentView));
        //gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(textbuffer),&start_iter,&end_iter);
        //string_to_set = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(textbuffer),&start_iter,&end_iter,TRUE);

        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(CommentEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->comment,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with comment '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed comment from selected files."));
    }
    else if (object==G_OBJECT(ComposerMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(ComposerEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->composer,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with composer '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed composer from selected files."));
    }
    else if (object==G_OBJECT(OrigArtistMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(OrigArtistEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->orig_artist,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with original artist '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed original artist from selected files."));
    }
    else if (object==G_OBJECT(CopyrightMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(CopyrightEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->copyright,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with copyright '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed copyright from selected files."));
    }
    else if (object==G_OBJECT(URLMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(URLEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->url,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with URL '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed URL from selected files."));
    }
    else if (object==G_OBJECT(EncodedByMButton))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(EncodedByEntry),0,-1);
        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->encoded_by,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with encoder name '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed encoder name from selected files."));
    }
    else if (object==G_OBJECT(PictureMButton))
    {
        Picture *res = NULL, *pic, *prev_pic = NULL;
        GtkTreeModel *model;
        GtkTreeIter iter;

        model = gtk_tree_view_get_model(GTK_TREE_VIEW(PictureEntryView));
        if (gtk_tree_model_get_iter_first(model, &iter))
        {
            do
            {
                gtk_tree_model_get(model, &iter, PICTURE_COLUMN_DATA, &pic, -1);
                pic = Picture_Copy_One(pic);
                if (!res)
                    res = pic;
                else
                    prev_pic->next = pic;
                prev_pic = pic;
            } while (gtk_tree_model_iter_next(model, &iter));
        }

        while (etfilelist)
        {
            etfile = (ET_File *)etfilelist->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Picture((Picture **)&FileTag->picture, res);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

            if (!etfilelist->next) break;
            etfilelist = g_list_next(etfilelist);
        }
        if (res)
            msg = g_strdup(_("Selected files tagged with pictures."));
        else
            msg = g_strdup(_("Removed pictures from selected files."));
        Picture_Free(res);
    }

    g_list_free(etfilelist);

    // Refresh the whole list (faster than file by file) to show changes
    Browser_List_Refresh_Whole_List();

    /* Display the current file (Needed when sequencing tracks) */
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);

    if (msg)
    {
        Log_Print(LOG_OK,"%s",msg);
        Statusbar_Message(msg,TRUE);
        g_free(msg);
    }
    g_free(string_to_set);
    g_free(string_to_set1);

    /* To update state of Undo button */
    Update_Command_Buttons_Sensivity();
}




/*
 * Action when selecting all files
 */
void Action_Select_All_Files (void)
{
    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    Browser_List_Select_All_Files();
    Update_Command_Buttons_Sensivity();
}


/*
 * Action when unselecting all files
 */
void Action_Unselect_All_Files (void)
{
    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    Browser_List_Unselect_All_Files();
    ETCore->ETFileDisplayed = NULL;
}


/*
 * Action when inverting files selection
 */
void Action_Invert_Files_Selection (void)
{
    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    Browser_List_Invert_File_Selection();
    Update_Command_Buttons_Sensivity();
}



/*
 * Action when First button is selected
 */
void Action_Select_First_File (void)
{
    GList *etfilelist;

    if (!ETCore->ETFileDisplayedList)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Go to the first item of the list */
    etfilelist = ET_Displayed_File_List_First();
    if (etfilelist)
    {
        Browser_List_Unselect_All_Files(); // To avoid the last line still selected
        Browser_List_Select_File_By_Etfile((ET_File *)etfilelist->data,TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

    Update_Command_Buttons_Sensivity();
    Scan_Rename_File_Generate_Preview();
    Scan_Fill_Tag_Generate_Preview();

    if (SET_FOCUS_TO_FIRST_TAG_FIELD)
        gtk_widget_grab_focus(GTK_WIDGET(TitleEntry));
}


/*
 * Action when Prev button is selected
 */
void Action_Select_Prev_File (void)
{
    GList *etfilelist;

    if (!ETCore->ETFileDisplayedList || !ETCore->ETFileDisplayedList->prev)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Go to the prev item of the list */
    etfilelist = ET_Displayed_File_List_Previous();
    if (etfilelist)
    {
        Browser_List_Unselect_All_Files();
        Browser_List_Select_File_By_Etfile((ET_File *)etfilelist->data,TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

//    if (!ETFileList->prev)
//        gdk_beep(); // Warm the user

    Update_Command_Buttons_Sensivity();
    Scan_Rename_File_Generate_Preview();
    Scan_Fill_Tag_Generate_Preview();

    if (SET_FOCUS_TO_FIRST_TAG_FIELD)
        gtk_widget_grab_focus(GTK_WIDGET(TitleEntry));
}


/*
 * Action when Next button is selected
 */
void Action_Select_Next_File (void)
{
    GList *etfilelist;

    if (!ETCore->ETFileDisplayedList || !ETCore->ETFileDisplayedList->next)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Go to the next item of the list */
    etfilelist = ET_Displayed_File_List_Next();
    if (etfilelist)
    {
        Browser_List_Unselect_All_Files();
        Browser_List_Select_File_By_Etfile((ET_File *)etfilelist->data,TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

//    if (!ETFileList->next)
//        gdk_beep(); // Warm the user

    Update_Command_Buttons_Sensivity();
    Scan_Rename_File_Generate_Preview();
    Scan_Fill_Tag_Generate_Preview();

    if (SET_FOCUS_TO_FIRST_TAG_FIELD)
        gtk_widget_grab_focus(GTK_WIDGET(TitleEntry));
}


/*
 * Action when Last button is selected
 */
void Action_Select_Last_File (void)
{
    GList *etfilelist;

    if (!ETCore->ETFileDisplayedList || !ETCore->ETFileDisplayedList->next)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Go to the last item of the list */
    etfilelist = ET_Displayed_File_List_Last();
    if (etfilelist)
    {
        Browser_List_Unselect_All_Files();
        Browser_List_Select_File_By_Etfile((ET_File *)etfilelist->data,TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

    Update_Command_Buttons_Sensivity();
    Scan_Rename_File_Generate_Preview();
    Scan_Fill_Tag_Generate_Preview();

    if (SET_FOCUS_TO_FIRST_TAG_FIELD)
        gtk_widget_grab_focus(GTK_WIDGET(TitleEntry));
}


/*
 * Select a file in the "main list" using the ETFile adress of each item.
 */
void Action_Select_Nth_File_By_Etfile (ET_File *ETFile)
{
    if (!ETCore->ETFileDisplayedList)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Display the item */
    Browser_List_Select_File_By_Etfile(ETFile,TRUE);
    ET_Displayed_File_List_By_Etfile(ETFile); // Just to update 'ETFileDisplayedList'
    ET_Display_File_Data_To_UI(ETFile);

    Update_Command_Buttons_Sensivity();
    Scan_Rename_File_Generate_Preview();
    Scan_Fill_Tag_Generate_Preview();
}


/*
 * Action when Scan button is pressed
 */
void Action_Scan_Selected_Files (void)
{
    gint progress_bar_index;
    gint selectcount;
    gchar progress_bar_text[30];
    double fraction;
    GList *selfilelist = NULL;
    ET_File *etfile;
    GtkTreeSelection *selection;

    if (!ETCore->ETFileDisplayedList || !BrowserList) return;

    /* Check if scanner window is opened */
    if (!ScannerWindow)
    {
        Open_ScannerWindow(SCANNER_TYPE);
        Statusbar_Message(_("Select Mode and Mask, and redo the same action"),TRUE);
        return;
    }

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Initialize status bar */
    selectcount = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList)));
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar),0);
    progress_bar_index = 0;
    g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, selectcount);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), progress_bar_text);

    /* Set to unsensitive all command buttons (except Quit button) */
    Disable_Command_Buttons();

    progress_bar_index = 0;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);
    while (selfilelist)
    {
        etfile = Browser_List_Get_ETFile_From_Path(selfilelist->data);

        // Run the current scanner
        Scan_Select_Mode_And_Run_Scanner(etfile);

        fraction = (++progress_bar_index) / (double) selectcount;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), fraction);
        g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, selectcount);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), progress_bar_text);

        /* Needed to refresh status bar */
        while (gtk_events_pending())
            gtk_main_iteration();

        if (!selfilelist->next) break;
        selfilelist = g_list_next(selfilelist);
    }

    g_list_foreach(selfilelist, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selfilelist);

    // Refresh the whole list (faster than file by file) to show changes
    Browser_List_Refresh_Whole_List();

    /* Display the current file */
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);

    /* To update state of command buttons */
    Update_Command_Buttons_Sensivity();

    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), "");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), 0);
    Statusbar_Message(_("All tags have been scanned"),TRUE);
}



/*
 * Action when Remove button is pressed
 */
void Action_Remove_Selected_Tags (void)
{
    GList *selfilelist = NULL;
    ET_File *etfile;
    File_Tag *FileTag;
    gint progress_bar_index;
    gint selectcount;
    double fraction;
    GtkTreeSelection *selection;

    if (!ETCore->ETFileDisplayedList || !BrowserList) return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Initialize status bar */
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), 0.0);
    selectcount = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList)));
    progress_bar_index = 0;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);
    while (selfilelist)
    {
        etfile = Browser_List_Get_ETFile_From_Path(selfilelist->data);
        FileTag = ET_File_Tag_Item_New();
        ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

        fraction = (++progress_bar_index) / (double) selectcount;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), fraction);
        /* Needed to refresh status bar */
        while (gtk_events_pending())
            gtk_main_iteration();

        if (!selfilelist->next) break;
        selfilelist = g_list_next(selfilelist);
    }

    g_list_foreach(selfilelist, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selfilelist);

    // Refresh the whole list (faster than file by file) to show changes
    Browser_List_Refresh_Whole_List();

    /* Display the current file */
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
    Update_Command_Buttons_Sensivity();

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), 0.0);
    Statusbar_Message(_("All tags have been removed"),TRUE);
}



/*
 * Action when Undo button is pressed.
 * Action_Undo_Selected_Files: Undo the last changes for the selected files.
 * Action_Undo_From_History_List: Undo the changes of the last modified file of the list.
 */
gint Action_Undo_Selected_Files (void)
{
    GList *selfilelist = NULL;
    gboolean state = FALSE;
    ET_File *etfile;
    GtkTreeSelection *selection;

    if (!ETCore->ETFileDisplayedList || !BrowserList) return FALSE;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);
    while (selfilelist)
    {
        etfile = Browser_List_Get_ETFile_From_Path(selfilelist->data);
        state |= ET_Undo_File_Data(etfile);

        if (!selfilelist->next) break;
        selfilelist = g_list_next(selfilelist);
    }

    g_list_foreach(selfilelist, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selfilelist);

    // Refresh the whole list (faster than file by file) to show changes
    Browser_List_Refresh_Whole_List();

    /* Display the current file */
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
    Update_Command_Buttons_Sensivity();

    //ET_Debug_Print_File_List(ETCore->ETFileList,__FILE__,__LINE__,__FUNCTION__);

    return state;
}


void Action_Undo_From_History_List (void)
{
    ET_File *ETFile;

    if (!ETCore->ETFileList) return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    ETFile = ET_Undo_History_File_Data();
    if (ETFile)
    {
        ET_Display_File_Data_To_UI(ETFile);
        Browser_List_Select_File_By_Etfile(ETFile,TRUE);
        Browser_List_Refresh_File_In_List(ETFile);
    }

    Update_Command_Buttons_Sensivity();
}



/*
 * Action when Redo button is pressed.
 * Action_Redo_Selected_Files: Redo the last changes for the selected files.
 * Action_Redo_From_History_List: Redo the changes of the last modified file of the list.
 */
gint Action_Redo_Selected_File (void)
{
    GList *selfilelist = NULL;
    gboolean state = FALSE;
    ET_File *etfile;
    GtkTreeSelection *selection;

    if (!ETCore->ETFileDisplayedList || !BrowserList) return FALSE;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);
    while (selfilelist)
    {
        etfile = Browser_List_Get_ETFile_From_Path(selfilelist->data);
        state |= ET_Redo_File_Data(etfile);

        if (!selfilelist->next) break;
        selfilelist = g_list_next(selfilelist);
    }

    g_list_foreach(selfilelist, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selfilelist);

    // Refresh the whole list (faster than file by file) to show changes
    Browser_List_Refresh_Whole_List();

    /* Display the current file */
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
    Update_Command_Buttons_Sensivity();

    return state;
}


void Action_Redo_From_History_List (void)
{
    ET_File *ETFile;

    if (!ETCore->ETFileDisplayedList) return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    ETFile = ET_Redo_History_File_Data();
    if (ETFile)
    {
        ET_Display_File_Data_To_UI(ETFile);
        Browser_List_Select_File_By_Etfile(ETFile,TRUE);
        Browser_List_Refresh_File_In_List(ETFile);
    }

    Update_Command_Buttons_Sensivity();
}




/*
 * Action when Save button is pressed
 */
void Action_Save_Selected_Files (void)
{
    Save_Selected_Files_With_Answer(FALSE);
}

void Action_Force_Saving_Selected_Files (void)
{
    Save_Selected_Files_With_Answer(TRUE);
}


/*
 * Will save the full list of file (not only the selected files in list)
 * and check if we must save also only the changed files or all files
 * (force_saving_files==TRUE)
 */
gint Save_All_Files_With_Answer (gboolean force_saving_files)
{
    GList *etfilelist;

    if (!ETCore || !ETCore->ETFileList) return FALSE;
    etfilelist = g_list_first(ETCore->ETFileList);
    return Save_List_Of_Files(etfilelist,force_saving_files);
}

/*
 * Will save only the selected files in the file list
 */
gint Save_Selected_Files_With_Answer (gboolean force_saving_files)
{
    gint toreturn;
    GList *etfilelist = NULL;
    GList *selfilelist = NULL;
    ET_File *etfile;
    GtkTreeSelection *selection;

    if (!BrowserList) return FALSE;

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

    toreturn = Save_List_Of_Files(etfilelist, force_saving_files);
    g_list_free(etfilelist);
    return toreturn;
}

/*
 * Save_List_Of_Files: Function to save a list of files.
 *  - force_saving_files = TRUE => force saving the file even if it wasn't changed
 *  - force_saving_files = FALSE => force saving only the changed files
 */
gint Save_List_Of_Files (GList *etfilelist, gboolean force_saving_files)
{
    gint       progress_bar_index;
    gint       saving_answer;
    gint       nb_files_to_save;
    gint       nb_files_changed_by_ext_program;
    gchar     *msg;
    gchar      progress_bar_text[30];
    GList     *etfilelist_tmp;
    ET_File   *etfile_save_position = NULL;
    File_Tag  *FileTag;
    File_Name *FileNameNew;
    double     fraction;
    GtkAction *uiaction;
    GtkWidget *TBViewMode;
    GtkWidget *widget_focused;
    GtkTreePath *currentPath = NULL;

    if (!ETCore) return FALSE;

    /* Save the current position in the list */
    etfile_save_position = ETCore->ETFileDisplayed;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Save widget that has current focus, to give it again the focus after saving */
    widget_focused = gtk_window_get_focus(GTK_WINDOW(MainWindow));

    /* Count the number of files to save */
    /* Count the number of files changed by an external program */
    nb_files_to_save = 0;
    nb_files_changed_by_ext_program = 0;
    etfilelist_tmp = etfilelist;
    while (etfilelist_tmp)
    {
        struct stat   statbuf;
        ET_File   *ETFile   = (ET_File *)etfilelist_tmp->data;
        File_Tag  *FileTag  = (File_Tag *)ETFile->FileTag->data;
        File_Name *FileName = (File_Name *)ETFile->FileNameNew->data;
        gchar *filename_cur = ((File_Name *)ETFile->FileNameCur->data)->value;
        gchar *filename_cur_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;
        gchar *basename_cur_utf8 = g_path_get_basename(filename_cur_utf8);

        // Count only the changed files or all files if force_saving_files==TRUE
        if ( force_saving_files
        || (FileName && FileName->saved==FALSE) || (FileTag && FileTag->saved==FALSE) )
            nb_files_to_save++;

        stat(filename_cur,&statbuf);
        if (ETFile->FileModificationTime != statbuf.st_mtime)
            nb_files_changed_by_ext_program++;
        g_free(basename_cur_utf8);

        etfilelist_tmp = etfilelist_tmp->next;
    }

    /* Initialize status bar */
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar),0);
    progress_bar_index = 0;
    g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, nb_files_to_save);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), progress_bar_text);

    /* Set to unsensitive all command buttons (except Quit button) */
    Disable_Command_Buttons();
    Browser_Area_Set_Sensitive(FALSE);
    Tag_Area_Set_Sensitive(FALSE);
    File_Area_Set_Sensitive(FALSE);

    /* Show msgbox (if needed) to ask confirmation ('SF' for Save File) */
    SF_HideMsgbox_Write_Tag = 0;
    SF_HideMsgbox_Rename_File = 0;

    Main_Stop_Button_Pressed = 0;
    uiaction = gtk_ui_manager_get_action(UIManager, "/ToolBar/Stop"); // Activate the stop button
    g_object_set(uiaction, "sensitive", FALSE, NULL);

    /*
     * Check if file was changed by an external program
     */
    if (nb_files_changed_by_ext_program > 0)
    {
        // Some files were changed by other program than EasyTAG
        GtkWidget *msgbox = NULL;
        gchar *msg;
        gint response;

        msg = g_strdup_printf(_("Be careful, severals files (%d file(s)) were changed by an external program.\n"
                                "Do you want to continue anyway?"),nb_files_changed_by_ext_program);
        msgbox = msg_box_new(_("Saving File(s)..."),
                             GTK_WINDOW(MainWindow),
                             NULL,
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             msg,
                             GTK_STOCK_DIALOG_WARNING,
                             GTK_STOCK_NO,  GTK_RESPONSE_NO,
							 GTK_STOCK_YES, GTK_RESPONSE_YES,
                             NULL);
        g_free(msg);

        response = gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);

        switch (response)
        {
            case GTK_RESPONSE_YES:
                break;
            case GTK_RESPONSE_NO:
            case GTK_RESPONSE_NONE:
                Main_Stop_Button_Pressed = 1; // To don't enter to following loop
                break;
        }
    }

    etfilelist_tmp = etfilelist;
    while (etfilelist_tmp && !Main_Stop_Button_Pressed)
    {
        FileTag     = ((ET_File *)etfilelist_tmp->data)->FileTag->data;
        FileNameNew = ((ET_File *)etfilelist_tmp->data)->FileNameNew->data;

        /* We process only the files changed and not saved, or we force to save all
         * files if force_saving_files==TRUE */
        if ( force_saving_files
        || FileTag->saved == FALSE || FileNameNew->saved == FALSE )
        {
            ET_Display_File_Data_To_UI((ET_File *)etfilelist_tmp->data);
            // Use of 'currentPath' to try to increase speed. Indeed, in many
            // cases, the next file to select, is the next in the list
            currentPath = Browser_List_Select_File_By_Etfile2((ET_File *)etfilelist_tmp->data,FALSE,currentPath);

            fraction = (++progress_bar_index) / (double) nb_files_to_save;
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), fraction);
            g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, nb_files_to_save);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), progress_bar_text);

            /* Needed to refresh status bar */
            while (gtk_events_pending())
                gtk_main_iteration();

            // Save tag and rename file
            saving_answer = Save_File((ET_File *)etfilelist_tmp->data, (nb_files_to_save>1) ? TRUE : FALSE, force_saving_files);

            if (saving_answer == -1)
            {
                /* Stop saving files + reinit progress bar */
                gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), "");
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), 0.0);
                Statusbar_Message(_("Saving files was stopped..."),TRUE);
                /* To update state of command buttons */
                Update_Command_Buttons_Sensivity();
                Browser_Area_Set_Sensitive(TRUE);
                Tag_Area_Set_Sensitive(TRUE);
                File_Area_Set_Sensitive(TRUE);

                return -1; /* We stop all actions */
            }
        }

        etfilelist_tmp = etfilelist_tmp->next;
        if (Main_Stop_Button_Pressed == 1 )
            break;

    }

    if (currentPath)
        gtk_tree_path_free(currentPath);

    if (Main_Stop_Button_Pressed)
        msg = g_strdup(_("Saving files was stopped..."));
    else
        msg = g_strdup(_("All files have been saved..."));

    Main_Stop_Button_Pressed = 0;
    uiaction = gtk_ui_manager_get_action(UIManager, "/ToolBar/Stop");
    g_object_set(uiaction, "sensitive", FALSE, NULL);

    /* Return to the saved position in the list */
    ET_Display_File_Data_To_UI(etfile_save_position);
    Browser_List_Select_File_By_Etfile(etfile_save_position,TRUE);

    /* Browser is on mode : Artist + Album list */
    TBViewMode = gtk_ui_manager_get_widget(UIManager, "/ToolBar/ViewModeToggle");
    if ( gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(TBViewMode)) )
        Browser_Display_Tree_Or_Artist_Album_List();

    /* To update state of command buttons */
    Update_Command_Buttons_Sensivity();
    Browser_Area_Set_Sensitive(TRUE);
    Tag_Area_Set_Sensitive(TRUE);
    File_Area_Set_Sensitive(TRUE);

    /* Give again focus to the first entry, else the focus is passed to an other */
    gtk_widget_grab_focus(GTK_WIDGET(widget_focused));

    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), "");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), 0);
    Statusbar_Message(msg,TRUE);
    g_free(msg);

    return TRUE;
}



/*
 * Delete a file on the hard disk
 */
void Action_Delete_Selected_Files (void)
{
    Delete_Selected_Files_With_Answer();
}


gint Delete_Selected_Files_With_Answer (void)
{
    GList *selfilelist;
    GList *rowreflist = NULL;
    gint   progress_bar_index;
    gint   saving_answer;
    gint   nb_files_to_delete;
    gint   nb_files_deleted = 0;
    gchar *msg;
    gchar progress_bar_text[30];
    double fraction;
    GtkTreeModel *treemodel;
    GtkTreeRowReference *rowref;
    GtkTreeSelection *selection;

    if (!ETCore->ETFileDisplayedList || !BrowserList) return FALSE;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Number of files to save */
    nb_files_to_delete = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList)));

    /* Initialize status bar */
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar),0);
    progress_bar_index = 0;
    g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, nb_files_to_delete);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), progress_bar_text);

    /* Set to unsensitive all command buttons (except Quit button) */
    Disable_Command_Buttons();
    Browser_Area_Set_Sensitive(FALSE);
    Tag_Area_Set_Sensitive(FALSE);
    File_Area_Set_Sensitive(FALSE);

    /* Show msgbox (if needed) to ask confirmation */
    SF_HideMsgbox_Delete_File = 0;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    selfilelist = gtk_tree_selection_get_selected_rows(selection, &treemodel);
    while (selfilelist)
    {
        rowref = gtk_tree_row_reference_new(treemodel, selfilelist->data);
        rowreflist = g_list_append(rowreflist, rowref);

        if (!selfilelist->next) break;
        selfilelist = selfilelist->next;
    }

    while (rowreflist)
    {
        GtkTreePath *path;
        ET_File *ETFile;

        path = gtk_tree_row_reference_get_path(rowreflist->data);
        ETFile = Browser_List_Get_ETFile_From_Path(path);
        gtk_tree_path_free(path);

        ET_Display_File_Data_To_UI(ETFile);
        Browser_List_Select_File_By_Etfile(ETFile,FALSE);
        fraction = (++progress_bar_index) / (double) nb_files_to_delete;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), fraction);
        g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, nb_files_to_delete);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), progress_bar_text);
         /* Needed to refresh status bar */
        while (gtk_events_pending())
            gtk_main_iteration();

        saving_answer = Delete_File(ETFile,(nb_files_to_delete>1)?TRUE:FALSE);

        switch (saving_answer)
        {
            case 1:
                nb_files_deleted += saving_answer;
                // Remove file in the browser (corresponding line in the clist)
                Browser_List_Remove_File(ETFile);
                // Remove file from file list
                ET_Remove_File_From_File_List(ETFile);
                break;
            case 0:
                break;
            case -1:
                // Stop deleting files + reinit progress bar
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar),0.0);
                // To update state of command buttons
                Update_Command_Buttons_Sensivity();
                Browser_Area_Set_Sensitive(TRUE);
                Tag_Area_Set_Sensitive(TRUE);
                File_Area_Set_Sensitive(TRUE);

                g_list_foreach(selfilelist, (GFunc) gtk_tree_path_free, NULL);
                g_list_free(selfilelist);
                return -1; // We stop all actions
        }

        if (!rowreflist->next) break;
        rowreflist = rowreflist->next;
    }

    g_list_foreach(selfilelist, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selfilelist);
    g_list_foreach(rowreflist, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_free(rowreflist);

    if (nb_files_deleted < nb_files_to_delete)
        msg = g_strdup(_("Files have been partially deleted..."));
    else
        msg = g_strdup(_("All files have been deleted..."));

    // It's important to displayed the new item, as it'll check the changes in Browser_Display_Tree_Or_Artist_Album_List
    if (ETCore->ETFileDisplayed)
        ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
    /*else if (ET_Displayed_File_List_Current())
        ET_Display_File_Data_To_UI((ET_File *)ET_Displayed_File_List_Current()->data);*/

    // Load list...
    Browser_List_Load_File_List(ETCore->ETFileDisplayedList, NULL);
    // Rebuild the list...
    Browser_Display_Tree_Or_Artist_Album_List();

    /* To update state of command buttons */
    Update_Command_Buttons_Sensivity();
    Browser_Area_Set_Sensitive(TRUE);
    Tag_Area_Set_Sensitive(TRUE);
    File_Area_Set_Sensitive(TRUE);

    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), "");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), 0);
    Statusbar_Message(msg,TRUE);
    g_free(msg);

    return TRUE;
}



/*
 * Save changes of the ETFile (write tag and rename file)
 *  - multiple_files = TRUE  : when saving files, a msgbox appears with ability
 *                             to do the same action for all files.
 *  - multiple_files = FALSE : appears only a msgbox to ask confirmation.
 */
gint Save_File (ET_File *ETFile, gboolean multiple_files, gboolean force_saving_files)
{
    File_Tag  *FileTag;
    File_Name *FileNameNew;
    gchar *msg = NULL;
    gchar *msg_title = NULL;
    gint stop_loop = 0;
    //struct stat   statbuf;
    //gchar *filename_cur = ((File_Name *)ETFile->FileNameCur->data)->value;
    gchar *filename_cur_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;
    gchar *filename_new_utf8 = ((File_Name *)ETFile->FileNameNew->data)->value_utf8;
    gchar *basename_cur_utf8, *basename_new_utf8;
    gchar *dirname_cur_utf8, *dirname_new_utf8;


    if (!ETFile) return 0;

    basename_cur_utf8 = g_path_get_basename(filename_cur_utf8);
    basename_new_utf8 = g_path_get_basename(filename_new_utf8);

    /* Save the current displayed data */
    //ET_Save_File_Data_From_UI((ET_File *)ETFileList->data); // Not needed, because it was done before
    FileTag     = ETFile->FileTag->data;
    FileNameNew = ETFile->FileNameNew->data;

    /*
     * Check if file was changed by an external program
     */
    /*stat(filename_cur,&statbuf);
    if (ETFile->FileModificationTime != statbuf.st_mtime)
    {
        // File was changed
        GtkWidget *msgbox = NULL;
        gint response;

        msg = g_strdup_printf(_("The file '%s' was changed by an external program.\nDo you want to continue?"),basename_cur_utf8);
        msgbox = msg_box_new(_("Write File..."),
                             GTK_WINDOW(MainWindow),
                             NULL,
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             msg,
                             GTK_STOCK_DIALOG_WARNING,
                             GTK_STOCK_NO,  GTK_RESPONSE_NO,
							 GTK_STOCK_YES, GTK_RESPONSE_YES,
                             NULL);
        g_free(msg);

        response = gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);

        switch (response)
        {
            case GTK_RESPONSE_YES:
                break;
            case GTK_RESPONSE_NO:
            case GTK_RESPONSE_NONE:
                stop_loop = -1;
                return stop_loop;
                break;
        }
    }*/


    /*
     * First part: write tag informations (artist, title,...)
     */
    // Note : the option 'force_saving_files' is only used to save tags
    if ( force_saving_files
    || FileTag->saved == FALSE ) // This tag had been already saved ?
    {
        GtkWidget *msgbox = NULL;
        GtkWidget *msgbox_check_button = NULL;
        gint response;

        if (CONFIRM_WRITE_TAG && !SF_HideMsgbox_Write_Tag)
        {
            ET_Display_File_Data_To_UI(ETFile);

            msg = g_strdup_printf(_("Do you want to write the tag of file\n'%s' ?"),basename_cur_utf8);

            if (multiple_files)
            {
                msgbox = msg_box_new(_("Write Tag..."),
                                     GTK_WINDOW(MainWindow),
                                     &msgbox_check_button,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     msg,
                                     GTK_STOCK_DIALOG_QUESTION,
                                     GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_NO,    GTK_RESPONSE_NO,
									 GTK_STOCK_YES,   GTK_RESPONSE_YES,
                                     NULL);
                GTK_TOGGLE_BUTTON(msgbox_check_button)->active = TRUE; // Checked by default
            }else
            {
                msgbox = msg_box_new(_("Write Tag..."),
                                     GTK_WINDOW(MainWindow),
                                     NULL,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     msg,
                                     GTK_STOCK_DIALOG_QUESTION,
                                     GTK_STOCK_NO,  GTK_RESPONSE_NO,
									 GTK_STOCK_YES, GTK_RESPONSE_YES,
                                     NULL);
            }
            g_free(msg);

            SF_ButtonPressed_Write_Tag = response = gtk_dialog_run(GTK_DIALOG(msgbox));
            // When check button in msgbox was activated : do not display the message again
            if (msgbox_check_button && GTK_TOGGLE_BUTTON(msgbox_check_button)->active)
                SF_HideMsgbox_Write_Tag = GTK_TOGGLE_BUTTON(msgbox_check_button)->active;
            gtk_widget_destroy(msgbox);
        }else
        {
            if (SF_HideMsgbox_Write_Tag)
                response = SF_ButtonPressed_Write_Tag;
            else
                response = GTK_RESPONSE_YES;
        }

        switch (response)
        {
            case GTK_RESPONSE_YES:
            {
                gboolean rc;

                // if 'SF_HideMsgbox_Write_Tag is TRUE', then errors are displayed only in log
                rc = Write_File_Tag(ETFile,SF_HideMsgbox_Write_Tag);
                // if an error occurs when 'SF_HideMsgbox_Write_Tag is TRUE', we don't stop saving...
                if (rc != TRUE && !SF_HideMsgbox_Write_Tag)
                {
                    stop_loop = -1;
                    return stop_loop;
                }
                break;
            }
            case GTK_RESPONSE_NO:
                break;
            case GTK_RESPONSE_CANCEL:
            case GTK_RESPONSE_NONE:
                stop_loop = -1;
                return stop_loop;
                break;
        }
    }


    /*
     * Second part: rename the file
     */
    // Do only if changed! (don't take force_saving_files into account)
    if ( FileNameNew->saved == FALSE ) // This filename had been already saved ?
    {
        GtkWidget *msgbox = NULL;
        GtkWidget *msgbox_check_button = NULL;
        gint response;

        if (CONFIRM_RENAME_FILE && !SF_HideMsgbox_Rename_File)
        {
            ET_Display_File_Data_To_UI(ETFile);

            dirname_cur_utf8 = g_path_get_dirname(filename_cur_utf8);
            dirname_new_utf8 = g_path_get_dirname(filename_new_utf8);

            // Directories were renamed? or only filename?
            if (g_utf8_collate(dirname_cur_utf8,dirname_new_utf8) != 0)
            {
                if (g_utf8_collate(basename_cur_utf8,basename_new_utf8) != 0)
                {
                    // Directories and filename changed
                    msg_title = g_strdup(_("Rename File and Directory..."));
                    msg = g_strdup_printf(_("Do you want to rename the file and directory \n'%s'\nto \n'%s' ?"),
                                          filename_cur_utf8, filename_new_utf8);
                }else
                {
                    // Only directories changed
                    msg_title = g_strdup(_("Rename Directory..."));
                    msg = g_strdup_printf(_("Do you want to rename the directory \n'%s'\nto \n'%s' ?"),
                                          dirname_cur_utf8, dirname_new_utf8);
                }
            }else
            {
                // Only filename changed
                msg_title = g_strdup(_("Rename File..."));
                msg = g_strdup_printf(_("Do you want to rename the file \n'%s'\nto \n'%s' ?"),
                                      basename_cur_utf8, basename_new_utf8);
            }

            g_free(dirname_cur_utf8);
            g_free(dirname_new_utf8);

            if (multiple_files)
            {
                // Allow to cancel for all other files
                msgbox = msg_box_new(msg_title,
                                     GTK_WINDOW(MainWindow),
                                     &msgbox_check_button,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     msg,
                                     GTK_STOCK_DIALOG_QUESTION,
                                     GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_NO,    GTK_RESPONSE_NO,
									 GTK_STOCK_YES,   GTK_RESPONSE_YES,
                                     NULL);
                GTK_TOGGLE_BUTTON(msgbox_check_button)->active = TRUE; // Checked by default
            }else
            {
                msgbox = msg_box_new(msg_title,
                                     GTK_WINDOW(MainWindow),
                                     NULL,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     msg,
                                     GTK_STOCK_DIALOG_QUESTION,
                                     GTK_STOCK_NO,  GTK_RESPONSE_NO,
									 GTK_STOCK_YES, GTK_RESPONSE_YES,
                                     NULL);
            }
            g_free(msg);
            g_free(msg_title);
            SF_ButtonPressed_Rename_File = response = gtk_dialog_run(GTK_DIALOG(msgbox));
            if (msgbox_check_button && GTK_TOGGLE_BUTTON(msgbox_check_button)->active)
                SF_HideMsgbox_Rename_File = GTK_TOGGLE_BUTTON(msgbox_check_button)->active;
            gtk_widget_destroy(msgbox);
        }else
        {
            if (SF_HideMsgbox_Rename_File)
                response = SF_ButtonPressed_Rename_File;
            else
                response = GTK_RESPONSE_YES;
        }

        switch(response)
        {
            case GTK_RESPONSE_YES:
            {
                gboolean rc;

                // if 'SF_HideMsgbox_Rename_File is TRUE', then errors are displayed only in log
                rc = Rename_File(ETFile,SF_HideMsgbox_Rename_File);
                // if an error occurs when 'SF_HideMsgbox_Rename_File is TRUE', we don't stop saving...
                if (rc != TRUE && !SF_HideMsgbox_Rename_File)
                {
                    stop_loop = -1;
                    return stop_loop;
                }
                break;
            }
            case GTK_RESPONSE_NO:
                break;
            case GTK_RESPONSE_CANCEL:
            case GTK_RESPONSE_NONE:
                stop_loop = -1;
                return stop_loop;
                break;
        }
    }

    g_free(basename_cur_utf8);
    g_free(basename_new_utf8);

    /* Refresh file into browser list */
    Browser_List_Refresh_File_In_List(ETFile);

    return 1;
}


/*
 * Write tag of the ETFile
 * Return TRUE => OK
 *        FALSE => error
 */
gboolean Write_File_Tag (ET_File *ETFile, gboolean hide_msgbox)
{
    gchar *cur_filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;
    gchar *msg;
    gchar *msg1;
    gchar *basename_utf8;
    GtkWidget *msgbox;

    basename_utf8 = g_path_get_basename(cur_filename_utf8);
    msg = g_strdup_printf(_("Writing tag of '%s'"),basename_utf8);
    Statusbar_Message(msg,TRUE);
    g_free(msg);

    if (ET_Save_File_Tag_To_HD(ETFile))
    {
        Statusbar_Message(_("Tag(s) written"),TRUE);
        return TRUE;
    }

    switch ( ((ET_File_Description *)ETFile->ETFileDescription)->TagType)
    {
#ifdef ENABLE_OGG
        case OGG_TAG:
            // Special for Ogg Vorbis because the error is defined into 'vcedit_error(state)'
            msg = g_strdup_printf(_("Can't write tag in file '%s'!\n(%s)"),
                                  basename_utf8,ogg_error_msg);
            msg1 = g_strdup_printf(_("Can't write tag in file '%s'! (%s)"),
                                  basename_utf8,ogg_error_msg);
            break;
#endif
        default:
            msg = g_strdup_printf(_("Can't write tag in file '%s'!\n(%s)"),
                                  basename_utf8,g_strerror(errno));
            msg1 = g_strdup_printf(_("Can't write tag in file '%s'! (%s)"),
                                  basename_utf8,g_strerror(errno));
    }

    Log_Print(LOG_ERROR,"%s", msg1);
    g_free(msg1);

    if (!hide_msgbox)
    {
        msgbox = msg_box_new(_("Error..."),
                             GTK_WINDOW(MainWindow),
                             NULL,
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             msg,
                             GTK_STOCK_DIALOG_ERROR,
                             GTK_STOCK_OK, GTK_RESPONSE_OK,
                             NULL);
        gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);
    }
    g_free(msg);
    g_free(basename_utf8);

    return FALSE;
}


/*
 * Make dir and all parents with permission mode
 */
gint Make_Dir (const gchar *dirname_old, const gchar *dirname_new)
{
    gchar *parent, *temp;
    struct stat dirstat;
#ifdef WIN32
    gboolean first = TRUE;
#endif

    // Use same permissions as the old directory
    stat(dirname_old,&dirstat);

    temp = parent = g_strdup(dirname_new);
    for (temp++;*temp;temp++)
    {
        if (*temp!=G_DIR_SEPARATOR)
            continue;

#ifdef WIN32
        if (first)
        {
            first = FALSE;
            continue;
        }
#endif

        *temp=0; // To truncate temporarly dirname_new

        if (mkdir(parent,dirstat.st_mode)==-1 && errno!=EEXIST)
        {
            g_free(parent);
            return(-1);
        }
        *temp=G_DIR_SEPARATOR; // To cancel the '*temp=0;'
    }
    g_free(parent);

    if (mkdir(dirname_new,dirstat.st_mode)==-1 && errno!=EEXIST)
        return(-1);

    return(0);
}

/*
 * Remove old directories after renaming the file
 * Badly coded, but works....
 */
gint Remove_Dir (const gchar *dirname_old, const gchar *dirname_new)
{
    gchar *temp_old, *temp_new;
    gchar *temp_end_old, *temp_end_new;

    temp_old = g_strdup(dirname_old);
    temp_new = g_strdup(dirname_new);

    while (temp_old && temp_new && strcmp(temp_old,temp_new)!=0 )
    {
        if (rmdir(temp_old)==-1)
        {
            // Patch from vdaghan : ENOTEMPTY & EEXIST are synonymous and used by some systems
            if (errno != ENOTEMPTY
            &&  errno != EEXIST)
            {
                g_free(temp_old);
                g_free(temp_new);
                return(-1);
            }else
            {
                break;
            }
        }

        temp_end_old = temp_old + strlen(temp_old) - 1;
        temp_end_new = temp_new + strlen(temp_new) - 1;

        while (*temp_end_old && *temp_end_old!=G_DIR_SEPARATOR)
        {
            temp_end_old--;
        }
        *temp_end_old=0;
        while (*temp_end_new && *temp_end_new!=G_DIR_SEPARATOR)
        {
            temp_end_new--;
        }
        *temp_end_new=0;
    }
    g_free(temp_old);
    g_free(temp_new);

    return(0);
}


/*
 * Rename the file ETFile
 * Return TRUE => OK
 *        FALSE => error
 */
gboolean Rename_File (ET_File *ETFile, gboolean hide_msgbox)
{
    FILE  *file;
    gchar *tmp_filename = NULL;
    gchar *cur_filename = ((File_Name *)ETFile->FileNameCur->data)->value;
    gchar *new_filename = ((File_Name *)ETFile->FileNameNew->data)->value;
    gchar *cur_filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;      // Filename + path
    gchar *new_filename_utf8 = ((File_Name *)ETFile->FileNameNew->data)->value_utf8;
    gchar *cur_basename_utf8 = g_path_get_basename(cur_filename_utf8); // Only filename
    gchar *new_basename_utf8 = g_path_get_basename(new_filename_utf8);
    gint   fd_tmp;
    gchar *msg, *msg1;
    gchar *dirname_cur;
    gchar *dirname_new;
    gchar *dirname_cur_utf8;
    gchar *dirname_new_utf8;

    msg = g_strdup_printf(_("Renaming file '%s'"),cur_filename_utf8);
    Statusbar_Message(msg,TRUE);
    g_free(msg);

    /* We use two stages to rename file, to avoid problem with some system
     * that doesn't allow to rename the file if only the case has changed. */
    tmp_filename = g_strdup_printf("%s.XXXXXX",cur_filename);
    if ( (fd_tmp = mkstemp(tmp_filename)) >= 0 )
    {
        close(fd_tmp);
        unlink(tmp_filename);
    }

    // Rename to the temporary name
    if ( rename(cur_filename,tmp_filename)!=0 ) // => rename() fails
    {
        gchar *msg, *msg1;
        GtkWidget *msgbox;

        /* Renaming file to the temporary filename has failed */
        msg = g_strdup_printf(_("Can't rename file '%s'\n to \n'%s'!\n(%s)"),
                                cur_basename_utf8,new_basename_utf8,g_strerror(errno));
        msg1 = g_strdup_printf(_("Can't rename file '%s' to '%s'! (%s)"),
                                cur_basename_utf8,new_basename_utf8,g_strerror(errno));
        if (!hide_msgbox)
        {
            msgbox = msg_box_new(_("Error..."),
                                 GTK_WINDOW(MainWindow),
                                 NULL,
                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                 msg,
                                 GTK_STOCK_DIALOG_ERROR,
                                 GTK_STOCK_OK, GTK_RESPONSE_OK,
                                 NULL);
            gtk_dialog_run(GTK_DIALOG(msgbox));
            gtk_widget_destroy(msgbox);
        }
        g_free(msg);

        Log_Print(LOG_ERROR,"%s", msg1);
        g_free(msg1);

        Statusbar_Message(_("File(s) not renamed..."),TRUE);
        g_free(tmp_filename);
        g_free(cur_basename_utf8);
        g_free(new_basename_utf8);
        return FALSE;
    }

    /* Check if the new file name already exists. Must be done after changing
     * filename to the temporary name, else we can't detect the problem under
     * Linux when renaming a file 'aa.mp3' to 'AA.mp3' and if the last one
     * already exists */
    if ( (file=fopen(new_filename,"r"))!=NULL )
    {
        GtkWidget *msgbox;

        fclose(file);

        // Restore the initial name
        if ( rename(tmp_filename,cur_filename)!=0 ) // => rename() fails
        {
            gchar *msg, *msg1;
            GtkWidget *msgbox;

            /* Renaming file from the temporary filename has failed */
            msg = g_strdup_printf(_("Can't rename file '%s'\n to \n'%s'!\n(%s)"),
                                    new_basename_utf8,cur_basename_utf8,g_strerror(errno));
            msg1 = g_strdup_printf(_("Can't rename file '%s' to '%s'! (%s)"),
                                    new_basename_utf8,cur_basename_utf8,g_strerror(errno));
            if (!hide_msgbox)
            {
                msgbox = msg_box_new(_("Error..."),
                                     GTK_WINDOW(MainWindow),
                                     NULL,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     msg,
                                     GTK_STOCK_DIALOG_ERROR,
                                     GTK_STOCK_OK, GTK_RESPONSE_OK,
                                     NULL);
                gtk_dialog_run(GTK_DIALOG(msgbox));
                gtk_widget_destroy(msgbox);
            }
            g_free(msg);

            Log_Print(LOG_ERROR,"%s", msg1);
            g_free(msg1);

            Statusbar_Message(_("File(s) not renamed..."),TRUE);
            g_free(tmp_filename);
            g_free(cur_basename_utf8);
            g_free(new_basename_utf8);
            return FALSE;
        }

        msg = g_strdup_printf(_("Can't rename file \n'%s'\nbecause the following "
                    "file already exists:\n'%s'"),cur_basename_utf8,new_basename_utf8);
        msg1 = g_strdup_printf(_("Can't rename file '%s' because the following "
                    "file already exists: '%s'"),cur_basename_utf8,new_basename_utf8);
        if (!hide_msgbox)
        {
            msgbox = msg_box_new(_("Error..."),
                                 GTK_WINDOW(MainWindow),
                                 NULL,
                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                 msg,
                                 GTK_STOCK_DIALOG_ERROR,
                                 GTK_STOCK_OK, GTK_RESPONSE_OK,
                                 NULL);
            gtk_dialog_run(GTK_DIALOG(msgbox));
            gtk_widget_destroy(msgbox);
        }
        g_free(msg);

        Log_Print(LOG_ERROR,"%s", msg1);
        g_free(msg1);

        Statusbar_Message(_("File(s) not renamed..."),TRUE);

        g_free(new_basename_utf8);
        g_free(cur_basename_utf8);
        return FALSE;
    }

    /* If files are in different directories, we need to create the new
     * destination directory */
    dirname_cur = g_path_get_dirname(tmp_filename);
    dirname_new = g_path_get_dirname(new_filename);

    if (dirname_cur && dirname_new && strcmp(dirname_cur,dirname_new)) /* Need to create target directory? */
    {
        if (Make_Dir(dirname_cur,dirname_new))
        {
            gchar *msg, *msg1;
            GtkWidget *msgbox;

            /* Renaming file has failed, but we try to set the initial name */
            rename(tmp_filename,cur_filename);

            dirname_new_utf8 = filename_to_display(dirname_new);
            msg = g_strdup_printf(_("Can't create target directory\n'%s'!\n(%s)"),
                                  dirname_new_utf8,g_strerror(errno));
            msg1 = g_strdup_printf(_("Can't create target directory '%s'! (%s)"),
                                  dirname_new_utf8,g_strerror(errno));
            if (!hide_msgbox)
            {
                msgbox = msg_box_new(_("Error..."),
                                     GTK_WINDOW(MainWindow),
                                     NULL,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     msg,
                                     GTK_STOCK_DIALOG_ERROR,
                                     GTK_STOCK_OK, GTK_RESPONSE_OK,
                                     NULL);
                gtk_dialog_run(GTK_DIALOG(msgbox));
                gtk_widget_destroy(msgbox);
            }
            g_free(msg);
            g_free(dirname_new_utf8);

            Log_Print(LOG_ERROR,"%s", msg1);
            g_free(msg1);

            g_free(tmp_filename);
            g_free(cur_basename_utf8);
            g_free(new_basename_utf8);
            g_free(dirname_cur);
            g_free(dirname_new);
            return FALSE;
        }
    }

    /* Now, we rename the file to his final name */
    if ( rename(tmp_filename,new_filename)==0 )
    {
        /* Renaming file has succeeded */
        Log_Print(LOG_OK,_("Renamed file '%s' to '%s'"),cur_basename_utf8,new_basename_utf8);

        ETFile->FileNameCur = ETFile->FileNameNew;
        /* Now the file was renamed, so mark his state */
        ET_Mark_File_Name_As_Saved(ETFile);

        Statusbar_Message(_("File(s) renamed..."),TRUE);

        /* Remove the of directory (check automatically if it is empty) */
        if (Remove_Dir(dirname_cur,dirname_new))
        {
            gchar *msg, *msg1;
            GtkWidget *msgbox;

            /* Removing directories failed */
            dirname_cur_utf8 = filename_to_display(dirname_cur);
            msg = g_strdup_printf(_("Can't remove old directory\n'%s'!\n(%s)"),
                                  dirname_cur_utf8,g_strerror(errno));
            msg1 = g_strdup_printf(_("Can't remove old directory '%s'! (%s)"),
                                  dirname_cur_utf8,g_strerror(errno));
            if (!hide_msgbox)
            {
                msgbox = msg_box_new(_("Error..."),
                                     GTK_WINDOW(MainWindow),
                                     NULL,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     msg,
                                     GTK_STOCK_DIALOG_ERROR,
                                     GTK_STOCK_OK, GTK_RESPONSE_OK,
                                     NULL);
                gtk_dialog_run(GTK_DIALOG(msgbox));
                gtk_widget_destroy(msgbox);
            }
            g_free(msg);
            g_free(dirname_cur_utf8);

            Log_Print(LOG_ERROR,"%s", msg1);
            g_free(msg1);

            g_free(tmp_filename);
            g_free(cur_basename_utf8);
            g_free(new_basename_utf8);
            g_free(dirname_cur);
            g_free(dirname_new);
            return FALSE;
        }
    }else if ( errno==EXDEV )
    {
        /* For the error "Invalid cross-device link" during renaming, when the
         * new filename isn't on the same device of the omd filename. In this
         * case, we need to move the file, and not only to update the hard disk
         * index table. For example : 'renaming' /mnt/D/file.mp3 to /mnt/E/file.mp3
         *
         * So, we need to copy the old file to the new location, and then to
         * deleted the old file */
        if ( Copy_File(tmp_filename,new_filename) )
        {
            /* Delete the old file */
            unlink(tmp_filename);

            /* Renaming file has succeeded */
            Log_Print(LOG_OK,_("Moved file '%s' to '%s'"),cur_basename_utf8,new_basename_utf8);

            ETFile->FileNameCur = ETFile->FileNameNew;
            /* Now the file was renamed, so mark his state */
            ET_Mark_File_Name_As_Saved(ETFile);

            Statusbar_Message(_("File(s) moved..."),TRUE);

            /* Remove the of directory (check automatically if it is empty) */
            if (Remove_Dir(dirname_cur,dirname_new))
            {
                gchar *msg, *msg1;
                GtkWidget *msgbox;

                /* Removing directories failed */
                dirname_cur_utf8 = filename_to_display(dirname_cur);
                msg = g_strdup_printf(_("Can't remove old directory\n'%s'!\n(%s)"),
                                      dirname_cur_utf8,g_strerror(errno));
                msg1 = g_strdup_printf(_("Can't remove old directory '%s'! (%s)"),
                                      dirname_cur_utf8,g_strerror(errno));
                if (!hide_msgbox)
                {
                    msgbox = msg_box_new(_("Error..."),
                                         GTK_WINDOW(MainWindow),
                                         NULL,
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         msg,
                                         GTK_STOCK_DIALOG_ERROR,
                                         GTK_STOCK_OK, GTK_RESPONSE_OK,
                                         NULL);
                    gtk_dialog_run(GTK_DIALOG(msgbox));
                    gtk_widget_destroy(msgbox);
                }
                g_free(msg);
                g_free(dirname_cur_utf8);

                Log_Print(LOG_ERROR,"%s", msg1);
                g_free(msg1);

                g_free(tmp_filename);
                g_free(cur_basename_utf8);
                g_free(new_basename_utf8);
                g_free(dirname_cur);
                g_free(dirname_new);
                return FALSE;
            }
        }else
        {
            gchar *msg, *msg1;
            GtkWidget *msgbox;

            /* Moving file has failed */
            msg = g_strdup_printf(_("Can't move file '%s'\n to \n'%s'!\n(%s)"),
                                  cur_basename_utf8,new_basename_utf8,g_strerror(errno));
            msg1 = g_strdup_printf(_("Can't move file '%s' to '%s'! (%s)"),
                                  cur_basename_utf8,new_basename_utf8,g_strerror(errno));
            if (!hide_msgbox)
            {
                msgbox = msg_box_new(_("Error..."),
                                     GTK_WINDOW(MainWindow),
                                     NULL,
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     msg,
                                     GTK_STOCK_DIALOG_ERROR,
                                     GTK_STOCK_OK, GTK_RESPONSE_OK,
                                     NULL);
                gtk_dialog_run(GTK_DIALOG(msgbox));
                gtk_widget_destroy(msgbox);
            }
            g_free(msg);

            Log_Print(LOG_ERROR,"%s", msg1);
            g_free(msg1);

            Statusbar_Message(_("File(s) not moved..."),TRUE);

            g_free(tmp_filename);
            g_free(cur_basename_utf8);
            g_free(new_basename_utf8);
            g_free(dirname_cur);
            g_free(dirname_new);
            return FALSE;
        }
    }else
    {
        gchar *msg, *msg1;
        GtkWidget *msgbox;

        /* Renaming file has failed, but we try to set the initial name */
        rename(tmp_filename,cur_filename);

        msg = g_strdup_printf(_("Can't rename file '%s'\n to \n'%s'!\n(%s)"),
                              cur_basename_utf8,new_basename_utf8,g_strerror(errno));
        msg1 = g_strdup_printf(_("Can't rename file '%s' to '%s'! (%s)"),
                              cur_basename_utf8,new_basename_utf8,g_strerror(errno));
        if (!hide_msgbox)
        {
            msgbox = msg_box_new(_("Error..."),
                                 GTK_WINDOW(MainWindow),
                                 NULL,
                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                 msg,
                                 GTK_STOCK_DIALOG_ERROR,
                                 GTK_STOCK_OK, GTK_RESPONSE_OK,
                                 NULL);
            gtk_dialog_run(GTK_DIALOG(msgbox));
            gtk_widget_destroy(msgbox);
        }
        g_free(msg);

        Log_Print(LOG_ERROR,"%s", msg1);
        g_free(msg1);

        Statusbar_Message(_("File(s) not renamed..."),TRUE);

        g_free(tmp_filename);
        g_free(cur_basename_utf8);
        g_free(new_basename_utf8);
        g_free(dirname_cur);
        g_free(dirname_new);
        return FALSE;
    }

    return TRUE;
}

/*
 * Delete the file ETFile
 */
gint Delete_File (ET_File *ETFile, gboolean multiple_files)
{
    GtkWidget *msgbox = NULL;
    GtkWidget *msgbox_check_button = NULL;
    gchar *cur_filename;
    gchar *cur_filename_utf8;
    gchar *basename_utf8;
    gchar *msg;
    gint response;
    gint stop_loop;

    if (!ETFile) return FALSE;

    // Filename of the file to delete
    cur_filename      = ((File_Name *)(ETFile->FileNameCur)->data)->value;
    cur_filename_utf8 = ((File_Name *)(ETFile->FileNameCur)->data)->value_utf8;
    basename_utf8 = g_path_get_basename(cur_filename);

    /*
     * Remove the file
     */
    if (CONFIRM_DELETE_FILE && !SF_HideMsgbox_Delete_File)
    {
        msg = g_strdup_printf(_("Do you really want to delete definitively the file\n'%s' ?"),basename_utf8);
        if (multiple_files)
        {
            msgbox = msg_box_new(_("Delete File..."),
                                 GTK_WINDOW(MainWindow),
                                 &msgbox_check_button,
                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                 msg,
                                 GTK_STOCK_DIALOG_QUESTION,
                                 GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_NO,    GTK_RESPONSE_NO,
								 GTK_STOCK_YES,   GTK_RESPONSE_YES,
                                 NULL);
            //GTK_TOGGLE_BUTTON(msgbox_check_button)->active = TRUE; // Checked by default
        }else
        {
            msgbox = msg_box_new(_("Delete File..."),
                                 GTK_WINDOW(MainWindow),
                                 NULL,
                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                 msg,
                                 GTK_STOCK_DIALOG_QUESTION,
                                 GTK_STOCK_NO,    GTK_RESPONSE_NO,
								 GTK_STOCK_YES,   GTK_RESPONSE_YES,
                                 NULL);
        }
        g_free(msg);
        SF_ButtonPressed_Delete_File = response = gtk_dialog_run(GTK_DIALOG(msgbox));
        if (msgbox_check_button && GTK_TOGGLE_BUTTON(msgbox_check_button)->active)
            SF_HideMsgbox_Delete_File = GTK_TOGGLE_BUTTON(msgbox_check_button)->active;
        gtk_widget_destroy(msgbox);
    }else
    {
        if (SF_HideMsgbox_Delete_File)
            response = SF_ButtonPressed_Delete_File;
        else
            response = GTK_RESPONSE_YES;
    }

    switch (response)
    {
        case GTK_RESPONSE_YES:
            if (remove(cur_filename)==0)
            {
                msg = g_strdup_printf(_("File '%s' deleted"), basename_utf8);
                Statusbar_Message(msg,FALSE);
                g_free(msg);
                g_free(basename_utf8);
                return 1;
            }
            break;
        case GTK_RESPONSE_NO:
            break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_NONE:
            stop_loop = -1;
            g_free(basename_utf8);
            return stop_loop;
            break;
    }

    g_free(basename_utf8);
    return 0;
}

/*
 * Copy a file to a new location
 */
gint Copy_File (gchar const *fileold, gchar const *filenew)
{
    FILE* fOld;
    FILE* fNew;
    gchar buffer[512];
    gint NbRead;
    struct stat    statbuf;
    struct utimbuf utimbufbuf;

    if ( (fOld=fopen(fileold, "rb")) == NULL )
    {
        return FALSE;
    }

    if ( (fNew=fopen(filenew, "wb")) == NULL )
    {
        fclose(fOld);
        return FALSE;
    }

    while ( (NbRead=fread(buffer, 1, 512, fOld)) != 0 )
    {
        fwrite(buffer, 1, NbRead, fNew);
    }

    fclose(fNew);
    fclose(fOld);

    // Copy properties of the old file to the new one.
    stat(fileold,&statbuf);
    chmod(filenew,statbuf.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO));
    chown(filenew,statbuf.st_uid,statbuf.st_gid);
    utimbufbuf.actime  = statbuf.st_atime; // Last access time
    utimbufbuf.modtime = statbuf.st_mtime; // Last modification time
    utime(filenew,&utimbufbuf);

    return TRUE;
}

void Action_Select_Browser_Style (void)
{
    if (!ETCore->ETFileDisplayedList) return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    Browser_Display_Tree_Or_Artist_Album_List();

    Update_Command_Buttons_Sensivity();
}





/*
 * Scans the specified directory: and load files into a list.
 * If the path doesn't exist, we free the previous loaded list of files.
 */
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
gboolean Read_Directory (gchar *path_real)
{
    DIR   *dir;
    gchar *msg;
    gchar *tmp;
    gchar  progress_bar_text[30];
    guint  nbrfile = 0;
    double fraction;
    GList *FileList = NULL;
    gint   progress_bar_index = 0;
    GtkAction *uiaction;
    GtkWidget *TBViewMode;

    if (!path_real)
        return FALSE;

    ReadingDirectory = TRUE;    /* A flag to avoid to start an other reading */

    /* Initialize file list */
    ET_Core_Free();
    ET_Core_Initialize();
    Update_Command_Buttons_Sensivity();

    /* Initialize browser list */
    Browser_List_Clear();

    /* Clear entry boxes  */
    Clear_File_Entry_Field();
    Clear_Header_Fields();
    Clear_Tag_Entry_Fields();
    gtk_label_set_text(GTK_LABEL(FileIndex),"0/0:");

    TBViewMode = gtk_ui_manager_get_widget(UIManager, "/ToolBar/ViewModeToggle");
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(TBViewMode),FALSE);
    //Browser_Display_Tree_Or_Artist_Album_List(); // To show the corresponding lists...

    // Set to unsensitive the Browser Area, to avoid to select an other file while loading the first one
    Browser_Area_Set_Sensitive(FALSE);

    /* Placed only here, to empty the previous list of files */
    if ((dir=opendir(path_real)) == NULL)
    {
        // Message if the directory doesn't exist...
        GtkWidget *msgbox;
        gchar *path_utf8 = filename_to_display(path_real);
        gchar *msg;

        msg = g_strdup_printf(_("Can't read directory :\n'%s'\n(%s)"),path_utf8,g_strerror(errno));
        msgbox = msg_box_new(_("Error..."),
                             GTK_WINDOW(MainWindow),
                             NULL,
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             msg,
                             GTK_STOCK_DIALOG_ERROR,
                             GTK_STOCK_OK, GTK_RESPONSE_OK,
                             NULL);
        gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);
        g_free(msg);
        g_free(path_utf8);

        ReadingDirectory = FALSE; //Allow a new reading
        Browser_Area_Set_Sensitive(TRUE);
        return FALSE;
    }
    closedir(dir);

    /* Open the window to quit recursion (since 27/04/2007 : not only into recursion mode) */
    Set_Busy_Cursor();
    uiaction = gtk_ui_manager_get_action(UIManager, "/ToolBar/Stop");
    g_object_set(uiaction, "sensitive", BROWSE_SUBDIR, NULL);
    //if (BROWSE_SUBDIR)
        Open_Quit_Recursion_Function_Window();

    /* Read the directory recursively */
    msg = g_strdup_printf(_("Search in progress..."));
    Statusbar_Message(msg,FALSE);
    g_free(msg);
    // Search the supported files
    FileList = Read_Directory_Recursively(FileList,path_real,BROWSE_SUBDIR);
    nbrfile = g_list_length(FileList);

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), 0.0);
    g_snprintf(progress_bar_text, 30, "%d/%d", 0, nbrfile);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), progress_bar_text);

    // Load the supported files (Extension recognized)
    while (FileList)
    {
        gchar *filename_real = FileList->data; // Contains real filenames
        gchar *filename_utf8 = filename_to_display(filename_real);

        msg = g_strdup_printf(_("File: '%s'"),filename_utf8);
        Statusbar_Message(msg,FALSE);
        g_free(msg);
        g_free(filename_utf8);

        // Warning: Do not free filename_real because ET_Add_File.. uses it for internal structures
        ET_Add_File_To_File_List(filename_real);

        // Update the progress bar
        fraction = (++progress_bar_index) / (double) nbrfile;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), fraction);
        g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, nbrfile);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), progress_bar_text);
        while (gtk_events_pending())
            gtk_main_iteration();

        if ( !FileList->next || (Main_Stop_Button_Pressed==1) ) break;
        FileList = FileList->next;
    }
    if (FileList) g_list_free(FileList);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), "");

    /* Close window to quit recursion */
    Destroy_Quit_Recursion_Function_Window();
    Main_Stop_Button_Pressed = 0;
    uiaction = gtk_ui_manager_get_action(UIManager, "/ToolBar/Stop");
    g_object_set(uiaction, "sensitive", FALSE, NULL);

    //ET_Debug_Print_File_List(ETCore->ETFileList,__FILE__,__LINE__,__FUNCTION__);

    if (ETCore->ETFileList)
    {
        //GList *etfilelist;
        /* Load the list of file into the browser list widget */
        Browser_Display_Tree_Or_Artist_Album_List();

        /* Load the list attached to the TrackEntry */
        Load_Track_List_To_UI();

        /* Display the first file */
        //No need to select first item, because Browser_Display_Tree_Or_Artist_Album_List() does this
        //etfilelist = ET_Displayed_File_List_First();
        //if (etfilelist)
        //{
        //    ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
        //    Browser_List_Select_File_By_Etfile((ET_File *)etfilelist->data,FALSE);
        //}

        /* Prepare message for the status bar */
        if (BROWSE_SUBDIR)
            msg = g_strdup_printf(_("Found %d file(s) in this directory and subdirectories."),ETCore->ETFileDisplayedList_Length);
        else
            msg = g_strdup_printf(_("Found %d file(s) in this directory."),ETCore->ETFileDisplayedList_Length);

    }else
    {
        /* Clear entry boxes */
        Clear_File_Entry_Field();
        Clear_Header_Fields();
        Clear_Tag_Entry_Fields();

        if (FileIndex)
            gtk_label_set_text(GTK_LABEL(FileIndex),"0/0:");


        tmp = g_strdup_printf(_("%u file(s)"),0); // See in ET_Display_Filename_To_UI
        Browser_Label_Set_Text(tmp);
        g_free(tmp);

        /* Prepare message for the status bar */
        if (BROWSE_SUBDIR)
            msg = g_strdup(_("No file found in this directory and subdirectories!"));
        else
            msg = g_strdup(_("No file found in this directory!"));
    }

    /* Update sensitivity of buttons and menus */
    Update_Command_Buttons_Sensivity();

    Browser_Area_Set_Sensitive(TRUE);

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), 0.0);
    Statusbar_Message(msg,FALSE);
    g_free(msg);
    Set_Unbusy_Cursor();
    ReadingDirectory = FALSE;

    return TRUE;
}



/*
 * Recurse the path to create a list of files. Return a GList of the files found.
 */
GList *Read_Directory_Recursively (GList *file_list, gchar *path_real, gint recurse)
{
    DIR *dir;
    struct dirent *dirent;
    struct stat statbuf;
    gchar *filename;

    if ((dir = opendir(path_real)) == NULL)
        return file_list;

    while ((dirent = readdir(dir)) != NULL)
    {
        if (Main_Stop_Button_Pressed == 1)
        {
            closedir(dir);
            return file_list;
        }

        // We don't read the directories '.' and '..', but may read hidden directories like '.mydir'
        if ( (g_ascii_strcasecmp (dirent->d_name,"..")   != 0)
        &&  ((g_ascii_strncasecmp(dirent->d_name,".", 1) != 0) || (BROWSE_HIDDEN_DIR && strlen(dirent->d_name) > 1)) )
        {
            if (path_real[strlen(path_real)-1]!=G_DIR_SEPARATOR)
                filename = g_strconcat(path_real,G_DIR_SEPARATOR_S,dirent->d_name,NULL);
            else
                filename = g_strconcat(path_real,dirent->d_name,NULL);

            if (stat(filename, &statbuf) == -1)
            {
                g_free(filename);
                continue;
            }

            if (S_ISDIR(statbuf.st_mode))
            {
                if (recurse)
                    file_list = Read_Directory_Recursively(file_list,filename,recurse);
            //}else if ( (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) && ET_File_Is_Supported(filename))
            }else if ( S_ISREG(statbuf.st_mode) && ET_File_Is_Supported(filename))
            {
                file_list = g_list_append(file_list,g_strdup(filename));
            }
            g_free(filename);

            // Just to not block X events
            while (gtk_events_pending())
                gtk_main_iteration();
        }
    }

    closedir(dir);
    return file_list;
}


/*
 * Window with the 'STOP' button to stop recursion when reading directories
 */
void Open_Quit_Recursion_Function_Window (void)
{
    GtkWidget *button;
    GtkWidget *frame;

    if (QuitRecursionWindow != NULL)
        return;
    QuitRecursionWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(QuitRecursionWindow),_("Searching..."));
    gtk_window_set_transient_for(GTK_WINDOW(QuitRecursionWindow),GTK_WINDOW(MainWindow));
    //gtk_window_set_policy(GTK_WINDOW(QuitRecursionWindow),FALSE,FALSE,TRUE);

    // Just center on mainwindow
    gtk_window_set_position(GTK_WINDOW(QuitRecursionWindow), GTK_WIN_POS_CENTER_ON_PARENT);

    g_signal_connect(G_OBJECT(QuitRecursionWindow),"destroy",
                     G_CALLBACK(Quit_Recursion_Function_Button_Pressed),NULL);
    g_signal_connect(G_OBJECT(QuitRecursionWindow),"delete_event",
                     G_CALLBACK(Quit_Recursion_Function_Button_Pressed),NULL);
    g_signal_connect(G_OBJECT(QuitRecursionWindow),"key_press_event",
                     G_CALLBACK(Quit_Recursion_Window_Key_Press),NULL);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(QuitRecursionWindow),frame);
    gtk_container_set_border_width(GTK_CONTAINER(frame),2);

    // Button to stop...
    button = Create_Button_With_Icon_And_Label(GTK_STOCK_STOP,_("  STOP the search...  "));
    gtk_container_set_border_width(GTK_CONTAINER(button),8);
    gtk_container_add(GTK_CONTAINER(frame),button);
    g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(Quit_Recursion_Function_Button_Pressed),NULL);

    gtk_widget_show_all(QuitRecursionWindow);
}
void Destroy_Quit_Recursion_Function_Window (void)
{
    if (QuitRecursionWindow)
    {
        gtk_widget_destroy(QuitRecursionWindow);
        QuitRecursionWindow = (GtkWidget *)NULL;
        /*Statusbar_Message(_("Recursive file search interrupted."),FALSE);*/
    }
}
void Quit_Recursion_Function_Button_Pressed (void)
{
    Action_Main_Stop_Button_Pressed();
    Destroy_Quit_Recursion_Function_Window();
}
void Quit_Recursion_Window_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_Escape:
                Destroy_Quit_Recursion_Function_Window();
                break;
        }
    }
}


/*
 * To stop the recursive search within directories or saving files
 */
void Action_Main_Stop_Button_Pressed (void)
{
    GtkAction *uiaction;
    Main_Stop_Button_Pressed = 1;
    uiaction = gtk_ui_manager_get_action(UIManager, "/ToolBar/Stop");
    g_object_set(uiaction, "sensitive", FALSE, NULL);
}

void ui_widget_set_sensitive (const gchar *menu, const gchar *action, gboolean sensitive)
{
    GtkAction *uiaction;
    gchar *path;

    path = g_strconcat("/MenuBar/", menu,"/", action, NULL);

    uiaction = gtk_ui_manager_get_action(UIManager, path);
    if (uiaction)
        g_object_set(uiaction, "sensitive", sensitive, NULL);
    g_free(path);
}

/*
 * Update_Command_Buttons_Sensivity: Set to sensitive/unsensitive the state of each button into
 * the commands area and menu items in function of state of the "main list".
 */
void Update_Command_Buttons_Sensivity (void)
{
    GtkAction *uiaction;

    if (!ETCore->ETFileDisplayedList)
    {
        /* No file found */

        /* File and Tag frames */
        File_Area_Set_Sensitive(FALSE);
        Tag_Area_Set_Sensitive(FALSE);

        /* Tool bar buttons (the others are covered by the menu) */
        uiaction = gtk_ui_manager_get_action(UIManager, "/ToolBar/Stop");
        g_object_set(uiaction, "sensitive", FALSE, NULL);

        /* Scanner Window */
        if (SWScanButton)
            gtk_widget_set_sensitive(GTK_WIDGET(SWScanButton),FALSE);

        /* Menu commands */
        ui_widget_set_sensitive(MENU_FILE, AM_OPEN_FILE_WITH, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_SELECT_ALL_FILES, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_UNSELECT_ALL_FILES, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_INVERT_SELECTION, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_DELETE_FILE, FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_ASCENDING_FILENAME, FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_DESCENDING_FILENAME,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_ASCENDING_CREATION_DATE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_DESCENDING_CREATION_DATE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_ASCENDING_TRACK_NUMBER,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_DESCENDING_TRACK_NUMBER,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_ASCENDING_TITLE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_DESCENDING_TITLE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_ASCENDING_ARTIST,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_DESCENDING_ARTIST,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_ASCENDING_ALBUM,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_DESCENDING_ALBUM,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_ASCENDING_YEAR,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_DESCENDING_YEAR,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_ASCENDING_GENRE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_DESCENDING_GENRE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_ASCENDING_COMMENT,FALSE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH, AM_SORT_DESCENDING_COMMENT,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_ASCENDING_FILE_TYPE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_DESCENDING_FILE_TYPE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_ASCENDING_FILE_SIZE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_DESCENDING_FILE_SIZE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_ASCENDING_FILE_DURATION,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_DESCENDING_FILE_DURATION,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_ASCENDING_FILE_BITRATE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_DESCENDING_FILE_BITRATE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_ASCENDING_FILE_SAMPLERATE,FALSE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH, AM_SORT_DESCENDING_FILE_SAMPLERATE,FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_PREV, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_NEXT, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_FIRST, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_LAST, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_SCAN, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_REMOVE, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_UNDO, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_REDO, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_SAVE, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_SAVE_FORCED, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_UNDO_HISTORY, FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_REDO_HISTORY, FALSE);
        ui_widget_set_sensitive(MENU_MISC, AM_SEARCH_FILE, FALSE);
        ui_widget_set_sensitive(MENU_MISC, AM_FILENAME_FROM_TXT, FALSE);
        ui_widget_set_sensitive(MENU_MISC, AM_WRITE_PLAYLIST, FALSE);
        ui_widget_set_sensitive(MENU_MISC, AM_RUN_AUDIO_PLAYER, FALSE);
        ui_widget_set_sensitive(MENU_SCANNER, AM_SCANNER_FILL_TAG, FALSE);
        ui_widget_set_sensitive(MENU_SCANNER, AM_SCANNER_RENAME_FILE, FALSE);
        ui_widget_set_sensitive(MENU_SCANNER, AM_SCANNER_PROCESS_FIELDS, FALSE);

        return;
    }else
    {
        GList *selfilelist = NULL;
        ET_File *etfile;
        gboolean has_undo = FALSE;
        gboolean has_redo = FALSE;
        //gboolean has_to_save = FALSE;
        GtkTreeSelection *selection;

        /* File and Tag frames */
        File_Area_Set_Sensitive(TRUE);
        Tag_Area_Set_Sensitive(TRUE);

        /* Tool bar buttons */
        uiaction = gtk_ui_manager_get_action(UIManager, "/ToolBar/Stop");
        g_object_set(uiaction, "sensitive", FALSE, NULL);

        /* Scanner Window */
        if (SWScanButton)    gtk_widget_set_sensitive(GTK_WIDGET(SWScanButton),TRUE);

        /* Commands into menu */
        ui_widget_set_sensitive(MENU_FILE, AM_OPEN_FILE_WITH,TRUE);
        ui_widget_set_sensitive(MENU_FILE, AM_SELECT_ALL_FILES,TRUE);
        ui_widget_set_sensitive(MENU_FILE, AM_UNSELECT_ALL_FILES,TRUE);
        ui_widget_set_sensitive(MENU_FILE, AM_INVERT_SELECTION,TRUE);
        ui_widget_set_sensitive(MENU_FILE, AM_DELETE_FILE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_ASCENDING_FILENAME,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_DESCENDING_FILENAME,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_ASCENDING_CREATION_DATE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_DESCENDING_CREATION_DATE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_ASCENDING_TRACK_NUMBER,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_DESCENDING_TRACK_NUMBER,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_ASCENDING_TITLE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_DESCENDING_TITLE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_ASCENDING_ARTIST,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_DESCENDING_ARTIST,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_ASCENDING_ALBUM,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_DESCENDING_ALBUM,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_ASCENDING_YEAR,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_DESCENDING_YEAR,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_ASCENDING_GENRE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_DESCENDING_GENRE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_ASCENDING_COMMENT,TRUE);
        ui_widget_set_sensitive(MENU_SORT_TAG_PATH,AM_SORT_DESCENDING_COMMENT,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_ASCENDING_FILE_TYPE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_DESCENDING_FILE_TYPE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_ASCENDING_FILE_SIZE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_DESCENDING_FILE_SIZE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_ASCENDING_FILE_DURATION,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_DESCENDING_FILE_DURATION,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_ASCENDING_FILE_BITRATE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_DESCENDING_FILE_BITRATE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_ASCENDING_FILE_SAMPLERATE,TRUE);
        ui_widget_set_sensitive(MENU_SORT_PROP_PATH,AM_SORT_DESCENDING_FILE_SAMPLERATE,TRUE);
        ui_widget_set_sensitive(MENU_FILE,AM_SCAN,TRUE);
        ui_widget_set_sensitive(MENU_FILE,AM_REMOVE,TRUE);
        ui_widget_set_sensitive(MENU_MISC,AM_SEARCH_FILE,TRUE);
        ui_widget_set_sensitive(MENU_MISC,AM_FILENAME_FROM_TXT,TRUE);
        ui_widget_set_sensitive(MENU_MISC,AM_WRITE_PLAYLIST,TRUE);
        ui_widget_set_sensitive(MENU_MISC,AM_RUN_AUDIO_PLAYER,TRUE);
        ui_widget_set_sensitive(MENU_SCANNER,AM_SCANNER_FILL_TAG,TRUE);
        ui_widget_set_sensitive(MENU_SCANNER,AM_SCANNER_RENAME_FILE,TRUE);
        ui_widget_set_sensitive(MENU_SCANNER,AM_SCANNER_PROCESS_FIELDS,TRUE);

        /* Check if one of the selected files has undo or redo data */
        if (BrowserList)
        {
            selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
            selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);
            while (selfilelist)
            {
                etfile = Browser_List_Get_ETFile_From_Path(selfilelist->data);
                has_undo    |= ET_File_Data_Has_Undo_Data(etfile);
                has_redo    |= ET_File_Data_Has_Redo_Data(etfile);
                //has_to_save |= ET_Check_If_File_Is_Saved(etfile);
                if ((has_undo && has_redo /*&& has_to_save*/) || !selfilelist->next) // Useless to check the other files
                    break;
                selfilelist = g_list_next(selfilelist);
            }
            g_list_foreach(selfilelist, (GFunc) gtk_tree_path_free, NULL);
            g_list_free(selfilelist);
        }

        /* Enable undo commands if there are undo data */
        if (has_undo)
            ui_widget_set_sensitive(MENU_FILE, AM_UNDO, TRUE);
        else
            ui_widget_set_sensitive(MENU_FILE, AM_UNDO, FALSE);

        /* Enable redo commands if there are redo data */
        if (has_redo)
            ui_widget_set_sensitive(MENU_FILE, AM_REDO, TRUE);
        else
            ui_widget_set_sensitive(MENU_FILE, AM_REDO, FALSE);

        /* Enable save file command if file has been changed */
        // Desactivated because problem with only one file in the list, as we can't change the selected file => can't mark file as changed
        /*if (has_to_save)
            ui_widget_set_sensitive(MENU_FILE, AM_SAVE, FALSE);
        else*/
            ui_widget_set_sensitive(MENU_FILE, AM_SAVE, TRUE);
        
        ui_widget_set_sensitive(MENU_FILE, AM_SAVE_FORCED, TRUE);

        /* Enable undo command if there are data into main undo list (history list) */
        if (ET_History_File_List_Has_Undo_Data())
            ui_widget_set_sensitive(MENU_FILE, AM_UNDO_HISTORY, TRUE);
        else
            ui_widget_set_sensitive(MENU_FILE, AM_UNDO_HISTORY, FALSE);

        /* Enable redo commands if there are data into main redo list (history list) */
        if (ET_History_File_List_Has_Redo_Data())
            ui_widget_set_sensitive(MENU_FILE, AM_REDO_HISTORY, TRUE);
        else
            ui_widget_set_sensitive(MENU_FILE, AM_REDO_HISTORY, FALSE);
    }

    if (!ETCore->ETFileDisplayedList->prev)    /* Is it the 1st item ? */
    {
        ui_widget_set_sensitive(MENU_FILE, AM_PREV,FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_FIRST,FALSE);
    }else
    {
        ui_widget_set_sensitive(MENU_FILE, AM_PREV,TRUE);
        ui_widget_set_sensitive(MENU_FILE, AM_FIRST,TRUE);
    }
    if (!ETCore->ETFileDisplayedList->next)    /* Is it the last item ? */
    {
        ui_widget_set_sensitive(MENU_FILE, AM_NEXT,FALSE);
        ui_widget_set_sensitive(MENU_FILE, AM_LAST,FALSE);
    }else
    {
        ui_widget_set_sensitive(MENU_FILE, AM_NEXT,TRUE);
        ui_widget_set_sensitive(MENU_FILE, AM_LAST,TRUE);
    }
}

/*
 * Just to disable buttons when we are saving files (do not disable Quit button)
 */
void Disable_Command_Buttons (void)
{
    /* Scanner Window */
    if (SWScanButton)
        gtk_widget_set_sensitive(SWScanButton,FALSE);

    /* "File" menu commands */
    ui_widget_set_sensitive(MENU_FILE,AM_OPEN_FILE_WITH,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_SELECT_ALL_FILES,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_UNSELECT_ALL_FILES,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_INVERT_SELECTION,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_DELETE_FILE,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_FIRST,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_PREV,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_NEXT,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_LAST,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_SCAN,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_REMOVE,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_UNDO,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_REDO,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_SAVE,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_SAVE_FORCED,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_UNDO_HISTORY,FALSE);
    ui_widget_set_sensitive(MENU_FILE,AM_REDO_HISTORY,FALSE);

    /* "Scanner" menu commands */
    ui_widget_set_sensitive(MENU_SCANNER,AM_SCANNER_FILL_TAG,FALSE);
    ui_widget_set_sensitive(MENU_SCANNER,AM_SCANNER_RENAME_FILE,FALSE);
    ui_widget_set_sensitive(MENU_SCANNER,AM_SCANNER_PROCESS_FIELDS,FALSE);

}

/*
 * Disable (FALSE) / Enable (TRUE) all user widgets in the tag area
 */
void Tag_Area_Set_Sensitive (gboolean activate)
{
    if (!TagArea) return;

    // TAG Area (entries + buttons)
    gtk_widget_set_sensitive(GTK_BIN(TagArea)->child,activate);

    /*// TAG Area
    gtk_widget_set_sensitive(GTK_WIDGET(TitleEntry),            activate);
    gtk_widget_set_sensitive(GTK_WIDGET(ArtistEntry),           activate);
    gtk_widget_set_sensitive(GTK_WIDGET(AlbumEntry),            activate);
    gtk_widget_set_sensitive(GTK_WIDGET(DiscNumberEntry),       activate);
    gtk_widget_set_sensitive(GTK_WIDGET(YearEntry),             activate);
    gtk_widget_set_sensitive(GTK_WIDGET(TrackEntryCombo),       activate);
    gtk_widget_set_sensitive(GTK_WIDGET(TrackTotalEntry),       activate);
    gtk_widget_set_sensitive(GTK_WIDGET(CommentEntry),          activate);
    gtk_widget_set_sensitive(GTK_WIDGET(GenreCombo),            activate);
    gtk_widget_set_sensitive(GTK_WIDGET(PictureScrollWindow),   activate);
    // Mini buttons
    gtk_widget_set_sensitive(GTK_WIDGET(TitleMButton),          activate);
    gtk_widget_set_sensitive(GTK_WIDGET(ArtistMButton),         activate);
    gtk_widget_set_sensitive(GTK_WIDGET(DiscNumberMButton),     activate);
    gtk_widget_set_sensitive(GTK_WIDGET(AlbumMButton),          activate);
    gtk_widget_set_sensitive(GTK_WIDGET(YearMButton),           activate);
    gtk_widget_set_sensitive(GTK_WIDGET(TrackMButton),          activate);
    gtk_widget_set_sensitive(GTK_WIDGET(TrackMButtonSequence),  activate);
    gtk_widget_set_sensitive(GTK_WIDGET(TrackMButtonNbrFiles),  activate);
    gtk_widget_set_sensitive(GTK_WIDGET(CommentMButton),        activate);
    gtk_widget_set_sensitive(GTK_WIDGET(GenreMButton),          activate);
    gtk_widget_set_sensitive(GTK_WIDGET(PictureMButton),        activate);*/
}

/*
 * Disable (FALSE) / Enable (TRUE) all user widgets in the file area
 */
void File_Area_Set_Sensitive (gboolean activate)
{
    if (!FileArea) return;

    // File Area
    gtk_widget_set_sensitive(GTK_BIN(FileArea)->child,activate);
    /*gtk_widget_set_sensitive(GTK_WIDGET(FileEntry),activate);*/
}

/*
 * Display controls according the kind of tag... (Hide some controls if not available for a tag type)
 */
void Tag_Area_Display_Controls (ET_File *ETFile)
{
    if (!ETFile || !ETFile->ETFileDescription || !TitleLabel)
        return;

    // Commun controls for all tags
    gtk_widget_show(GTK_WIDGET(TitleLabel));
    gtk_widget_show(GTK_WIDGET(TitleEntry));
    gtk_widget_show(GTK_WIDGET(TitleMButton));
    gtk_widget_show(GTK_WIDGET(ArtistLabel));
    gtk_widget_show(GTK_WIDGET(ArtistEntry));
    gtk_widget_show(GTK_WIDGET(ArtistMButton));
    gtk_widget_show(GTK_WIDGET(AlbumLabel));
    gtk_widget_show(GTK_WIDGET(AlbumEntry));
    gtk_widget_show(GTK_WIDGET(AlbumMButton));
    gtk_widget_show(GTK_WIDGET(YearLabel));
    gtk_widget_show(GTK_WIDGET(YearEntry));
    gtk_widget_show(GTK_WIDGET(YearMButton));
    gtk_widget_show(GTK_WIDGET(TrackLabel));
    gtk_widget_show(GTK_WIDGET(TrackEntryCombo));
    gtk_widget_show(GTK_WIDGET(TrackTotalEntry));
    gtk_widget_show(GTK_WIDGET(TrackMButton));
    gtk_widget_show(GTK_WIDGET(TrackMButtonSequence));
    gtk_widget_show(GTK_WIDGET(TrackMButtonNbrFiles));
    gtk_widget_show(GTK_WIDGET(GenreLabel));
    gtk_widget_show(GTK_WIDGET(GenreCombo));
    gtk_widget_show(GTK_WIDGET(GenreMButton));
    gtk_widget_show(GTK_WIDGET(CommentLabel));
    gtk_widget_show(GTK_WIDGET(CommentEntry));
    gtk_widget_show(GTK_WIDGET(CommentMButton));

    // Special controls to display or not!
    switch (ETFile->ETFileDescription->TagType)
    {
        case ID3_TAG:
            if (!FILE_WRITING_ID3V2_WRITE_TAG)
            {
                // ID3v1 : Hide specifics ID3v2 fields if not activated!
                gtk_widget_hide(GTK_WIDGET(DiscNumberLabel));
                gtk_widget_hide(GTK_WIDGET(DiscNumberEntry));
                gtk_widget_hide(GTK_WIDGET(DiscNumberMButton));
                gtk_widget_hide(GTK_WIDGET(ComposerLabel));
                gtk_widget_hide(GTK_WIDGET(ComposerEntry));
                gtk_widget_hide(GTK_WIDGET(ComposerMButton));
                gtk_widget_hide(GTK_WIDGET(OrigArtistLabel));
                gtk_widget_hide(GTK_WIDGET(OrigArtistEntry));
                gtk_widget_hide(GTK_WIDGET(OrigArtistMButton));
                gtk_widget_hide(GTK_WIDGET(CopyrightLabel));
                gtk_widget_hide(GTK_WIDGET(CopyrightEntry));
                gtk_widget_hide(GTK_WIDGET(CopyrightMButton));
                gtk_widget_hide(GTK_WIDGET(URLLabel));
                gtk_widget_hide(GTK_WIDGET(URLEntry));
                gtk_widget_hide(GTK_WIDGET(URLMButton));
                gtk_widget_hide(GTK_WIDGET(EncodedByLabel));
                gtk_widget_hide(GTK_WIDGET(EncodedByEntry));
                gtk_widget_hide(GTK_WIDGET(EncodedByMButton));
                gtk_widget_hide(GTK_WIDGET(PictureLabel));
                gtk_widget_hide(GTK_WIDGET(PictureScrollWindow));
                gtk_widget_hide(GTK_WIDGET(PictureMButton));
                gtk_widget_hide(GTK_WIDGET(PictureClearButton));
                gtk_widget_hide(GTK_WIDGET(PictureAddButton));
                gtk_widget_hide(GTK_WIDGET(PictureSaveButton));
                gtk_widget_hide(GTK_WIDGET(PicturePropertiesButton));
            }else
            {
                gtk_widget_show(GTK_WIDGET(DiscNumberLabel));
                gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
                gtk_widget_show(GTK_WIDGET(DiscNumberMButton));
                gtk_widget_show(GTK_WIDGET(ComposerLabel));
                gtk_widget_show(GTK_WIDGET(ComposerEntry));
                gtk_widget_show(GTK_WIDGET(ComposerMButton));
                gtk_widget_show(GTK_WIDGET(OrigArtistLabel));
                gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
                gtk_widget_show(GTK_WIDGET(OrigArtistMButton));
                gtk_widget_show(GTK_WIDGET(CopyrightLabel));
                gtk_widget_show(GTK_WIDGET(CopyrightEntry));
                gtk_widget_show(GTK_WIDGET(CopyrightMButton));
                gtk_widget_show(GTK_WIDGET(URLLabel));
                gtk_widget_show(GTK_WIDGET(URLEntry));
                gtk_widget_show(GTK_WIDGET(URLMButton));
                gtk_widget_show(GTK_WIDGET(EncodedByLabel));
                gtk_widget_show(GTK_WIDGET(EncodedByEntry));
                gtk_widget_show(GTK_WIDGET(EncodedByMButton));
                gtk_widget_show(GTK_WIDGET(PictureLabel));
                gtk_widget_show(GTK_WIDGET(PictureScrollWindow));
                gtk_widget_show(GTK_WIDGET(PictureMButton));
                gtk_widget_show(GTK_WIDGET(PictureClearButton));
                gtk_widget_show(GTK_WIDGET(PictureAddButton));
                gtk_widget_show(GTK_WIDGET(PictureSaveButton));
                gtk_widget_show(GTK_WIDGET(PicturePropertiesButton));
            }
            break;

#ifdef ENABLE_OGG
        case OGG_TAG:
            gtk_widget_show(GTK_WIDGET(DiscNumberLabel));
            gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_show(GTK_WIDGET(DiscNumberMButton));
            gtk_widget_show(GTK_WIDGET(ComposerLabel));
            gtk_widget_show(GTK_WIDGET(ComposerEntry));
            gtk_widget_show(GTK_WIDGET(ComposerMButton));
            gtk_widget_show(GTK_WIDGET(OrigArtistLabel));
            gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_show(GTK_WIDGET(OrigArtistMButton));
            gtk_widget_show(GTK_WIDGET(CopyrightLabel));
            gtk_widget_show(GTK_WIDGET(CopyrightEntry));
            gtk_widget_show(GTK_WIDGET(CopyrightMButton));
            gtk_widget_show(GTK_WIDGET(URLLabel));
            gtk_widget_show(GTK_WIDGET(URLEntry));
            gtk_widget_show(GTK_WIDGET(URLMButton));
            gtk_widget_show(GTK_WIDGET(EncodedByLabel));
            gtk_widget_show(GTK_WIDGET(EncodedByEntry));
            gtk_widget_show(GTK_WIDGET(EncodedByMButton));
            gtk_widget_show(GTK_WIDGET(PictureLabel));
            gtk_widget_show(GTK_WIDGET(PictureScrollWindow));
            gtk_widget_show(GTK_WIDGET(PictureMButton));
            gtk_widget_show(GTK_WIDGET(PictureClearButton));
            gtk_widget_show(GTK_WIDGET(PictureAddButton));
            gtk_widget_show(GTK_WIDGET(PictureSaveButton));
            gtk_widget_show(GTK_WIDGET(PicturePropertiesButton));
            break;
#endif

#ifdef ENABLE_FLAC
        case FLAC_TAG:
            gtk_widget_show(GTK_WIDGET(DiscNumberLabel));
            gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_show(GTK_WIDGET(DiscNumberMButton));
            gtk_widget_show(GTK_WIDGET(ComposerLabel));
            gtk_widget_show(GTK_WIDGET(ComposerEntry));
            gtk_widget_show(GTK_WIDGET(ComposerMButton));
            gtk_widget_show(GTK_WIDGET(OrigArtistLabel));
            gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_show(GTK_WIDGET(OrigArtistMButton));
            gtk_widget_show(GTK_WIDGET(CopyrightLabel));
            gtk_widget_show(GTK_WIDGET(CopyrightEntry));
            gtk_widget_show(GTK_WIDGET(CopyrightMButton));
            gtk_widget_show(GTK_WIDGET(URLLabel));
            gtk_widget_show(GTK_WIDGET(URLEntry));
            gtk_widget_show(GTK_WIDGET(URLMButton));
            gtk_widget_show(GTK_WIDGET(EncodedByLabel));
            gtk_widget_show(GTK_WIDGET(EncodedByEntry));
            gtk_widget_show(GTK_WIDGET(EncodedByMButton));
            #ifndef LEGACY_FLAC // Picture supported for FLAC >= 1.1.3...
            gtk_widget_show(GTK_WIDGET(PictureLabel));
            gtk_widget_show(GTK_WIDGET(PictureScrollWindow));
            gtk_widget_show(GTK_WIDGET(PictureMButton));
            gtk_widget_show(GTK_WIDGET(PictureClearButton));
            gtk_widget_show(GTK_WIDGET(PictureAddButton));
            gtk_widget_show(GTK_WIDGET(PictureSaveButton));
            gtk_widget_show(GTK_WIDGET(PicturePropertiesButton));
            #else
            if (WRITE_ID3_TAGS_IN_FLAC_FILE)
            {
                gtk_widget_show(GTK_WIDGET(PictureLabel));
                gtk_widget_show(GTK_WIDGET(PictureScrollWindow));
                gtk_widget_show(GTK_WIDGET(PictureMButton));
                gtk_widget_show(GTK_WIDGET(PictureClearButton));
                gtk_widget_show(GTK_WIDGET(PictureAddButton));
                gtk_widget_show(GTK_WIDGET(PictureSaveButton));
                gtk_widget_show(GTK_WIDGET(PicturePropertiesButton));
            }else
            {
                gtk_widget_hide(GTK_WIDGET(PictureLabel));
                gtk_widget_hide(GTK_WIDGET(PictureScrollWindow));
                gtk_widget_hide(GTK_WIDGET(PictureMButton));
                gtk_widget_hide(GTK_WIDGET(PictureClearButton));
                gtk_widget_hide(GTK_WIDGET(PictureAddButton));
                gtk_widget_hide(GTK_WIDGET(PictureSaveButton));
                gtk_widget_hide(GTK_WIDGET(PicturePropertiesButton));
            }
            #endif
            break;
#endif

        case APE_TAG:
            gtk_widget_show(GTK_WIDGET(DiscNumberLabel));
            gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_show(GTK_WIDGET(DiscNumberMButton));
            gtk_widget_show(GTK_WIDGET(ComposerLabel));
            gtk_widget_show(GTK_WIDGET(ComposerEntry));
            gtk_widget_show(GTK_WIDGET(ComposerMButton));
            gtk_widget_show(GTK_WIDGET(OrigArtistLabel));
            gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_show(GTK_WIDGET(OrigArtistMButton));
            gtk_widget_show(GTK_WIDGET(CopyrightLabel));
            gtk_widget_show(GTK_WIDGET(CopyrightEntry));
            gtk_widget_show(GTK_WIDGET(CopyrightMButton));
            gtk_widget_show(GTK_WIDGET(URLLabel));
            gtk_widget_show(GTK_WIDGET(URLEntry));
            gtk_widget_show(GTK_WIDGET(URLMButton));
            gtk_widget_show(GTK_WIDGET(EncodedByLabel));
            gtk_widget_show(GTK_WIDGET(EncodedByEntry));
            gtk_widget_show(GTK_WIDGET(EncodedByMButton));
            gtk_widget_hide(GTK_WIDGET(PictureLabel));
            gtk_widget_hide(GTK_WIDGET(PictureScrollWindow));
            gtk_widget_hide(GTK_WIDGET(PictureMButton));
            gtk_widget_hide(GTK_WIDGET(PictureClearButton));
            gtk_widget_hide(GTK_WIDGET(PictureAddButton));
            gtk_widget_hide(GTK_WIDGET(PictureSaveButton));
            gtk_widget_hide(GTK_WIDGET(PicturePropertiesButton));
            break;

#ifdef ENABLE_MP4
        case MP4_TAG:
            gtk_widget_show(GTK_WIDGET(DiscNumberLabel));
            gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_show(GTK_WIDGET(DiscNumberMButton));
            gtk_widget_show(GTK_WIDGET(ComposerLabel));
            gtk_widget_show(GTK_WIDGET(ComposerEntry));
            gtk_widget_show(GTK_WIDGET(ComposerMButton));
            gtk_widget_hide(GTK_WIDGET(OrigArtistLabel));
            gtk_widget_hide(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_hide(GTK_WIDGET(OrigArtistMButton));
            gtk_widget_hide(GTK_WIDGET(CopyrightLabel));
            gtk_widget_hide(GTK_WIDGET(CopyrightEntry));
            gtk_widget_hide(GTK_WIDGET(CopyrightMButton));
            gtk_widget_hide(GTK_WIDGET(URLLabel));
            gtk_widget_hide(GTK_WIDGET(URLEntry));
            gtk_widget_hide(GTK_WIDGET(URLMButton));
            gtk_widget_show(GTK_WIDGET(EncodedByLabel));
            gtk_widget_show(GTK_WIDGET(EncodedByEntry));
            gtk_widget_show(GTK_WIDGET(EncodedByMButton));
            gtk_widget_show(GTK_WIDGET(PictureLabel));
            gtk_widget_show(GTK_WIDGET(PictureScrollWindow));
            gtk_widget_show(GTK_WIDGET(PictureMButton));
            gtk_widget_show(GTK_WIDGET(PictureClearButton));
            gtk_widget_show(GTK_WIDGET(PictureAddButton));
            gtk_widget_show(GTK_WIDGET(PictureSaveButton));
            gtk_widget_show(GTK_WIDGET(PicturePropertiesButton));
            break;
#endif

#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
            gtk_widget_show(GTK_WIDGET(DiscNumberLabel));
            gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_show(GTK_WIDGET(DiscNumberMButton));
            gtk_widget_show(GTK_WIDGET(ComposerLabel));
            gtk_widget_show(GTK_WIDGET(ComposerEntry));
            gtk_widget_show(GTK_WIDGET(ComposerMButton));
            gtk_widget_show(GTK_WIDGET(OrigArtistLabel));
            gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_show(GTK_WIDGET(OrigArtistMButton));
            gtk_widget_show(GTK_WIDGET(CopyrightLabel));
            gtk_widget_show(GTK_WIDGET(CopyrightEntry));
            gtk_widget_show(GTK_WIDGET(CopyrightMButton));
            gtk_widget_show(GTK_WIDGET(URLLabel));
            gtk_widget_show(GTK_WIDGET(URLEntry));
            gtk_widget_show(GTK_WIDGET(URLMButton));
            gtk_widget_show(GTK_WIDGET(EncodedByLabel));
            gtk_widget_show(GTK_WIDGET(EncodedByEntry));
            gtk_widget_show(GTK_WIDGET(EncodedByMButton));
            gtk_widget_hide(GTK_WIDGET(PictureLabel));
            gtk_widget_hide(GTK_WIDGET(PictureScrollWindow));
            gtk_widget_hide(GTK_WIDGET(PictureMButton));
            gtk_widget_hide(GTK_WIDGET(PictureClearButton));
            gtk_widget_hide(GTK_WIDGET(PictureAddButton));
            gtk_widget_hide(GTK_WIDGET(PictureSaveButton));
            gtk_widget_hide(GTK_WIDGET(PicturePropertiesButton));
            break;
#endif /* ENABLE_WAVPACK */

        case UNKNOWN_TAG:
        default:
            gtk_widget_hide(GTK_WIDGET(DiscNumberLabel));
            gtk_widget_hide(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_hide(GTK_WIDGET(DiscNumberMButton));
            gtk_widget_hide(GTK_WIDGET(ComposerLabel));
            gtk_widget_hide(GTK_WIDGET(ComposerEntry));
            gtk_widget_hide(GTK_WIDGET(ComposerMButton));
            gtk_widget_hide(GTK_WIDGET(OrigArtistLabel));
            gtk_widget_hide(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_hide(GTK_WIDGET(OrigArtistMButton));
            gtk_widget_hide(GTK_WIDGET(CopyrightLabel));
            gtk_widget_hide(GTK_WIDGET(CopyrightEntry));
            gtk_widget_hide(GTK_WIDGET(CopyrightMButton));
            gtk_widget_hide(GTK_WIDGET(URLLabel));
            gtk_widget_hide(GTK_WIDGET(URLEntry));
            gtk_widget_hide(GTK_WIDGET(URLMButton));
            gtk_widget_hide(GTK_WIDGET(EncodedByLabel));
            gtk_widget_hide(GTK_WIDGET(EncodedByEntry));
            gtk_widget_hide(GTK_WIDGET(EncodedByMButton));
            gtk_widget_hide(GTK_WIDGET(PictureLabel));
            gtk_widget_hide(GTK_WIDGET(PictureScrollWindow));
            gtk_widget_hide(GTK_WIDGET(PictureMButton));
            gtk_widget_hide(GTK_WIDGET(PictureClearButton));
            gtk_widget_hide(GTK_WIDGET(PictureAddButton));
            gtk_widget_hide(GTK_WIDGET(PictureSaveButton));
            gtk_widget_hide(GTK_WIDGET(PicturePropertiesButton));
            break;
    }
}


/*
 * Clear the entries of tag area
 */
void Clear_Tag_Entry_Fields (void)
{
    //GtkTextBuffer *textbuffer;

    if (!TitleEntry) return;

    gtk_entry_set_text(GTK_ENTRY(TitleEntry),                       "");
    gtk_entry_set_text(GTK_ENTRY(ArtistEntry),                      "");
    gtk_entry_set_text(GTK_ENTRY(AlbumEntry),                       "");
    gtk_entry_set_text(GTK_ENTRY(DiscNumberEntry),                  "");
    gtk_entry_set_text(GTK_ENTRY(YearEntry),                        "");
    gtk_entry_set_text(GTK_ENTRY(GTK_BIN(TrackEntryCombo)->child),  "");
    gtk_entry_set_text(GTK_ENTRY(TrackTotalEntry),                  "");
    gtk_entry_set_text(GTK_ENTRY(GTK_BIN(GenreCombo)->child),       "");
    gtk_entry_set_text(GTK_ENTRY(CommentEntry),                     "");
    //textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(CommentView));
    //gtk_text_buffer_set_text(GTK_TEXT_BUFFER(textbuffer),           "", -1);
    gtk_entry_set_text(GTK_ENTRY(ComposerEntry),                    "");
    gtk_entry_set_text(GTK_ENTRY(OrigArtistEntry),                  "");
    gtk_entry_set_text(GTK_ENTRY(CopyrightEntry),                   "");
    gtk_entry_set_text(GTK_ENTRY(URLEntry),                         "");
    gtk_entry_set_text(GTK_ENTRY(EncodedByEntry),                   "");
    PictureEntry_Clear();
}


/*
 * Clear the entry of file area
 */
void Clear_File_Entry_Field (void)
{
    if (!FileEntry) return;

    gtk_entry_set_text(GTK_ENTRY(FileEntry),"");
}


/*
 * Clear the header informations
 */
void Clear_Header_Fields (void)
{
    if (!VersionValueLabel) return;

    /* Default values are MPs data */
    gtk_label_set_text(GTK_LABEL(VersionLabel),        _("MPEG"));
    gtk_label_set_text(GTK_LABEL(VersionValueLabel),   _("?, Layer ?"));
    gtk_label_set_text(GTK_LABEL(BitrateValueLabel),   _("? kb/s"));
    gtk_label_set_text(GTK_LABEL(SampleRateValueLabel),_("? Hz"));
    gtk_label_set_text(GTK_LABEL(ModeLabel),           _("Mode:"));
    gtk_label_set_text(GTK_LABEL(ModeValueLabel),      _("?"));
    gtk_label_set_text(GTK_LABEL(SizeValueLabel),      _("?"));
    gtk_label_set_text(GTK_LABEL(DurationValueLabel),  _("?"));
}




/*
 * Load the default directory when the user interface is completely displayed
 * to avoid bad visualization effect at startup.
 */
void Init_Load_Default_Dir (void)
{
    //ETCore->ETFileList = (GList *)NULL;
    ET_Core_Free();
    ET_Core_Initialize();

    // Open the scanner window
    if (OPEN_SCANNER_WINDOW_ON_STARTUP)
        Open_ScannerWindow(SCANNER_TYPE); // Open the last selected scanner

    Statusbar_Message(_("Select a directory to browse!"),FALSE);
    if (INIT_DIRECTORY)
    {
        Browser_Tree_Select_Dir(INIT_DIRECTORY);
    }else
    {
        Browser_Load_Default_Directory();
    }

    // To set sensivity of buttons in the case if the default directory is invalid
    Update_Command_Buttons_Sensivity();

    g_source_remove(idle_handler_id);
}



void Convert_P20_And_Undescore_Into_Spaces (GtkWidget *entry)
{
    gchar *string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    Scan_Convert_Underscore_Into_Space(string);
    Scan_Convert_P20_Into_Space(string);
    gtk_entry_set_text(GTK_ENTRY(entry),string);
    g_free(string);
}

void Convert_Space_Into_Undescore (GtkWidget *entry)
{
    gchar *string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    Scan_Convert_Space_Into_Undescore(string);
    gtk_entry_set_text(GTK_ENTRY(entry),string);
    g_free(string);
}

void Convert_All_Uppercase (GtkWidget *entry)
{
    gchar *string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    Scan_Process_Fields_All_Uppercase(string);
    gtk_entry_set_text(GTK_ENTRY(entry),string);
    g_free(string);
}

void Convert_All_Downcase (GtkWidget *entry)
{
    gchar *string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    Scan_Process_Fields_All_Downcase(string);
    gtk_entry_set_text(GTK_ENTRY(entry),string);
    g_free(string);
}

void Convert_Letter_Uppercase (GtkWidget *entry)
{
    gchar *string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    Scan_Process_Fields_Letter_Uppercase(string);
    gtk_entry_set_text(GTK_ENTRY(entry),string);
    g_free(string);
}

void Convert_First_Letters_Uppercase (GtkWidget *entry)
{
    gchar *string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    Scan_Process_Fields_First_Letters_Uppercase(string);
    gtk_entry_set_text(GTK_ENTRY(entry),string);
    g_free(string);
}

void Convert_Remove_Space (GtkWidget *entry)
{
    gchar *string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    Scan_Process_Fields_Remove_Space(string);
    gtk_entry_set_text(GTK_ENTRY(entry),string);
    g_free(string);
}

void Convert_Insert_Space (GtkWidget *entry)
{
    // FIX ME : we suppose that it will not grow more than 2 times its size...
    guint string_length = 2 * strlen(gtk_entry_get_text(GTK_ENTRY(entry)));
    gchar *string       = g_malloc(string_length+1);
    strncpy(string,gtk_entry_get_text(GTK_ENTRY(entry)),string_length);
    string[string_length]='\0';

    Scan_Process_Fields_Insert_Space(&string);
    gtk_entry_set_text(GTK_ENTRY(entry),string);
    g_free(string);
}

void Convert_Only_One_Space (GtkWidget *entry)
{
    gchar *string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    Scan_Process_Fields_Keep_One_Space(string);
    gtk_entry_set_text(GTK_ENTRY(entry),string);
    g_free(string);
}

/*
 * Entry_Popup_Menu_Handler: show the popup menu when the third mouse button is pressed.
 */
gboolean Entry_Popup_Menu_Handler (GtkMenu *menu, GdkEventButton *event)
{
    if (event && (event->type==GDK_BUTTON_PRESS) && (event->button==3))
    {
        /* FIX ME : this is not very clean, but if we use 'event->button' (contains value of
         * the 3rd button) instead of '1', we need to click two times the left mouse button
         * to activate an item of the opened popup menu (when menu is attached to an entry). */
        //gtk_menu_popup(menu,NULL,NULL,NULL,NULL,event->button,event->time);
        gtk_menu_popup(menu,NULL,NULL,NULL,NULL,1,event->time);
        return TRUE;
    }
    return FALSE;
}

/*
 * Popup menu attached to all entries of tag + filename + rename combobox.
 * Displayed when pressing the right mouse button and contains functions to process ths strings.
 */
void Attach_Popup_Menu_To_Tag_Entries (GtkEntry *entry)
{
    GtkWidget *PopupMenu;
    GtkWidget *Image;
    GtkWidget *MenuItem;


    PopupMenu = gtk_menu_new();
    g_signal_connect_swapped(G_OBJECT(entry),"button_press_event",
        G_CALLBACK(Entry_Popup_Menu_Handler),G_OBJECT(PopupMenu));

    /* Menu items */
    MenuItem = gtk_image_menu_item_new_with_label(_("Tag selected files with this field"));
    Image = gtk_image_new_from_stock(GTK_STOCK_JUMP_TO,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
                             G_CALLBACK(Menu_Mini_Button_Clicked),G_OBJECT(entry));

    /* Separator */
    MenuItem = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);

    MenuItem = gtk_image_menu_item_new_with_label(_("Convert '_' and '%20' to spaces"));
    Image = gtk_image_new_from_stock(GTK_STOCK_CONVERT,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
        G_CALLBACK(Convert_P20_And_Undescore_Into_Spaces),G_OBJECT(entry));

    MenuItem = gtk_image_menu_item_new_with_label(_("Convert ' ' to '_'"));
    Image = gtk_image_new_from_stock(GTK_STOCK_CONVERT,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
        G_CALLBACK(Convert_Space_Into_Undescore),G_OBJECT(entry));

    /* Separator */
    MenuItem = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);

    MenuItem = gtk_image_menu_item_new_with_label(_("All uppercase"));
    Image = gtk_image_new_from_stock("easytag-all-uppercase",GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
        G_CALLBACK(Convert_All_Uppercase),G_OBJECT(entry));

    MenuItem = gtk_image_menu_item_new_with_label(_("All downcase"));
    Image = gtk_image_new_from_stock("easytag-all-downcase",GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
        G_CALLBACK(Convert_All_Downcase),G_OBJECT(entry));

    MenuItem = gtk_image_menu_item_new_with_label(_("First letter uppercase"));
    Image = gtk_image_new_from_stock("easytag-first-letter-uppercase",GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
        G_CALLBACK(Convert_Letter_Uppercase),G_OBJECT(entry));

    MenuItem = gtk_image_menu_item_new_with_label(_("First letter uppercase of each word"));
    Image = gtk_image_new_from_stock("easytag-first-letter-uppercase-word",GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
        G_CALLBACK(Convert_First_Letters_Uppercase),G_OBJECT(entry));

    /* Separator */
    MenuItem = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);

    MenuItem = gtk_image_menu_item_new_with_label(_("Remove spaces"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
        G_CALLBACK(Convert_Remove_Space),G_OBJECT(entry));

    MenuItem = gtk_image_menu_item_new_with_label(_("Insert space before uppercase letter"));
    Image = gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
        G_CALLBACK(Convert_Insert_Space),G_OBJECT(entry));

    MenuItem = gtk_image_menu_item_new_with_label(_("Remove duplicate spaces or underscores"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(PopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate",
        G_CALLBACK(Convert_Only_One_Space),G_OBJECT(entry));

    gtk_widget_show_all(PopupMenu);
}



/*
 * Function to manage the received signals (specially for segfaults)
 * Handle crashs
 */
void Handle_Crash (gint signal_id)
{
    //gchar commmand[256];

    Log_Print(LOG_ERROR,_("EasyTAG %s: Abnormal exit! (PId: %d)."),VERSION,getpid());
    Log_Print(LOG_ERROR,_("Received signal %s (%d)\a"),signal_to_string(signal_id),signal_id);

    Log_Print(LOG_ERROR,_("You have probably found a bug in EasyTAG. Please, send a bug "
              "report with a gdb backtrace ('gdb easytag core' then 'bt' and "
              "'l') and informations to reproduce it to easytag@gmail.com"));

    // To send messages to the console...
    g_print(_("EasyTAG %s: Abnormal exit! (PId: %d)."),VERSION,getpid());
    g_print("\n");
    g_print(_("Received signal %s (%d)\a"),signal_to_string(signal_id),signal_id);
    g_print("\n");
    g_print(_("You have probably found a bug in EasyTAG. Please, send a bug "
            "report with a gdb backtrace ('gdb easytag core' then 'bt' and "
            "'l') and informations to reproduce it to easytag@gmail.com"));
    g_print("\n");

    signal(signal_id,SIG_DFL); // Let the OS handle recursive seg faults
    //signal(SIGTSTP, exit);
    //snprintf(commmand,sizeof(commmand),"gdb -x /root/core.txt easytag %d", getpid());
    //system(commmand);
}

gchar *signal_to_string (gint signal)
{
#ifdef SIGHUP
    if (signal == SIGHUP)     return ("SIGHUP");
#endif
#ifdef SIGINT
    if (signal == SIGINT)     return ("SIGINT");
#endif
#ifdef SIGQUIT
    if (signal == SIGQUIT)    return ("SIGQUIT");
#endif
#ifdef SIGILL
    if (signal == SIGILL)     return ("SIGILL");
#endif
#ifdef SIGTRAP
    if (signal == SIGTRAP)    return ("SIGTRAP");
#endif
#ifdef SIGABRT
    if (signal == SIGABRT)    return ("SIGABRT");
#endif
#ifdef SIGIOT
    if (signal == SIGIOT)     return ("SIGIOT");
#endif
#ifdef SIGEMT
    if (signal == SIGEMT)     return ("SIGEMT");
#endif
#ifdef SIGFPE
    if (signal == SIGFPE)     return ("SIGFPE");
#endif
#ifdef SIGKILL
    if (signal == SIGKILL)    return ("SIGKILL");
#endif
#ifdef SIGBUS
    if (signal == SIGBUS)     return ("SIGBUS");
#endif
#ifdef SIGSEGV
    if (signal == SIGSEGV)    return ("SIGSEGV");
#endif
#ifdef SIGSYS
    if (signal == SIGSYS)     return ("SIGSYS");
#endif
#ifdef SIGPIPE
    if (signal == SIGPIPE)    return ("SIGPIPE");
#endif
#ifdef SIGALRM
    if (signal == SIGALRM)    return ("SIGALRM");
#endif
#ifdef SIGTERM
    if (signal == SIGTERM)    return ("SIGTERM");
#endif
#ifdef SIGUSR1
    if (signal == SIGUSR1)    return ("SIGUSR1");
#endif
#ifdef SIGUSR2
    if (signal == SIGUSR2)    return ("SIGUSR2");
#endif
#ifdef SIGCHLD
    if (signal == SIGCHLD)    return ("SIGCHLD");
#endif
#ifdef SIGCLD
    if (signal == SIGCLD)     return ("SIGCLD");
#endif
#ifdef SIGPWR
    if (signal == SIGPWR)     return ("SIGPWR");
#endif
#ifdef SIGVTALRM
    if (signal == SIGVTALRM)  return ("SIGVTALRM");
#endif
#ifdef SIGPROF
    if (signal == SIGPROF)    return ("SIGPROF");
#endif
#ifdef SIGIO
    if (signal == SIGIO)      return ("SIGIO");
#endif
#ifdef SIGPOLL
    if (signal == SIGPOLL)    return ("SIGPOLL");
#endif
#ifdef SIGWINCH
    if (signal == SIGWINCH)   return ("SIGWINCH");
#endif
#ifdef SIGWINDOW
    if (signal == SIGWINDOW)  return ("SIGWINDOW");
#endif
#ifdef SIGSTOP
    if (signal == SIGSTOP)    return ("SIGSTOP");
#endif
#ifdef SIGTSTP
    if (signal == SIGTSTP)    return ("SIGTSTP");
#endif
#ifdef SIGCONT
    if (signal == SIGCONT)    return ("SIGCONT");
#endif
#ifdef SIGTTIN
    if (signal == SIGTTIN)    return ("SIGTTIN");
#endif
#ifdef SIGTTOU
    if (signal == SIGTTOU)    return ("SIGTTOU");
#endif
#ifdef SIGURG
    if (signal == SIGURG)     return ("SIGURG");
#endif
#ifdef SIGLOST
    if (signal == SIGLOST)    return ("SIGLOST");
#endif
#ifdef SIGRESERVE
    if (signal == SIGRESERVE) return ("SIGRESERVE");
#endif
#ifdef SIGDIL
    if (signal == SIGDIL)     return ("SIGDIL");
#endif
#ifdef SIGXCPU
    if (signal == SIGXCPU)    return ("SIGXCPU");
#endif
#ifdef SIGXFSZ
    if (signal == SIGXFSZ)    return ("SIGXFSZ");
#endif
    return (_("Unknown signal"));
}


/*
 * Display usage informations
 */
void Display_Usage (void)
{
    // Fix from Steve Ralston for gcc-3.2.2
#ifdef WIN32
    #define xPREFIX "c:"
#else
    #define xPREFIX ""
#endif

    g_print(_("\nUsage: easytag [option] "
              "\n   or: easytag [directory]\n"
              "\n"
              "Option:\n"
              "-------\n"
              "-h, --help        Display this text and exit.\n"
              "-v, --version     Print basic informations and exit.\n"
              "\n"
              "Directory:\n"
              "----------\n"
              "%s/path_to/files  Use an absolute path to load,\n"
              "path_to/files     Use a relative path.\n"
              "\n"),xPREFIX);

    #undef xPREFIX

    exit(0);
}



/*
 * Exit the program
 */
void EasyTAG_Exit (void)
{
    ET_Core_Destroy();
    Charset_Insert_Locales_Destroy();
    Log_Print(LOG_OK,_("EasyTAG: Normal exit."));
    gtk_main_quit();
#ifdef WIN32
    weasytag_cleanup();
#endif
    exit(0);
}

void Quit_MainWindow_Confirmed (void)
{
    // Save the configuration when exiting...
    Save_Changes_Of_UI();
    
    // Quit EasyTAG
    EasyTAG_Exit();
}

void Quit_MainWindow_Save_And_Quit (void)
{
    /* Save modified tags */
    if (Save_All_Files_With_Answer(FALSE) == -1)
        return;
    Quit_MainWindow_Confirmed();
}

void Quit_MainWindow (void)
{
    GtkWidget *msgbox;
    gint response;

    /* If you change the displayed data and quit immediately */
    if (ETCore->ETFileList)
        ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed); // To detect change before exiting

    /* Save combobox history list before exit */
    Save_Path_Entry_List(BrowserEntryModel, MISC_COMBO_TEXT);

    /* Exit ? */
    if (ET_Check_If_All_Files_Are_Saved() != TRUE)
    {
        /* Some files haven't been saved */
        msgbox = msg_box_new(_("Confirm..."),
                             GTK_WINDOW(MainWindow),
                             NULL,
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             _("Some files have been modified but not saved...\nDo you want to save them before exiting the program?"),
                             GTK_STOCK_DIALOG_QUESTION,
                             GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
                             GTK_STOCK_NO,    GTK_RESPONSE_NO,
                             GTK_STOCK_YES,   GTK_RESPONSE_YES,
                             NULL);
        response = gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);
        switch (response)
        {
            case GTK_RESPONSE_YES:
                Quit_MainWindow_Save_And_Quit();
                break;
            case GTK_RESPONSE_NO:
                Quit_MainWindow_Confirmed();
                break;
            case GTK_RESPONSE_CANCEL:
            case GTK_RESPONSE_NONE:
                return;
        }

    } else if (CONFIRM_BEFORE_EXIT)
    {
         msgbox = msg_box_new(_("Confirm..."),
                             GTK_WINDOW(MainWindow),
                             NULL,
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             _(" Do you really want to exit the program? "),
                             GTK_STOCK_DIALOG_QUESTION,
                             GTK_STOCK_NO,    GTK_RESPONSE_NO,
                             GTK_STOCK_YES,   GTK_RESPONSE_YES,
                             NULL);
        response = gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);
        switch (response)
        {
            case GTK_RESPONSE_YES:
                Quit_MainWindow_Confirmed();
                break;
            case GTK_RESPONSE_NO:
            case GTK_RESPONSE_NONE:
                return;
                break;
        }
    }else
    {
        Quit_MainWindow_Confirmed();
    }

}

/*
 * For the configuration file...
 */
void MainWindow_Apply_Changes (void)
{
    if ( MainWindow && MainWindow->window && gdk_window_is_visible(MainWindow->window)
    &&   gdk_window_get_state(MainWindow->window)!=GDK_WINDOW_STATE_MAXIMIZED )
    {
        gint x, y, width, height;

        // Position and Origin of the window
        gdk_window_get_root_origin(MainWindow->window,&x,&y);
        MAIN_WINDOW_X = x;
        MAIN_WINDOW_Y = y;
        gdk_window_get_size(MainWindow->window,&width,&height);
        MAIN_WINDOW_WIDTH  = width;
        MAIN_WINDOW_HEIGHT = height;

        // Handle panes position
        PANE_HANDLE_POSITION1 = GTK_PANED(MainWindowHPaned)->child1_size;
        PANE_HANDLE_POSITION2 = GTK_PANED(BrowserHPaned)->child1_size;
        PANE_HANDLE_POSITION3 = GTK_PANED(ArtistAlbumVPaned)->child1_size;
        PANE_HANDLE_POSITION4 = GTK_PANED(MainWindowVPaned)->child1_size;
    }

}
