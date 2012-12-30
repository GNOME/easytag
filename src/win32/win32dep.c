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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
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
 * DATA STRUCTS
 */

/* For shfolder.dll */
typedef HRESULT (CALLBACK* LPFNSHGETFOLDERPATHA)(HWND, int, HANDLE, DWORD, LPSTR);
typedef HRESULT (CALLBACK* LPFNSHGETFOLDERPATHW)(HWND, int, HANDLE, DWORD, LPWSTR);

// Defined also in "#include <shlobj.h>"
typedef enum
{
    SHGFP_TYPE_CURRENT  = 0,   // current value for user, verify it exists
    SHGFP_TYPE_DEFAULT  = 1,   // default value, may not exist
} SHGFP_TYPE;

/*
 * LOCALS
 */
static char *app_data_dir = NULL, *install_dir = NULL,
	*lib_dir = NULL, *locale_dir = NULL;

static HINSTANCE libeasytagdll_hInstance = 0;



/*
 *  PUBLIC CODE
 */

/* Escape windows dir separators.  This is needed when paths are saved,
   and on being read back have their '\' chars used as an escape char.
   Returns an allocated string which needs to be freed.
*/
char *weasytag_escape_dirsep(const char *filename) {
	int sepcount = 0;
	const char *tmp = filename;
	char *ret;
	int cnt = 0;

	g_return_val_if_fail(filename != NULL, NULL);

	while(*tmp) {
		if(*tmp == '\\')
			sepcount++;
		tmp++;
	}
	ret = g_malloc0(strlen(filename) + sepcount + 1);
	while(*filename) {
		ret[cnt] = *filename;
		if(*filename == '\\')
			ret[++cnt] = '\\';
		filename++;
		cnt++;
	}
	ret[cnt] = '\0';
	return ret;
}

/* Determine whether the specified dll contains the specified procedure.
   If so, load it (if not already loaded). */
FARPROC weasytag_find_and_loadproc(const char *dllname, const char *procedure) {
    HMODULE hmod;
	BOOL did_load = FALSE;
    FARPROC proc = 0;

	if(!(hmod = GetModuleHandle(dllname))) {
        //Log_Print(_("DLL '%s' not already loaded. Try loading it..."), dllname);
        g_printf(_("DLL '%s' not already loaded. Try loading it..."), dllname);
        g_print("\n");
		if(!(hmod = LoadLibrary(dllname))) {
            //Log_Print(_("DLL '%s' could not be loaded"), dllname);
            g_print(_("DLL '%s' could not be loaded"), dllname);
            g_print("\n");
            return NULL;
        }
        else
			did_load = TRUE;
    }

	if((proc = GetProcAddress(hmod, procedure))) {
        //Log_Print(_("This version of '%s' contains '%s'"), dllname, procedure);
        g_print(_("This version of '%s' contains '%s'"), dllname, procedure);
        g_print("\n");
        return proc;
    }
	else {
        //Log_Print(_("Function '%s' not found in dll '%s'"), procedure, dllname);
        g_print(_("Function '%s' not found in DLL '%s'"), procedure, dllname);
        g_print("\n");
		if(did_load) {
            /* unload dll */
            FreeLibrary(hmod);
        }
        return NULL;
    }
}

/* Determine Easytag Paths during Runtime */

/* Get paths to special Windows folders. */
char *weasytag_get_special_folder(int folder_type) {
	static LPFNSHGETFOLDERPATHA MySHGetFolderPathA = NULL;
	static LPFNSHGETFOLDERPATHW MySHGetFolderPathW = NULL;
	char *retval = NULL;

	if (!MySHGetFolderPathW) {
		MySHGetFolderPathW = (LPFNSHGETFOLDERPATHW)
			weasytag_find_and_loadproc("shfolder.dll", "SHGetFolderPathW");
	}

	if (MySHGetFolderPathW) {
		wchar_t utf_16_dir[MAX_PATH + 1];

		if (SUCCEEDED(MySHGetFolderPathW(NULL, folder_type, NULL,
						SHGFP_TYPE_CURRENT, utf_16_dir))) {
			retval = g_utf16_to_utf8(utf_16_dir, -1, NULL, NULL, NULL);
		}
	}

	if (!retval) {
		if (!MySHGetFolderPathA) {
			MySHGetFolderPathA = (LPFNSHGETFOLDERPATHA)
				weasytag_find_and_loadproc("shfolder.dll", "SHGetFolderPathA");
		}
		if (MySHGetFolderPathA) {
			char locale_dir[MAX_PATH + 1];

			if (SUCCEEDED(MySHGetFolderPathA(NULL, folder_type, NULL,
							SHGFP_TYPE_CURRENT, locale_dir))) {
				retval = g_locale_to_utf8(locale_dir, -1, NULL, NULL, NULL);
			}
		}
	}

	return retval;
}

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

