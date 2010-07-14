/*
 *  Code taken from GAIM 2.0.0b5
 *
 */
/*
 *  win_easytag.c
 *
 *  Date: June, 2002
 *  Description: Entry point for win32 easytag, and various win32 dependant
 *  routines.
 *
 * EasyTAG is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
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

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif
#include <windows.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define WIN32_PROXY_REGKEY "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

/* These will hopefully be in the win32api next time it is updated - at which point, we'll remove them */
#ifndef LANG_PERSIAN
#define LANG_PERSIAN 0x29
#endif
#ifndef LANG_BOSNIAN
#define SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN	0x05
#define SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC	0x08
#endif
#ifndef SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN
#define SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN	0x04
#endif
#ifndef LANG_XHOSA
#define LANG_XHOSA 0x34
#endif


typedef int (CALLBACK* LPFNEASYTAGMAIN)(HINSTANCE, int, char**);
typedef void (CALLBACK* LPFNSETDLLDIRECTORY)(LPCTSTR);
typedef BOOL (CALLBACK* LPFNATTACHCONSOLE)(DWORD);

/*
 *  PROTOTYPES
 */
static LPFNEASYTAGMAIN easytag_main = NULL;
static LPFNSETDLLDIRECTORY MySetDllDirectory = NULL;


static const char *get_win32_error_message(DWORD err) {
	static char err_msg[512];

	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &err_msg, sizeof(err_msg), NULL);

	return err_msg;
}

static BOOL read_reg_string(HKEY key, char* sub_key, char* val_name, LPBYTE data, LPDWORD data_len) {
	HKEY hkey;
	BOOL ret = FALSE;
	LONG retv;

	if (ERROR_SUCCESS == (retv = RegOpenKeyEx(key, sub_key, 0,
					KEY_QUERY_VALUE, &hkey))) {
		if (ERROR_SUCCESS == (retv = RegQueryValueEx(hkey, val_name,
						NULL, NULL, data, data_len)))
			ret = TRUE;
		else {
			const char *err_msg = get_win32_error_message(retv);

			printf("Could not read reg key '%s' subkey '%s' value: '%s'. Message: (%ld) %s\n",
					((key == HKEY_LOCAL_MACHINE) ? "HKLM" :
					 (key == HKEY_CURRENT_USER) ? "HKCU" :
					 "???"),
					sub_key, val_name, retv, err_msg);
		}
		RegCloseKey(hkey);
	}
	else {
		TCHAR szBuf[80];

		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, retv, 0,
				(LPTSTR) &szBuf, sizeof(szBuf), NULL);
        printf("Could not open reg subkey: '%s'. Error: (%ld) %s\n",
				sub_key, retv, szBuf);
	}

	return ret;
}

