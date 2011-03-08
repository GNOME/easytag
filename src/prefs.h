/* prefs.h - 2000/05/06 */
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


#ifndef __PREFS_H__
#define __PREFS_H__


/***************
 * Declaration *
 ***************/

GtkWidget *OptionsWindow;
GtkWidget *OptionsNoteBook;
gint OptionsNoteBook_Scanner_Page_Num;    /* Contains the number of the page "Scanner Option" */


/* Widgets included in config */
/* Common */
GtkWidget *LoadOnStartup;
GtkWidget *DefaultPathToMp3;
GtkWidget *BrowserLineStyleOptionMenu;
GtkWidget *BrowserExpanderStyleOptionMenu;
GtkWidget *BrowseSubdir;
GtkWidget *BrowseHiddendir;
GtkWidget *OpenSelectedBrowserNode;

GtkListStore *DefaultPathModel;

/* User interface */
GtkWidget *ShowHeaderInfos;
GtkWidget *ChangedFilesDisplayedToRed;
GtkWidget *ChangedFilesDisplayedToBold;

/* Misc */
GtkWidget *SortingFileCombo;
GtkWidget *SortingFileCaseSensitive;

GtkWidget *MessageBoxPositionNone;
GtkWidget *MessageBoxPositionCenter;
GtkWidget *MessageBoxPositionMouse;
GtkWidget *MessageBoxPositionCenterOnParent;

GtkWidget *FilePlayerCombo;
GtkListStore *FilePlayerModel;

/* File Settings */
GtkWidget *ReplaceIllegalCharactersInFilename;
GtkWidget *FilenameExtensionNoChange;
GtkWidget *FilenameExtensionLowerCase;
GtkWidget *FilenameExtensionUpperCase;
GtkWidget *PreserveModificationTime;
GtkWidget *UpdateParentDirectoryModificationTime;

GtkWidget *FilenameCharacterSetOther;
GtkWidget *FilenameCharacterSetApproximate;
GtkWidget *FilenameCharacterSetDiscard;

/* Tag Settings */
GtkWidget *FileWritingId3v2WriteTag;
GtkWidget *FileWritingId3v1WriteTag;
GtkWidget *WriteId3TagsInFlacFiles;
GtkWidget *FileWritingId3v2UseCrc32;
GtkWidget *FileWritingId3v2UseCompression;
GtkWidget *StripTagWhenEmptyFields;
GtkWidget *ConvertOldId3v2TagVersion;

GtkWidget *FileWritingId3v2VersionCombo;
GtkWidget *FileWritingId3v2UnicodeCharacterSetCombo;
GtkWidget *FileWritingId3v2NoUnicodeCharacterSetCombo;
GtkWidget *FileWritingId3v1CharacterSetCombo;
GtkWidget *FileWritingId3v2UseUnicodeCharacterSet;
GtkWidget *FileWritingId3v2UseNoUnicodeCharacterSet;
GtkWidget *UseNonStandardId3ReadingCharacterSet;
GtkWidget *FileReadingId3v1v2CharacterSetCombo;

GtkWidget *FileReadingId3v1v2CharacterSetCombo;
GtkWidget *FileWritingId3v2IconvOptionsNo;
GtkWidget *FileWritingId3v2IconvOptionsTranslit;
GtkWidget *FileWritingId3v2IconvOptionsIgnore;
GtkWidget *FileWritingId3v1IconvOptionsNo;
GtkWidget *FileWritingId3v1IconvOptionsTranslit;
GtkWidget *FileWritingId3v1IconvOptionsIgnore;

GtkWidget *LabelAdditionalId3v1IconvOptions;
GtkWidget *LabelAdditionalId3v2IconvOptions;
GtkWidget *LabelId3v2Charset;
GtkWidget *LabelId3v1Charset;
GtkWidget *LabelId3v2Version;

GtkWidget *DateAutoCompletion;
GtkWidget *NumberTrackFormated;
GtkWidget *NumberTrackFormatedSpinButton;
GtkWidget *OggTagWriteXmmsComment;
GtkWidget *SetFocusToSameTagField;
GtkWidget *SetFocusToFirstTagField;
GtkWidget *LogMaxLinesSpinButton;
GtkWidget *ShowLogView;


/* Scanner */
GtkWidget *FTSConvertUnderscoreAndP20IntoSpace;
GtkWidget *FTSConvertSpaceIntoUnderscore;
GtkWidget *RFSConvertUnderscoreAndP20IntoSpace;
GtkWidget *RFSConvertSpaceIntoUnderscore;
GtkWidget *PFSDontUpperSomeWords;
GtkWidget *OverwriteTagField;
GtkWidget *OpenScannerWindowOnStartup;
GtkWidget *ScannerWindowOnTop;

GtkWidget *SetDefaultComment;
GtkWidget *DefaultComment;
GtkWidget *Crc32Comment;
GtkListStore *DefaultCommentModel;

/* CDDB */
GtkWidget *CddbServerNameAutomaticSearch;
GtkWidget *CddbServerPortAutomaticSearch;
GtkWidget *CddbServerCgiPathAutomaticSearch;
GtkWidget *CddbServerNameAutomaticSearch2;
GtkWidget *CddbServerPortAutomaticSearch2;
GtkWidget *CddbServerCgiPathAutomaticSearch2;
GtkWidget *CddbServerNameManualSearch;
GtkWidget *CddbServerPortManualSearch;
GtkWidget *CddbServerCgiPathManualSearch;
GtkWidget *CddbUseProxy;
GtkWidget *CddbProxyName;
GtkWidget *CddbProxyPort;
GtkWidget *CddbProxyUserName;
GtkWidget *CddbProxyUserPassword;

GtkWidget *CddbLocalPath;
GtkListStore *CddbLocalPathModel;

GtkWidget *SetCddbWindowSize;
GtkWidget *CddbWindowWidth;
GtkWidget *CddbWindowHeight;
GtkWidget *CddbWindowButton;
GtkWidget *SetCddbPaneHandlePosition;
GtkWidget *CddbPaneHandlePosition;
GtkWidget *CddbPaneHandleButton;

GtkWidget *CddbFollowFile;
GtkWidget *CddbUseDLM;  // Also used in the cddb.c


/* Confirmation */
GtkWidget *ConfirmBeforeExit;
GtkWidget *ConfirmWriteTag;
GtkWidget *ConfirmRenameFile;
GtkWidget *ConfirmDeleteFile;
GtkWidget *ConfirmWritePlayList;



/**************
 * Prototypes *
 **************/

void Init_OptionsWindow (void);
void Open_OptionsWindow (void);
void OptionsWindow_Apply_Changes (void);

void File_Selection_Window_For_File      (GtkWidget *widget);
void File_Selection_Window_For_Directory (GtkWidget *widget);


#endif /* __PREFS_H__ */
