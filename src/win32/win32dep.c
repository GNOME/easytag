/*
 * easytag
 *
 * File: win32dep.c
 * Date: June, 2002
 * Description: Windows dependant code for Easytag
 * this code if largely taken from win32 Gaim and Purple
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

/* Needed for G_OS_WIN32. */
#include <glib.h>

#ifdef G_OS_WIN32

#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <winuser.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timeb.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <gdk/gdkwin32.h>

#include "resource.h"
//#include "../log.h"

#include <libintl.h>

#include "win32dep.h"

/*
 *  DEFINES & MACROS
 */
#define _(x) gettext(x)

/*
 * LOCALS
 */
static char *install_dir = NULL, *locale_dir = NULL;

/* Prototypes. */
void str_replace_char (gchar *str, gchar in_char, gchar out_char);


/*
 *  PUBLIC CODE
 */

/* Determine Easytag Paths during Runtime */
const char *weasytag_install_dir(void) {
	static gboolean initialized = FALSE;

	if (!initialized) {
		char *tmp = NULL;
		if (G_WIN32_HAVE_WIDECHAR_API()) {
			wchar_t winstall_dir[MAXPATHLEN];
			if (GetModuleFileNameW(NULL, winstall_dir,
					MAXPATHLEN) > 0) {
				tmp = g_utf16_to_utf8(winstall_dir, -1,
					NULL, NULL, NULL);
			}
		} else {
			gchar cpinstall_dir[MAXPATHLEN];
			if (GetModuleFileNameA(NULL, cpinstall_dir,
					MAXPATHLEN) > 0) {
				tmp = g_locale_to_utf8(cpinstall_dir,
					-1, NULL, NULL, NULL);
			}
		}

		if (tmp == NULL) {
			tmp = g_win32_error_message(GetLastError());
            //Log_Print("GetModuleFileName error: %s", tmp);
            g_print("GetModuleFileName error: %s", tmp);
            g_print("\n");
			g_free(tmp);
			return NULL;
		} else {
			install_dir = g_path_get_dirname(tmp);
			g_free(tmp);
			initialized = TRUE;
		}
	}

	return install_dir;
}

const char *weasytag_locale_dir(void) {
	static gboolean initialized = FALSE;

	if (!initialized) {
		const char *inst_dir = weasytag_install_dir();
		if (inst_dir != NULL) {
			locale_dir = g_build_filename (inst_dir, "..", "lib", "locale", NULL);
			initialized = TRUE;
		} else {
			return NULL;
		}
	}

	return locale_dir;
}

/* Miscellaneous */
/* emulate the unix function */
gint
et_w32_mkstemp (gchar *template)
{
    int fd = -1;

    char *str = mktemp(template);
    if(str != NULL)
    {
        fd  = open(str, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    }

    return fd;
}

void
str_replace_char (gchar *str, gchar in_char, gchar out_char)
{
    while(*str)
    {
        if(*str == in_char)
            *str = out_char;
        str++;
    }
}

/* Remove trailing '/' if any */
void ET_Win32_Path_Remove_Trailing_Slash (gchar *path)
{
    int path_len = strlen(path);
  
    if(path_len > 3 && path[path_len - 1] == '/')
    {
        path[path_len - 1] = '\0';
    }
}

/* Remove trailing '\' if any, but not when 'C:\' */
void ET_Win32_Path_Remove_Trailing_Backslash (gchar *path)
{
    int path_len = strlen(path);
  
    if(path_len > 3 && path[path_len - 1] == '\\')
    {
        path[path_len - 1] = '\0';
    }
}

void ET_Win32_Path_Replace_Backslashes (gchar *path)
{
    str_replace_char(path, '\\', '/');
}

void ET_Win32_Path_Replace_Slashes (gchar *path)
{
    str_replace_char(path, '/', '\\');
}

/* find a default player executable */
gchar *
ET_Win32_Get_Audio_File_Player (void)
{
    return g_strdup("");
}

gint
et_w32_truncate (const gchar *path, off_t length)
{
    HANDLE h;
    gint ret;

    h = CreateFile (path, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

    if (h == INVALID_HANDLE_VALUE)
    {
        /* errno = map_errno (GetLastError ()); */
        return -1;
    }

    ret = chsize ((gint)h, length);
    CloseHandle (h);

    return ret;
}

gint
et_w32_ftruncate (gint fd, off_t length)
{
    HANDLE h;

    h = (HANDLE)_get_osfhandle (fd);

    if (h == (HANDLE) - 1) return -1;

    return chsize ((gint)h, length);
}

#endif /* G_OS_WIN32 */
