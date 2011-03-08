/*
 * easytag
 *
 * File: win32dep.c
 * Date: June, 2002
 * Description: Windows dependant code for Easytag
 * this code if largely taken from win32 Gaim and Pidgin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */
#include <windows.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <winuser.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>

#include "resource.h"

#include <libintl.h>

#include "win32dep.h"
//#include "../log.h"



/*
 *  GLOBALS
 */
HINSTANCE exe_hInstance = 0;
HINSTANCE dll_hInstance = 0;
HWND messagewin_hwnd;

typedef BOOL (CALLBACK* LPFNFLASHWINDOWEX)(PFLASHWINFO);
static LPFNFLASHWINDOWEX MyFlashWindowEx = NULL;

/*
 *  PUBLIC CODE
 */

HINSTANCE wineasytag_exe_hinstance (void)
{
    return exe_hInstance;
}

HINSTANCE wineasytag_dll_hinstance(void) {
	return dll_hInstance;
}

void wineasytag_shell_execute(const char *target, const char *verb, const char *clazz) {

	g_return_if_fail(target != NULL);
	g_return_if_fail(verb != NULL);

	if (G_WIN32_HAVE_WIDECHAR_API()) {
		SHELLEXECUTEINFOW wsinfo;
		wchar_t *w_uri, *w_verb, *w_clazz = NULL;

		w_uri = g_utf8_to_utf16(target, -1, NULL, NULL, NULL);
		w_verb = g_utf8_to_utf16(verb, -1, NULL, NULL, NULL);

		memset(&wsinfo, 0, sizeof(wsinfo));
		wsinfo.cbSize = sizeof(wsinfo);
		wsinfo.lpVerb = w_verb;
		wsinfo.lpFile = w_uri;
		wsinfo.nShow = SW_SHOWNORMAL;
		if (clazz != NULL) {
			w_clazz = g_utf8_to_utf16(clazz, -1, NULL, NULL, NULL);
			wsinfo.fMask |= SEE_MASK_CLASSNAME;
			wsinfo.lpClass = w_clazz;
		}

		if(!ShellExecuteExW(&wsinfo))
			g_print("Error opening URI: %s error: %d\n",
				target, (int) wsinfo.hInstApp);

		g_free(w_uri);
		g_free(w_verb);
		g_free(w_clazz);
	} else {
		SHELLEXECUTEINFOA sinfo;
		gchar *locale_uri;

		locale_uri = g_locale_from_utf8(target, -1, NULL, NULL, NULL);

		memset(&sinfo, 0, sizeof(sinfo));
		sinfo.cbSize = sizeof(sinfo);
		sinfo.lpVerb = verb;
		sinfo.lpFile = locale_uri;
		sinfo.nShow = SW_SHOWNORMAL;
		if (clazz != NULL) {
			sinfo.fMask |= SEE_MASK_CLASSNAME;
			sinfo.lpClass = clazz;
		}

		if(!ShellExecuteExA(&sinfo))
			g_print("Error opening URI: %s error: %d\n",
				target, (int) sinfo.hInstApp);

		g_free(locale_uri);
	}

}

void wineasytag_notify_uri(const char *uri) {
	/* We'll allow whatever URI schemes are supported by the
	 * default http browser.
	 */
	wineasytag_shell_execute(uri, "open", "http");
}

/*
#define EASYTAG_WM_FOCUS_REQUEST (WM_APP + 13)
#define EASYTAG_WM_PROTOCOL_HANDLE (WM_APP + 14)

static LRESULT CALLBACK message_window_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

	if (msg == PIDGIN_WM_FOCUS_REQUEST) {
		g_print("Got external Buddy List focus request.");
		purple_blist_set_visible(TRUE);
		return TRUE;
	} else if (msg == PIDGIN_WM_PROTOCOL_HANDLE) {
		char *proto_msg = (char *) lparam;
		g_print("Got protocol handler request: %s\n", proto_msg ? proto_msg : "");
		purple_got_protocol_handler_uri(proto_msg);
		return TRUE;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}*/

/*static HWND wineasytag_message_window_init(void) {
	HWND win_hwnd;
	WNDCLASSEX wcx;
	LPCTSTR wname;

	wname = TEXT("WinpidginMsgWinCls");

	wcx.cbSize = sizeof(wcx);
	wcx.style = 0;
	wcx.lpfnWndProc = message_window_handler;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = wineasytag_exe_hinstance();
	wcx.hIcon = NULL;
	wcx.hCursor = NULL;
	wcx.hbrBackground = NULL;
	wcx.lpszMenuName = NULL;
	wcx.lpszClassName = wname;
	wcx.hIconSm = NULL;

	RegisterClassEx(&wcx);

	// Create the window 
	if(!(win_hwnd = CreateWindow(wname, TEXT("WinpidginMsgWin"), 0, 0, 0, 0, 0,
			NULL, NULL, winpidgin_exe_hinstance(), 0))) {
		
		g_print("Unable to create message window.\n");
		return NULL;
	}

	return win_hwnd;
}*/

void wineasytag_init(HINSTANCE hint) {

	g_print("wineasytag_init start\n");

	exe_hInstance = hint;

	/* IdleTracker Initialization */
	////if(!wineasytag_set_idlehooks())
	////	g_print("Failed to initialize idle tracker\n");

	////wineasytag_spell_init();
	g_print("GTK+ :%u.%u.%u\n",
		gtk_major_version, gtk_minor_version, gtk_micro_version);

	////messagewin_hwnd = wineasytag_message_window_init();

	MyFlashWindowEx = (LPFNFLASHWINDOWEX) weasytag_find_and_loadproc("user32.dll", "FlashWindowEx");

	g_print("wineasytag_init end\n");
}

void wineasytag_post_init(void) {

	//purple_prefs_add_none(PIDGIN_PREFS_ROOT "/win32");
	//purple_prefs_add_bool(PIDGIN_PREFS_ROOT "/win32/blink_im", TRUE);

	//purple_signal_connect(pidgin_conversations_get_handle(),
	//	"displaying-im-msg", &gtkwin32_handle, PURPLE_CALLBACK(wineasytag_conv_im_blink),
	//	NULL);

}

/* Windows Cleanup */

void wineasytag_cleanup(void) {
	g_print("wineasytag_cleanup\n");

	if(messagewin_hwnd)
		DestroyWindow(messagewin_hwnd);

	/* Idle tracker cleanup */
	////wineasytag_remove_idlehooks();

}

/* DLL initializer */
/* suppress gcc "no previous prototype" warning */
/*BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	dll_hInstance = hinstDLL;
	return TRUE;
}*/

