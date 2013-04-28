/* easytag.h - 2000/04/28 */
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef __EASYTAG_H__
#define __EASYTAG_H__


/* 'include' and 'define' created by autoconf/automake */
#include "config.h"

#include "et_core.h"


#define MAX_STRING_LEN     1024


/***************
 * Declaration *
 ***************/

/* Variable to force to quit recursive functions (reading dirs) or stop saving files */
gboolean Main_Stop_Button_Pressed;

GtkWidget *MainWindow;
GtkWidget *MenuArea;
GtkWidget *ToolArea;
GtkWidget *BrowseArea;
GtkWidget *FileArea;
GtkWidget *TagArea;
GtkWidget *StatusArea;
GtkWidget *ProgressArea;
GtkWidget *LogArea;
GtkWidget *MainWindowHPaned;
GtkWidget *MainWindowVPaned;

/* File Area */
GtkWidget *FileFrame;
GtkWidget *FileIndex;
GtkWidget *FileEntry;
GtkWidget *HeaderInfosTable;    /* declarated here to show/hide file header infos */
GtkWidget *VersionLabel;
GtkWidget *VersionValueLabel;
GtkWidget *BitrateLabel;
GtkWidget *BitrateValueLabel;
GtkWidget *SampleRateLabel;
GtkWidget *SampleRateValueLabel;
GtkWidget *ModeLabel;
GtkWidget *ModeValueLabel;
GtkWidget *SizeLabel;
GtkWidget *SizeValueLabel;
GtkWidget *DurationLabel;
GtkWidget *DurationValueLabel;

/* TAG Area */
GtkWidget    *TagFrame;
GtkWidget    *TagNoteBook;
GtkWidget    *TitleEntry;
GtkWidget    *ArtistEntry;
GtkWidget    *AlbumArtistEntry;
GtkWidget    *AlbumEntry;
GtkWidget    *DiscNumberEntry;
GtkWidget    *YearEntry;
GtkWidget    *TrackEntryCombo;
GtkListStore *TrackEntryComboModel;
GtkWidget    *TrackTotalEntry;
GtkWidget    *GenreCombo;
GtkListStore *GenreComboModel;
GtkWidget    *CommentEntry;
//GtkWidget    *CommentView;
GtkWidget    *ComposerEntry;
GtkWidget    *OrigArtistEntry;
GtkWidget    *CopyrightEntry;
GtkWidget    *URLEntry;
GtkWidget    *EncodedByEntry;
GtkWidget    *PictureEntryView;
GtkListStore *PictureEntryModel;
// Labels
GtkWidget    *TitleLabel;
GtkWidget    *ArtistLabel;
GtkWidget    *AlbumArtistLabel;
GtkWidget    *AlbumLabel;
GtkWidget    *DiscNumberLabel;
GtkWidget    *YearLabel;
GtkWidget    *TrackLabel;
GtkWidget    *GenreLabel;
GtkWidget    *CommentLabel;
GtkWidget    *ComposerLabel;
GtkWidget    *OrigArtistLabel;
GtkWidget    *CopyrightLabel;
GtkWidget    *URLLabel;
GtkWidget    *EncodedByLabel;
// Mini buttons
GtkWidget    *TrackMButtonSequence;
GtkWidget    *TrackMButtonNbrFiles;

/* Other for picture. */
GtkToolItem *remove_image_toolitem;
GtkToolItem *add_image_toolitem;
GtkToolItem *save_image_toolitem;
GtkToolItem *image_properties_toolitem;
GtkToolItem *apply_image_toolitem;
GtkWidget *PictureScrollWindow;

GdkCursor *MouseCursor;

gchar *INIT_DIRECTORY;

#ifndef errno
extern int errno;
#endif

/* A flag to start/avoid a new reading while another one is running */
gboolean ReadingDirectory;


/**************
 * Prototypes *
 **************/
void et_on_action_select_all (void);
void Action_Unselect_All_Files          (void);
void Action_Invert_Files_Selection      (void);
void Action_Select_Prev_File            (void);
void Action_Select_Next_File            (void);
void Action_Select_First_File           (void);
void Action_Select_Last_File            (void);
void Action_Select_Nth_File_By_Position (gulong num_item);
void Action_Select_Nth_File_By_Etfile   (ET_File *ETFile);

void Action_Scan_Selected_Files         (void);
void Action_Remove_Selected_Tags        (void);
gint Action_Undo_Selected_Files         (void);
gint Action_Redo_Selected_File          (void);
void Action_Undo_All_Files              (void);
void Action_Redo_All_Files              (void);
void Action_Save_Selected_Files         (void);
void Action_Force_Saving_Selected_Files (void);
void Action_Undo_From_History_List      (void);
void Action_Redo_From_History_List      (void);
void Action_Delete_Selected_Files       (void);
gint Save_All_Files_With_Answer         (gboolean force_saving_files);

void Action_Main_Stop_Button_Pressed    (void);
void Action_Select_Browser_Style        (void);
void et_on_action_select_scan_mode (GtkRadioAction *action,
                                    GtkRadioAction *current,
                                    gpointer user_data);

void Tag_Area_Display_Controls (ET_File *ETFile);

gboolean Read_Directory               (gchar *path);
void Quit_MainWindow                  (void);
void MainWindow_Apply_Changes         (void);
void Update_Command_Buttons_Sensivity (void);
void Attach_Popup_Menu_To_Tag_Entries (GtkEntry *entry);

void Clear_File_Entry_Field (void);
void Clear_Tag_Entry_Fields (void);
void Clear_Header_Fields    (void);


#endif /* __EASYTAG_H__ */
