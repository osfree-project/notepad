/*
 *  Notepad (dialog.c)
 *
 *  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
 *  Copyright 2007 Rolf Kalbermatter
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
 */

//#define UNICODE

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <dos.h>

#include <windows.h>
#include <commdlg.h>

#include "main.h"
#include "dialog.h"

#define SPACES_IN_TAB 8
#define PRINT_LEN_MAX 500

static const char helpfileW[] = { 'n','o','t','e','p','a','d','.','h','l','p',0 };

static int WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

VOID ShowLastError(void)
{
//    DWORD error = GetLastError();
	/*
    if (error != NO_ERROR)
    {
        LPSTR lpMsgBuf;
        char szTitle[MAX_STRING_LEN];

        LoadString(Globals.hInstance, STRING_ERROR, szTitle, SIZEOF(szTitle));
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, error, 0,
            (LPSTR) &lpMsgBuf, 0, NULL);
        MessageBox(NULL, lpMsgBuf, szTitle, MB_OK | MB_ICONERROR);
        LocalFree(lpMsgBuf);
    }
	*/
}

/**
 * Sets the caption of the main window according to Globals.szFileTitle:
 *    Untitled - Notepad        if no file is open
 *    filename - Notepad        if a file is given
 */
static void UpdateWindowCaption(void)
{
  char szCaption[MAX_STRING_LEN];
  char szNotepad[MAX_STRING_LEN];

  if (Globals.szFileTitle[0] != '\0')
      lstrcpy(szCaption, Globals.szFileTitle);
  else
      LoadString(Globals.hInstance, STRING_UNTITLED, szCaption, SIZEOF(szCaption));

  LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, SIZEOF(szNotepad));
  lstrcat(szCaption, " - ");
  lstrcat(szCaption, szNotepad);

  SetWindowText(Globals.hMainWnd, szCaption);
}

int DIALOG_StringMsgBox(HWND hParent, int formatId, LPCSTR szString, DWORD dwFlags)
{
   char szMessage[MAX_STRING_LEN];
   char szResource[MAX_STRING_LEN];

   /* Load and format szMessage */
   LoadString(Globals.hInstance, formatId, szResource, SIZEOF(szResource));
   wsprintf(szMessage,  szResource, szString);//SIZEOF(szMessage),

   /* Load szCaption */
   if ((dwFlags & MB_ICONMASK) == MB_ICONEXCLAMATION)
     LoadString(Globals.hInstance, STRING_ERROR,  szResource, SIZEOF(szResource));
   else
     LoadString(Globals.hInstance, STRING_NOTEPAD,  szResource, SIZEOF(szResource));

   /* Display Modal Dialog */
   if (hParent == 0)
     hParent = Globals.hMainWnd;
   return MessageBox(hParent, szMessage, szResource, dwFlags);
}

static void AlertFileNotFound(LPCSTR szFileName)
{
   DIALOG_StringMsgBox(0, STRING_NOTFOUND, szFileName, MB_ICONEXCLAMATION|MB_OK);
}

static int AlertFileNotSaved(LPCSTR szFileName)
{
   char szUntitled[MAX_STRING_LEN];

   LoadString(Globals.hInstance, STRING_UNTITLED, szUntitled, SIZEOF(szUntitled));
   return DIALOG_StringMsgBox(0, STRING_NOTSAVED, szFileName[0] ? szFileName : szUntitled,
     MB_ICONQUESTION|MB_YESNOCANCEL);
}

/**
 * Returns:
 *   TRUE  - if file exists
 *   FALSE - if file does not exist
 */
BOOL FileExists(LPCSTR szFilename)
{
	struct find_t entry;
	char buf[MAX_PATH];

	lstrcpy(buf, szFilename);
	return !_dos_findfirst(buf, _A_NORMAL, &entry);
}


static VOID DoSaveFile(VOID)
{
	HANDLE hFile;
	DWORD dwNumWrite;
	LPSTR pTemp;
	DWORD size;


	hFile=_lcreat(Globals.szFileName, 0);
	
	if(hFile == HFILE_ERROR)
	{
		ShowLastError();
		return;
	}

	size = GetWindowTextLength(Globals.hEdit) + 1;
	pTemp = GlobalAllocPtr(GPTR, size);
	if (!pTemp)
	{
		_lclose(hFile);
			ShowLastError();
		return;
	}
	size = GetWindowText(Globals.hEdit, pTemp, size);

	if ((dwNumWrite=_lwrite(hFile, pTemp, size))==HFILE_ERROR)
		ShowLastError();
	else
		SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0);

	//SetEndOfFile(hFile);
	_lclose(hFile);
	GlobalFreePtr(pTemp);
}

