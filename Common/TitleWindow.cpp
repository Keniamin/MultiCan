#include <stdio.h>
#include <windows.h>

#include "TitleWindow.h"
#include "WinWndClasser.h"

#define WM_SETBITMAP (WM_USER+1)
#define WM_CANCLOSE (WM_USER+2)
#define WM_EXTCLOSE (WM_USER+3)

const char TW_CLASSNAME[] = "TitleWndClass";

LRESULT WINAPI TitleWindowProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI WindowThread(LPVOID);

bool TitleWindow::classReady = false;

TitleWindow::TitleWindow(void):
	hInst(NULL),
	wThread(NULL),
	thrEvent(NULL),
	wndHandle(NULL),
	actWnd(NULL),

	canClose(false),
	bitmapHandle(NULL)
{
	thrEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

TitleWindow::~TitleWindow(void)
{
	Destroy();
	if (wThread)
		CloseHandle(wThread);
	if (thrEvent)
		CloseHandle(thrEvent);
}

bool TitleWindow::Create(HINSTANCE hInstance)
{
	DWORD tid;

	CheckThread();
	if (wThread != 0)
		return false;

	hInst = hInstance;

	InitClass();
	ResetEvent(thrEvent);
	if ((wThread = CreateThread(NULL, 0, WindowThread, this, 0, &tid)))
		WaitForSingleObject(thrEvent, INFINITE);
	return wThread;
}

void TitleWindow::SetBitmap(HBITMAP bitmap)
{
	CheckThread();
	if (wThread == 0)
		bitmapHandle = bitmap;
	else
		PostMessage(wndHandle, WM_SETBITMAP, 0, (LPARAM) bitmap);
}

void TitleWindow::Destroy(HWND hWnd /* = NULL */)
{
	CheckThread();
	if (wThread == 0)
		return;

	ResetEvent(thrEvent);
	PostMessage(wndHandle, WM_EXTCLOSE, 0, (LPARAM) hWnd);
	WaitForSingleObject(thrEvent, INFINITE);
}

void TitleWindow::Release(HWND hWnd /* = NULL */)
{
	CheckThread();
	if (wThread == 0)
		return;

	PostMessage(wndHandle, WM_CANCLOSE, 0, (LPARAM) hWnd);
}

LRESULT TitleWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		break;

	case WM_DESTROY:
		wndHandle = NULL;
		PostQuitMessage(0);
		break;

	case WM_CLOSE:
		if (actWnd && GetForegroundWindow() == wndHandle)
			SetForegroundWindow(actWnd);
		break;

	case WM_PAINT:
		DrawWindow();
		break;

	case WM_SETBITMAP:
		bitmapHandle = (HBITMAP) lParam;
		SetSizes();
		return 0;

	case WM_CANCLOSE:
		actWnd = (HWND) lParam;
		canClose = true;
		return 0;

	case WM_EXTCLOSE:
		actWnd = (HWND) lParam;
		PostMessage(wndHandle, WM_CLOSE, 0, 0);
		return 0;

	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (canClose)
			PostMessage(wndHandle, WM_CLOSE, 0, 0);
		break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE && canClose)
			PostMessage(wndHandle, WM_CLOSE, 0, 0);
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int TitleWindow::wFunc(void)
{
	int ret;
	MSG winMsg;

	wwcObject = (LONG_PTR) this;
	wwcFunction = TitleWindowProc;
	if (!CreateWindow(TW_CLASSNAME, "", WS_POPUP, 0, 0, defSize, defSize, NULL, NULL, hInst, NULL))
	{
		SetEvent(thrEvent);
		return 0;
	}
	SetEvent(thrEvent);

	SetSizes();
	ShowWindow(wndHandle, SW_SHOWNORMAL);
	while ((ret = GetMessage(&winMsg, NULL, 0, 0)))
		if (ret != -1)
		{
			TranslateMessage(&winMsg);
			DispatchMessage(&winMsg);
		}
	SetEvent(thrEvent);
	return winMsg.wParam;
}

void TitleWindow::SetSizes(void)
{
	int x, y, tmp;
	BITMAP bmpInfo;

	if (!wndHandle)
		return;

	x = y = 0;
	bmpInfo.bmWidth = bmpInfo.bmHeight = defSize;

	if (bitmapHandle)
		GetObject(bitmapHandle, sizeof(bmpInfo), &bmpInfo);
	if ((tmp = GetSystemMetrics(SM_CYSCREEN)))
		y = (tmp - bmpInfo.bmHeight) / 2;
	if ((tmp = GetSystemMetrics(SM_CXSCREEN)))
		x = (tmp - bmpInfo.bmWidth) / 2;

	SetWindowPos(wndHandle, NULL, x, y, bmpInfo.bmWidth, bmpInfo.bmHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	RedrawWindow(wndHandle, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE);
}

void TitleWindow::DrawWindow(void)
{
	HDC hDC;
	HPEN pen;
	HBRUSH brush;
	RECT wndClRect;
	PAINTSTRUCT paintInfo;
	HGDIOBJ oldBrush, oldPen;

	GetClientRect(wndHandle, &wndClRect);
	if (bitmapHandle)
	{
		++wndClRect.right;
		++wndClRect.bottom;
		pen = (HPEN) GetStockObject(NULL_PEN);
		brush = CreatePatternBrush(bitmapHandle);
	}
	else
	{
		pen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNTEXT));
		brush = GetSysColorBrush(COLOR_WINDOW);
	}

	hDC = BeginPaint(wndHandle, &paintInfo);
	oldBrush = SelectObject(hDC, brush);
	oldPen = SelectObject(hDC, pen);

	Rectangle(hDC, 0, 0, wndClRect.right, wndClRect.bottom);

	if (oldPen)
		SelectObject(hDC, oldPen);
	if (oldBrush)
		SelectObject(hDC, oldBrush);
	EndPaint(wndHandle, &paintInfo);

	if (bitmapHandle)
		DeleteObject(brush);
	else
		DeleteObject(pen);
}

void TitleWindow::CheckThread(void)
{
	DWORD state;
	if (wThread)
	{
		if (GetExitCodeThread(wThread, &state))
			if (state != STILL_ACTIVE)
			{
				CloseHandle(wThread);
				wThread = 0;
			}
	}
}

void TitleWindow::InitClass(void)
{
	WNDCLASS wndClass;
	if (classReady)
		return;

	wndClass.hIcon = NULL;
	wndClass.cbClsExtra = 0;
    wndClass.hInstance = hInst;
    wndClass.lpszMenuName = NULL;
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);

	wndClass.cbWndExtra = sizeof(LONG_PTR);
    wndClass.lpfnWndProc = WinWndClasserProc;
	wndClass.lpszClassName = TW_CLASSNAME;

	if (RegisterClass(&wndClass))
		classReady = true;
}

LRESULT TitleWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR wndPtr;
	if ((wndPtr = GetWindowLongPtr(hWnd, 0)))
		return ((TitleWindow*) wndPtr) -> WindowProc(hWnd, uMsg, wParam, lParam);
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI WindowThread(LPVOID param)
{
	if (param == NULL)
		return 0;

	return ((TitleWindow*) param)->wFunc();
}