const char *weasytag_lib_dir(void) {
	static gboolean initialized = FALSE;

	if (!initialized) {
		const char *inst_dir = weasytag_install_dir();
		if (inst_dir != NULL) {
			lib_dir = g_strdup_printf("%s" G_DIR_SEPARATOR_S "library", inst_dir);
			initialized = TRUE;
		} else {
			return NULL;
		}
	}

	return lib_dir;
}

const char *weasytag_locale_dir(void) {
	static gboolean initialized = FALSE;

	if (!initialized) {
		const char *inst_dir = weasytag_install_dir();
		if (inst_dir != NULL) {
			locale_dir = g_strdup_printf("%s" G_DIR_SEPARATOR_S "locale", inst_dir);
			initialized = TRUE;
		} else {
			return NULL;
		}
	}

	return locale_dir;
}

const char *weasytag_data_dir(void) {

	if (!app_data_dir) {
		/* Set app data dir, used by easytag_home_dir */
		const char *newenv = g_getenv("EASYTAGHOME");
		if (newenv)
			app_data_dir = g_strdup(newenv);
		else {
			app_data_dir = weasytag_get_special_folder(CSIDL_APPDATA);
			if (!app_data_dir)
				app_data_dir = g_strdup("C:");
		}

        //ET_Win32_Path_Replace_Backslashes(app_data_dir);

        //Log_Print(_("EasyTAG settings directory: '%s'"), app_data_dir);
        g_print(_("EasyTAG settings directory: '%s'"), app_data_dir);
        g_print("\n");
	}

	return app_data_dir;
}

/* Miscellaneous */

gboolean weasytag_write_reg_string(HKEY rootkey, const char *subkey, const char *valname,
		const char *value) {
	HKEY reg_key;
	gboolean success = FALSE;

	if(G_WIN32_HAVE_WIDECHAR_API()) {
		wchar_t *wc_subkey = g_utf8_to_utf16(subkey, -1, NULL,
			NULL, NULL);

		if(RegOpenKeyExW(rootkey, wc_subkey, 0,
				KEY_SET_VALUE, &reg_key) == ERROR_SUCCESS) {
			wchar_t *wc_valname = NULL;

			if (valname)
				wc_valname = g_utf8_to_utf16(valname, -1,
					NULL, NULL, NULL);

			if(value) {
				wchar_t *wc_value = g_utf8_to_utf16(value, -1,
					NULL, NULL, NULL);
				int len = (wcslen(wc_value) * sizeof(wchar_t)) + 1;
				if(RegSetValueExW(reg_key, wc_valname, 0, REG_SZ,
						(LPBYTE)wc_value, len
						) == ERROR_SUCCESS)
					success = TRUE;
				g_free(wc_value);
			} else
				if(RegDeleteValueW(reg_key, wc_valname) ==  ERROR_SUCCESS)
					success = TRUE;

			g_free(wc_valname);
		}
		g_free(wc_subkey);
	} else {
		char *cp_subkey = g_locale_from_utf8(subkey, -1, NULL,
			NULL, NULL);
		if(RegOpenKeyExA(rootkey, cp_subkey, 0,
				KEY_SET_VALUE, &reg_key) == ERROR_SUCCESS) {
			char *cp_valname = NULL;
			if(valname)
				cp_valname = g_locale_from_utf8(valname, -1,
					NULL, NULL, NULL);

			if (value) {
				char *cp_value = g_locale_from_utf8(value, -1,
					NULL, NULL, NULL);
				int len = strlen(cp_value) + 1;
				if(RegSetValueExA(reg_key, cp_valname, 0, REG_SZ,
						cp_value, len
						) == ERROR_SUCCESS)
					success = TRUE;
				g_free(cp_value);
			} else
				if(RegDeleteValueA(reg_key, cp_valname) ==  ERROR_SUCCESS)
					success = TRUE;

			g_free(cp_valname);
		}
		g_free(cp_subkey);
	}

	if(reg_key != NULL)
		RegCloseKey(reg_key);

	return success;
}