/**
 * Returns:
 *   TRUE  - User agreed to close (both save/don't save)
 *   FALSE - User cancelled close by selecting "Cancel"
 */
BOOL DoCloseFile(void)
{
    int nResult;
    static const char empty_strW[] = { 0 };

    if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0))
    {
        /* prompt user to save changes */
        nResult = AlertFileNotSaved(Globals.szFileName);
        switch (nResult) {
            case IDYES:     DIALOG_FileSave();
                            break;

            case IDNO:      break;

            case IDCANCEL:  return(FALSE);

            default:        return(FALSE);
        } /* switch */
    } /* if */

    SetFileName(empty_strW);

    UpdateWindowCaption();
    return(TRUE);
}


void DoOpenFile(LPCSTR szFileName)
{
	HANDLE hFile;
	LPSTR pTemp;
	DWORD size;
	DWORD dwNumRead;
	char log[5];

	/* Close any files and prompt to save changes */
	if (!DoCloseFile())
		return;

	hFile = _lopen(szFileName, READ | OF_SHARE_DENY_WRITE);

	if(hFile == HFILE_ERROR)
	{
		AlertFileNotFound(szFileName);
		return;
	}

	size = _llseek(hFile, 0, SEEK_END);
	if (size == HFILE_ERROR)
	{
		_lclose(hFile);
		ShowLastError();
		return;
	}
	size++;

	_llseek(hFile, 0, SEEK_SET);

	pTemp = GlobalAllocPtr(GPTR, size);
	if (!pTemp)
	{
		_lclose(hFile);
		ShowLastError();
		return;
	}

	if ((dwNumRead=_lread(hFile, pTemp, size))==HFILE_ERROR)
	{
		_lclose(hFile);
		GlobalFreePtr(pTemp);
		ShowLastError();
		return;
	}

	_lclose(hFile);
	pTemp[dwNumRead] = 0;

	SetWindowText(Globals.hEdit, pTemp);

	GlobalFreePtr(pTemp);

	SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0);
	SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
	SetFocus(Globals.hEdit);
    
	/*  If the file starts with .LOG, add a time/date at the end and set cursor after */
	if (GetWindowText(Globals.hEdit, log, sizeof(log)/sizeof(log[0])) && !lstrcmp(log, ".LOG"))
	{
		int len = GetWindowTextLength(Globals.hEdit);
		SendMessage(Globals.hEdit, EM_SETSEL, 1, MAKELPARAM(len, len));
//		SendMessage(Globals.hEdit, EM_SETSEL, GetWindowTextLength(Globals.hEdit), -1);
		SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)"\r\n");
		DIALOG_EditTimeDate();
		SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)"\r\n");
	}

	SetFileName(szFileName);
	UpdateWindowCaption();
}

VOID DIALOG_FileNew(VOID)
{
    /* Close any files and prompt to save changes */
    if (DoCloseFile()) {
        SetWindowText(Globals.hEdit, "");
        SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
        SetFocus(Globals.hEdit);
    }
}

VOID DIALOG_FileOpen(VOID)
{
	OPENFILENAME openfilename = {0};
	char szPath[MAX_PATH] = {0};
	char szFile[MAX_PATH] = {0};

	openfilename.lStructSize       = sizeof(openfilename);
	openfilename.hwndOwner         = Globals.hMainWnd;
	openfilename.hInstance         = Globals.hInstance;
	openfilename.lpstrFilter       = Globals.szFilter;
	openfilename.lpstrFile         = szPath;
	openfilename.nMaxFile          = sizeof(szPath);
	openfilename.lpstrFileTitle    = szFile;
	openfilename.nMaxFileTitle     = sizeof(szFile);
	openfilename.Flags             = OFN_FILEMUSTEXIST;// | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	openfilename.lpstrDefExt       = "txt";


	if (GetOpenFileName(&openfilename))
	{
        	DoOpenFile(openfilename.lpstrFile);
	}
}


