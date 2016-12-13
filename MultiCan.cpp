#include <windows.h>
#include <htmlhelp.h>

#include "MainWindow.h"

#include "Common/CmdLine.h"
#include "Common/TitleWindow.h"

#include "MultiCan/Resources.h"

const unsigned int TITLE_TIME = 1000;

TitleWindow *gTitle;
MainWindow *gMain;

VOID CALLBACK TitleTimerProc(HWND, UINT, UINT, DWORD);

int WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
	int ret;
	MSG winMsg;
	CmdLine args;
	DWORD hlpCookie;
	HACCEL accTable;
	MainWindow mainWnd;
	HBITMAP titleBitmap;
	TitleWindow titleWnd;
	
	titleBitmap = (HBITMAP) LoadImage(hInstance, MAKEINTRESOURCE(BITMAP_TITLE), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	titleWnd.SetBitmap(titleBitmap);
	titleWnd.Create(hInstance);
	
	gMain = &mainWnd;
	gTitle = &titleWnd;
	SetTimer(NULL, 0, TITLE_TIME, TitleTimerProc);
	
	HtmlHelp(NULL, NULL, HH_INITIALIZE, (DWORD_PTR) &hlpCookie);
	
	srand(GetTickCount());
	args = lpCmdLine;
	if (args.count() > 0)
		mainWnd.SetWorkFile(args[0]);
	if (!mainWnd.Create(hInstance))
		return 0;
	
	ShowWindow(mainWnd, nCmdShow);
	accTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(ACCTABLE_MENU));
	while ((ret = GetMessage(&winMsg, NULL, 0, 0)))
	{
		if (ret != -1 && !HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD_PTR) &winMsg) && !TranslateAccelerator(mainWnd, accTable, &winMsg))
		{
			TranslateMessage(&winMsg);
			DispatchMessage(&winMsg);
		}
	}
	
	HtmlHelp(NULL, NULL, HH_UNINITIALIZE, hlpCookie);
	DeleteObject(titleBitmap);
	return winMsg.wParam;
}

VOID CALLBACK TitleTimerProc(HWND, UINT, UINT id, DWORD)
{
	if (gTitle)
	{
		HWND wnd = NULL;
		
		if (gMain)
			wnd = *gMain;
		gTitle->Release(wnd);
	}
	KillTimer(NULL, id);
}