static void common_dll_prep(const char *path) {
	HMODULE hmod;
	HKEY hkey;

	printf("GTK+ path found: %s\n", path);

	if ((hmod = GetModuleHandle("kernel32.dll"))) {
		MySetDllDirectory = (LPFNSETDLLDIRECTORY) GetProcAddress(
			hmod, "SetDllDirectoryA");
		if (!MySetDllDirectory)
			printf("SetDllDirectory not supported\n");
	} else
		printf("Error getting kernel32.dll module handle\n");

	/* For Windows XP SP1+ / Server 2003 we use SetDllDirectory to avoid dll hell */
	if (MySetDllDirectory) {
		printf("Using SetDllDirectory\n");
		MySetDllDirectory(path);
	}

	/* For the rest, we set the current directory and make sure
	 * SafeDllSearch is set to 0 where needed. */
	else {
		OSVERSIONINFO osinfo;

		printf("Setting current directory to GTK+ dll directory\n");
		SetCurrentDirectory(path);
		/* For Windows 2000 (SP3+) / WinXP (No SP):
		 * If SafeDllSearchMode is set to 1, Windows system directories are
		 * searched for dlls before the current directory. Therefore we set it
		 * to 0.
		 */
		osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osinfo);
		if ((osinfo.dwMajorVersion == 5 &&
			osinfo.dwMinorVersion == 0 &&
			strcmp(osinfo.szCSDVersion, "Service Pack 3") >= 0) ||
			(osinfo.dwMajorVersion == 5 &&
			osinfo.dwMinorVersion == 1 &&
			strcmp(osinfo.szCSDVersion, "") >= 0)
		) {
			DWORD regval = 1;
			DWORD reglen = sizeof(DWORD);

			printf("Using Win2k (SP3+) / WinXP (No SP)... Checking SafeDllSearch\n");
			read_reg_string(HKEY_LOCAL_MACHINE,
				"System\\CurrentControlSet\\Control\\Session Manager",
				"SafeDllSearchMode",
				(LPBYTE) &regval,
				&reglen);

			if (regval != 0) {
				printf("Trying to set SafeDllSearchMode to 0\n");
				regval = 0;
				if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					"System\\CurrentControlSet\\Control\\Session Manager",
					0,  KEY_SET_VALUE, &hkey
				) == ERROR_SUCCESS) {
					if (RegSetValueEx(hkey,
						"SafeDllSearchMode", 0,
						REG_DWORD, (LPBYTE) &regval,
						sizeof(DWORD)
					) != ERROR_SUCCESS)
						printf("Error writing SafeDllSearchMode. Error: %u\n",
						(UINT) GetLastError());
					RegCloseKey(hkey);
				} else
					printf("Error opening Session Manager key for writing. Error: %u\n",
						(UINT) GetLastError());
			} else
				printf("SafeDllSearchMode is set to 0\n");
		}/*end else*/
	}
}

static void dll_prep() {
	char path[MAX_PATH + 1];
	HKEY hkey;
	char gtkpath[MAX_PATH + 1];
	DWORD plen;

	plen = sizeof(gtkpath);
	hkey = HKEY_CURRENT_USER;
	if (!read_reg_string(hkey, "SOFTWARE\\GTK\\2.0", "Path",
			(LPBYTE) &gtkpath, &plen)) {
		hkey = HKEY_LOCAL_MACHINE;
		if (!read_reg_string(hkey, "SOFTWARE\\GTK\\2.0", "Path",
				(LPBYTE) &gtkpath, &plen)) {
			printf("GTK+ Path Registry Key not found. "
				"Assuming GTK+ is in the PATH.\n");
			return;
		}
	}

	/* this value is replaced during a successful RegQueryValueEx() */
	plen = sizeof(path);
	/* Determine GTK+ dll path .. */
	if (!read_reg_string(hkey, "SOFTWARE\\GTK\\2.0", "DllPath",
				(LPBYTE) &path, &plen)) {
		strcpy(path, gtkpath);
		strcat(path, "\\bin");
	}

	common_dll_prep(path);
}