VOID DIALOG_FileSave(VOID)
{
    if (Globals.szFileName[0] == '\0')
        DIALOG_FileSaveAs();
    else
        DoSaveFile();
}

VOID DIALOG_FileSaveAs(VOID)
{
	OPENFILENAME saveas = {0};
	char szPath[MAX_PATH] = {0};

	saveas.lStructSize       = sizeof(OPENFILENAME);
	saveas.hwndOwner         = Globals.hMainWnd;
	saveas.hInstance         = Globals.hInstance;
	saveas.lpstrFilter       = Globals.szFilter;
	saveas.lpstrFile         = szPath;
	saveas.nMaxFile          = SIZEOF(szPath);
	saveas.lpstrInitialDir   = NULL;
	saveas.Flags             = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	saveas.lpstrDefExt       = "txt";

	if (GetSaveFileName(&saveas)) {
		SetFileName(szPath);
		UpdateWindowCaption();
		DoSaveFile();
	}
}

typedef struct {
    LPSTR mptr;
    LPSTR mend;
    LPSTR lptr;
    DWORD len;
} TEXTINFO, *LPTEXTINFO;

static int notepad_print_header(HDC hdc, RECT *rc, BOOL dopage, BOOL header, int page, LPSTR text)
{
    SIZE szMetric;

    if (*text)
    {
        /* Write the header or footer */
        GetTextExtentPoint(hdc, text, lstrlen(text), &szMetric);
        if (dopage)
            ExtTextOut(hdc, (rc->left + rc->right - szMetric.cx) / 2,
                       header ? rc->top : rc->bottom - szMetric.cy,
                       ETO_CLIPPED, rc, text, lstrlen(text), NULL);
        return 1;
    }
    return 0;
}

static BOOL notepad_print_page(HDC hdc, RECT *rc, BOOL dopage, int page, LPTEXTINFO tInfo)
{
    int b, y;
    TEXTMETRIC tm;
    SIZE szMetrics;

    if (dopage)
    {
        if (StartPage(hdc) <= 0)
        {
            static const char failedW[] = { 'S','t','a','r','t','P','a','g','e',' ','f','a','i','l','e','d',0 };
            static const char errorW[] = { 'P','r','i','n','t',' ','E','r','r','o','r',0 };
            MessageBox(Globals.hMainWnd, failedW, errorW, MB_ICONEXCLAMATION);
            return FALSE;
        }
    }

    GetTextMetrics(hdc, &tm);
    y = rc->top + notepad_print_header(hdc, rc, dopage, TRUE, page, Globals.szFileName) * tm.tmHeight;
    b = rc->bottom - 2 * notepad_print_header(hdc, rc, FALSE, FALSE, page, Globals.szFooter) * tm.tmHeight;

    do {
        int m, n;

        if (!tInfo->len)
        {
            /* find the end of the line */
            while (tInfo->mptr < tInfo->mend && *tInfo->mptr != '\n' && *tInfo->mptr != '\r')
            {
                if (*tInfo->mptr == '\t')
                {
                    /* replace tabs with spaces */
                    for (m = 0; m < SPACES_IN_TAB; m++)
                    {
                        if (tInfo->len < PRINT_LEN_MAX)
                            tInfo->lptr[tInfo->len++] = ' ';
                        else if (Globals.bWrapLongLines)
                            break;
                    }
                }
                else if (tInfo->len < PRINT_LEN_MAX)
                    tInfo->lptr[tInfo->len++] = *tInfo->mptr;

                if (tInfo->len >= PRINT_LEN_MAX && Globals.bWrapLongLines)
                     break;

                tInfo->mptr++;
            }
        }

        /* Find out how much we should print if line wrapping is enabled */
        if (Globals.bWrapLongLines)
        {
            GetTextExtentPoint(hdc, tInfo->lptr, tInfo->len, /*rc->right - rc->left, &n, NULL,*/ &szMetrics);
            if (n < tInfo->len && tInfo->lptr[n] != ' ')
            {
                m = n;
                /* Don't wrap words unless it's a single word over the entire line */
                while (m  && tInfo->lptr[m] != ' ') m--;
                if (m > 0) n = m + 1;
            }
        }
        else
            n = tInfo->len;

        if (dopage)
            ExtTextOut(hdc, rc->left, y, ETO_CLIPPED, rc, tInfo->lptr, n, NULL);

        tInfo->len -= n;

        if (tInfo->len)
        {
            _fmemcpy(tInfo->lptr, tInfo->lptr + n, tInfo->len * sizeof(char));
            y += tm.tmHeight + tm.tmExternalLeading;
        }
        else
        {
            /* find the next line */
            while (tInfo->mptr < tInfo->mend && y < b && (*tInfo->mptr == '\n' || *tInfo->mptr == '\r'))
            {
                if (*tInfo->mptr == '\n')
                    y += tm.tmHeight + tm.tmExternalLeading;
                tInfo->mptr++;
            }
        }
    } while (tInfo->mptr < tInfo->mend && y < b);

    notepad_print_header(hdc, rc, dopage, FALSE, page, Globals.szFooter);
    if (dopage)
    {
        EndPage(hdc);
    }
    return TRUE;
}

