#include <cstdio>
#include <cstdlib>
#include <shlobj.h>
#include <algorithm>
#include <windows.h>
#include <commctrl.h>
#include <htmlhelp.h>

#include "CanAn.h"
#include "MainWindow.h"

#include "MultiCan/Strings.h"
#include "MultiCan/Commands.h"
#include "MultiCan/Resources.h"

#include "Common/FileIO.h"
#include "Common/GridWindow.h"
#include "Common/WinWndClasser.h"

struct CN_DIALOG_DATA
{
	const char *dlgTitle = nullptr;
	const char *wndDesc = nullptr;
	const char *allowedSymbols = nullptr;
	const char *disallowError = nullptr;
	char resultText[128];
	int maxLength = sizeof(resultText) / sizeof(char) - 1;
};

const char* const gDataTabs[] = {STR_TAB_FACT, STR_TAB_SET, STR_TAB_COR};
const char* const gResTab[] = {STR_TAB_VAR, STR_TAB_PNT, STR_TAB_STD, STR_TAB_DIST};


const char* const gFactCols[] = {NULL, STR_COL_FACTIVE, STR_COL_NAME, STR_COL_STDDEV};
const unsigned int gFactColsWid[] = {75, 70, 100, 90};
const char* const gFactRows = STR_HDR_FACT;
const unsigned int gFactFstRowHght = 50;

const char* const gSetCols[] = {NULL, STR_COL_NAME, STR_COL_SETMARK, STR_COL_SETVOL, STR_HDR_MEANBYFACT};
const unsigned int gSetColsWid[] = {80, 150, 70, 70, 85};
const char* const gSetRows = STR_HDR_SET;
const unsigned int gSetFstRowHght = 35;

const char* const gCorCols[] = {NULL};
const unsigned int gCorColsWid[] = {75, 75};
const char* const gCorHdrs = STR_HDR_FACT;


const char* const gVarRows[] = {NULL, STR_ROW_EIGVAL, STR_ROW_CUMPROP, STR_ROW_CONST};
const unsigned int gVarRowsHght[] = {20, 35, 35, 20};
const char* const gVarCols = STR_HDR_VAR;
const unsigned int gVarFstColWid = 100;
const unsigned int gVarColsWid = 100;

const char* const gPntCols[] = {NULL, STR_COL_SETMARK, STR_HDR_VAR};
const unsigned int gPntColsWid[] = {150, 80, 100};
const unsigned int gPntFstRowHght = 35;

const char* const gStdRows[] = {NULL, STR_ROW_EIGVAL, STR_ROW_CUMPROP};
const unsigned int gStdRowsHght[] = {20, 35, 35};
const char* const gStdCols = STR_HDR_VAR;
const unsigned int gStdFstColWid = 100;
const unsigned int gStdColsWid = 100;

const char* const gDistCols[] = {NULL};
const unsigned int gDistColsWid[] = {150, 150};


const char CN_MW_CLASSNAME[] = "MultiCan_MainWndClass";


LRESULT WINAPI MainWindowProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PatternInpDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);

bool SaveGridData(GridWindow *grid, HANDLE file, int first_col = 1);
bool LoadGridData(GridWindow *grid, HANDLE file, int first_col = 1);
bool SaveGridSizes(GridWindow *grid, HANDLE file);
bool LoadGridSizes(GridWindow *grid, HANDLE file);

void ChangeDecSep(GridWindow *grid, int row, int col);
bool ReadDouble(GridWindow *grid, int row, int col, double *value);
void PrintDouble(GridWindow *grid, int row, int col, double value);
bool ReadUnsignedInt(GridWindow *grid, int row, int col, unsigned int *value);
void PrintUnsignedInt(GridWindow *grid, int row, int col, unsigned int value);


bool MainWindow::classReady = false;

MainWindow::MainWindow(void):
	wndMenu(0),
	wndHandle(0),
	tabCtrl(0),
	gFont(0),

	workFile(NULL),
	fileType(static_cast<FileTypeEnum>('\0')),

	changed(false),
	pathLen(-1)
{
	INITCOMMONCONTROLSEX initCC;

	initCC.dwICC = ICC_STANDARD_CLASSES | ICC_TAB_CLASSES;
	initCC.dwSize = sizeof(initCC);
	InitCommonControlsEx(&initCC);
}

MainWindow::~MainWindow()
{
	if (!wndHandle && wndMenu)
		DestroyMenu(wndMenu);
	if (workFile)
		delete[] workFile;
}

bool MainWindow::AskSave(void)
{
	if (!changed)
		return true;

	switch (MessageBox(wndHandle, STR_MSG_WANTSAVE, STR_TITLE_MAINWINDOW, MB_YESNOCANCEL | MB_ICONQUESTION))
	{
	case IDYES:
		SendMessage(wndHandle, WM_COMMAND, MAKEWPARAM(COMID_SAVE, 0), 0);
		return (!changed);

	case IDNO:
		return true;

	default:
		return false;
	}
}

bool MainWindow::Create(HINSTANCE hInst)
{
	if (wndHandle)
		return 0;

	InitClass(hInst);
	if (!wndMenu)
		wndMenu = LoadMenu(hInst, MAKEINTRESOURCE(MENU_MAIN));

	wwcObject = (LONG_PTR) this;
	wwcFunction = MainWindowProc;
	return CreateWindow(CN_MW_CLASSNAME, "", WS_SIZEBOX | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 768, 540, NULL, wndMenu, hInst, NULL);
}

void MainWindow::SetWorkFile(const char *name, int plen /* = -1 */)
{
	size_t len;
	if (workFile)
		delete[] workFile;

	if (!name || !(len = strlen(name)))
	{
		workFile = NULL;
		pathLen = -1;
	}
	else
	{
		workFile = new char [len+1];
		sprintf(workFile, "%s", name);
		workFile[len] = 0;

		if (plen >= 0)
			pathLen = plen;
		else
		{
			pathLen = len;
			while (pathLen > 0 && workFile[pathLen-1] != '\\')
				--pathLen;
		}
	}
}

