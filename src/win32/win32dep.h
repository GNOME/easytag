/*
 * easytag
 *
 * File: win32dep.h
 *
 * Copyright (C) 2002-2003, Herman Bloggs <hermanator12002@yahoo.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef _WIN32DEP_H_
#define _WIN32DEP_H_


#include <winsock2.h>
#include <process.h>
#include <gtk/gtk.h>
#include <gdk/gdkevents.h>

#define lstat stat
#define mkdir(a,b) mkdir(a)
#define chown(a,b,c)
#define chmod(a,b)

//#define strcasestr(haystack, needle) strstr(haystack, needle)
char *strcasestr(const char *s1, const char *s2);



/*
 *  PROTOS
 */

/**
 ** win32dep.c
 **/
/* Windows helper functions */
extern int       mkstemp(char *template);

extern HINSTANCE ET_Win32_Hinstance       (void);
extern gboolean  ET_Win32_Read_Reg_String (HKEY key, char* sub_key, char* val_name, LPBYTE data, LPDWORD data_len);
extern char*     ET_Win32_Escape_Dirsep   (char*);

/* Determine ET paths */
extern char*     ET_Win32_Install_Dir (void);
extern char*     ET_Win32_Locale_Dir  (void);
extern char*     ET_Win32_Data_Dir    (void);

/* Misc */
extern char *    ET_Win32_Get_Audio_File_Player (void);
extern void      ET_Win32_Notify_Uri (const char *uri);

/* init / cleanup */
extern void      ET_Win32_Init    (HINSTANCE);
extern void      ET_Win32_Cleanup (void);

extern void      ET_Win32_Path_Remove_Trailing_Slash     (gchar *path);
extern void      ET_Win32_Path_Remove_Trailing_Backslash (gchar *path);
extern void      ET_Win32_Path_Replace_Backslashes       (gchar *path);
extern void      ET_Win32_Path_Replace_Slashes           (gchar *path);

/*
 *  MACROS
 */

/*
 *  ET specific
 */
#define DATADIR ET_Win32_Install_Dir()
#define LOCALE ET_Win32_Locale_Dir()

#endif /* _WIN32DEP_H_ */
