/*
 *  Notepad
 *
 *  Copyright 2000 Mike McCormack <Mike_McCormack@looksmart.com.au>
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

//#define UNICODE

#include <windows.h>
//#include <shlwapi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "dialog.h"
#include "notepad_res.h"

NOTEPAD_GLOBALS Globals;
static ATOM aFINDMSGSTRING;
static RECT main_rect;

static const char notepad_reg_key[] = {'S','o','f','t','w','a','r','e','\\',
                                        'M','i','c','r','o','s','o','f','t','\\','N','o','t','e','p','a','d','\0'};
static const char value_fWrap[]            = {'f','W','r','a','p','\0'};
static const char value_iPointSize[]       = {'i','P','o','i','n','t','S','i','z','e','\0'};
static const char value_iWindowPosDX[]     = {'i','W','i','n','d','o','w','P','o','s','D','X','\0'};
static const char value_iWindowPosDY[]     = {'i','W','i','n','d','o','w','P','o','s','D','Y','\0'};
static const char value_iWindowPosX[]      = {'i','W','i','n','d','o','w','P','o','s','X','\0'};
static const char value_iWindowPosY[]      = {'i','W','i','n','d','o','w','P','o','s','Y','\0'};
static const char value_lfCharSet[]        = {'l','f','C','h','a','r','S','e','t','\0'};
static const char value_lfClipPrecision[]  = {'l','f','C','l','i','p','P','r','e','c','i','s','i','o','n','\0'};
static const char value_lfEscapement[]     = {'l','f','E','s','c','a','p','e','m','e','n','t','\0'};
static const char value_lfItalic[]         = {'l','f','I','t','a','l','i','c','\0'};
static const char value_lfOrientation[]    = {'l','f','O','r','i','e','n','t','a','t','i','o','n','\0'};
static const char value_lfOutPrecision[]   = {'l','f','O','u','t','P','r','e','c','i','s','i','o','n','\0'};
static const char value_lfPitchAndFamily[] = {'l','f','P','i','t','c','h','A','n','d','F','a','m','i','l','y','\0'};
static const char value_lfQuality[]        = {'l','f','Q','u','a','l','i','t','y','\0'};
static const char value_lfStrikeOut[]      = {'l','f','S','t','r','i','k','e','O','u','t','\0'};
static const char value_lfUnderline[]      = {'l','f','U','n','d','e','r','l','i','n','e','\0'};
static const char value_lfWeight[]         = {'l','f','W','e','i','g','h','t','\0'};
static const char value_lfFaceName[]       = {'l','f','F','a','c','e','N','a','m','e','\0'};
static const char value_iMarginTop[]       = {'i','M','a','r','g','i','n','T','o','p','\0'};
static const char value_iMarginBottom[]    = {'i','M','a','r','g','i','n','B','o','t','t','o','m','\0'};
static const char value_iMarginLeft[]      = {'i','M','a','r','g','i','n','L','e','f','t','\0'};
static const char value_iMarginRight[]     = {'i','M','a','r','g','i','n','R','i','g','h','t','\0'};
static const char value_szHeader[]         = {'s','z','H','e','a','d','e','r','\0'};
static const char value_szFooter[]         = {'s','z','T','r','a','i','l','e','r','\0'};

/***********************************************************************
 *
 *           SetFileName
 *
 *  Sets Global File Name.
 */
VOID SetFileName(LPCSTR szFileName)
{
	char ext[_MAX_EXT];
	char fname[_MAX_FNAME];
	char path_buffer[_MAX_PATH];

	lstrcpy(Globals.szFileName, szFileName);
	Globals.szFileTitle[0] = 0;
	lstrcpy(path_buffer, szFileName);
	_splitpath(path_buffer, NULL, NULL, fname, ext );
	lstrcat(Globals.szFileTitle, fname);
	lstrcat(Globals.szFileTitle, ext);
}

#if 0
/******************************************************************************
 *      get_dpi
 *
 * Get the dpi from registry HKCC\Software\Fonts\LogPixels.
 */