VOID DIALOG_FilePrint(VOID)
{
    DOCINFO di;
    PRINTDLG printer;
    int page, dopage, copy;
    LOGFONT lfFont;
    HFONT hTextFont, old_font = 0;
    DWORD size;
    BOOL ret = FALSE;
    RECT rc;
    LPSTR pTemp;
    TEXTINFO tInfo;
    char cTemp[PRINT_LEN_MAX];

    /* Get Current Settings */
    memset(&printer, 0, sizeof(printer));
    printer.lStructSize           = sizeof(printer);
    printer.hwndOwner             = Globals.hMainWnd;
    printer.hDevMode              = Globals.hDevMode;
    printer.hDevNames             = Globals.hDevNames;
    printer.hInstance             = Globals.hInstance;

    /* Set some default flags */
    printer.Flags                 = PD_RETURNDC | PD_NOSELECTION;
    printer.nFromPage             = 0;
    printer.nMinPage              = 1;
    /* we really need to calculate number of pages to set nMaxPage and nToPage */
    printer.nToPage               = 0;
    printer.nMaxPage              = -1;
    /* Let commdlg manage copy settings */
    printer.nCopies               = (WORD)PD_USEDEVMODECOPIES;

    if (!PrintDlg(&printer)) return;

    Globals.hDevMode = printer.hDevMode;
    Globals.hDevNames = printer.hDevNames;

    SetMapMode(printer.hDC, MM_TEXT);

    /* initialize DOCINFO */
    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = Globals.szFileTitle;
    di.lpszOutput = NULL;
    //di.lpszDatatype = NULL;
    //di.fwType = 0; 

    /* Get the file text */
    size = GetWindowTextLength(Globals.hEdit) + 1;
    pTemp = GlobalAllocPtr(GPTR, size * sizeof(char));
    if (!pTemp)
    {
       DeleteDC(printer.hDC);
       ShowLastError();
       return;
    }
    size = GetWindowText(Globals.hEdit, pTemp, size);

    if (StartDoc(printer.hDC, &di) > 0)
    {
        /* Get the page margins in pixels. */
        /*
		rc.top =    MulDiv(Globals.iMarginTop, GetDeviceCaps(printer.hDC, LOGPIXELSY), 2540) -
                    GetDeviceCaps(printer.hDC, PHYSICALOFFSETY);
        rc.bottom = GetDeviceCaps(printer.hDC, PHYSICALHEIGHT) -
                    MulDiv(Globals.iMarginBottom, GetDeviceCaps(printer.hDC, LOGPIXELSY), 2540);
        rc.left =   MulDiv(Globals.iMarginLeft, GetDeviceCaps(printer.hDC, LOGPIXELSX), 2540) -
                    GetDeviceCaps(printer.hDC, PHYSICALOFFSETX);
        rc.right =  GetDeviceCaps(printer.hDC, PHYSICALWIDTH) -
                    MulDiv(Globals.iMarginRight, GetDeviceCaps(printer.hDC, LOGPIXELSX), 2540);
*/
        /* Create a font for the printer resolution */
        lfFont = Globals.lfFont;
        //lfFont.lfHeight = MulDiv(lfFont.lfHeight, GetDeviceCaps(printer.hDC, LOGPIXELSY), get_dpi());
        /* Make the font a bit lighter */
        lfFont.lfWeight -= 100;
        hTextFont = CreateFontIndirect(&lfFont);
        old_font = SelectObject(printer.hDC, hTextFont);

        for (copy = 1; copy <= printer.nCopies; copy++)
        {
            page = 1;

            tInfo.mptr = pTemp;
            tInfo.mend = pTemp + size;
            tInfo.lptr = cTemp;
            tInfo.len = 0;

            do {
                if (printer.Flags & PD_PAGENUMS)
                {
                    /* a specific range of pages is selected, so
                     * skip pages that are not to be printed
                     */
                    if (page > printer.nToPage)
                        break;
                    else if (page >= printer.nFromPage)
                        dopage = 1;
                    else
                        dopage = 0;
                }
                else
                    dopage = 1;

                ret = notepad_print_page(printer.hDC, &rc, dopage, page, &tInfo);
                page++;
            } while (ret && tInfo.mptr < tInfo.mend);

            if (!ret) break;
        }
        EndDoc(printer.hDC);
        SelectObject(printer.hDC, old_font);
        DeleteObject(hTextFont);
    }
    DeleteDC(printer.hDC);
    GlobalFreePtr(pTemp);
}

