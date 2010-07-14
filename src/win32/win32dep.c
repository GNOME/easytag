/*
 * easytag
 *
 * File: win32dep.c
 * Date: June, 2002
 * Description: Windows dependant code for Easytag
 * this code if largely taken from win32 gaim
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
#include <shlobj.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timeb.h>

#include <gtk/gtk.h>
#include <glib.h>
#if GLIB_CHECK_VERSION(2,6,0)
#   include <glib/gstdio.h>
#else
#   define g_fopen fopen
#   define g_unlink unlink
#endif
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

typedef enum
{
    SHGFP_TYPE_CURRENT  = 0,   // current value for user, verify it exists
    SHGFP_TYPE_DEFAULT  = 1,   // default value, may not exist
} SHGFP_TYPE;

/*
 * LOCALS
 */
static char app_data_dir[MAX_PATH + 1] = "C:";
static char install_dir[MAXPATHLEN];
static char locale_dir[MAXPATHLEN];

static void str_replace_char (gchar *str, gchar in_char, gchar out_char);

/*
 *  GLOBALS
 */
HINSTANCE ET_exe_hInstance = 0;
HINSTANCE ET_dll_hInstance = 0;

/*
 *  PROTOS
 */
LPFNSHGETFOLDERPATHA MySHGetFolderPathA = NULL;
LPFNSHGETFOLDERPATHW MySHGetFolderPathW = NULL;

FARPROC ET_Win32_Find_And_Loadproc (char*, char*);
char* ET_Win32_Data_Dir (void);


/*
 *  PUBLIC CODE
 */

HINSTANCE ET_Win32_Hinstance (void)
{
    return ET_exe_hInstance;
}

/* Escape windows dir separators.  This is needed when paths are saved,
   and on being read back have their '\' chars used as an escape char.
   Returns an allocated string which needs to be freed.
*/
char* ET_Win32_Escape_Dirsep (char* filename )
{
    int sepcount=0;
    char* ret=NULL;
    int cnt=0;

    ret = filename;
    while(*ret)
    {
        if(*ret == '\\')
            sepcount++;
        ret++;
    }
    ret = g_malloc0(strlen(filename) + sepcount + 1);
    while(*filename)
    {
        ret[cnt] = *filename;
        if(*filename == '\\')
            ret[++cnt] = '\\';
        filename++;
        cnt++;
    }
    ret[cnt] = '\0';
    return ret;
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

/* Determine whether the specified dll contains the specified procedure.
   If so, load it (if not already loaded). */
FARPROC ET_Win32_Find_And_Loadproc ( char* dllname, char* procedure )
{
    HMODULE hmod;
    int did_load=0;
    FARPROC proc = 0;

    if(!(hmod=GetModuleHandle(dllname)))
    {
        //Log_Print(_("DLL '%s' not found. Try loading it..."), dllname);
        g_printf(_("DLL '%s' not found. Try loading it..."), dllname);
        g_print("\n");
        if(!(hmod = LoadLibrary(dllname)))
        {
            //Log_Print(_("DLL '%s' could not be loaded"), dllname);
            g_print(_("DLL '%s' could not be loaded"), dllname);
            g_print("\n");
            return NULL;
        }
        else
			did_load = TRUE;
    }

    if((proc=GetProcAddress(hmod, procedure)))
    {
        //Log_Print(_("This version of '%s' contains '%s'"), dllname, procedure);
        g_print(_("This version of '%s' contains '%s'"), dllname, procedure);
        g_print("\n");
        return proc;
    }
    else
    {
        //Log_Print(_("Function '%s' not found in dll '%s'"), procedure, dllname);
        g_print(_("Function '%s' not found in dll '%s'"), procedure, dllname);
        g_print("\n");
        if(did_load)
        {
            /* unload dll */
            FreeLibrary(hmod);
        }
        return NULL;
    }
}

/* Determine Easytag Paths during Runtime */

char* ET_Win32_Install_Dir (void)
{
    HMODULE hmod;
    char* buf;

    hmod = GetModuleHandle(NULL);
    if ( hmod == 0 )
    {
        buf = g_win32_error_message( GetLastError() );
        //Log_Print("GetModuleHandle error: %s\n", buf);
        g_print("GetModuleHandle error: %s\n", buf);
        g_free(buf);
        return NULL;
    }
    if (GetModuleFileName( hmod, (char*)&install_dir, MAXPATHLEN ) == 0)
    {
        buf = g_win32_error_message( GetLastError() );
        //Log_Print("GetModuleFileName error: %s\n", buf);
        g_print("GetModuleFileName error: %s\n", buf);
        g_free(buf);
        return NULL;
    }
    buf = g_path_get_dirname( install_dir );
    strcpy( (char*)&install_dir, buf );
    g_free( buf );

    return (char*)&install_dir;
}


char* ET_Win32_Locale_Dir (void)
{
    strcpy(locale_dir, ET_Win32_Install_Dir());
    g_strlcat(locale_dir, G_DIR_SEPARATOR_S "locale", sizeof(locale_dir));
    return (char*)&locale_dir;
}

char* ET_Win32_Data_Dir (void)
{
    return (char*)&app_data_dir;
}


/* Miscellaneous */

gboolean ET_Win32_Read_Reg_String (HKEY key, char* sub_key, char* val_name, LPBYTE data, LPDWORD data_len)
{
    HKEY hkey;
    gboolean ret = FALSE;

    if(ERROR_SUCCESS == RegOpenKeyEx(key, sub_key, 0, KEY_QUERY_VALUE, &hkey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, val_name, 0, NULL, data, data_len))
            ret = TRUE;
        RegCloseKey(key);
    }
    return ret;
}