DWORD get_dpi(void)
{
    static const char dpi_key_name[] = {'S','o','f','t','w','a','r','e','\\','F','o','n','t','s','\0'};
    static const char dpi_value_name[] = {'L','o','g','P','i','x','e','l','s','\0'};
    DWORD dpi = 96;
    HKEY hkey;

    if (RegOpenKey(HKEY_CURRENT_CONFIG, dpi_key_name, &hkey) == ERROR_SUCCESS)
    {
        DWORD type, size, new_dpi;

        size = sizeof(new_dpi);
        if(RegQueryValueEx(hkey, dpi_value_name, NULL, &type, (void *)&new_dpi, &size) == ERROR_SUCCESS)
        {
            if(type == REG_DWORD && new_dpi != 0)
                dpi = new_dpi;
        }
        RegCloseKey(hkey);
    }
    return dpi;
}

/***********************************************************************
 *
 *           NOTEPAD_SaveSettingToRegistry
 *
 *  Save setting to registry HKCU\Software\Microsoft\Notepad.
 */
static VOID NOTEPAD_SaveSettingToRegistry(void)
{
    HKEY hkey;
    DWORD disp;

    if(RegCreateKeyEx(HKEY_CURRENT_USER, notepad_reg_key, 0, NULL,
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &disp) == ERROR_SUCCESS)
    {
        DWORD data;

        GetWindowRect(Globals.hMainWnd, &main_rect);

#define SET_NOTEPAD_REG(hkey, value_name, value_data) do { DWORD data = (DWORD)(value_data); RegSetValueEx(hkey, value_name, 0, REG_DWORD, (LPBYTE)&data, sizeof(DWORD)); }while(0)
        SET_NOTEPAD_REG(hkey, value_fWrap,            Globals.bWrapLongLines);
        SET_NOTEPAD_REG(hkey, value_iWindowPosX,      main_rect.left);
        SET_NOTEPAD_REG(hkey, value_iWindowPosY,      main_rect.top);
        SET_NOTEPAD_REG(hkey, value_iWindowPosDX,     main_rect.right - main_rect.left);
        SET_NOTEPAD_REG(hkey, value_iWindowPosDY,     main_rect.bottom - main_rect.top);
        SET_NOTEPAD_REG(hkey, value_lfCharSet,        Globals.lfFont.lfCharSet);
        SET_NOTEPAD_REG(hkey, value_lfClipPrecision,  Globals.lfFont.lfClipPrecision);
        SET_NOTEPAD_REG(hkey, value_lfEscapement,     Globals.lfFont.lfEscapement);
        SET_NOTEPAD_REG(hkey, value_lfItalic,         Globals.lfFont.lfItalic);
        SET_NOTEPAD_REG(hkey, value_lfOrientation,    Globals.lfFont.lfOrientation);
        SET_NOTEPAD_REG(hkey, value_lfOutPrecision,   Globals.lfFont.lfOutPrecision);
        SET_NOTEPAD_REG(hkey, value_lfPitchAndFamily, Globals.lfFont.lfPitchAndFamily);
        SET_NOTEPAD_REG(hkey, value_lfQuality,        Globals.lfFont.lfQuality);
        SET_NOTEPAD_REG(hkey, value_lfStrikeOut,      Globals.lfFont.lfStrikeOut);
        SET_NOTEPAD_REG(hkey, value_lfUnderline,      Globals.lfFont.lfUnderline);
        SET_NOTEPAD_REG(hkey, value_lfWeight,         Globals.lfFont.lfWeight);
        SET_NOTEPAD_REG(hkey, value_iMarginTop,       Globals.iMarginTop);
        SET_NOTEPAD_REG(hkey, value_iMarginBottom,    Globals.iMarginBottom);
        SET_NOTEPAD_REG(hkey, value_iMarginLeft,      Globals.iMarginLeft);
        SET_NOTEPAD_REG(hkey, value_iMarginRight,     Globals.iMarginRight);
#undef SET_NOTEPAD_REG

        /* Store the current value as 10 * twips */
        data = MulDiv(abs(Globals.lfFont.lfHeight), 720 , get_dpi());
        RegSetValueEx(hkey, value_iPointSize, 0, REG_DWORD, (LPBYTE)&data, sizeof(DWORD));

        RegSetValueEx(hkey, value_lfFaceName, 0, REG_SZ, (LPBYTE)&Globals.lfFont.lfFaceName,
                      lstrlen(Globals.lfFont.lfFaceName) * sizeof(Globals.lfFont.lfFaceName[0]));

        RegSetValueEx(hkey, value_szHeader, 0, REG_SZ, (LPBYTE)&Globals.szHeader,
                      lstrlen(Globals.szHeader) * sizeof(Globals.szHeader[0]));

        RegSetValueEx(hkey, value_szFooter, 0, REG_SZ, (LPBYTE)&Globals.szFooter,
                      lstrlen(Globals.szFooter) * sizeof(Globals.szFooter[0]));

        RegCloseKey(hkey);
    }
}