VOID DIALOG_FilePrinterSetup(VOID)
{
    PRINTDLG printer;

    memset(&printer, 0, sizeof(printer));
    printer.lStructSize         = sizeof(printer);
    printer.hwndOwner           = Globals.hMainWnd;
    printer.hDevMode            = Globals.hDevMode;
    printer.hDevNames           = Globals.hDevNames;
    printer.hInstance           = Globals.hInstance;
    printer.Flags               = PD_PRINTSETUP;
    printer.nCopies             = 1;

    PrintDlg(&printer);

    Globals.hDevMode = printer.hDevMode;
    Globals.hDevNames = printer.hDevNames;
}

VOID DIALOG_FileExit(VOID)
{
    PostMessage(Globals.hMainWnd, WM_CLOSE, 0, 0l);
}

VOID DIALOG_EditUndo(VOID)
{
    SendMessage(Globals.hEdit, EM_UNDO, 0, 0);
}

VOID DIALOG_EditCut(VOID)
{
    SendMessage(Globals.hEdit, WM_CUT, 0, 0);
}

VOID DIALOG_EditCopy(VOID)
{
    SendMessage(Globals.hEdit, WM_COPY, 0, 0);
}

VOID DIALOG_EditPaste(VOID)
{
    SendMessage(Globals.hEdit, WM_PASTE, 0, 0);
}

VOID DIALOG_EditDelete(VOID)
{
    SendMessage(Globals.hEdit, WM_CLEAR, 0, 0);
}

VOID DIALOG_EditSelectAll(VOID)
{
    int len = GetWindowTextLength(Globals.hEdit);
//    SendMessage(Globals.hEdit, EM_SETSEL, 0, (LPARAM)-1);
	SendMessage(Globals.hEdit, EM_SETSEL, 1, MAKELPARAM(0, len));
}

void FormatDate(char * szDate)
{
    struct dosdate_t d;
    char f[3][5]={"","",""};
	int i;
	char dFormat[20]="";
	char buf[20]="";

	strcpy(dFormat,"%[dMy]/%[dMy]/%[dMy]");

    _dos_getdate (&d);

	if (3==sscanf(Globals.sShortDate, dFormat, &f[0],&f[1],&f[2]))
	{
		for (i=0; i<3; i++)
		{
			if (!strcmp("d", f[i])) {
				sprintf(buf, "%d", d.day);
				strcat(szDate, buf);
			}
			else if (!strcmp("dd", f[i])) {
				sprintf(buf, "%02d", d.day);
				strcat(szDate, buf);
			}
			else if (!strcmp("M", f[i])) {
				sprintf(buf, "%d", d.month);
				strcat(szDate, buf);
			}
			else if (!strcmp("MM", f[i])) {
				sprintf(buf, "%02d", d.month);
				strcat(szDate, buf);
			}
			else if (!strcmp("yy", f[i])) {
				sprintf(buf, "%02d", d.year % 100);
				strcat(szDate, buf);
			}
			else if (!strcmp("yyyy", f[i])) {
				sprintf(buf, "%04d", d.year);
				strcat(szDate, buf);
			} else {
				//strcat(szDate, "error");
			};
			if (i<2) strcat(szDate, Globals.sDate);
		}
	}
}
 