static char* wineasytag_lcid_to_posix(LCID lcid) {
	char *posix = NULL;
	int lang_id = PRIMARYLANGID(lcid);
	int sub_id = SUBLANGID(lcid);

	switch (lang_id) {
		case LANG_AFRIKAANS: posix = "af"; break;
		case LANG_ARABIC: posix = "ar"; break;
		case LANG_AZERI: posix = "az"; break;
		case LANG_BENGALI: posix = "bn"; break;
		case LANG_BULGARIAN: posix = "bg"; break;
		case LANG_CATALAN: posix = "ca"; break;
		case LANG_CZECH: posix = "cs"; break;
		case LANG_DANISH: posix = "da"; break;
		case LANG_ESTONIAN: posix = "et"; break;
		case LANG_PERSIAN: posix = "fa"; break;
		case LANG_GERMAN: posix = "de"; break;
		case LANG_GREEK: posix = "el"; break;
		case LANG_ENGLISH:
			switch (sub_id) {
				case SUBLANG_ENGLISH_UK:
					posix = "en_GB"; break;
				case SUBLANG_ENGLISH_AUS:
					posix = "en_AU"; break;
				case SUBLANG_ENGLISH_CAN:
					posix = "en_CA"; break;
				default:
					posix = "en"; break;
			}
			break;
		case LANG_SPANISH: posix = "es"; break;
		case LANG_BASQUE: posix = "eu"; break;
		case LANG_FINNISH: posix = "fi"; break;
		case LANG_FRENCH: posix = "fr"; break;
		case LANG_GALICIAN: posix = "gl"; break;
		case LANG_GUJARATI: posix = "gu"; break;
		case LANG_HEBREW: posix = "he"; break;
		case LANG_HINDI: posix = "hi"; break;
		case LANG_HUNGARIAN: posix = "hu"; break;
		case LANG_ICELANDIC: break;
		case LANG_INDONESIAN: posix = "id"; break;
		case LANG_ITALIAN: posix = "it"; break;
		case LANG_JAPANESE: posix = "ja"; break;
		case LANG_GEORGIAN: posix = "ka"; break;
		case LANG_KANNADA: posix = "kn"; break;
		case LANG_KOREAN: posix = "ko"; break;
		case LANG_LITHUANIAN: posix = "lt"; break;
		case LANG_MACEDONIAN: posix = "mk"; break;
		case LANG_DUTCH: posix = "nl"; break;
		case LANG_NEPALI: posix = "ne"; break;
		case LANG_NORWEGIAN:
			switch (sub_id) {
				case SUBLANG_NORWEGIAN_BOKMAL:
					posix = "nb"; break;
				case SUBLANG_NORWEGIAN_NYNORSK:
					posix = "nn"; break;
			}
			break;
		case LANG_PUNJABI: posix = "pa"; break;
		case LANG_POLISH: posix = "pl"; break;
		case LANG_PASHTO: posix = "ps"; break;
		case LANG_PORTUGUESE:
			switch (sub_id) {
				case SUBLANG_PORTUGUESE_BRAZILIAN:
					posix = "pt_BR"; break;
				default:
				posix = "pt"; break;
			}
			break;
		case LANG_ROMANIAN: posix = "ro"; break;
		case LANG_RUSSIAN: posix = "ru"; break;
		case LANG_SLOVAK: posix = "sk"; break;
		case LANG_SLOVENIAN: posix = "sl"; break;
		case LANG_ALBANIAN: posix = "sq"; break;
		/* LANG_CROATIAN == LANG_SERBIAN == LANG_BOSNIAN */
		case LANG_SERBIAN:
			switch (sub_id) {
				case SUBLANG_SERBIAN_LATIN:
					posix = "sr@Latn"; break;
				case SUBLANG_SERBIAN_CYRILLIC:
					posix = "sr"; break;
				case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC:
				case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN:
					posix = "bs"; break;
				case SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN:
					posix = "hr"; break;
			}
			break;
		case LANG_SWEDISH: posix = "sv"; break;
		case LANG_TAMIL: posix = "ta"; break;
		case LANG_TELUGU: posix = "te"; break;
		case LANG_THAI: posix = "th"; break;
		case LANG_TURKISH: posix = "tr"; break;
		case LANG_UKRAINIAN: posix = "uk"; break;
		case LANG_VIETNAMESE: posix = "vi"; break;
		case LANG_XHOSA: posix = "xh"; break;
		case LANG_CHINESE:
			switch (sub_id) {
				case SUBLANG_CHINESE_SIMPLIFIED:
					posix = "zh_CN"; break;
				case SUBLANG_CHINESE_TRADITIONAL:
					posix = "zh_TW"; break;
				default:
					posix = "zh"; break;
			}
			break;
		case LANG_URDU: break;
		case LANG_BELARUSIAN: break;
		case LANG_LATVIAN: break;
		case LANG_ARMENIAN: break;
		case LANG_FAEROESE: break;
		case LANG_MALAY: break;
		case LANG_KAZAK: break;
		case LANG_KYRGYZ: break;
		case LANG_SWAHILI: break;
		case LANG_UZBEK: break;
		case LANG_TATAR: break;
		case LANG_ORIYA: break;
		case LANG_MALAYALAM: break;
		case LANG_ASSAMESE: break;
		case LANG_MARATHI: break;
		case LANG_SANSKRIT: break;
		case LANG_MONGOLIAN: break;
		case LANG_KONKANI: break;
		case LANG_MANIPURI: break;
		case LANG_SINDHI: break;
		case LANG_SYRIAC: break;
		case LANG_KASHMIRI: break;
		case LANG_DIVEHI: break;
	}

	/* Deal with exceptions */
	if (posix == NULL) {
		switch (lcid) {
			case 0x0455: posix = "my_MM"; break; /* Myanmar (Burmese) */
			case 9999: posix = "ku"; break; /* Kurdish (from NSIS) */
		}
	}

	return posix;
}