static HKEY _reg_open_key(HKEY rootkey, const char *subkey, REGSAM access) {
	HKEY reg_key = NULL;
	LONG rv;

	if(G_WIN32_HAVE_WIDECHAR_API()) {
		wchar_t *wc_subkey = g_utf8_to_utf16(subkey, -1, NULL,
			NULL, NULL);
		rv = RegOpenKeyExW(rootkey, wc_subkey, 0, access, &reg_key);
		g_free(wc_subkey);
	} else {
		char *cp_subkey = g_locale_from_utf8(subkey, -1, NULL,
			NULL, NULL);
		rv = RegOpenKeyExA(rootkey, cp_subkey, 0, access, &reg_key);
		g_free(cp_subkey);
	}

	if (rv != ERROR_SUCCESS) {
		char *errmsg = g_win32_error_message(rv);
		g_print("Could not open reg key '%s' subkey '%s'. Message: (%ld) %s\n",
					((rootkey == HKEY_LOCAL_MACHINE) ? "HKLM" :
					 (rootkey == HKEY_CURRENT_USER) ? "HKCU" :
					  (rootkey == HKEY_CLASSES_ROOT) ? "HKCR" : "???"),
					subkey, rv, errmsg);
		g_free(errmsg);
	}

	return reg_key;
}

static gboolean _reg_read(HKEY reg_key, const char *valname, LPDWORD type, LPBYTE data, LPDWORD data_len) {
	LONG rv;

	if(G_WIN32_HAVE_WIDECHAR_API()) {
		wchar_t *wc_valname = NULL;
		if (valname)
			wc_valname = g_utf8_to_utf16(valname, -1, NULL, NULL, NULL);
		rv = RegQueryValueExW(reg_key, wc_valname, 0, type, data, data_len);
		g_free(wc_valname);
	} else {
		char *cp_valname = NULL;
		if(valname)
			cp_valname = g_locale_from_utf8(valname, -1, NULL, NULL, NULL);
		rv = RegQueryValueExA(reg_key, cp_valname, 0, type, data, data_len);
		g_free(cp_valname);
	}

	if (rv != ERROR_SUCCESS) {
		char *errmsg = g_win32_error_message(rv);
		g_print("Could not read from reg key value '%s'. Message: (%ld) %s\n",
					valname, rv, errmsg);
		g_free(errmsg);
	}

	return (rv == ERROR_SUCCESS);
}

gboolean weasytag_read_reg_dword(HKEY rootkey, const char *subkey, const char *valname, LPDWORD result) {

	DWORD type;
	DWORD nbytes;
	HKEY reg_key = _reg_open_key(rootkey, subkey, KEY_QUERY_VALUE);
	gboolean success = FALSE;

	if(reg_key) {
		if(_reg_read(reg_key, valname, &type, (LPBYTE)result, &nbytes))
			success = TRUE;
		RegCloseKey(reg_key);
	}

	return success;
}