/***********************************************************************
 *
 *           NOTEPAD_LoadSettingFromRegistry
 *
 *  Load setting from registry HKCU\Software\Microsoft\Notepad.
 */
static VOID NOTEPAD_LoadSettingFromRegistry(void)
{
    static const char systemW[] = { 'S','y','s','t','e','m','\0' };
    HKEY hkey;
    INT base_length, dx, dy;

    base_length = (GetSystemMetrics(SM_CXSCREEN) > GetSystemMetrics(SM_CYSCREEN))?
        GetSystemMetrics(SM_CYSCREEN) : GetSystemMetrics(SM_CXSCREEN);

    dx = base_length * .95;
    dy = dx * 3 / 4;
    SetRect( &main_rect, 0, 0, dx, dy );

    Globals.bWrapLongLines  = TRUE;
    Globals.iMarginTop = 2500;
    Globals.iMarginBottom = 2500;
    Globals.iMarginLeft = 2000;
    Globals.iMarginRight = 2000;
    
    Globals.lfFont.lfHeight         = -12;
    Globals.lfFont.lfWidth          = 0;
    Globals.lfFont.lfEscapement     = 0;
    Globals.lfFont.lfOrientation    = 0;
    Globals.lfFont.lfWeight         = FW_REGULAR;
    Globals.lfFont.lfItalic         = FALSE;
    Globals.lfFont.lfUnderline      = FALSE;
    Globals.lfFont.lfStrikeOut      = FALSE;
    Globals.lfFont.lfCharSet        = DEFAULT_CHARSET;
    Globals.lfFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    Globals.lfFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    Globals.lfFont.lfQuality        = DEFAULT_QUALITY;
    Globals.lfFont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    lstrcpy(Globals.lfFont.lfFaceName, systemW);

    LoadString(Globals.hInstance, STRING_PAGESETUP_HEADERVALUE, Globals.szHeader,
               sizeof(Globals.szHeader) / sizeof(Globals.szHeader[0]));
    LoadString(Globals.hInstance, STRING_PAGESETUP_FOOTERVALUE, Globals.szFooter,
               sizeof(Globals.szFooter) / sizeof(Globals.szFooter[0]));

    if(RegOpenKey(HKEY_CURRENT_USER, notepad_reg_key, &hkey) == ERROR_SUCCESS)
    {
        WORD  data_helper[MAX_PATH];
        DWORD type, data, size;

#define QUERY_NOTEPAD_REG(hkey, value_name, ret) do { DWORD type, data; DWORD size = sizeof(DWORD); if(RegQueryValueEx(hkey, value_name, 0, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS) if(type == REG_DWORD) ret = (typeof(ret))data; } while(0)
        QUERY_NOTEPAD_REG(hkey, value_fWrap,            Globals.bWrapLongLines);
        QUERY_NOTEPAD_REG(hkey, value_iWindowPosX,      main_rect.left);
        QUERY_NOTEPAD_REG(hkey, value_iWindowPosY,      main_rect.top);
        QUERY_NOTEPAD_REG(hkey, value_iWindowPosDX,     dx);
        QUERY_NOTEPAD_REG(hkey, value_iWindowPosDY,     dy);
        QUERY_NOTEPAD_REG(hkey, value_lfCharSet,        Globals.lfFont.lfCharSet);
        QUERY_NOTEPAD_REG(hkey, value_lfClipPrecision,  Globals.lfFont.lfClipPrecision);
        QUERY_NOTEPAD_REG(hkey, value_lfEscapement,     Globals.lfFont.lfEscapement);
        QUERY_NOTEPAD_REG(hkey, value_lfItalic,         Globals.lfFont.lfItalic);
        QUERY_NOTEPAD_REG(hkey, value_lfOrientation,    Globals.lfFont.lfOrientation);
        QUERY_NOTEPAD_REG(hkey, value_lfOutPrecision,   Globals.lfFont.lfOutPrecision);
        QUERY_NOTEPAD_REG(hkey, value_lfPitchAndFamily, Globals.lfFont.lfPitchAndFamily);
        QUERY_NOTEPAD_REG(hkey, value_lfQuality,        Globals.lfFont.lfQuality);
        QUERY_NOTEPAD_REG(hkey, value_lfStrikeOut,      Globals.lfFont.lfStrikeOut);
        QUERY_NOTEPAD_REG(hkey, value_lfUnderline,      Globals.lfFont.lfUnderline);
        QUERY_NOTEPAD_REG(hkey, value_lfWeight,         Globals.lfFont.lfWeight);
        QUERY_NOTEPAD_REG(hkey, value_iMarginTop,       Globals.iMarginTop);
        QUERY_NOTEPAD_REG(hkey, value_iMarginBottom,    Globals.iMarginBottom);
        QUERY_NOTEPAD_REG(hkey, value_iMarginLeft,      Globals.iMarginLeft);
        QUERY_NOTEPAD_REG(hkey, value_iMarginRight,     Globals.iMarginRight);
#undef QUERY_NOTEPAD_REG

        main_rect.right = main_rect.left + dx;
        main_rect.bottom = main_rect.top + dy;

        size = sizeof(DWORD);
        if(RegQueryValueEx(hkey, value_iPointSize, 0, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS)
            if(type == REG_DWORD)
                /* The value is stored as 10 * twips */
                Globals.lfFont.lfHeight = -MulDiv(abs(data), get_dpi(), 720);

        size = sizeof(Globals.lfFont.lfFaceName);
        if(RegQueryValueEx(hkey, value_lfFaceName, 0, &type, (LPBYTE)&data_helper, &size) == ERROR_SUCCESS)
            if(type == REG_SZ)
                lstrcpy(Globals.lfFont.lfFaceName, data_helper);
        
        size = sizeof(Globals.szHeader);
        if(RegQueryValueEx(hkey, value_szHeader, 0, &type, (LPBYTE)&data_helper, &size) == ERROR_SUCCESS)
            if(type == REG_SZ)
                lstrcpy(Globals.szHeader, data_helper);

        size = sizeof(Globals.szFooter);
        if(RegQueryValueEx(hkey, value_szFooter, 0, &type, (LPBYTE)&data_helper, &size) == ERROR_SUCCESS)
            if(type == REG_SZ)
                lstrcpy(Globals.szFooter, data_helper);
        RegCloseKey(hkey);
    }
}
#endif

/***********************************************************************
 *
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */
static int NOTEPAD_MenuCommand(WPARAM wParam)
{
    switch (wParam)
    {
    case CMD_NEW:               DIALOG_FileNew(); break;
    case CMD_OPEN:              DIALOG_FileOpen(); break;
    case CMD_SAVE:              DIALOG_FileSave(); break;
    case CMD_SAVE_AS:           DIALOG_FileSaveAs(); break;
    case CMD_PRINT:             DIALOG_FilePrint(); break;
    case CMD_PAGE_SETUP:        DIALOG_FilePageSetup(); break;
    case CMD_PRINTER_SETUP:     DIALOG_FilePrinterSetup();break;
    case CMD_EXIT:              DIALOG_FileExit(); break;

    case CMD_UNDO:             DIALOG_EditUndo(); break;
    case CMD_CUT:              DIALOG_EditCut(); break;
    case CMD_COPY:             DIALOG_EditCopy(); break;
    case CMD_PASTE:            DIALOG_EditPaste(); break;
    case CMD_DELETE:           DIALOG_EditDelete(); break;
    case CMD_SELECT_ALL:       DIALOG_EditSelectAll(); break;
    case CMD_TIME_DATE:        DIALOG_EditTimeDate();break;

    case CMD_SEARCH:           DIALOG_Search(); break;
    case CMD_SEARCH_NEXT:      DIALOG_SearchNext(); break;
                               
    case CMD_WRAP:             DIALOG_EditWrap(); break;
    case CMD_FONT:             DIALOG_SelectFont(); break;

    case CMD_HELP_CONTENTS:    DIALOG_HelpContents(); break;
    case CMD_HELP_SEARCH:      DIALOG_HelpSearch(); break;
    case CMD_HELP_ON_HELP:     DIALOG_HelpHelp(); break;
    case CMD_HELP_ABOUT_NOTEPAD: DIALOG_HelpAboutNotepad(); break;

    default:
	break;
    }
   return 0;
}

/***********************************************************************
 * Data Initialization
 */
static VOID NOTEPAD_InitData(VOID)
{
    LPSTR p = Globals.szFilter;

    LoadString(Globals.hInstance, STRING_TEXT_FILES_TXT, p, MAX_STRING_LEN);
    p += lstrlen(p) + 1;
    lstrcpy(p, "*.txt");
    p += lstrlen(p) + 1;
    LoadString(Globals.hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN);
    p += lstrlen(p) + 1;
    lstrcpy(p, "*.*");
    p += lstrlen(p) + 1;
    *p = '\0';
    Globals.hDevMode = NULL;
    Globals.hDevNames = NULL;

    CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_WRAP,
            MF_BYCOMMAND | (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
}

/***********************************************************************
 * Enable/disable items on the menu based on control state
 */
static VOID NOTEPAD_InitMenuPopup(HMENU menu, int index)
{
    int enable;

    EnableMenuItem(menu, CMD_UNDO,
        SendMessage(Globals.hEdit, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(menu, CMD_PASTE,
        IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED);
    enable = SendMessage(Globals.hEdit, EM_GETSEL, 0, 0);
    enable = (HIWORD(enable) == LOWORD(enable)) ? MF_GRAYED : MF_ENABLED;
    EnableMenuItem(menu, CMD_CUT, enable);
    EnableMenuItem(menu, CMD_COPY, enable);
    EnableMenuItem(menu, CMD_DELETE, enable);
    
    EnableMenuItem(menu, CMD_SELECT_ALL,
        GetWindowTextLength(Globals.hEdit) ? MF_ENABLED : MF_GRAYED);
}

static LPSTR NOTEPAD_StrRStr(LPSTR pszSource, LPSTR pszLast, LPSTR pszSrch)
{
    int len = lstrlen(pszSrch);

    pszLast--;
    while ((pszLast - pszSource) >=0)
    {
        if (_fstrncmp(pszLast, pszSrch, len) == 0)
            return pszLast;
        pszLast--;
    }
    return NULL;
}

/***********************************************************************
 * The user activated the Find dialog
 */
void NOTEPAD_DoFind(FINDREPLACE FAR *fr)
{
	LPSTR content;
	LPSTR found;
	LPSTR lpstrFindWhat;
	int len = lstrlen(fr->lpstrFindWhat);
	int fileLen;
	DWORD pos;
    
	fileLen = GetWindowTextLength(Globals.hEdit) + 1;
	content = GlobalAllocPtr(GPTR, fileLen);
	if (!content) return;
	GetWindowText(Globals.hEdit, content, fileLen);

	lpstrFindWhat=GlobalAllocPtr(GPTR,len+1);
	lstrcpy(lpstrFindWhat, fr->lpstrFindWhat);

	pos=HIWORD(SendMessage(Globals.hEdit, EM_GETSEL, 0, 0));

	switch (fr->Flags & (FR_DOWN|FR_MATCHCASE))
	{
		case 0:
			found = NOTEPAD_StrRStr(_fstrupr(content), _fstrupr(content+pos-len), _fstrupr(lpstrFindWhat));
			break;
		case FR_DOWN:
			found = _fstrstr(_fstrupr(content+pos), _fstrupr(lpstrFindWhat));
			break;
		case FR_MATCHCASE:                 
			found = NOTEPAD_StrRStr(content, content+pos-len, lpstrFindWhat);
			break;
		case FR_DOWN|FR_MATCHCASE:
			found = _fstrstr(content+pos, lpstrFindWhat);
			break;
		default:    /* shouldn't happen */
			return;
	}
	GlobalFreePtr(content);
	GlobalFreePtr(lpstrFindWhat);

	if (found == NULL)
	{
		DIALOG_StringMsgBox(Globals.hFindReplaceDlg, STRING_NOTFOUND, fr->lpstrFindWhat, MB_ICONINFORMATION|MB_OK);
		return;
	}

	SendMessage(Globals.hEdit, EM_SETSEL, 1, MAKELPARAM(found - content,found - content + len));
}

/***********************************************************************
 *
 *           NOTEPAD_WndProc
 */
static LRESULT WINAPI NOTEPAD_WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                               LPARAM lParam)
{
    if (msg == aFINDMSGSTRING)      /* not a constant so can't be used in switch */
    {
        FINDREPLACE FAR *fr = (FINDREPLACE FAR *)lParam;
        
        if (fr->Flags & FR_DIALOGTERM)
            Globals.hFindReplaceDlg = NULL;
        if (fr->Flags & FR_FINDNEXT)
        {
            Globals.lastFind = *fr;
            NOTEPAD_DoFind(fr);
        }
        return 0;
    }
    
    switch (msg) {

    case WM_CREATE:
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                        ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL;
        RECT rc;

        GetClientRect(hWnd, &rc);

        if (!Globals.bWrapLongLines) dwStyle |= WS_HSCROLL | ES_AUTOHSCROLL;

        Globals.hEdit = CreateWindow("Edit",
				NULL,
				dwStyle,
				0, 0, rc.right, rc.bottom, 
				hWnd,
				NULL, Globals.hInstance, NULL);

        Globals.hFont = CreateFontIndirect(&Globals.lfFont);
        SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, (LPARAM)FALSE);
        break;
    }

    case WM_COMMAND:
        NOTEPAD_MenuCommand(LOWORD(wParam));
        break;

    case WM_DESTROYCLIPBOARD:
        /*MessageBox(Globals.hMainWnd, "Empty clipboard", "Debug", MB_ICONEXCLAMATION);*/
        break;

    case WM_CLOSE:
        if (DoCloseFile()) {
            DestroyWindow(hWnd);
        }
        break;

    case WM_QUERYENDSESSION:
        if (DoCloseFile()) {
            return 1;
        }
        break;

    case WM_DESTROY:
        //NOTEPAD_SaveSettingToRegistry();

        PostQuitMessage(0);
        break;

    case WM_SIZE:
        SetWindowPos(Globals.hEdit, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam),
                     SWP_NOOWNERZORDER | SWP_NOZORDER);
        break;

    case WM_SETFOCUS:
        SetFocus(Globals.hEdit);
        break;

    case WM_DROPFILES:
    {
        char szFileName[MAX_PATH];
        HANDLE hDrop = (HANDLE) wParam;

        DragQueryFile(hDrop, 0, szFileName, SIZEOF(szFileName));
        DragFinish(hDrop);
        DoOpenFile(szFileName);
        break;
    }
    
    case WM_INITMENUPOPUP:
        NOTEPAD_InitMenuPopup((HMENU)wParam, lParam);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static int AlertFileDoesNotExist(LPCSTR szFileName)
{
   int nResult;
   char szMessage[MAX_STRING_LEN];
   char szResource[MAX_STRING_LEN];

   LoadString(Globals.hInstance, STRING_DOESNOTEXIST, szResource, SIZEOF(szResource));
   wsprintf(szMessage, szResource, szFileName);

   LoadString(Globals.hInstance, STRING_ERROR, szResource, SIZEOF(szResource));

   nResult = MessageBox(Globals.hMainWnd, szMessage, szResource,
                        MB_ICONEXCLAMATION | MB_YESNO);

   return(nResult);
}

static void HandleCommandLine(LPSTR cmdline)
{
    char delimiter;
    int opt_print=0;
    
    /* skip white space */
    while (*cmdline == ' ') cmdline++;

    /* skip executable name */
    delimiter = (*cmdline == '"' ? '"' : ' ');

    if (*cmdline == delimiter) cmdline++;

    while (*cmdline && *cmdline != delimiter) cmdline++;

    if (*cmdline == delimiter) cmdline++;

    while (*cmdline == ' ' || *cmdline == '-' || *cmdline == '/')
    {
        char option;

        if (*cmdline++ == ' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline == ' ') cmdline++;

        switch(option)
        {
            case 'p':
            case 'P':
                opt_print=1;
                break;
        }
    }

    if (*cmdline)
    {
        /* file name is passed in the command line */
        LPCSTR file_name;
        BOOL file_exists;
        char buf[MAX_PATH];

        if (cmdline[0] == '"')
        {
            char far* wc;
            cmdline++;
            wc=cmdline;
            /* Note: Double-quotes are not allowed in Windows filenames */
            while (*wc && *wc != '"') wc++;
            /* On Windows notepad ignores further arguments too */
            *wc = 0;
        }

        if (FileExists(cmdline))
        {
            file_exists = TRUE;
            file_name = cmdline;
        }
        else
        {
            static const char txt[] = ".txt";

            /* try to find file with ".txt" extension */
            if (!lstrcmp(txt, cmdline + lstrlen(cmdline) - lstrlen(txt)))
            {
                file_exists = FALSE;
                file_name = cmdline;
            }
            else
            {
                lstrcpyn(buf, cmdline, MAX_PATH - lstrlen(txt) - 1);
                lstrcat(buf, txt);
                file_name = buf;
                file_exists = FileExists(buf);
            }
        }

        if (file_exists)
        {
            DoOpenFile(file_name);
            InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            if (opt_print)
                DIALOG_FilePrint();
        }
        else
        {
            switch (AlertFileDoesNotExist(file_name)) {
            case IDYES:
                DoOpenFile(file_name);
                break;

            case IDNO:
                break;
            }
        }
     }
}


BOOL NOTEPAD_RegisterMainWinClass(void)
{
	WNDCLASS class;
	
	class.style         = CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc   = NOTEPAD_WndProc;
	class.cbClsExtra    = 0;
	class.cbWndExtra    = 0;
	class.hInstance     = Globals.hInstance;
	class.hIcon         = LoadIcon(Globals.hInstance, MAKEINTRESOURCE(IDI_NOTEPAD));
	class.hCursor       = LoadCursor(0, (LPCSTR)IDC_ARROW);
	class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	class.lpszMenuName  = MAKEINTRESOURCE(MAIN_MENU);
	class.lpszClassName = "Notepad";

	return RegisterClass(&class);
}

/***********************************************************************
 *
 *           WinMain
 */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
	MSG        msg;
	HACCEL      hAccel;
	int x, y;

	aFINDMSGSTRING = RegisterWindowMessage("commdlg_FindReplace");

	/* Setup Globals */
	memset(&Globals, 0, sizeof (Globals));
	Globals.hInstance       = hInstance;

	/* Read application configuration */

	/* Read Options from `win.ini' */
	GetProfileString("intl", "s1159", "AM", Globals.s1159, sizeof(Globals.s1159));
	GetProfileString("intl", "s2359", "PM", Globals.s2359, sizeof(Globals.s2359));
	GetProfileString("intl", "sTime", ":", Globals.sTime, sizeof(Globals.sTime));
	Globals.iTime=GetProfileInt("intl", "iTime", 0);
	Globals.iTLZero=GetProfileInt("intl", "iTLZero", 0);
	GetProfileString("intl", "sDate", "/", Globals.sDate, sizeof(Globals.sDate));
	GetProfileString("intl", "sShortDate", "MM/dd/yy", Globals.sShortDate, sizeof(Globals.sShortDate));

	//NOTEPAD_LoadSettingFromRegistry();

	if (!prev)
	{
		if (!NOTEPAD_RegisterMainWinClass()) return(FALSE);
	}

	/* Setup windows */
        x = y = CW_USEDEFAULT;

	Globals.hMainWnd = CreateWindow("Notepad", "Notepad", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
					x, y,
					CW_USEDEFAULT, CW_USEDEFAULT, 
					0,
					0, Globals.hInstance, 0);

	if (!Globals.hMainWnd)
	{
	        //ShowLastError();
	        //ExitProcess(1);
		return(1);
	}


	NOTEPAD_InitData();
	DIALOG_FileNew();

	ShowWindow(Globals.hMainWnd, show);
	UpdateWindow(Globals.hMainWnd);
	DragAcceptFiles(Globals.hMainWnd, TRUE);

	HandleCommandLine(cmdline);

	hAccel = LoadAccelerators( hInstance, MAKEINTRESOURCE(ID_ACCEL) );

	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!TranslateAccelerator(Globals.hMainWnd, hAccel, &msg) && !IsDialogMessage(Globals.hFindReplaceDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}