/* Determine and set Eastytag locale as follows (in order of priority):
   - Check EASYTAGLANG env var
   - Check NSIS Installer Language reg value
   - Use default user locale
*/
static const char *wineasytag_get_locale() {
	const char *locale = NULL;
	LCID lcid = 0;
	char data[10];
	DWORD datalen = 10;

	/* Check if user set EASYTAGLANG env var */
	if ((locale = getenv("EASYTAGLANG"))) {
		printf("Variable EASYTAGLANG defined: '%s'\n",locale);
		return locale;
	}else
		printf("Variable EASYTAGLANG not defined\n");

	if (read_reg_string(HKEY_CURRENT_USER, "SOFTWARE\\easytag",
			"Installer Language", (LPBYTE) &data, &datalen)) {
		if ((locale = wineasytag_lcid_to_posix(atoi(data))))
			return locale;
	}

    // List of LCID : http://www.microsoft.com/globaldev/reference/lcid-all.mspx
	lcid = GetUserDefaultLCID();
	if ((locale = wineasytag_lcid_to_posix(lcid)))
		return locale;

	return "en";
}

static void wineasytag_set_locale() {
	const char *locale = NULL;
	char envstr[25];

	locale = wineasytag_get_locale();

	snprintf(envstr, 25, "LANG=%s", locale);
	printf("Setting locale: %s\n", envstr);
	putenv(envstr);
}

#if 0
#define EASYTAG_WM_FOCUS_REQUEST (WM_APP + 13)

static BOOL wineasytag_set_running() {
	HANDLE h;

	if ((h = CreateMutex(NULL, FALSE, "easytag_is_running"))) {
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			HWND msg_win;

			if((msg_win = FindWindow(TEXT("WineasytagMsgWinCls"), NULL)))
				if(SendMessage(msg_win, EASYTAG_WM_FOCUS_REQUEST, (WPARAM) NULL, (LPARAM) NULL))
					return FALSE;

			/* If we get here, the focus request wasn't successful */

			MessageBox(NULL,
				"An instance of EasyTAG is already running",
				NULL, MB_OK | MB_TOPMOST);

			return FALSE;
		}
	}
	return TRUE;
}
#endif

static void set_proxy() {
	DWORD regval = 1;
	DWORD reglen = sizeof(DWORD);

	/* If the proxy server environment variables are already set,
	 * we shouldn't override them */
	if (getenv("HTTP_PROXY") || getenv("http_proxy") || getenv("HTTPPROXY"))
		return;

	if (read_reg_string(HKEY_CURRENT_USER, WIN32_PROXY_REGKEY,
				"ProxyEnable",
				(LPBYTE) &regval, &reglen) && (regval & 1)) {
		char proxy_server[2048];
		char *c = NULL;
		reglen = sizeof(proxy_server);

		if (!read_reg_string(HKEY_CURRENT_USER, WIN32_PROXY_REGKEY,
				"ProxyServer", (LPBYTE) &proxy_server, &reglen))
			return;

		if ((reglen > strlen("http="))
				&& (c = strstr(proxy_server, "http="))) {
			char *d;
			c += strlen("http=");
			d = strchr(c, ';');
			if (d) {
				*d = '\0';
			}
			/* c now points the proxy server (and port) */
		}

		if (c) {
			const char envstr_prefix[] = "HTTP_PROXY=http://";
			char envstr[sizeof(envstr_prefix) + strlen(c) + 1];
			snprintf(envstr, sizeof(envstr), "%s%s",
				envstr_prefix, c);
			printf("Setting HTTP Proxy: %s\n", envstr);
			putenv(envstr);
		}
	}

}