LRESULT MainWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (wndHandle)
	{
		if (hWnd != wndHandle)
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	else if (uMsg != WM_CREATE)
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_CREATE:
		wndHandle = hWnd;

		InitGridsFont();
		InitTabCtrl();

		if (!workFile)
			ResetFile();
		else if (!LoadFile(workFile))
		{
			MessageBox(wndHandle, STR_MSG_OPENERROR, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
			ResetFile();
		}
		else
		{
			changed = false;
			UpdateTitle();
		}
		break;

	case WM_DESTROY:
		factGrid.ClearFont();
		setGrid.ClearFont();
		corGrid.ClearFont();

		varGrid.ClearFont();
		pntGrid.ClearFont();
		stdGrid.ClearFont();
		distGrid.ClearFont();

		DeleteObject(gFont);
		wndHandle = 0;
		tabCtrl = 0;
		gFont = 0;

		PostQuitMessage(0);
		return 0;

	case WM_COMMAND:
		if (lParam == 0)
			switch (LOWORD(wParam))
			{
			case COMID_NEW:
				if (!AskSave())
					return 0;
				ResetFile();
				break;

			case COMID_OPEN:
				OpenCmd();
				break;

			case COMID_SAVE:
				if (workFile)
				{
					if (!SaveFile(workFile))
						MessageBox(wndHandle, STR_MSG_SAVEERROR, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
					else if (changed)
					{
						changed = false;
						UpdateTitle();
					}
				}
				else if (changed)
					SendMessage(wndHandle, WM_COMMAND, MAKEWPARAM(COMID_SAVEAS, 0), 0);
				break;

			case COMID_SAVEAS:
				SaveAsCmd();
				break;

			case COMID_EXIT:
				PostMessage(wndHandle, WM_CLOSE, 0, 0);
				break;

			case COMID_ADDSET:
			case COMID_DELSET:
			case COMID_ADDFACT:
			case COMID_DELFACT:
				DelAddCmd(LOWORD(wParam));
				break;

			case COMID_CANAN:
				StartCanAn();
				break;

			case COMID_CHDECSEP:
				ChangeDecSepCmd();
				break;

			case COMID_REGEXT:
				AssociateCmd();
				break;

			case COMID_SHOWHELP:
				SendMessage(wndHandle, WM_HELP, 0, 0);
				break;

			case COMID_ABOUT:
				AboutCmd();
				break;
			}
		break;

	case WM_CLOSE:
		if (!AskSave())
			return 0;
		break;

	case WM_HELP:
		HelpCmd();
		break;

	case WM_NOTIFY:
		ChildNotify((LPNMHDR) lParam);
		break;

	case WM_GETMINMAXINFO:
		{
			MINMAXINFO *mmi = (LPMINMAXINFO) lParam;
			mmi -> ptMinTrackSize.x = minWndWidth;
			mmi -> ptMinTrackSize.y = minWndHeight;
		}
		break;

	case WM_SIZE:
		ResizeChildren();
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void MainWindow::CreateTabs(void)
{
	size_t i, num;
	TC_ITEM newItem;
	const char* const *tabs;

	switch (fileType)
	{
	case FileTypeEnum::DATA:
		num = sizeof(gDataTabs);
		tabs = gDataTabs;
		break;

	case FileTypeEnum::RESULT:
		num = sizeof(gResTab) - 1;
		tabs = gResTab;
		break;

	case FileTypeEnum::RESULT_V2:
		num = sizeof(gResTab);
		tabs = gResTab;
		break;
	}

	num /= sizeof(char*);
	newItem.mask = TCIF_TEXT;
	for (i = 0; i < num; ++i)
	{
		newItem.pszText = (char*) tabs[i];
		SendMessage(tabCtrl, TCM_INSERTITEM, i, (LPARAM) &newItem);
	}
}

void MainWindow::UpdateTitle(void)
{
	const char *file;
	size_t cur;
	char *buf;

	if (!wndHandle)
		return;

	file = (workFile ? workFile + pathLen : STR_TITLE_NOFILE);
	buf = new char [strlen(STR_TITLE_MAINWINDOW) + strlen(file) + strlen(" - [*]") + 1];

	cur = sprintf(buf, "%s", STR_TITLE_MAINWINDOW);
	cur += sprintf(buf+cur, " - [");
	cur += sprintf(buf+cur, "%s", file);
	if (changed)
		cur += sprintf(buf+cur, "*");
	cur += sprintf(buf+cur, "]");

	SendMessage(wndHandle, WM_SETTEXT, 0, (LPARAM) buf);
	delete[] buf;
}

void MainWindow::ResizeChildren()
{
	RECT wndClRect, tabRect;

	GetClientRect(wndHandle, &wndClRect);
	tabRect = wndClRect;

	++tabRect.top;
	--tabRect.right;
	--tabRect.bottom;
	tabRect.left += 2;
	SendMessage(tabCtrl, TCM_ADJUSTRECT, FALSE, (LPARAM) &tabRect);

	tabRect.bottom = tabRect.top;
	SendMessage(tabCtrl, TCM_ADJUSTRECT, TRUE, (LPARAM) &tabRect);
	SetWindowPos(tabCtrl, NULL, tabRect.left, tabRect.top, tabRect.right-tabRect.left, tabRect.bottom-tabRect.top, SWP_NOZORDER | SWP_NOACTIVATE);

	wndClRect.bottom -= 2;
	wndClRect.left = tabRect.left;
	wndClRect.top = tabRect.bottom - 1;
	wndClRect.right = tabRect.right - 1;
	switch (fileType)
	{
	case FileTypeEnum::DATA:
		SetWindowPos(factGrid, NULL, wndClRect.left, wndClRect.top, wndClRect.right-wndClRect.left, wndClRect.bottom-wndClRect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(setGrid, NULL, wndClRect.left, wndClRect.top, wndClRect.right-wndClRect.left, wndClRect.bottom-wndClRect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(corGrid, NULL, wndClRect.left, wndClRect.top, wndClRect.right-wndClRect.left, wndClRect.bottom-wndClRect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		break;

	case FileTypeEnum::RESULT:
	case FileTypeEnum::RESULT_V2:
		SetWindowPos(varGrid, NULL, wndClRect.left, wndClRect.top, wndClRect.right-wndClRect.left, wndClRect.bottom-wndClRect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(pntGrid, NULL, wndClRect.left, wndClRect.top, wndClRect.right-wndClRect.left, wndClRect.bottom-wndClRect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(stdGrid, NULL, wndClRect.left, wndClRect.top, wndClRect.right-wndClRect.left, wndClRect.bottom-wndClRect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(distGrid, NULL, wndClRect.left, wndClRect.top, wndClRect.right-wndClRect.left, wndClRect.bottom-wndClRect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		break;
	}
}

void MainWindow::ShowTab(int num)
{
	SendMessage(tabCtrl, TCM_SETCURSEL, num, 0);
	switch (fileType)
	{
	case FileTypeEnum::DATA:
		ShowWindow(factGrid, ((num == 0) ? SW_SHOW : SW_HIDE));
		ShowWindow(setGrid, ((num == 1) ? SW_SHOW : SW_HIDE));
		ShowWindow(corGrid, ((num == 2) ? SW_SHOW : SW_HIDE));
		switch (num)
		{
		case 0:
			SetFocus(factGrid);
			break;

		case 1:
			SetFocus(setGrid);
			break;

		case 2:
			SetFocus(corGrid);
			break;
		}
		break;

	case FileTypeEnum::RESULT:
	case FileTypeEnum::RESULT_V2:
		ShowWindow(varGrid, ((num == 0) ? SW_SHOW : SW_HIDE));
		ShowWindow(pntGrid, ((num == 1) ? SW_SHOW : SW_HIDE));
		ShowWindow(stdGrid, ((num == 2) ? SW_SHOW : SW_HIDE));
		ShowWindow(distGrid, ((num == 3) ? SW_SHOW : SW_HIDE));
		switch (num)
		{
		case 0:
			SetFocus(varGrid);
			break;

		case 1:
			SetFocus(pntGrid);
			break;

		case 2:
			SetFocus(stdGrid);
			break;

		case 3:
			SetFocus(distGrid);
			break;
		}
		break;
	}
}

void MainWindow::SetFileType(FileTypeEnum newType)
{
	if (newType != fileType)
	{
		switch (fileType)
		{
		case FileTypeEnum::DATA:
			factGrid.DestroyAll();
			setGrid.DestroyAll();
			corGrid.DestroyAll();

			EnableMenuItem(GetMenu(wndHandle), COMID_CANAN, MF_BYCOMMAND | MF_GRAYED);
			break;

		case FileTypeEnum::RESULT:
		case FileTypeEnum::RESULT_V2:
			varGrid.DestroyAll();
			pntGrid.DestroyAll();
			stdGrid.DestroyAll();
			distGrid.DestroyAll();

			EnableMenuItem(GetMenu(wndHandle), COMID_CHDECSEP, MF_BYCOMMAND | MF_GRAYED);
			break;
		}

		SendMessage(tabCtrl, TCM_DELETEALLITEMS, 0, 0);
		fileType = newType;
		CreateTabs();

		switch (fileType)
		{
		case FileTypeEnum::DATA:
			factGrid.Create(wndHandle);
			setGrid.Create(wndHandle);
			corGrid.Create(wndHandle);

			EnableMenuItem(GetMenu(wndHandle), COMID_CANAN, MF_BYCOMMAND | MF_ENABLED);
			break;

		case FileTypeEnum::RESULT_V2:
			distGrid.Create(wndHandle);
			[[fallthrough]];
		case FileTypeEnum::RESULT:
			varGrid.Create(wndHandle);
			pntGrid.Create(wndHandle);
			stdGrid.Create(wndHandle);

			EnableMenuItem(GetMenu(wndHandle), COMID_CHDECSEP, MF_BYCOMMAND | MF_ENABLED);
			break;
		}
		ResizeChildren();
	}

	ShowTab(0);
	switch (fileType)
	{
	case FileTypeEnum::DATA:
		factGrid.Redraw();
		break;

	case FileTypeEnum::RESULT:
	case FileTypeEnum::RESULT_V2:
		varGrid.Redraw();
		break;
	}
}

void MainWindow::ResetFile(void)
{
	factGrid.SetSizes(1, sizeof(gFactCols) / sizeof(char*));
	setGrid.SetSizes(1, sizeof(gSetCols) / sizeof(char*) - 1);
	corGrid.SetSizes(1, sizeof(gCorCols) / sizeof(char*));

	SetDataGridsFixedCells(true, true, true);
	SetFileType(FileTypeEnum::DATA);
	SetWorkFile(NULL);

	changed = false;
	UpdateTitle();
}

bool MainWindow::SaveFile(char *file)
{
	int s, f, v;
	char buf[8];
	HANDLE hFile;

	if ((hFile = CreateFile(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
		return false;

	sprintf(buf, "MCan");
	buf[5] = 0;

	if (!WriteBuf(hFile, buf, 4))
	{
		CloseHandle(hFile);
		return false;
	}
	if (!WriteBuf(hFile, &fileType, sizeof(fileType)))
	{
		CloseHandle(hFile);
		return false;
	}

	switch (fileType)
	{
	case FileTypeEnum::DATA:
		f = factGrid.GetRowsCount()-1;
		s = setGrid.GetRowsCount()-1;

		if (!WriteBuf(hFile, &s, sizeof(s)) || !WriteBuf(hFile, &f, sizeof(f)))
		{
			CloseHandle(hFile);
			return false;
		}
		break;

	case FileTypeEnum::RESULT:
	case FileTypeEnum::RESULT_V2:
		v = varGrid.GetColsCount()-1;
		s = pntGrid.GetRowsCount()-1;
		f = varGrid.GetRowsCount() - sizeof(gVarRows) / sizeof(char*);

		if (!WriteBuf(hFile, &s, sizeof(s)) || !WriteBuf(hFile, &f, sizeof(f)) || !WriteBuf(hFile, &v, sizeof(v)))
		{
			CloseHandle(hFile);
			return false;
		}
		break;
	}

	if (!SaveData(hFile))
	{
		CloseHandle(hFile);
		return false;
	}

	SaveSizes(hFile);
	CloseHandle(hFile);
	return true;
}

bool MainWindow::LoadFile(char *file)
{
	int s, f, v;
	HANDLE hFile;
	char buf[8];
	FileTypeEnum newType;

	if ((hFile = CreateFile(file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
		return false;

	buf[4] = 0;
	if (!ReadBuf(hFile, buf, 4) || strcmp(buf, "MCan"))
	{
		CloseHandle(hFile);
		return false;
	}
	if (!ReadBuf(hFile, &newType, sizeof(newType)))
	{
		CloseHandle(hFile);
		return false;
	}

	switch (newType)
	{
	case FileTypeEnum::DATA:
		if (!ReadBuf(hFile, &s, sizeof(s)) || !ReadBuf(hFile, &f, sizeof(f)))
		{
			CloseHandle(hFile);
			return false;
		}

		factGrid.SetSizes(f+1, sizeof(gFactCols) / sizeof(char*));
		setGrid.SetSizes(s+1, f + sizeof(gSetCols) / sizeof(char*) - 1);
		corGrid.SetSizes(f+1, f + sizeof(gCorCols) / sizeof(char*));

		if (!LoadData(hFile, newType))
		{
			factGrid.DestroyAll();
			setGrid.DestroyAll();
			corGrid.DestroyAll();

			CloseHandle(hFile);
			return false;
		}

		SetDataGridsFixedCells(true, true, true);
		break;

	case FileTypeEnum::RESULT:
	case FileTypeEnum::RESULT_V2:
		if (!ReadBuf(hFile, &s, sizeof(s)) || !ReadBuf(hFile, &f, sizeof(f)) || !ReadBuf(hFile, &v, sizeof(v)))
		{
			CloseHandle(hFile);
			return false;
		}

		varGrid.SetSizes(f + sizeof(gVarRows) / sizeof(char*), v+1);
		pntGrid.SetSizes(s+1, v + sizeof(gPntCols) / sizeof(char*) - 1);
		stdGrid.SetSizes(f + sizeof(gStdRows) / sizeof(char*), v+1);
		distGrid.SetSizes(s+1, s+1);

		if (!LoadData(hFile, newType))
		{
			varGrid.DestroyAll();
			pntGrid.DestroyAll();
			stdGrid.DestroyAll();
			distGrid.DestroyAll();

			CloseHandle(hFile);
			return false;
		}

		SetResultGridsFixedCells(true, true, true, true);
		break;

	default:
		return false;
	}
	LoadSizes(hFile, newType);
	CloseHandle(hFile);

	SetFileType(newType);
	return true;
}

bool MainWindow::SaveData(HANDLE hFile)
{
	switch (fileType)
	{
	case FileTypeEnum::DATA:
		if (!SaveGridData(&factGrid, hFile))
			return false;
		if (!SaveGridData(&setGrid, hFile))
			return false;
		if (!SaveGridData(&corGrid, hFile))
			return false;
		break;

	case FileTypeEnum::RESULT_V2:
		if (!SaveGridData(&distGrid, hFile, 0))
			return false;
		[[fallthrough]];
	case FileTypeEnum::RESULT:
		if (!SaveGridData(&varGrid, hFile, 0))
			return false;
		if (!SaveGridData(&stdGrid, hFile, 0))
			return false;
		if (!SaveGridData(&pntGrid, hFile, 0))
			return false;
		break;
	}

	return true;
}

bool MainWindow::LoadData(HANDLE hFile, FileTypeEnum newType)
{
	switch (newType)
	{
	case FileTypeEnum::DATA:
		if (!LoadGridData(&factGrid, hFile))
			return false;
		if (!LoadGridData(&setGrid, hFile))
			return false;
		if (!LoadGridData(&corGrid, hFile))
			return false;
		break;

	case FileTypeEnum::RESULT_V2:
		if (!LoadGridData(&distGrid, hFile, 0))
			return false;
		[[fallthrough]];
	case FileTypeEnum::RESULT:
		if (!LoadGridData(&varGrid, hFile, 0))
			return false;
		if (!LoadGridData(&stdGrid, hFile, 0))
			return false;
		if (!LoadGridData(&pntGrid, hFile, 0))
			return false;
		break;
	}

	return true;
}

bool MainWindow::SaveSizes(HANDLE hFile)
{
	switch (fileType)
	{
	case FileTypeEnum::DATA:
		if (!SaveGridSizes(&factGrid, hFile))
			return false;
		if (!SaveGridSizes(&setGrid, hFile))
			return false;
		if (!SaveGridSizes(&corGrid, hFile))
			return false;
		break;

	case FileTypeEnum::RESULT_V2:
		if (!SaveGridSizes(&distGrid, hFile))
			return false;
		[[fallthrough]];
	case FileTypeEnum::RESULT:
		if (!SaveGridSizes(&varGrid, hFile))
			return false;
		if (!SaveGridSizes(&stdGrid, hFile))
			return false;
		if (!SaveGridSizes(&pntGrid, hFile))
			return false;
		break;
	}

	return true;
}

bool MainWindow::LoadSizes(HANDLE hFile, FileTypeEnum newType)
{
	switch (newType)
	{
	case FileTypeEnum::DATA:
		if (!LoadGridSizes(&factGrid, hFile))
			return false;
		if (!LoadGridSizes(&setGrid, hFile))
			return false;
		if (!LoadGridSizes(&corGrid, hFile))
			return false;
		break;

	case FileTypeEnum::RESULT_V2:
		if (!LoadGridSizes(&distGrid, hFile))
			return false;
		[[fallthrough]];
	case FileTypeEnum::RESULT:
		if (!LoadGridSizes(&varGrid, hFile))
			return false;
		if (!LoadGridSizes(&stdGrid, hFile))
			return false;
		if (!LoadGridSizes(&pntGrid, hFile))
			return false;
		break;
	}

	return true;
}

void MainWindow::SetResultGridsFixedCells(bool vg, bool pg, bool sg, bool dg, bool chSize /* = true */)
{
	BYTE r, g, b;
	char buf[128];
	COLORREF c1, c2, c3;
	int i, j, sCnt, cCnt, rCnt;

	c2 = GetSysColor(COLOR_WINDOW);
	c3 = GetSysColor(COLOR_BTNFACE);

	r = (255 - GetRValue(c2) + 9*GetRValue(c3)) / 10;
	g = (255 - GetGValue(c2) + 9*GetGValue(c3)) / 10;
	b = (255 - GetBValue(c2) + 9*GetBValue(c3)) / 10;
	c1 = RGB(r, g, b);

	r = (255 - GetRValue(c2) + GetRValue(c3)) / 2;
	g = (255 - GetGValue(c2) + GetGValue(c3)) / 2;
	b = (255 - GetBValue(c2) + GetBValue(c3)) / 2;
	c2 = RGB(r, g, b);

	if (vg)
	{
		sCnt = sizeof(gVarRows) / sizeof(char*);
		cCnt = varGrid.GetColsCount();
		rCnt = varGrid.GetRowsCount();
		for (i = 0; i < sCnt; ++i)
		{
			if (chSize)
				varGrid.SetRowHeight(i, gVarRowsHght[i]);
			varGrid.SetCellColor(i, 0, c1);
			varGrid.SetCellProp(i, 0, GWCA_RIGHT | GWCA_VCENTER | GWCP_READONLY);
			varGrid.SetCell(i, 0, gVarRows[i]);
		}
		for (; i < rCnt; ++i)
		{
			varGrid.SetCellColor(i, 0, c1);
			varGrid.SetCellProp(i, 0, GWCA_RIGHT | GWCA_VCENTER | GWCP_READONLY);
		}
		if (chSize)
			varGrid.SetColWidth(0, gVarFstColWid);
		for (i = 1; i < cCnt; ++i)
		{
			if (chSize)
				varGrid.SetColWidth(i, gVarColsWid);
			varGrid.SetCellColor(0, i, c1);
			varGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			sprintf(buf, "%s%d", gVarCols, i);
			varGrid.SetCell(0, i, buf);

			for (j = 1; j < rCnt; ++j)
				varGrid.SetCellProp(j, i, GWCP_READONLY);
		}
		varGrid.SelectCells(0, 0, 0, 0, true);
		if (cCnt > 1)
			varGrid.SelectCells(1, 1, 0, 0);
	}

	if (pg)
	{
		sCnt = sizeof(gPntCols) / sizeof(char*) - 1;
		cCnt = pntGrid.GetColsCount();
		rCnt = pntGrid.GetRowsCount();
		for (i = 0; i < sCnt; ++i)
		{
			if (chSize)
				pntGrid.SetColWidth(i, gPntColsWid[i]);
			pntGrid.SetCellColor(0, i, c1);
			pntGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			pntGrid.SetCell(0, i, gPntCols[i]);
		}
		for (; i < cCnt; ++i)
		{
			if (chSize)
				pntGrid.SetColWidth(i, gPntColsWid[sCnt]);
			pntGrid.SetCellColor(0, i, c1);
			pntGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			sprintf(buf, "%s%d", gPntCols[sCnt], i-sCnt+1);
			pntGrid.SetCell(0, i, buf);
		}
		if (chSize)
			pntGrid.SetRowHeight(0, gPntFstRowHght);
		for (i = 1; i < rCnt; ++i)
		{
			pntGrid.SetCellColor(i, 0, c1);
			pntGrid.SetCellProp(i, 0, GWCA_RIGHT | GWCA_VCENTER | GWCP_READONLY);

			for (j = 1; j < cCnt; ++j)
				pntGrid.SetCellProp(i, j, GWCP_READONLY);
		}
		pntGrid.SelectCells(0, 0, 0, 0, true);
		if (rCnt > 1)
			pntGrid.SelectCells(1, 1, 0, 0);
	}

	if (sg)
	{
		sCnt = sizeof(gStdRows) / sizeof(char*);
		cCnt = stdGrid.GetColsCount();
		rCnt = stdGrid.GetRowsCount();
		for (i = 0; i < sCnt; ++i)
		{
			if (chSize)
				stdGrid.SetRowHeight(i, gStdRowsHght[i]);
			stdGrid.SetCellColor(i, 0, c1);
			stdGrid.SetCellProp(i, 0, GWCA_RIGHT | GWCA_VCENTER | GWCP_READONLY);
			stdGrid.SetCell(i, 0, gStdRows[i]);
		}
		for (; i < rCnt; ++i)
		{
			stdGrid.SetCellColor(i, 0, c1);
			stdGrid.SetCellProp(i, 0, GWCA_RIGHT | GWCA_VCENTER | GWCP_READONLY);
		}
		if (chSize)
			stdGrid.SetColWidth(0, gStdFstColWid);
		for (i = 1; i < cCnt; ++i)
		{
			if (chSize)
				stdGrid.SetColWidth(i, gStdColsWid);
			stdGrid.SetCellColor(0, i, c1);
			stdGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			sprintf(buf, "%s%d", gStdCols, i);
			stdGrid.SetCell(0, i, buf);

			for (j = 1; j < rCnt; ++j)
				stdGrid.SetCellProp(j, i, GWCP_READONLY);
		}
		stdGrid.SelectCells(0, 0, 0, 0, true);
		if (cCnt > 1)
			stdGrid.SelectCells(1, 1, 0, 0);
	}

	if (dg)
	{
		sCnt = sizeof(gDistCols) / sizeof(char*);
		cCnt = distGrid.GetColsCount();
		rCnt = distGrid.GetRowsCount();
		for (i = 0; i < sCnt; ++i)
		{
			if (chSize)
				distGrid.SetColWidth(i, gDistColsWid[i]);
			distGrid.SetCellColor(0, i, c1);
			distGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			distGrid.SetCell(0, i, gDistCols[i]);
		}
		for (; i < cCnt; ++i)
		{
			if (chSize)
				distGrid.SetColWidth(i, gDistColsWid[sCnt]);
			distGrid.SetCellColor(0, i, c1);
			distGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			distGrid.SetCell(0, i, distGrid.GetCell(i, 0));
		}
		for (i = 1; i < rCnt; ++i)
		{
			distGrid.SetCellColor(i, 0, c1);
			distGrid.SetCellProp(i, 0, GWCA_RIGHT | GWCA_VCENTER | GWCP_READONLY);

			for (j = 1; j < cCnt; ++j)
				distGrid.SetCellProp(i, j, GWCP_READONLY);
		}
		distGrid.SelectCells(0, 0, 0, 0, true);
		if (rCnt > 1)
			distGrid.SelectCells(1, 1, 0, 0);
	}
}

void MainWindow::SetDataGridsFixedCells(bool fg, bool sg, bool cg, bool chSize /* = true */)
{
	char buf[128];
	BYTE r, g, b;
	COLORREF c1, c2, c3;
	int i, j, sCnt, cCnt, rCnt, mCol;

	c2 = GetSysColor(COLOR_WINDOW);
	c3 = GetSysColor(COLOR_BTNFACE);

	r = (255 - GetRValue(c2) + 9*GetRValue(c3)) / 10;
	g = (255 - GetGValue(c2) + 9*GetGValue(c3)) / 10;
	b = (255 - GetBValue(c2) + 9*GetBValue(c3)) / 10;
	c1 = RGB(r, g, b);

	r = (255 - GetRValue(c2) + GetRValue(c3)) / 2;
	g = (255 - GetGValue(c2) + GetGValue(c3)) / 2;
	b = (255 - GetBValue(c2) + GetBValue(c3)) / 2;
	c2 = RGB(r, g, b);

	mCol = -1;
	if (fg)
	{
		sCnt = sizeof(gFactCols) / sizeof(char*);
		cCnt = factGrid.GetColsCount();
		rCnt = factGrid.GetRowsCount();
		for (i = 0; i < sCnt; ++i)
		{
			if (chSize)
				factGrid.SetColWidth(i, gFactColsWid[i]);
			factGrid.SetCellColor(0, i, c1);
			factGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			factGrid.SetCell(0, i, gFactCols[i]);

			if (gFactCols[i] == STR_COL_FACTIVE)
				mCol = i;
		}
		if (chSize)
			factGrid.SetRowHeight(0, gFactFstRowHght);
		for (i = 1; i < rCnt; ++i)
		{
			factGrid.SetCellColor(i, 0, c1);
			factGrid.SetCellProp(i, 0, GWCA_RIGHT | GWCA_VCENTER | GWCP_READONLY);
			sprintf(buf, "%s%d", gFactRows, i);
			factGrid.SetCell(i, 0, buf);

			factGrid.SetCellTextColor(i, mCol, RGB(0, 192, 0));
			factGrid.SetCellProp(i, mCol, GWCP_MARKCELL | GWCM_MARKN | GWCM_MARKV);
		}
		factGrid.SelectCells(0, 0, 0, 0, true);
		if (rCnt > 1)
			factGrid.SelectCells(1, 1, 0, 0);
	}

	if (sg)
	{
		sCnt = sizeof(gSetCols) / sizeof(char*) - 1;
		cCnt = setGrid.GetColsCount();
		rCnt = setGrid.GetRowsCount();
		for (i = 0; i < sCnt; ++i)
		{
			if (chSize)
				setGrid.SetColWidth(i, gSetColsWid[i]);
			setGrid.SetCellColor(0, i, c1);
			setGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			setGrid.SetCell(0, i, gSetCols[i]);
		}
		for (; i < cCnt; ++i)
		{
			if (chSize)
				setGrid.SetColWidth(i, gSetColsWid[sCnt]);
			setGrid.SetCellColor(0, i, c1);
			setGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			sprintf(buf, "%s%d", gSetCols[sCnt], i-sCnt+1);
			setGrid.SetCell(0, i, buf);
		}
		if (chSize)
			setGrid.SetRowHeight(0, gSetFstRowHght);
		for (i = 1; i < rCnt; ++i)
		{
			setGrid.SetCellColor(i, 0, c1);
			setGrid.SetCellProp(i, 0, GWCA_RIGHT | GWCA_VCENTER | GWCP_READONLY);
			sprintf(buf, "%s%d", gSetRows, i);
			setGrid.SetCell(i, 0, buf);
		}
		setGrid.SelectCells(0, 0, 0, 0, true);
		if (rCnt > 1)
			setGrid.SelectCells(1, 1, 0, 0);
	}

	if (cg)
	{
		sCnt = sizeof(gCorCols) / sizeof(char*);
		cCnt = corGrid.GetColsCount();
		rCnt = corGrid.GetRowsCount();
		for (i = 0; i < sCnt; ++i)
		{
			if (chSize)
				corGrid.SetColWidth(i, gCorColsWid[i]);
			corGrid.SetCellColor(0, i, c1);
			corGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			corGrid.SetCell(0, i, gCorCols[i]);
		}
		for (; i < cCnt; ++i)
		{
			if (chSize)
				corGrid.SetColWidth(i, gCorColsWid[sCnt]);
			corGrid.SetCellColor(0, i, c1);
			corGrid.SetCellProp(0, i, GWCA_CENTER | GWCA_BOTTOM | GWCP_READONLY);
			sprintf(buf, "%s%d", gCorHdrs, i-sCnt+1);
			corGrid.SetCell(0, i, buf);
		}
		for (i = 1; i < rCnt; ++i)
		{
			corGrid.SetCellColor(i, 0, c1);
			corGrid.SetCellProp(i, 0, GWCA_RIGHT | GWCA_VCENTER | GWCP_READONLY);
			sprintf(buf, "%s%d", gCorHdrs, i);
			corGrid.SetCell(i, 0, buf);
		}
		corGrid.SelectCells(0, 0, 0, 0, true);
		if (rCnt > 1)
			corGrid.SelectCells(1, 1, 0, 0);

		sprintf(buf, "1.0");
		for (i = 1; i < rCnt; ++i)
		{
			j = i+sCnt-1;

			corGrid.SetCellTextColor(i, j, c2);
			corGrid.SetCellProp(i, j, GWCP_READONLY);
			corGrid.SetCell(i, j, buf);
		}
	}
}

void MainWindow::ShowIntervalError(const char *msg, const char *interval)
{
	const char *intervalEnd = interval;
	while (*intervalEnd && *intervalEnd != ';' && *intervalEnd != ',') {
		++intervalEnd;
	}
	const auto msgLen = strlen(msg);
	auto *fullMsg = new char [msgLen + intervalEnd - interval + 1];
	memcpy(fullMsg, msg, msgLen);
	memcpy(fullMsg + msgLen, interval, intervalEnd - interval);
	fullMsg[msgLen + intervalEnd - interval] = 0;
	MessageBox(wndHandle, fullMsg, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONEXCLAMATION);
	delete[] fullMsg;
}

std::vector<std::pair<unsigned int, unsigned int>>
	MainWindow::ParseIntervals(const char *intervals, unsigned int count)
{
	unsigned int intervalStart, intervalEnd;
	bool hasStart = false, waitEnd = false, hasEnd = false;
	const char *currentStart = intervals;

	std::vector<std::pair<unsigned int, unsigned int>> result;
	while (*intervals || hasStart)
	{
		if (!*intervals || *intervals == ';' || *intervals == ',')
		{
			if (waitEnd)
			{
				ShowIntervalError(STR_INVALID_INTERVAL, currentStart);
				return {};
			}
			if (hasStart)
			{
				if (!hasEnd)
					intervalEnd = intervalStart;
				result.emplace_back(intervalStart, intervalEnd);
				hasStart = hasEnd = false;
			}
			if (*intervals)
				++intervals;
			currentStart = intervals;
		}
		else if (*intervals == '-')
		{
			if (!hasStart || waitEnd || hasEnd)
			{
				ShowIntervalError(STR_INVALID_INTERVAL, currentStart);
				return { };
			}
			waitEnd = true;
			++intervals;
		}
		else
		{
			char *parsedUntil;
			const auto result = strtoul(intervals, &parsedUntil, 10);
			if (parsedUntil == intervals)
			{
				ShowIntervalError(STR_INVALID_INTERVAL_CHAR, currentStart);
				return { };
			}
			if (result < 1 || result > count)
			{
				ShowIntervalError(STR_INTERVAL_OVERFLOW, currentStart);
				return { };
			}
			intervals = parsedUntil;

			if (!hasStart)
			{
				hasStart = true;
				intervalStart = result;
			}
			else if (waitEnd)
			{
				hasEnd = true;
				waitEnd = false;
				intervalEnd = result;
				if (intervalEnd < intervalStart)
				{
					ShowIntervalError(STR_INVALID_INTERVAL_ORDER, currentStart);
					return { };
				}
			}
			else
			{
				ShowIntervalError(STR_INVALID_INTERVAL, currentStart);
				return { };
			}
		}
	}
	std::sort(result.begin(), result.end());

	intervalEnd = 0;
	for (const auto &el : result)
	{
		if (el.first <= intervalEnd)
		{
			MessageBox(wndHandle, STR_INTERVALS_OVERLAP, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONEXCLAMATION);
			return { };
		}
		intervalEnd = el.second;
	}
	return result;
}

void MainWindow::DeleteSets(const char *intervalsStr)
{
	auto intervals = ParseIntervals(intervalsStr, setGrid.GetRowsCount() - 1);
	if (intervals.empty())
		return;

	std::reverse(intervals.begin(), intervals.end());
	for (const auto &el : intervals)
	{
		for (auto num = el.second; num >= el.first; --num)
		{
			setGrid.DeleteRow(num);
		}
	}

	SetDataGridsFixedCells(false, true, false, false);

	setGrid.Redraw();
	if (!changed)
	{
		changed = true;
		UpdateTitle();
	}
}

void MainWindow::DeleteFactors(const char *intervalsStr)
{
	unsigned int sCnt;

	auto intervals = ParseIntervals(intervalsStr, factGrid.GetRowsCount() - 1);
	if (intervals.empty())
		return;

	std::reverse(intervals.begin(), intervals.end());
	for (const auto &el : intervals)
	{
		for (auto num = el.second; num >= el.first; --num)
		{
			factGrid.DeleteRow(num);
			corGrid.DeleteRow(num);

			sCnt = sizeof(gSetCols) / sizeof(char*) - 1;
			setGrid.DeleteCol(sCnt + num - 1);

			sCnt = sizeof(gCorCols) / sizeof(char*);
			corGrid.DeleteCol(sCnt + num - 1);
		}
	}

	SetDataGridsFixedCells(true, true, true, false);

	factGrid.Redraw();
	setGrid.Redraw();
	corGrid.Redraw();
	if (!changed)
	{
		changed = true;
		UpdateTitle();
	}
}

void MainWindow::AddSets(unsigned int cnt)
{
	if (cnt < 1)
		return;

	do
		setGrid.AddRow(setGrid.GetRowsCount());
	while (--cnt);
	SetDataGridsFixedCells(false, true, false, false);

	setGrid.Redraw();
	if (!changed)
	{
		changed = true;
		UpdateTitle();
	}
}

void MainWindow::AddFactors(unsigned int cnt)
{
	int mCol;
	if (cnt < 1)
		return;

	for (mCol = sizeof(gFactCols) / sizeof(char*) - 1; mCol >= 0; --mCol)
		if (gFactCols[mCol] == STR_COL_FACTIVE)
			break;

	do
	{
		factGrid.AddRow(factGrid.GetRowsCount());
		setGrid.AddCol(setGrid.GetColsCount());
		corGrid.AddRow(corGrid.GetRowsCount());
		corGrid.AddCol(corGrid.GetColsCount());

		factGrid.SetCellMark(factGrid.GetRowsCount()-1, mCol, GWCM_MARKV);
		corGrid.SetColWidth(corGrid.GetColsCount()-1, gCorColsWid[sizeof(gCorCols) / sizeof(char*)]);
	}
	while (--cnt);
	SetDataGridsFixedCells(true, true, true, false);

	factGrid.Redraw();
	setGrid.Redraw();
	corGrid.Redraw();
	if (!changed)
	{
		changed = true;
		UpdateTitle();
	}
}

void MainWindow::ChildNotify(NMHDR *notInfo)
{
	if (notInfo->hwndFrom == NULL)
		return;

	if (notInfo->hwndFrom == tabCtrl)
	{
		if (notInfo->code == TCN_SELCHANGE)
			ShowTab(SendMessage(tabCtrl, TCM_GETCURSEL, 0, 0));
	}
	else if (notInfo->hwndFrom == factGrid || notInfo->hwndFrom == setGrid || notInfo->hwndFrom == corGrid
		|| notInfo->hwndFrom == varGrid || notInfo->hwndFrom == pntGrid || notInfo->hwndFrom == stdGrid || notInfo->hwndFrom == distGrid)
	{
		if (notInfo->code == GN_CHANGE && !changed)
		{
			changed = true;
			UpdateTitle();
		}

		if (notInfo->hwndFrom == corGrid && notInfo->code == GN_CHANGE)
		{
			int sCnt, r, c;
			GWNM_CHANGE *gwcInfo;

			gwcInfo = (GWNM_CHANGE*) notInfo;
			sCnt = sizeof(gCorCols) / sizeof(char*);

			r = gwcInfo->row;
			c = gwcInfo->col;
			if (c >= sCnt)
			{
				const char *cell = corGrid.GetCell(r, c);

				r += sCnt - 1;
				c -= sCnt - 1;
				corGrid.SetCell(c, r, cell);
			}
		}
	}
}

void MainWindow::ChangeDecSepCmd(void)
{
	int r, c;
	int rCnt, cCnt;

	if (fileType != FileTypeEnum::RESULT && fileType != FileTypeEnum::RESULT_V2)
		return;

	rCnt = varGrid.GetRowsCount();
	cCnt = varGrid.GetColsCount();
	for (r = 1; r < rCnt; ++r)
		for (c = 1; c < cCnt; ++c)
			ChangeDecSep(&varGrid, r, c);

	rCnt = pntGrid.GetRowsCount();
	cCnt = pntGrid.GetColsCount();
	for (r = 1; r < rCnt; ++r)
		for (c = 1; c < cCnt; ++c)
			ChangeDecSep(&pntGrid, r, c);

	rCnt = stdGrid.GetRowsCount();
	cCnt = stdGrid.GetColsCount();
	for (r = 1; r < rCnt; ++r)
		for (c = 1; c < cCnt; ++c)
			ChangeDecSep(&stdGrid, r, c);

	rCnt = distGrid.GetRowsCount();
	cCnt = distGrid.GetColsCount();
	for (r = 1; r < rCnt; ++r)
		for (c = 1; c < cCnt; ++c)
			ChangeDecSep(&distGrid, r, c);

	varGrid.Redraw();
	pntGrid.Redraw();
	stdGrid.Redraw();
	distGrid.Redraw();

	changed = true;
	UpdateTitle();
}

const char STR_DEFICON[] = "DefaultIcon";
const char STR_OPENCMD[] = "shell\\open\\command";
void MainWindow::AssociateCmd(void)
{
	size_t len;
	HKEY key, subKey;
	char *str, fileName[MAX_PATH+4];

	len = strlen(STR_DATAEXT) + 2;
	str = new char [len];

	sprintf(str, ".%s", STR_DATAEXT);
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, str, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(key, NULL, 0, REG_SZ, (const BYTE*) STR_DATAEXT_REGTYPE, sizeof(STR_DATAEXT_REGTYPE));
		RegCloseKey(key);
	}
	delete[] str;

	len = strlen(STR_RESEXT) + 2;
	str = new char [len];

	sprintf(str, ".%s", STR_RESEXT);
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, str, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(key, NULL, 0, REG_SZ, (const BYTE*) STR_RESEXT_REGTYPE, sizeof(STR_RESEXT_REGTYPE));
		RegCloseKey(key);
	}
	delete[] str;

	GetModuleFileName(NULL, fileName, sizeof(fileName));
	len = strlen(fileName) + 16;
	str = new char [len];

	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, STR_DATAEXT_REGTYPE, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(key, NULL, 0, REG_SZ, (const BYTE*) STR_DATAEXT_REGDESC, sizeof(STR_DATAEXT_REGDESC));
		if (RegCreateKeyEx(key, STR_DEFICON, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS)
		{
			len = sprintf(str, "%s,%d", fileName, ICON_DATA - ICON_MAIN);
			RegSetValueEx(subKey, NULL, 0, REG_SZ, (const BYTE*) str, (len+1)*sizeof(str[0]));
			RegCloseKey(subKey);
		}
		if (RegCreateKeyEx(key, STR_OPENCMD, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS)
		{
			len = sprintf(str, "\"%s\" \"%%1\"", fileName);
			RegSetValueEx(subKey, NULL, 0, REG_SZ, (const BYTE*) str, (len+1)*sizeof(str[0]));
			RegCloseKey(subKey);
		}
		RegCloseKey(key);
	}
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, STR_RESEXT_REGTYPE, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(key, NULL, 0, REG_SZ, (const BYTE*) STR_RESEXT_REGDESC, sizeof(STR_RESEXT_REGDESC));
		if (RegCreateKeyEx(key, STR_DEFICON, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS)
		{
			len = sprintf(str, "%s,%d", fileName, ICON_RES - ICON_MAIN);
			RegSetValueEx(subKey, NULL, 0, REG_SZ, (const BYTE*) str, (len+1)*sizeof(*str));
			RegCloseKey(subKey);
		}
		if (RegCreateKeyEx(key, STR_OPENCMD, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &subKey, NULL) == ERROR_SUCCESS)
		{
			len = sprintf(str, "\"%s\" \"%%1\"", fileName);
			RegSetValueEx(subKey, NULL, 0, REG_SZ, (const BYTE*) str, (len+1)*sizeof(*str));
			RegCloseKey(subKey);
		}
		RegCloseKey(key);
	}
	delete[] str;
}

void MainWindow::DelAddCmd(int id)
{
	CN_DIALOG_DATA cnDlgProp;

	if (fileType != FileTypeEnum::DATA)
		return;

	switch (id)
	{
	case COMID_ADDFACT:
		cnDlgProp.dlgTitle = STR_TITLE_ADDDLG;
		cnDlgProp.wndDesc = STR_DLG_ADDFACT;
		cnDlgProp.allowedSymbols = STR_DLG_ADDPATTERN;
		cnDlgProp.disallowError = STR_DLG_ADDERROR;
		cnDlgProp.maxLength = 3;
		break;

	case COMID_DELFACT:
		cnDlgProp.dlgTitle = STR_TITLE_DELDLG;
		cnDlgProp.wndDesc = STR_DLG_DELFACT;
		cnDlgProp.allowedSymbols = STR_DLG_DELPATTERN;
		cnDlgProp.disallowError = STR_DLG_DELERROR;
		break;

	case COMID_ADDSET:
		cnDlgProp.dlgTitle = STR_TITLE_ADDDLG;
		cnDlgProp.wndDesc = STR_DLG_ADDSET;
		cnDlgProp.allowedSymbols = STR_DLG_ADDPATTERN;
		cnDlgProp.disallowError = STR_DLG_ADDERROR;
		cnDlgProp.maxLength = 3;
		break;

	case COMID_DELSET:
		cnDlgProp.dlgTitle = STR_TITLE_DELDLG;
		cnDlgProp.wndDesc = STR_DLG_DELSET;
		cnDlgProp.allowedSymbols = STR_DLG_DELPATTERN;
		cnDlgProp.disallowError = STR_DLG_DELERROR;
		break;
	}

	if (DialogBoxParam((HINSTANCE) GetWindowLong(wndHandle, GWL_HINSTANCE), MAKEINTRESOURCE(DIALOG_PATTERNINP), wndHandle, PatternInpDlgProc, (LPARAM) &cnDlgProp) != IDOK)
		return;

	switch (id)
	{
	case COMID_ADDFACT:
		AddFactors(atoi(cnDlgProp.resultText));
		break;

	case COMID_DELFACT:
		DeleteFactors(cnDlgProp.resultText);
		break;

	case COMID_ADDSET:
		AddSets(atoi(cnDlgProp.resultText));
		break;

	case COMID_DELSET:
		DeleteSets(cnDlgProp.resultText);
		break;
	}
}

void MainWindow::SaveAsCmd(void)
{
	size_t filSize;
	const char *fil, *ext;
	OPENFILENAME osDlgProp;
	char *filStr, name[MAX_PATH+4];

	switch (fileType)
	{
	case FileTypeEnum::DATA:
		filSize = sizeof(STR_FILTER_DATAEXT);
		fil = STR_FILTER_DATAEXT;
		ext = STR_DATAEXT;
		break;

	case FileTypeEnum::RESULT:
	case FileTypeEnum::RESULT_V2:
		filSize = sizeof(STR_FILTER_RESEXT);
		fil = STR_FILTER_RESEXT;
		ext = STR_RESEXT;
		break;
	}
	filStr = new char [filSize+1];

	memcpy(filStr, fil, filSize);
	filStr[--filSize] = 0;

	if (workFile)
		sprintf(name, "%s", workFile + pathLen);
	else
		name[0] = 0;

	memset(&osDlgProp, 0, sizeof(osDlgProp));
	osDlgProp.lStructSize = sizeof(osDlgProp);
	osDlgProp.nMaxFile = sizeof(name);
	osDlgProp.hwndOwner = wndHandle;
	osDlgProp.lpstrFilter = filStr;
	osDlgProp.lpstrDefExt = ext;
	osDlgProp.lpstrFile = name;

	osDlgProp.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&osDlgProp))
	{
		if (!SaveFile(name))
			MessageBox(wndHandle, STR_MSG_SAVEERROR, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
		else
		{
			SetWorkFile(name, osDlgProp.nFileOffset);
			changed = false;
			UpdateTitle();
		}
	}
	delete[] filStr;
}

void MainWindow::AboutCmd(void)
{
	HINSTANCE hInst;
	CN_DIALOG_DATA cnDlgProp;

	hInst = (HINSTANCE) GetWindowLong(wndHandle, GWL_HINSTANCE);

	cnDlgProp.dlgTitle = STR_TITLE_ABOUTDLG;
	cnDlgProp.wndDesc = STR_DLG_ABOUT;

	PlaySound(MAKEINTRESOURCE(MUSIC_CN), hInst, SND_RESOURCE | SND_ASYNC);
	DialogBoxParam(hInst, MAKEINTRESOURCE(DIALOG_ABOUT), wndHandle, AboutDlgProc, (LPARAM) &cnDlgProp);
	PlaySound(NULL, NULL, 0);
}

void MainWindow::HelpCmd(void)
{
	size_t len;
	HANDLE helpFile;
	char appDataPath[MAX_PATH+4], *helpFileName;

	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath) != S_OK)
		appDataPath[0] = 0;

	len = strlen(appDataPath);
	if (appDataPath[len] != '\\')
	{
		appDataPath[len] = '\\';
		appDataPath[++len] = 0;
	}

	len += strlen(STR_HELP_DIRNAME) + strlen(STR_HELP_FILENAME) + 3;
	helpFileName = new char [len+1];

	memcpy(helpFileName, appDataPath, len * sizeof(char));
	len = strlen(appDataPath);

	memcpy(helpFileName+len, STR_HELP_DIRNAME, strlen(STR_HELP_DIRNAME) * sizeof(char));
	helpFileName[len += strlen(STR_HELP_DIRNAME)] = 0;
	CreateDirectory(helpFileName, NULL);

	memcpy(helpFileName+len, STR_HELP_FILENAME, strlen(STR_HELP_FILENAME) * sizeof(char));
	helpFileName[len += strlen(STR_HELP_FILENAME)] = 0;

	if ((helpFile = CreateFile(helpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
	{
		HRSRC resInfo;
		size_t resSize;
		HGLOBAL resHandle;
		const void *resData;

		resInfo = FindResource(NULL, MAKEINTRESOURCE(MANUAL_RES), RT_RCDATA);
		resSize = SizeofResource(NULL, resInfo);
		resHandle = LoadResource(NULL, resInfo);
		resData = LockResource(resHandle);

		WriteBuf(helpFile, resData, resSize);
		CloseHandle(helpFile);
	}
	else if (GetFileAttributes(helpFileName) == 0xFFFFFFFF)
		return;

	len += 3;
	helpFileName[len] = 0;
	helpFileName[len-3] = ':';
	helpFileName[len-2] = ':';
	helpFileName[len-1] = '/';

	HtmlHelp(wndHandle, helpFileName, HH_DISPLAY_TOPIC, 0);
	delete[] helpFileName;
}

void MainWindow::OpenCmd(void)
{
	char *filStr, name[MAX_PATH+4];
	OPENFILENAME osDlgProp;
	size_t filSize;

	if (!AskSave())
		return;

	filSize = sizeof(STR_FILTER_NOEXT) + sizeof(STR_FILTER_ALLEXT) + sizeof(STR_FILTER_DATAEXT) + sizeof(STR_FILTER_RESEXT) + 1;
	filStr = new char [filSize];

	memcpy(filStr, STR_FILTER_ALLEXT, sizeof(STR_FILTER_ALLEXT));
	filSize = sizeof(STR_FILTER_ALLEXT)-1;
	memcpy(filStr+filSize, STR_FILTER_DATAEXT, sizeof(STR_FILTER_DATAEXT));
	filSize += sizeof(STR_FILTER_DATAEXT)-1;
	memcpy(filStr+filSize, STR_FILTER_RESEXT, sizeof(STR_FILTER_RESEXT));
	filSize += sizeof(STR_FILTER_RESEXT)-1;
	memcpy(filStr+filSize, STR_FILTER_NOEXT, sizeof(STR_FILTER_NOEXT));
	filSize += sizeof(STR_FILTER_NOEXT)-1;
	filStr[filSize] = 0;

	name[0] = 0;

	memset(&osDlgProp, 0, sizeof(osDlgProp));
	osDlgProp.lStructSize = sizeof(OPENFILENAME);
	osDlgProp.nMaxFile = sizeof(name);
	osDlgProp.hwndOwner = wndHandle;
	osDlgProp.lpstrFilter = filStr;
	osDlgProp.lpstrFile = name;

	osDlgProp.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&osDlgProp))
	{
		if (!LoadFile(name))
		{
			MessageBox(wndHandle, STR_MSG_OPENERROR, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
			ResetFile();
		}
		else
		{
			SetWorkFile(name, osDlgProp.nFileOffset);
			changed = false;
			UpdateTitle();
		}
	}
	delete[] filStr;
}

void MainWindow::StartCanAn(void)
{
	double d;
	int r, c, f;
	unsigned int i;
	CanAn analyzer;
	int mCol, rCnt, cCnt, sCnt;

	if (fileType != FileTypeEnum::DATA)
		return;
	if (!AskSave())
		return;

	for (mCol = sizeof(gFactCols) / sizeof(char*) - 1; mCol >= 0; --mCol)
		if (gFactCols[mCol] == STR_COL_FACTIVE)
			break;

	f = 0;
	rCnt = factGrid.GetRowsCount();
	for (r = 1; r < rCnt; ++r)
		if (factGrid.GetCellMark(r, mCol) == GWCM_MARKV)
			++f;

	r = setGrid.GetRowsCount()-1;
	if (r < 2 || f < 1)
	{
		MessageBox(wndHandle, STR_MSG_NOINFO, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
		return;
	}

	analyzer.SetSetCount(r);
	analyzer.SetFactCount(f);

	f = 0;
	rCnt = factGrid.GetRowsCount();
	cCnt = factGrid.GetColsCount();
	for (r = 1; r < rCnt; ++r)
	{
		if (factGrid.GetCellMark(r, mCol) != GWCM_MARKV)
			continue;

		for (c = 0; c < cCnt; ++c)
			if (gFactCols[c] == STR_COL_NAME)
				analyzer.SetFactName(f, factGrid.GetCell(r, c));
			else if (gFactCols[c] == STR_COL_STDDEV)
			{
				if (!ReadDouble(&factGrid, r, c, &d))
				{
					MessageBox(wndHandle, STR_MSG_BADVAL, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
					factGrid.SelectCells(r, c, 0, 0, true);
					ShowTab(0);
					return;
				}
				analyzer.SetStdDev(f, d);
			}
		++f;
	}

	sCnt = sizeof(gSetCols) / sizeof(char*) - 1;
	rCnt = setGrid.GetRowsCount();
	cCnt = setGrid.GetColsCount();
	for (r = 1; r < rCnt; ++r)
	{
		for (c = 0; c < sCnt; ++c)
			if (gSetCols[c] == STR_COL_NAME)
				analyzer.SetSetName(r-1, setGrid.GetCell(r, c));
			else if (gSetCols[c] == STR_COL_SETMARK)
			{
				if (!ReadUnsignedInt(&setGrid, r, c, &i))
				{
					MessageBox(wndHandle, STR_MSG_BADVAL, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
					setGrid.SelectCells(r, c, 0, 0, true);
					ShowTab(1);
					return;
				}
				analyzer.SetMark(r-1, i);
			}
			else if (gSetCols[c] == STR_COL_SETVOL)
			{
				if (!ReadUnsignedInt(&setGrid, r, c, &i))
				{
					MessageBox(wndHandle, STR_MSG_BADVAL, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
					setGrid.SelectCells(r, c, 0, 0, true);
					ShowTab(1);
					return;
				}
				analyzer.SetVol(r-1, i);
			}

		f = 0;
		for (; c < cCnt; ++c)
		{
			if (factGrid.GetCellMark(c-sCnt+1, mCol) != GWCM_MARKV)
				continue;

			if (!ReadDouble(&setGrid, r, c, &d))
			{
				MessageBox(wndHandle, STR_MSG_BADVAL, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
				setGrid.SelectCells(r, c, 0, 0, true);
				ShowTab(1);
				return;
			}
			analyzer.SetMean(r-1, f, d);
			++f;
		}
	}

	sCnt = sizeof(gCorCols) / sizeof(char*);
	cCnt = corGrid.GetColsCount();
	rCnt = corGrid.GetRowsCount();
	i = 0;
	for (r = 1; r < rCnt; ++r)
	{
		if (factGrid.GetCellMark(r, mCol) != GWCM_MARKV)
			continue;

		f = 0;
		for (c = sCnt; c < sCnt+r; ++c)
		{
			if (factGrid.GetCellMark(c-sCnt+1, mCol) != GWCM_MARKV)
				continue;

			if (!ReadDouble(&corGrid, r, c, &d))
			{
				MessageBox(wndHandle, STR_MSG_BADVAL, STR_TITLE_MAINWINDOW, MB_OK | MB_ICONWARNING);
				corGrid.SelectCells(r, c, 0, 0, true);
				ShowTab(2);
				return;
			}
			analyzer.SetCorMatr(i, f, d);
			++f;
		}
		++i;
	}

	analyzer.StartAnalysis();
	do
		Sleep(100);
	while (analyzer.AnalysisActive());

	r = analyzer.GetVarCount();
	c = analyzer.GetSetCount();
	f = analyzer.GetFactCount();
	varGrid.SetSizes(f + sizeof(gVarRows) / sizeof(char*), r+1);
	pntGrid.SetSizes(c+1, r + sizeof(gPntCols) / sizeof(char*) - 1);
	stdGrid.SetSizes(f + sizeof(gStdRows) / sizeof(char*), r+1);
	distGrid.SetSizes(c+1, c+1);

	sCnt = sizeof(gVarRows) / sizeof(char*);
	rCnt = varGrid.GetRowsCount();
	cCnt = varGrid.GetColsCount();
	for (r = sCnt; r < rCnt; ++r)
		varGrid.SetCell(r, 0, analyzer.GetFactName(r-sCnt));
	for (c = 1; c < cCnt; ++c)
	{
		for (r = 0; r < sCnt; ++r)
			if (gVarRows[r] == STR_ROW_EIGVAL)
				PrintDouble(&varGrid, r, c, analyzer.GetEigVal(c-1));
			else if (gVarRows[r] == STR_ROW_CUMPROP)
				PrintDouble(&varGrid, r, c, analyzer.GetCumProp(c-1));
			else if (gVarRows[r] == STR_ROW_CONST)
				PrintDouble(&varGrid, r, c, analyzer.GetConst(c-1));
		for (; r < rCnt; ++r)
			PrintDouble(&varGrid, r, c, analyzer.GetVarCoef(c-1, r-sCnt));
	}

	sCnt = sizeof(gPntCols) / sizeof(char*) - 1;
	rCnt = pntGrid.GetRowsCount();
	cCnt = pntGrid.GetColsCount();
	for (r = 1; r < rCnt; ++r)
		pntGrid.SetCell(r, 0, analyzer.GetSetName(r-1));
	for (r = 1; r < rCnt; ++r)
	{
		for (c = 0; c < sCnt; ++c)
			if (gPntCols[c] == STR_COL_SETMARK)
				PrintUnsignedInt(&pntGrid, r, c, analyzer.GetMark(r-1));
		for (; c < cCnt; ++c)
			PrintDouble(&pntGrid, r, c, analyzer.GetSetCoord(r-1, c-sCnt));
	}

	sCnt = sizeof(gStdRows) / sizeof(char*);
	rCnt = stdGrid.GetRowsCount();
	cCnt = stdGrid.GetColsCount();
	for (r = sCnt; r < rCnt; ++r)
		stdGrid.SetCell(r, 0, analyzer.GetFactName(r-sCnt));
	for (c = 1; c < cCnt; ++c)
	{
		for (r = 0; r < sCnt; ++r)
			if (gStdRows[r] == STR_ROW_EIGVAL)
				PrintDouble(&stdGrid, r, c, analyzer.GetEigVal(c-1));
			else if (gStdRows[r] == STR_ROW_CUMPROP)
				PrintDouble(&stdGrid, r, c, analyzer.GetCumProp(c-1));
		for (; r < rCnt; ++r)
			PrintDouble(&stdGrid, r, c, analyzer.GetStdCoef(c-1, r-sCnt));
	}

	sCnt = sizeof(gDistCols) / sizeof(char*);
	rCnt = distGrid.GetRowsCount();
	cCnt = distGrid.GetColsCount();
	for (r = 1; r < rCnt; ++r)
		distGrid.SetCell(r, 0, analyzer.GetSetName(r-1));
	for (r = 1; r < rCnt; ++r)
		for (c = sCnt; c < cCnt; ++c)
			PrintDouble(&distGrid, r, c, analyzer.GetSetsDist(r-1, c-sCnt));

	SetResultGridsFixedCells(true, true, true, true);
	SetFileType(FileTypeEnum::RESULT_V2);
	SetWorkFile(NULL);

	changed = true;
	UpdateTitle();
}

void MainWindow::InitTabCtrl(void)
{
	HINSTANCE hInst;

	hInst = (HINSTANCE) GetWindowLong(wndHandle, GWL_HINSTANCE);
	tabCtrl = CreateWindow(WC_TABCONTROL, "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_MULTILINE, 0, 0, 100, 100, wndHandle, 0, hInst, NULL);
}

void MainWindow::InitGridsFont(void)
{
	if (!gFont)
	{
		HDC hDC;
		int size;

		if (!(hDC = GetDC(wndHandle)))
			size = -13;
		else
		{
			size = -MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72);
			ReleaseDC(wndHandle, hDC);
		}

		gFont = CreateFont(size, 0, 0, 0, 0, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
	}

	factGrid.SetFont(gFont);
	setGrid.SetFont(gFont);
	corGrid.SetFont(gFont);

	varGrid.SetFont(gFont);
	pntGrid.SetFont(gFont);
	stdGrid.SetFont(gFont);
	distGrid.SetFont(gFont);
}

void MainWindow::InitClass(HINSTANCE hInst)
{
	WNDCLASS wndClass;
	if (classReady) return;

	wndClass.cbClsExtra = 0;
    wndClass.hInstance = hInst;
    wndClass.lpszMenuName = NULL;
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);

	wndClass.cbWndExtra = sizeof(LONG_PTR);
    wndClass.lpfnWndProc = WinWndClasserProc;
	wndClass.lpszClassName = CN_MW_CLASSNAME;

	wndClass.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
	wndClass.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ICON_MAIN));

	if (RegisterClass(&wndClass)) classReady = true;
}



LRESULT MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR wndPtr;
	if ((wndPtr = GetWindowLongPtr(hWnd, 0)))
		return ((MainWindow*) wndPtr) -> WindowProc(hWnd, uMsg, wParam, lParam);
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK PatternInpDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CN_DIALOG_DATA *dlgProp;

	HICON mainIcon;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		dlgProp = (CN_DIALOG_DATA*) lParam;

		mainIcon = (HICON) LoadImage((HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE), MAKEINTRESOURCE(ICON_MAIN), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		SendDlgItemMessage(hWnd, PATTERNINP_EDIT_ID, EM_LIMITTEXT, dlgProp->maxLength, 0);
		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) mainIcon);
		SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM) dlgProp->dlgTitle);
		SetDlgItemText(hWnd, PATTERNINP_TEXT_ID, dlgProp->wndDesc);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hWnd, PATTERNINP_EDIT_ID, dlgProp->resultText, sizeof(dlgProp->resultText));
			[[fallthrough]];
		case IDCANCEL:
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		case PATTERNINP_EDIT_ID:
			{
				bool needBaloon = (HIWORD(wParam) == EN_MAXTEXT);
				if (HIWORD(wParam) == EN_UPDATE)
				{
					const char *allowedEnd = dlgProp->allowedSymbols + strlen(dlgProp->allowedSymbols);

					DWORD caretPos;
					SendMessage((HWND) lParam, EM_GETSEL, (WPARAM) &caretPos, 0);
					GetDlgItemText(hWnd, PATTERNINP_EDIT_ID, dlgProp->resultText, sizeof(dlgProp->resultText));

					char *curPos, *checkPos;
					for (curPos = checkPos = dlgProp->resultText; *checkPos; ++checkPos) {
						if (std::find(dlgProp->allowedSymbols, allowedEnd, *checkPos) != allowedEnd)
						{
							if (curPos != checkPos)
								*curPos = *checkPos;
							++curPos;
						}
						else if (static_cast<int>(caretPos) > curPos - dlgProp->resultText)
							--caretPos;
					}
					*curPos = 0;

					if (curPos != checkPos)
					{
						needBaloon = true;
						SetDlgItemText(hWnd, PATTERNINP_EDIT_ID, dlgProp->resultText);
						SendMessage((HWND) lParam, EM_SETSEL, caretPos, caretPos);
					}
				}
				if (needBaloon)
				{
					wchar_t wideTitle[32], wideText[128];
					MultiByteToWideChar(CP_ACP, 0, STR_TITLE_PATTERNERROR, -1, wideTitle, sizeof(wideTitle) / sizeof(wchar_t));
					MultiByteToWideChar(CP_ACP, 0, dlgProp->disallowError, -1, wideText, sizeof(wideText) / sizeof(wchar_t));

					EDITBALLOONTIP tooltipInfo;
					tooltipInfo.cbStruct = sizeof(tooltipInfo);
					tooltipInfo.pszText = wideText;
					tooltipInfo.pszTitle = wideTitle;
					tooltipInfo.ttiIcon = TTI_ERROR;
					SendMessage((HWND) lParam, EM_SHOWBALLOONTIP, 0, (LPARAM) &tooltipInfo);
				}
				else
					SendMessage((HWND) lParam, EM_HIDEBALLOONTIP, 0, 0);
				return TRUE;
			}
		}
		break;

	case WM_CLOSE:
		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	}

	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	int size;
	char *str;
	size_t len;
	HFONT eFont;
	HICON mainIcon;
	CN_DIALOG_DATA *dlgProp;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		len = strlen(STR_TITLE_MAINWINDOW) + strlen(STR_VERSION) + 4;
		dlgProp = (CN_DIALOG_DATA*) lParam;
		str = new char [len];

		mainIcon = (HICON) LoadImage((HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE), MAKEINTRESOURCE(ICON_MAIN), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		len = sprintf(str, "%s %s", STR_TITLE_MAINWINDOW, STR_VERSION);
		str[len] = 0;

		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) mainIcon);
		SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM) dlgProp->dlgTitle);
		SetDlgItemText(hWnd, ABOUT_TEXT_ID, dlgProp->wndDesc);
		SetDlgItemText(hWnd, ABOUT_PROGINFO_ID, str);

		if (!(hDC = GetDC(hWnd)))
			size = -32;
		else
		{
			size = -MulDiv(24, GetDeviceCaps(hDC, LOGPIXELSY), 72);
			ReleaseDC(hWnd, hDC);
		}

		eFont = CreateFont(size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Times New Roman");
		SendDlgItemMessage(hWnd, ABOUT_PROGINFO_ID, WM_SETFONT, (WPARAM) eFont, 0);

		mainIcon = (HICON) LoadImage((HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE), MAKEINTRESOURCE(ICON_MAIN), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
		SendDlgItemMessage(hWnd, ABOUT_ICON_ID, STM_SETICON, (WPARAM) mainIcon, 0);

		delete[] str;
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			eFont = (HFONT) SendDlgItemMessage(hWnd, ABOUT_PROGINFO_ID, WM_GETFONT, 0, 0);
			SendDlgItemMessage(hWnd, ABOUT_PROGINFO_ID, WM_SETFONT, (WPARAM) NULL, 0);

			if (eFont)
				DeleteObject(eFont);

			EndDialog(hWnd, IDOK);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		eFont = (HFONT) SendDlgItemMessage(hWnd, ABOUT_PROGINFO_ID, WM_GETFONT, 0, 0);
		SendDlgItemMessage(hWnd, ABOUT_PROGINFO_ID, WM_SETFONT, (WPARAM) NULL, 0);

		if (eFont)
			DeleteObject(eFont);

		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	}

	return FALSE;
}



bool SaveGridData(GridWindow* grid, HANDLE hFile, int fstCol /* = 1 */)
{
	size_t len;
	const char *cell;
	unsigned int mark;
	int i, j, rows, cols;

	if (!grid || !hFile)
		return false;

	rows = grid->GetRowsCount();
	cols = grid->GetColsCount();

	if (!WriteBuf(hFile, &rows, sizeof(rows)))
		return false;
	if (!WriteBuf(hFile, &cols, sizeof(cols)))
		return false;

	for (i = 1; i < rows; ++i)
	{
		for (j = fstCol; j < cols; ++j)
		{
			if (grid->GetCellProp(i, j) & GWCP_MARKCELL)
			{
				mark = grid->GetCellMark(i, j);
				len = (size_t) -1;
				if (!WriteBuf(hFile, &len, sizeof(len)))
					return false;
				if (!WriteBuf(hFile, &mark, sizeof(mark)))
					return false;
			}
			else
			{
				cell = grid->GetCell(i, j);
				len = (cell ? strlen(cell) : 0);
				if (!WriteBuf(hFile, &len, sizeof(len)))
					return false;
				if (len && !WriteBuf(hFile, cell, len))
					return false;
			}
		}
	}
	return true;
}

bool LoadGridData(GridWindow* grid, HANDLE hFile, int fstCol /* = 1 */)
{
	char *cell;
	size_t len;
	unsigned int mark;
	int i, j, rows, cols;

	if (!grid || !hFile)
		return false;

	if (!ReadBuf(hFile, &rows, sizeof(rows)))
		return false;
	if (!ReadBuf(hFile, &cols, sizeof(cols)))
		return false;

	for (i = 1; i < rows; ++i)
	{
		for (j = fstCol; j < cols; ++j)
		{
			if (!ReadBuf(hFile, &len, sizeof(len)))
				return false;

			if (!len)
				grid->SetCell(i, j, NULL);
			else if (len == (size_t) -1)
			{
				if (!ReadBuf(hFile, &mark, sizeof(mark)))
					return false;

				grid->SetCellMark(i, j, mark);
			}
			else
			{
				cell = new char [len+1];
				if (!ReadBuf(hFile, cell, len))
				{
					delete[] cell;
					return false;
				}

				cell[len] = 0;
				grid->SetCell(i, j, cell);
				delete[] cell;
			}
		}
	}
	return true;
}

bool SaveGridSizes(GridWindow* grid, HANDLE hFile)
{
	int i, num;
	unsigned int val;

	if (!grid || !hFile)
		return false;

	num = grid->GetColsCount();
	if (!WriteBuf(hFile, &num, sizeof(num)))
		return false;

	for (i = 0; i < num; ++i)
	{
		val = grid->GetColWidth(i);
		if (!WriteBuf(hFile, &val, sizeof(val)))
			return false;
	}

	num = grid->GetRowsCount();
	if (!WriteBuf(hFile, &num, sizeof(num)))
		return false;

	for (i = 0; i < num; ++i)
	{
		val = grid->GetRowHeight(i);
		if (!WriteBuf(hFile, &val, sizeof(val)))
			return false;
	}

	return true;
}

bool LoadGridSizes(GridWindow* grid, HANDLE hFile)
{
	int i, num;
	unsigned int val;

	if (!grid || !hFile)
		return false;

	if (!ReadBuf(hFile, &num, sizeof(num)))
		return false;
	for (i = 0; i < num; ++i)
	{
		if (!ReadBuf(hFile, &val, sizeof(val)))
			return false;
		grid->SetColWidth(i, val);
	}

	if (!ReadBuf(hFile, &num, sizeof(num)))
		return false;
	for (i = 0; i < num; ++i)
	{
		if (!ReadBuf(hFile, &val, sizeof(val)))
			return false;
		grid->SetRowHeight(i, val);
	}

	return true;
}

void ChangeDecSep(GridWindow *grid, int r, int c)
{
	unsigned int i, len;
	const char *cell;
	char *buf;
	bool need;

	if (!grid || !(cell = grid->GetCell(r, c)))
		return;

	len = strlen(cell);
	buf = new char [len+1];
	memcpy(buf, cell, len);
	buf[len] = 0;

	need = false;
	for (i = 0; i < len; ++i)
	{
		if (buf[i] == ',')
		{
			buf[i] = '.';
			need = true;
		}
		else if (buf[i] == '.')
		{
			buf[i] = ',';
			need = true;
		}
	}

	if (need)
		grid->SetCell(r, c, buf);
	delete[] buf;
}


bool ReadDouble(GridWindow *grid, int r, int c, double *val)
{
	unsigned int i, len, read;
	const char *cell;
	char *buf;

	if (!val || !grid || !(cell = grid->GetCell(r, c)))
		return false;

	len = strlen(cell);
	buf = new char [len+1];
	memcpy(buf, cell, len);
	buf[len] = 0;

	for (i = 0; i < len; ++i)
		if (buf[i] == ',')
			buf[i] = '.';

	sscanf(buf, "%lf%n", val, &read);
	delete[] buf;

	return (read == len);
}

bool ReadUnsignedInt(GridWindow *grid, int r, int c, unsigned int *val)
{
	const char *cell;
	unsigned int len;

	if (!val || !grid || !(cell = grid->GetCell(r, c)))
		return false;

	sscanf(cell, "%u%n", val, &len);
	return (len == strlen(cell));
}

void PrintDouble(GridWindow *grid, int r, int c, double val)
{
	char text[64];

	if (!grid)
		return;

	sprintf(text, "%.5lf", val);
	grid->SetCell(r, c, text);
}

void PrintUnsignedInt(GridWindow *grid, int r, int c, unsigned int val)
{
	char text[64];

	if (!grid)
		return;

	sprintf(text, "%u", val);
	grid->SetCell(r, c, text);
}