/* find a default player executable */
char*  ET_Win32_Get_Audio_File_Player (void)
{
    DWORD len = 256;
    char key_value[256];

    char *player;

    if(ET_Win32_Read_Reg_String(HKEY_CURRENT_USER, "Software\\foobar2000", "InstallDir", key_value, &len))
    {
        player = g_strconcat(key_value, "\\foobar2000.exe", NULL);
    }
    else if(ET_Win32_Read_Reg_String(HKEY_CURRENT_USER, "Software\\Winamp", "", key_value, &len))
    {
        player = g_strconcat(key_value, "\\winamp.exe", NULL);
    }
    else
    {
        player = g_strdup("");
    }
     
    //Log_Print(_("Audio player: '%s'"), player);
    g_print(_("Audio player: '%s'"), player);
    g_print("\n");

    return player;
}


void ET_Win32_Notify_Uri (const char *uri)
{
    SHELLEXECUTEINFO sinfo;

    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SEE_MASK_CLASSNAME;
    sinfo.lpVerb = "open";
    sinfo.lpFile = uri; 
    sinfo.nShow = SW_SHOWNORMAL; 
    sinfo.lpClass = "http";

    /* We'll allow whatever URI schemes are supported by the
       default http browser.
    */
    if(!ShellExecuteEx(&sinfo))
        //Log_Print("Error opening URI: %s error: %d\n", uri, (int)sinfo.hInstApp);
        g_print("Error opening URI: %s error: %d\n", uri, (int)sinfo.hInstApp);
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

void ET_Win32_Init (HINSTANCE hint)
{
    WORD wVersionRequested;
    WSADATA wsaData;
        char *newenv;

    ET_exe_hInstance = hint;

    /* Winsock init */
    wVersionRequested = MAKEWORD( 2, 2 );
    WSAStartup( wVersionRequested, &wsaData );

    /* Confirm that the winsock DLL supports 2.2 */
    /* Note that if the DLL supports versions greater than
       2.2 in addition to 2.2, it will still return 2.2 in 
       wVersion since that is the version we requested. */
    if ( LOBYTE( wsaData.wVersion ) != 2
    ||   HIBYTE( wsaData.wVersion ) != 2 )
    {
        g_print("Could not find a usable WinSock DLL.  Oh well.\n");
        WSACleanup();
    }

    /* Set app data dir, used by easytag_home_dir */
    newenv = (char*)g_getenv("EASYTAGHOME");
    if(!newenv)
    {
#if GLIB_CHECK_VERSION(2,6,0)
        if ((MySHGetFolderPathW = (LPFNSHGETFOLDERPATHW) ET_Win32_Find_And_Loadproc("shfolder.dll", "SHGetFolderPathW")))
        {
            wchar_t utf_16_dir[MAX_PATH +1];
            char *temp;
            MySHGetFolderPathW(NULL, CSIDL_APPDATA, NULL,
                    SHGFP_TYPE_CURRENT, utf_16_dir);
            temp = g_utf16_to_utf8(utf_16_dir, -1, NULL, NULL, NULL);
            g_strlcpy(app_data_dir, temp, sizeof(app_data_dir));
            g_free(temp);
        } else if ((MySHGetFolderPathA = (LPFNSHGETFOLDERPATHA) ET_Win32_Find_And_Loadproc("shfolder.dll", "SHGetFolderPathA")))
        {
            char locale_dir[MAX_PATH + 1];
            char *temp;
            MySHGetFolderPathA(NULL, CSIDL_APPDATA, NULL,
                    SHGFP_TYPE_CURRENT, locale_dir);
            temp = g_locale_to_utf8(locale_dir, -1, NULL, NULL, NULL);
            g_strlcpy(app_data_dir, temp, sizeof(app_data_dir));
            g_free(temp);
        }
#else
        if ((MySHGetFolderPathA = (LPFNSHGETFOLDERPATHA) ET_Win32_Find_And_Loadproc("shfolder.dll", "SHGetFolderPathA")))
        {
            MySHGetFolderPathA(NULL, CSIDL_APPDATA, NULL,
                    SHGFP_TYPE_CURRENT, app_data_dir);
        }
#endif
        else
        {
            strcpy(app_data_dir, "C:");
        }
    }
    else
    {
        g_strlcpy(app_data_dir, newenv, sizeof(app_data_dir));
    }

    //ET_Win32_Path_Replace_Backslashes(app_data_dir);

    //Log_Print(_("EasyTAG settings dir: '%s'"), app_data_dir);
    g_print(_("EasyTAG settings dir: '%s'"), app_data_dir);
    g_print("\n");

    newenv = g_strdup_printf("HOME=%s", app_data_dir);

    if (putenv(newenv)<0)
        g_print("putenv failed\n");
    g_free(newenv);

}

/* Windows Cleanup */

void ET_Win32_Cleanup (void)
{
    /* winsock cleanup */
    WSACleanup();
}

/* DLL initializer */
BOOL WINAPI DllMain ( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
    ET_dll_hInstance = hinstDLL;
    return TRUE;
}
