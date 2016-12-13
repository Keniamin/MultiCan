#ifndef _WINWND_CLASSER_H
#define _WINWND_CLASSER_H

#include <windows.h>

static LONG_PTR wwcObject;
static LRESULT WINAPI (*wwcFunction) (HWND, UINT, WPARAM, LPARAM);

static LRESULT WINAPI WinWndClasserProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SetWindowLongPtr(hWnd, 0, wwcObject);
	SetWindowLongPtr(hWnd, GWL_WNDPROC, (LONG_PTR) wwcFunction);
	return wwcFunction(hWnd, uMsg, wParam, lParam);
}

#endif