void FormatTime(char * szTime)
{
	char tFormat[20]="";
	struct dostime_t t;
	int hour;
	char buf[3]="";

	_dos_gettime (&t);

	if (Globals.iTime) // 24h
	{
		hour=t.hour;
	} else {  // 12h
		hour=(t.hour % 12)?(t.hour % 12):12;
	}

	if (Globals.iTLZero) 
	{
		sprintf(tFormat, "%02d%s%02d%s", hour, Globals.sTime, t.minute, Globals.sTime);
	} else {
		if (hour<10)
		{
			sprintf(tFormat, "  %d%s%02d%s", hour, Globals.sTime, t.minute, Globals.sTime);
		} else {
			sprintf(tFormat, "%d%s%02d%s", hour, Globals.sTime, t.minute, Globals.sTime);
		}
	}

	strcat(tFormat, "%02d");
		if (!Globals.iTime)
		{
			strcat(tFormat, " ");
			strcat(tFormat, (t.hour<12)?Globals.s1159:Globals.s2359);
		}

	sprintf(szTime, tFormat, t.second);
}

VOID DIALOG_EditTimeDate(VOID)
{
    char        szDate[MAX_STRING_LEN]={0};

    FormatTime(szDate);
    SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szDate);

    SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)" ");

    szDate[0]='\0';
    FormatDate(szDate);
    SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szDate);
}

VOID DIALOG_EditWrap(VOID)
{
    BOOL modify = FALSE;
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                    ES_AUTOVSCROLL | ES_MULTILINE;
    RECT rc;
    DWORD size;
    LPSTR pTemp;

    size = GetWindowTextLength(Globals.hEdit) + 1;
    pTemp = GlobalAllocPtr(GPTR, size * sizeof(char));
    if (!pTemp)
    {
        ShowLastError();
        return;
    }
    GetWindowText(Globals.hEdit, pTemp, size);
    modify = SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0);
    DestroyWindow(Globals.hEdit);
    GetClientRect(Globals.hMainWnd, &rc);
    if( Globals.bWrapLongLines ) dwStyle |= WS_HSCROLL | ES_AUTOHSCROLL;
    Globals.hEdit = CreateWindow("Edit", NULL, dwStyle,
                         0, 0, rc.right, rc.bottom, Globals.hMainWnd,
                         0, Globals.hInstance, NULL);
    SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, (LPARAM)FALSE);
    SetWindowText(Globals.hEdit, pTemp);
    SendMessage(Globals.hEdit, EM_SETMODIFY, (WPARAM)modify, 0);
    SetFocus(Globals.hEdit);
    GlobalFreePtr(pTemp);
    
    Globals.bWrapLongLines = !Globals.bWrapLongLines;
    CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_WRAP,
        MF_BYCOMMAND | (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
}

VOID DIALOG_SelectFont(VOID)
{
    CHOOSEFONT cf;
    LOGFONT lf=Globals.lfFont;

    memset(&cf, 0, sizeof(cf));
    cf.lStructSize=sizeof(cf);
    cf.hwndOwner=Globals.hMainWnd;
    cf.lpLogFont=&lf;
    cf.Flags=CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;

    if( ChooseFont(&cf) )
    {
        HFONT currfont=Globals.hFont;

        Globals.hFont=CreateFontIndirect( &lf );
        Globals.lfFont=lf;
        SendMessage( Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, (LPARAM)TRUE );
        if( currfont!=0 )
            DeleteObject( currfont );
    }
}

VOID DIALOG_Search(VOID)
{
        memset(&Globals.find, 0, sizeof(Globals.find));
        Globals.find.lStructSize      = sizeof(Globals.find);
        Globals.find.hwndOwner        = Globals.hMainWnd;
        Globals.find.hInstance        = Globals.hInstance;
        Globals.find.lpstrFindWhat    = Globals.szFindText;
        Globals.find.wFindWhatLen     = SIZEOF(Globals.szFindText);
        Globals.find.Flags            = FR_DOWN|FR_HIDEWHOLEWORD;

        /* We only need to create the modal FindReplace dialog which will */
        /* notify us of incoming events using hMainWnd Window Messages    */

        Globals.hFindReplaceDlg = FindText(&Globals.find);
        assert(Globals.hFindReplaceDlg !=0);
}

