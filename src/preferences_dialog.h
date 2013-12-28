/*
 * EasyTAG - Tag editor for audio files
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 * Copyright (C) 2013  David King <amigadave@amigadave.com>
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
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ET_PREFERENCES_DIALOG_H_
#define ET_PREFERENCES_DIALOG_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ET_TYPE_PREFERENCES_DIALOG (et_preferences_dialog_get_type ())
#define ET_PREFERENCES_DIALOG(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_PREFERENCES_DIALOG, EtPreferencesDialog))

typedef struct _EtPreferencesDialog EtPreferencesDialog;
typedef struct _EtPreferencesDialogClass EtPreferencesDialogClass;
typedef struct _EtPreferencesDialogPrivate EtPreferencesDialogPrivate;

struct _EtPreferencesDialog
{
    /*< private >*/
    GtkDialog parent_instance;
    EtPreferencesDialogPrivate *priv;
};

struct _EtPreferencesDialogClass
{
    /*< private >*/
    GtkDialogClass parent_class;
};

GType et_preferences_dialog_get_type (void);
EtPreferencesDialog *et_preferences_dialog_new (void);
void et_preferences_dialog_apply_changes (EtPreferencesDialog *self);
void et_preferences_dialog_show_scanner (EtPreferencesDialog *self);

G_END_DECLS

/* FIXME: Remove widget declarations when switching to GSettings. */
/* Widgets included in config */
/* Common */
GtkWidget *LoadOnStartup;
GtkWidget *DefaultPathToMp3;
GtkWidget *BrowserLineStyleOptionMenu;
GtkWidget *BrowserExpanderStyleOptionMenu;
GtkWidget *BrowseSubdir;
GtkWidget *BrowseHiddendir;
GtkWidget *OpenSelectedBrowserNode;

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
GtkWidget *FileWritingId3v2UseCrc32;
GtkWidget *FileWritingId3v2UseCompression;
GtkWidget *FileWritingId3v2TextOnlyGenre;
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
GtkWidget *pad_disc_number;
GtkWidget *pad_disc_number_spinbutton;
GtkWidget *SetFocusToSameTagField;
GtkWidget *SetFocusToFirstTagField;
GtkWidget *LogMaxLinesSpinButton;
GtkWidget *ShowLogView;

GtkWidget *VorbisSplitFieldTitle;
GtkWidget *VorbisSplitFieldArtist;
GtkWidget *VorbisSplitFieldAlbum;
GtkWidget *VorbisSplitFieldGenre;
GtkWidget *VorbisSplitFieldComment;
GtkWidget *VorbisSplitFieldComposer;
GtkWidget *VorbisSplitFieldOrigArtist;

/* Scanner */
GtkWidget *FTSConvertUnderscoreAndP20IntoSpace;
GtkWidget *FTSConvertSpaceIntoUnderscore;
GtkWidget *RFSConvertUnderscoreAndP20IntoSpace;
GtkWidget *RFSConvertSpaceIntoUnderscore;
GtkWidget *RFSRemoveSpaces;
GtkWidget *PFSDontUpperSomeWords;
GtkWidget *OverwriteTagField;
GtkWidget *OpenScannerWindowOnStartup;

GtkWidget *SetDefaultComment;
GtkWidget *DefaultComment;
GtkWidget *Crc32Comment;

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
GtkWidget *ConfirmWhenUnsavedFiles;

#endif /* ET_PREFERENCES_DIALOG_H_ */