char *weasytag_read_reg_string(HKEY rootkey, const char *subkey, const char *valname) {

	DWORD type;
	DWORD nbytes;
	HKEY reg_key = _reg_open_key(rootkey, subkey, KEY_QUERY_VALUE);
	char *result = NULL;

	if(reg_key) {
		if(_reg_read(reg_key, valname, &type, NULL, &nbytes) && type == REG_SZ) {
			LPBYTE data;
			if(G_WIN32_HAVE_WIDECHAR_API())
				data = (LPBYTE) g_new(wchar_t, ((nbytes + 1) / sizeof(wchar_t)) + 1);
			else
				data = (LPBYTE) g_malloc(nbytes + 1);

			if(_reg_read(reg_key, valname, &type, data, &nbytes)) {
				if(G_WIN32_HAVE_WIDECHAR_API()) {
					wchar_t *wc_temp = (wchar_t*) data;
					wc_temp[nbytes / sizeof(wchar_t)] = '\0';
					result = g_utf16_to_utf8(wc_temp, -1,
						NULL, NULL, NULL);
				} else {
					char *cp_temp = (char*) data;
					cp_temp[nbytes] = '\0';
					result = g_locale_to_utf8(cp_temp, -1,
						NULL, NULL, NULL);
				}
			}
			g_free(data);
		}
		RegCloseKey(reg_key);
	}

	return result;
}


void weasytag_init(void) {
    WORD wVersionRequested;
    WSADATA wsaData;
	char *newenv;

    //Log_Print(_("weasytag_init start..."));
    g_print(_("weasytag_init start..."));
    g_print("\n");
    //Log_Print(_("EasyTAG version: %s"),VERSION);
    //g_print(_("EasyTAG version: %s"),VERSION);
    //g_print("\n");

	g_print(_("GLib version: %u.%u.%u\n"),glib_major_version, glib_minor_version, glib_micro_version);

    /* Winsock init */
    wVersionRequested = MAKEWORD( 2, 2 );
    WSAStartup( wVersionRequested, &wsaData );

    /* Confirm that the winsock DLL supports 2.2 */
    /* Note that if the DLL supports versions greater than
       2.2 in addition to 2.2, it will still return 2.2 in 
       wVersion since that is the version we requested. */
	if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        g_print("Could not find a usable WinSock DLL.  Oh well.\n");
        WSACleanup();
    }

	/* Set Environmental Variables */

    //Log_Print(_("weasytag_init end..."));
    g_print(_("weasytag_init end..."));
    g_print("\n");
}

/* Windows Cleanup */

void weasytag_cleanup(void) {
	//Log_Print("weasytag_cleanup");
    g_print("weasytag_cleanup\n");

    /* winsock cleanup */
    WSACleanup();

	g_free(app_data_dir);
	app_data_dir = NULL;

	libeasytagdll_hInstance = NULL;
}

/* DLL initializer */
/* suppress gcc "no previous prototype" warning */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	libeasytagdll_hInstance = hinstDLL;
    return TRUE;
}



/* this is used by libmp4v2 : what is it doing here you think ? well...search! */
int gettimeofday (struct timeval *t, void *foo)
{
    struct _timeb temp;

	if (t != 0) {
        _ftime(&temp);
        t->tv_sec = temp.time;              /* seconds since 1-1-1970 */
        t->tv_usec = temp.millitm * 1000; 	/* microseconds */
    }
    return (0);
}


/* emulate the unix function */
int mkstemp (char *template)
{
    int fd = -1;

    char *str = mktemp(template);
    if(str != NULL)
    {
        fd  = open(str, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    }

    return fd;
}

void str_replace_char (gchar *str, gchar in_char, gchar out_char)
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
char*  ET_Win32_Get_Audio_File_Player (void)
{
    char *key_value;
    char *player;

    if (key_value=weasytag_read_reg_string(HKEY_CURRENT_USER, "Software\\foobar2000", "InstallDir"))
    {
        player = g_strconcat(key_value, "\\foobar2000.exe", NULL);
        g_free(key_value);
        
    }else if(key_value=weasytag_read_reg_string(HKEY_CURRENT_USER, "Software\\Winamp", ""))
    {
        player = g_strconcat(key_value, "\\winamp.exe", NULL);
        g_free(key_value);
        
    }else
    {
        player = g_strdup("");
    }
     
    //Log_Print(_("Audio player: '%s'"), player);
    g_print(_("Audio player: '%s'"), player);
    g_print("\n");

    return player;
}