VOID DIALOG_SearchNext(VOID)
{
	
    if (Globals.lastFind.lpstrFindWhat == NULL)
        DIALOG_Search();
    else                /* use the last find data */
        NOTEPAD_DoFind(&Globals.lastFind);
		
}

VOID DIALOG_HelpContents(VOID)
{
    WinHelp(Globals.hMainWnd, helpfileW, HELP_INDEX, 0);
}

VOID DIALOG_HelpSearch(VOID)
{
        /* Search Help */
}

VOID DIALOG_HelpHelp(VOID)
{
    WinHelp(Globals.hMainWnd, helpfileW, HELP_HELPONHELP, 0);
}

VOID DIALOG_HelpAboutNotepad(VOID)
{
    char szNotepad[MAX_STRING_LEN];
    HICON icon = LoadIcon( Globals.hInstance, MAKEINTRESOURCE(IDI_NOTEPAD));

    LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, SIZEOF(szNotepad));
    ShellAbout(Globals.hMainWnd, szNotepad, NULL, icon);
}


/***********************************************************************
 *
 *           DIALOG_FilePageSetup
 */
VOID DIALOG_FilePageSetup(void)
{
  DialogBox(Globals.hInstance, MAKEINTRESOURCE(DIALOG_PAGESETUP),
            Globals.hMainWnd, DIALOG_PAGESETUP_DlgProc);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_PAGESETUP_DlgProc
 */

static int WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

   switch (msg)
    {
    case WM_COMMAND:
      switch (wParam)
        {
        case IDOK:
          /* save user input and close dialog */
          GetDlgItemText(hDlg, IDC_PAGESETUP_HEADERVALUE, Globals.szHeader, SIZEOF(Globals.szHeader));
          GetDlgItemText(hDlg, IDC_PAGESETUP_FOOTERVALUE, Globals.szFooter, SIZEOF(Globals.szFooter));

          Globals.iMarginTop = GetDlgItemInt(hDlg, IDC_PAGESETUP_TOPVALUE, NULL, FALSE) * 100;
          Globals.iMarginBottom = GetDlgItemInt(hDlg, IDC_PAGESETUP_BOTTOMVALUE, NULL, FALSE) * 100;
          Globals.iMarginLeft = GetDlgItemInt(hDlg, IDC_PAGESETUP_LEFTVALUE, NULL, FALSE) * 100;
          Globals.iMarginRight = GetDlgItemInt(hDlg, IDC_PAGESETUP_RIGHTVALUE, NULL, FALSE) * 100;
          EndDialog(hDlg, IDOK);
          return TRUE;

        case IDCANCEL:
          /* discard user input and close dialog */
          EndDialog(hDlg, IDCANCEL);
          return TRUE;
#if 0
        case IDHELP:
        {
          /* FIXME: Bring this to work */
          static const char sorryW[] = { 'S','o','r','r','y',',',' ','n','o',' ','h','e','l','p',' ','a','v','a','i','l','a','b','l','e',0 };
          static const char helpW[] = { 'H','e','l','p',0 };
          MessageBox(Globals.hMainWnd, sorryW, helpW, MB_ICONEXCLAMATION);
          return TRUE;
        }
#endif
	default:
	    break;
        }
      break;

    case WM_INITDIALOG:
       /* fetch last user input prior to display dialog */
       SetDlgItemText(hDlg, IDC_PAGESETUP_HEADERVALUE, Globals.szHeader);
       SetDlgItemText(hDlg, IDC_PAGESETUP_FOOTERVALUE, Globals.szFooter);
       SetDlgItemInt(hDlg, IDC_PAGESETUP_TOPVALUE, Globals.iMarginTop / 100, FALSE);
       SetDlgItemInt(hDlg, IDC_PAGESETUP_BOTTOMVALUE, Globals.iMarginBottom / 100, FALSE);
       SetDlgItemInt(hDlg, IDC_PAGESETUP_LEFTVALUE, Globals.iMarginLeft / 100, FALSE);
       SetDlgItemInt(hDlg, IDC_PAGESETUP_RIGHTVALUE, Globals.iMarginRight / 100, FALSE);
       break;
    }

  return FALSE;
}