#ifdef __GNUC__
#  ifndef _stdcall
#    define _stdcall  __attribute__((stdcall))
#  endif
#endif

int _stdcall
WinMain (struct HINSTANCE__ *hInstance, struct HINSTANCE__ *hPrevInstance,
		char *lpszCmdLine, int nCmdShow) {
	char errbuf[512];
	char pidgin_dir[MAX_PATH];
	char exe_name[MAX_PATH];
	HMODULE hmod;
	char *tmp;
	int easytag_argc = __argc;
	char **easytag_argv = __argv;

	/* If debug or help or version flag used, create console for output */
	if (strstr(lpszCmdLine, "-d") || strstr(lpszCmdLine, "-h") || strstr(lpszCmdLine, "-v")) {
		/* If stdout hasn't been redirected to a file, alloc a console
		 *  (_istty() doesn't work for stuff using the GUI subsystem) */
		if (_fileno(stdout) == -1 || _fileno(stdout) == -2) {
			LPFNATTACHCONSOLE MyAttachConsole = NULL;
			if ((hmod = GetModuleHandle("kernel32.dll"))) {
				MyAttachConsole =
					(LPFNATTACHCONSOLE)
					GetProcAddress(hmod, "AttachConsole");
			}
			if ((MyAttachConsole && MyAttachConsole(ATTACH_PARENT_PROCESS))
					|| AllocConsole()) {
				freopen("CONOUT$", "w", stdout);
				freopen("CONOUT$", "w", stderr);
			}
		}
	}

#if 0
	/* Load exception handler if we have it */
	if (GetModuleFileName(NULL, easytag_dir, MAX_PATH) != 0) {
		char *prev = NULL;
		tmp = easytag_dir;

		/* primitive dirname() */
		while ((tmp = strchr(tmp, '\\'))) {
			prev = tmp;
			tmp++;
		}

		if (prev) {
			prev[0] = '\0';

			/* prev++ will now point to the executable file name */
			strcpy(exe_name, prev + 1);

			strcat(easytag_dir, "\\exchndl.dll");
			if (LoadLibrary(easytag_dir))
				printf("Loaded exchndl.dll\n");

			prev[0] = '\0';
		}
	} else {
		DWORD dw = GetLastError();
		const char *err_msg = get_win32_error_message(dw);
		snprintf(errbuf, 512,
			"Error getting module filename. Error: (%u) %s",
			(UINT) dw, err_msg);
		printf("%s", errbuf);
		MessageBox(NULL, errbuf, NULL, MB_OK | MB_TOPMOST);
		easytag_dir[0] = '\0';
	}
#endif

		dll_prep();

    wineasytag_set_locale();
	/* If help or version flag used, do not check Mutex */
	/*if (!strstr(lpszCmdLine, "-h") && !strstr(lpszCmdLine, "-v"))
		if (!getenv("GAIM_MULTI_INST") && !wineasytag_set_running())
			return 0;*/

    set_proxy();

	/* Now we are ready for EasyTag .. */
	if ((hmod = LoadLibrary("easytag.dll"))) {
		easytag_main = (LPFNEASYTAGMAIN) GetProcAddress(hmod, "easytag_main");
	}

	if (!easytag_main) {
		DWORD dw = GetLastError();
		BOOL mod_not_found = (dw == ERROR_MOD_NOT_FOUND || dw == ERROR_DLL_NOT_FOUND);
		const char *err_msg = get_win32_error_message(dw);

		snprintf(errbuf, 512, "Error loading 'easytag.dll'. Error: (%u) %s%s%s",
			(UINT) dw, err_msg,
			mod_not_found ? "\n" : "",
			mod_not_found ? "This probably means that a dependency (like GTK+, libogg or libvorbis) can't be found." : "");
		printf("%s", errbuf);
		MessageBox(NULL, errbuf, TEXT("Error"), MB_OK | MB_TOPMOST);

		return 0;
	}

	return easytag_main (hInstance, easytag_argc, easytag_argv);
}
