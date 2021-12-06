#ifndef _TITLEWINDOW_H
#define _TITLEWINDOW_H

#include <windows.h>

class TitleWindow
{
private:
	static const int defSize = 128;

	static bool classReady;

	HINSTANCE hInst;
	HANDLE wThread, thrEvent;
	HWND wndHandle, actWnd;

	bool canClose;
	HBITMAP bitmapHandle;

	int wFunc(void);
	void SetSizes(void);
	void DrawWindow(void);
	void CheckThread(void);
	void InitClass(void);

	friend DWORD WINAPI WindowThread(LPVOID);

public:
	TitleWindow(void);
	~TitleWindow(void);

	bool Create(HINSTANCE);
	void SetBitmap(HBITMAP bitmap);
	void Destroy(HWND make_active = NULL);
	void Release(HWND make_active = NULL);
	LRESULT WindowProc(HWND, UINT, WPARAM, LPARAM);

	void ClearBitmap(void) { SetBitmap(NULL); }
};

#endif
