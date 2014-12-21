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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#ifndef _WIN32DEP_H_
#define _WIN32DEP_H_

#include <glib.h> /* Needed for G_OS_WIN32. */

#ifdef G_OS_WIN32
#include <winsock2.h>
#include <shlobj.h>
#include <process.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define lstat stat
#define mkdir(a,b) mkdir(a)
#define chown(a,b,c) 0
#define chmod(a,b)


/*
 *  PROTOS
 */

/**
 ** win32dep.c
 **/
/* Windows helper functions */

/* Determine EasyTAG paths */
const gchar * weasytag_install_dir (void);
const gchar * weasytag_locale_dir (void);

/* Misc */
extern char *ET_Win32_Get_Audio_File_Player (void);

extern void  ET_Win32_Path_Remove_Trailing_Slash     (gchar *path);
extern void  ET_Win32_Path_Remove_Trailing_Backslash (gchar *path);
extern void  ET_Win32_Path_Replace_Backslashes       (gchar *path);
extern void  ET_Win32_Path_Replace_Slashes           (gchar *path);

#ifndef HAVE_MKSTEMP
#define et_w32_mkstemp mkstemp
extern gint et_w32_mkstemp (char *template);
#endif /* HAVE_MKSTEMP */

#ifndef HAVE_TRUNCATE
#define et_w32_truncate truncate
extern gint et_w32_truncate (const gchar *path, off_t length);
#endif /* !HAVE_TRUNCATE */

/*
 *  MACROS
 */

/*
 *  EasyTAG specific
 */
#undef DATADIR
#undef LOCALEDIR
#define DATADIR weasytag_install_dir()
#define LOCALEDIR weasytag_locale_dir()

G_END_DECLS

#endif /* G_OS_WIN32 */

#endif /* _WIN32DEP_H_ */
