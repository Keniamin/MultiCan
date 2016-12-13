#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "UndoList.h"
#include "GridWindow.h"
#include "WinWndClasser.h"

#define EN_KEYDOWN WM_KEYDOWN

class GridUndoOperation: public UndoListElem
{
public:
	char type;
	int row, col;
	GridWindow *wnd;
	unsigned int tag;
	union
	{
		char *text;
		unsigned int prop;
		unsigned int mark;
		COLORREF cellColor;
		COLORREF textColor;
		unsigned int size;
	};
	
	GridUndoOperation(void);
	~GridUndoOperation(void);
	
	void Apply(void);
};

struct EKDNM
{
	NMHDR hdr;
	WPARAM wParam;
	LPARAM lParam;
	bool *stopProc;
};

struct MenuItem
{
    const char *itemName;
	int chOrComId;
};

const MenuItem POP_MENU[] = {
	{"Отменить", WM_UNDO},
	{"Вернуть", GM_REDO},
	{NULL, 0},
	{"Вырезать", WM_CUT},
	{"Копировать", WM_COPY},
	{"Вставить", WM_PASTE},
	{"Удалить", WM_CLEAR},
	{NULL, 0},
	{"Выделить все", GM_SELALL}
};

const unsigned char SELECT_CELLS = 'S';
const unsigned char DRAG_ROW = 'R';
const unsigned char DRAG_COL = 'C';

const UINT ENSC_CID = 1;
const UINT DBLCLK_TIMERID = 1;
const UINT SCROLL_TIMERID = 2;
const char GW_CLASSNAME[] = "GridWndClass";
const char CF_HTMLFORMAT[] = "HTML Format";

const char HTML_FSTRTEXT[] = "<!--StartFragment-->\r\n";
const char HTML_FENDTEXT[] = "<!--EndFragment-->\r\n";

const char HTML_FRAGSTART[] = "\r\nStartFragment:0000000000";
const char HTML_FRAGEND[] = "\r\nEndFragment:0000000000";
const char HTML_START[] = "\r\nStartHTML:0000000000";
const char HTML_END[] = "\r\nEndHTML:0000000000";
const char HTML_VER[] = "Version:1.0";
const int HTML_FIELDSIZE = 10;

void SetVal(ExtString* string, int position, int value);
LRESULT WINAPI GridWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI EditNotifyProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);



GridUndoOperation::GridUndoOperation(void):
	type(0),
	row(-1), col(-1),
	wnd(NULL), tag(0)
{
	text = 0;
	prop = 0;
	mark = 0;
	cellColor = 0;
	textColor = 0;
	size = 0;
}

GridUndoOperation::~GridUndoOperation(void)
{
	if (type == GWUO_TEXT && text)
		delete[] text;
}

void GridUndoOperation::Apply(void)
{
	COLORREF crTmp;
	unsigned int uiTmp;
	
	if (!wnd || type == 0)
		return;
	
	switch (type)
	{
	case GWUO_TEXT:
		if (row >= 0 && col >= 0 && row < wnd->nRows && col < wnd->nCols)
		{
			char *tmp;
			size_t index = wnd->nCols;
			index *= row;
			index += col;
			
			wnd->EnsureCells();
			
			tmp = text;
			text = wnd->cells[index].text;
			wnd->cells[index].text = tmp;
			
			wnd->SendChangeNotify(row, col);
		}
		break;
	
	case GWUO_PROP:
		uiTmp = wnd->GetCellProp(row, col);
		wnd->SetCellProp(row, col, prop);
		prop = uiTmp;
		break;
	
	case GWUO_MARK:
		uiTmp = wnd->GetCellMark(row, col);
		wnd->SetCellMark(row, col, mark);
		mark = uiTmp;
		break;
	
	case GWUO_CCOL:
		crTmp = wnd->GetCellColor(row, col);
		wnd->SetCellColor(row, col, cellColor);
		cellColor = crTmp;
		break;
	
	case GWUO_TCOL:
		crTmp = wnd->GetCellTextColor(row, col);
		wnd->SetCellTextColor(row, col, textColor);
		textColor = crTmp;
		break;
	
	case GWUO_SIZE:
		if (row < 0)
		{
			uiTmp = wnd->GetColWidth(col);
			wnd->SetColWidth(col, size);
			size = uiTmp;
		}
		else
		{
			uiTmp = wnd->GetRowHeight(row);
			wnd->SetRowHeight(row, size);
			size = uiTmp;
		}
		break;
	}
}



GridCell::GridCell(void):
	text(NULL),
	prop(GWCP_NORMAL),
	mark(0),
	
	textColor(0),
	cellColor(0),
	selColor(0),
	markBoxColor(0)
{
	textColor = GetSysColor(COLOR_BTNTEXT);
	cellColor = GetSysColor(COLOR_WINDOW);
	InitColors();
}

void GridCell::InitColors(void)
{
	BYTE r, g, b;
	DWORD hl;
	
	hl = GetSysColor(COLOR_HIGHLIGHT);
	r = (2*GetRValue(cellColor) + GetRValue(hl)) / 3;
	g = (2*GetGValue(cellColor) + GetGValue(hl)) / 3;
	b = (2*GetBValue(cellColor) + GetBValue(hl)) / 3;
	selColor = RGB(r, g, b);
	
	r = 255 - GetRValue(cellColor);
	g = 255 - GetGValue(cellColor);
	b = 255 - GetBValue(cellColor);
	markBoxColor = RGB(r, g, b);
}

void GridCell::Grab(GridCell *cell)
{
	if (!cell)
		return;
	
	text = cell->text;
	prop = cell->prop;
	mark = cell->mark;
	
	textColor = cell->textColor;
	cellColor = cell->cellColor;
	selColor = cell->selColor;
	markBoxColor = cell->markBoxColor;
	
	cell->text = NULL;
}

bool GridCell::SwitchMark(void)
{
	int i;
	for (i = 1; i <= GWCM_MARKCNT; ++i)
	{
		if (!(mark >>= 1))
			mark = 1 << (GWCM_MARKCNT-1);
		if ((mark << GWCM_MARKBASE) & prop)
			break;
	}
	return (i < GWCM_MARKCNT);
}

void GridCell::DrawCell(HDC hDC, const RECT& cellRect, bool sel) const
{
	HBRUSH cellBrush, nullBrush;
	HPEN nullPen;
	
	if (sel)
		cellBrush = CreateSolidBrush(selColor);
	else
		cellBrush = CreateSolidBrush(cellColor);
	nullBrush = (HBRUSH) GetStockObject(NULL_BRUSH);
	nullPen = (HPEN) GetStockObject(NULL_PEN);
	
	SelectObject(hDC, nullPen);
	SelectObject(hDC, cellBrush);
	Rectangle(hDC, cellRect.left, cellRect.top, cellRect.right+1, cellRect.bottom+1);
	
	SelectObject(hDC, nullBrush);
	if (cellBrush)
		DeleteObject(cellBrush);
	
	if (prop & GWCP_MARKCELL)
	{
		HRGN clipRgn, cRg;
		HPEN markPen;
		POINT pts[5];
		RECT markRect;
		HBRUSH markBrush;
		int tmp, tmp2;
		
		markRect = cellRect;
		if (prop & GWCA_LEFT)
		{
			markRect.left += markPadding + 1;
			markRect.right = markRect.left + markSize;
		}
		else if (prop & GWCA_CENTER)
		{
			tmp = cellRect.left + cellRect.right;
			markRect.right = (tmp+markSize) / 2;
			markRect.left = (tmp-markSize) / 2;
		}
		else
		{
			markRect.right -= markPadding + 1;
			markRect.left = markRect.right - markSize;
		}
		
		if (prop & GWCA_BOTTOM)
		{
			markRect.bottom -= markPadding + 1;
			markRect.top = markRect.bottom - markSize;
		}
		else if (prop & GWCA_VCENTER)
		{
			tmp = cellRect.top + cellRect.bottom;
			markRect.bottom = (tmp+markSize) / 2;
			markRect.top = (tmp-markSize) / 2;
		}
		else
		{
			markRect.top += markPadding + 1;
			markRect.bottom = markRect.top + markSize;
		}
		
		if ((cRg = clipRgn = CreateRectRgnIndirect(&cellRect)))
		{
			SelectClipRgn(hDC, clipRgn);
			DeleteObject(clipRgn);
		}
		
		SelectObject(hDC, nullBrush);
		if ((mark << GWCM_MARKBASE) != GWCM_MARKN)
		{
			markPen = CreatePen(PS_SOLID, 1, textColor);
			markBrush = CreateSolidBrush(textColor);
			SelectObject(hDC, markBrush);
			SelectObject(hDC, markPen);
			
			switch (mark << GWCM_MARKBASE)
			{
			case GWCM_MARKP:
				tmp2 = markSize / 5;
				tmp = (markRect.left + markRect.right) / 2;
				
				pts[0].x = markRect.left + tmp2;
				pts[0].y = markRect.bottom-1;
				pts[1].x = tmp;
				pts[1].y = markRect.top;
				pts[2].x = markRect.right-1 - tmp2;
				pts[2].y = markRect.bottom-1;
				pts[3].x = markRect.right - tmp2;
				pts[3].y = markRect.bottom;
				pts[4].x = tmp;
				pts[4].y = markRect.bottom-2*tmp2;
				Polygon(hDC, pts, 5);
				
				pts[0].x = markRect.left;
				pts[0].y = markRect.top + 2*tmp2;
				pts[1].x = markRect.right-1;
				pts[1].y = markRect.top + 2*tmp2;
				pts[2].x = tmp;
				pts[2].y = markRect.bottom-2*tmp2;
				Polygon(hDC, pts, 3);
				break;
			
			case GWCM_MARKV:
				tmp = (markRect.top + markRect.bottom) / 2;
				pts[0].x = markRect.left;
				pts[0].y = tmp;
				
				tmp = (markRect.left + markRect.right) / 2;
				tmp2 = (2*markRect.top + 3*markRect.bottom) / 5;
				
				pts[1].x = tmp;
				pts[1].y = markRect.bottom-1;
				pts[2].x = markRect.right;
				pts[2].y = markRect.top-1;
				pts[3].x = tmp-1;
				pts[3].y = tmp2;
				Polygon(hDC, pts, 4);
				break;
			
			case GWCM_MARKX:
				tmp = (2*markRect.left + 3*markRect.right) / 5;
				tmp2 = (2*markRect.top + 3*markRect.bottom) / 5;
				
				pts[0].x = markRect.left-1;
				pts[0].y = markRect.top-1;
				pts[1].x = tmp;
				pts[1].y = markRect.top + markRect.bottom - tmp2-1;
				pts[2].x = markRect.right;
				pts[2].y = markRect.bottom;
				pts[3].x = markRect.left + markRect.right - tmp-1;
				pts[3].y = tmp2;
				Polygon(hDC, pts, 4);
				
				tmp = (2*markRect.left + 3*markRect.right) / 5;
				tmp2 = (2*markRect.top + 3*markRect.bottom) / 5;
				
				pts[0].x = markRect.left-1;
				pts[0].y = markRect.bottom;
				pts[1].x = tmp;
				pts[1].y = tmp2;
				pts[2].x = markRect.right;
				pts[2].y = markRect.top-1;
				pts[3].x = markRect.left + markRect.right - tmp-1;
				pts[3].y = markRect.top + markRect.bottom - tmp2-1;
				Polygon(hDC, pts, 4);
				break;
			}
			
			SelectObject(hDC, nullBrush);
			SelectObject(hDC, nullPen);
			
			if (markBrush)
				DeleteObject(markBrush);
			if (markPen)
				DeleteObject(markPen);
		}
		
		markPen = CreatePen(PS_SOLID, 1, markBoxColor);
		
		SelectObject(hDC, markPen);
		Rectangle(hDC, markRect.left-1, markRect.top-1, markRect.right+1, markRect.bottom+1);
		
		SelectObject(hDC, nullPen);
		SelectClipRgn(hDC, NULL);
		if (markPen)
			DeleteObject(markPen);
	}
}

void GridCell::DrawCellText(HDC hDC, const RECT& clipRect) const
{
	int textHeight, x, y;
	size_t cur, prev;
	SIZE textSize;
	UINT format;
	
	if (!text || (prop & GWCP_MARKCELL))
		return;
	
	format = TA_NOUPDATECP | TA_TOP;
	if (prop & GWCA_LEFT)
	{
		x = clipRect.left+1;
		format |= TA_LEFT;
	}
	else if (prop & GWCA_CENTER)
	{
		x = (clipRect.left + clipRect.right) / 2;
		format |= TA_CENTER;
	}
	else
	{
		x = clipRect.right-1;
		format |= TA_RIGHT;
	}
	
	textHeight = 0;
	if ((prop & GWCA_BOTTOM) || (prop & GWCA_VCENTER))
	{
		prev = cur = 0;
		for (;;)
		{
			if (text[cur] == 0 || text[cur] == '\n')
			{
				GetTextExtentPoint32(hDC, text+prev, cur-prev+1, &textSize);
				textHeight += textSize.cy;
				prev = cur+1;
			}
			
			if (!text[cur])
				break;
			++cur;
		}
	}
	
	if (prop & GWCA_BOTTOM)
		y = clipRect.bottom-1 - textHeight;
	else if (prop & GWCA_VCENTER)
		y = (clipRect.top + clipRect.bottom - textHeight) / 2;
	else
		y = clipRect.top+1;
	
	SetTextAlign(hDC, format);
	SetTextColor(hDC, textColor);
	
	prev = cur = 0;
	for (;;)
	{
		if (text[cur] == 0 || text[cur] == '\n')
		{
			if (cur > prev)
				ExtTextOut(hDC, x, y, ETO_CLIPPED, &clipRect, text+prev, cur-prev, NULL);
			GetTextExtentPoint32(hDC, text+prev, cur-prev+1, &textSize);
			
			prev = cur+1;
			y += textSize.cy;
			if (y > clipRect.bottom)
				break;
		}
		
		if (!text[cur])
			break;
		++cur;
	}
}



bool GridWindow::classReady = false;

GridWindow::GridWindow(void):
	wndHandle(NULL), editWnd(NULL),
	horBar(NULL), vertBar(NULL),
	popMenu(NULL),
	
	gridFont(NULL),
	borderPen(NULL), selPen(NULL),
	curHor(NULL), curVert(NULL), 
	
	inFocus(false), moveMode(0),
	dragNum(0), dragStart(0),
	
	nCols(0), nRows(0),
	cells(NULL),
	
	colsWidth(NULL), rowsHeight(NULL),
	fstVisRow(0), fstVisCol(0),
	addSelRow(0), addSelCol(0),
	selRow(-1), selCol(-1),
	wheelDelta(0),
	
	undoList()
{
	INITCOMMONCONTROLSEX initCC;
	MENUITEMINFO newItem;
	unsigned int i;
	
	initCC.dwICC = ICC_STANDARD_CLASSES;
	initCC.dwSize = sizeof(initCC);
	InitCommonControlsEx(&initCC);
	
	popMenu = CreatePopupMenu();
	
	newItem.fState = 0;
	newItem.dwItemData = 0;
    newItem.hSubMenu = NULL;
	newItem.hbmpChecked = NULL;
    newItem.hbmpUnchecked = NULL;
    newItem.cbSize = sizeof(newItem);
    newItem.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE;
    
	for (i = 0; i < sizeof(POP_MENU) / sizeof(MenuItem); ++i)
	{
		newItem.cch = (POP_MENU[i].itemName ? strlen(POP_MENU[i].itemName) : 0);
		newItem.fType = (POP_MENU[i].itemName ? MFT_STRING : MFT_SEPARATOR);
		newItem.dwTypeData = (char*) POP_MENU[i].itemName;
		newItem.wID = POP_MENU[i].chOrComId;
		
		InsertMenuItem(popMenu, i, TRUE, &newItem);
	}
}

GridWindow::~GridWindow(void)
{
	Destroy();
	DeleteCells();
	if (popMenu)
		DestroyMenu(popMenu);
}

bool GridWindow::Create(HWND parentWnd)
{
	HINSTANCE hInst;
	if (wndHandle)
	{
		SetParent(wndHandle, parentWnd);
		SetWindowPos(wndHandle, NULL, 0, 0, defSize, defSize, SWP_NOZORDER | SWP_NOACTIVATE);
		return true;
	}
	
	hInst = (HINSTANCE) GetWindowLong(parentWnd, GWL_HINSTANCE);
	
	InitClass(hInst);
	wwcObject = (LONG_PTR) this;
	wwcFunction = GridWindowProc;
	return CreateWindow(GW_CLASSNAME, "", WS_CHILD | WS_CLIPCHILDREN, 0, 0, defSize, defSize, parentWnd, NULL, hInst, NULL);
}

LRESULT GridWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int tmp;
	POINT curPos;
	
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
		
		InitScrollBar();
		InitEditBox();
		
		borderPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
		selPen = CreatePen(PS_SOLID, 3, GetSysColor(COLOR_BTNTEXT));
		
		curVert = LoadCursor(NULL, IDC_SIZENS);
		curHor = LoadCursor(NULL, IDC_SIZEWE);
		
		canScroll = true;
		dblClick = false;
		inFocus = false;
		moveMode = 0;
		break;
	
	case WM_DESTROY:
		if (borderPen)
			DeleteObject(borderPen);
		if (selPen)
			DeleteObject(selPen);
		
		wndHandle = NULL;
		editWnd = NULL;
		vertBar = NULL;
		horBar = NULL;
		break;
	
	case WM_PAINT:
		DrawWindow();
		break;
	
	case WM_SIZE:
		ResizeWindow();
		break;
	
	case WM_TIMER:
		KillTimer(wndHandle, wParam);
		switch (wParam)
		{
		case SCROLL_TIMERID:
			canScroll = true;
			GetCursorPos(&curPos);
			ScreenToClient(wndHandle, &curPos);
			SendMessage(wndHandle, WM_MOUSEMOVE, 0, MAKELPARAM(curPos.x, curPos.y));
			break;
		
		case DBLCLK_TIMERID:
			dblClick = false;
			break;
		}
		break;
	
	case WM_CONTEXTMENU:
		ShowMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	
	case WM_LBUTTONDOWN:
		MouseClick(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	
	case WM_MOUSEMOVE:
		MouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	
	case WM_LBUTTONUP:
		ReleaseCapture();
	case WM_CAPTURECHANGED:
		if (moveMode)
		{
			moveMode = 0;
			ClipCursor(NULL);
		}
		break;
	
	case WM_KEYDOWN:
		KeyDown(wParam, lParam & 0xFFFF);
		break;
	
	case WM_CHAR:
		KeyChar(wParam, lParam & 0xFFFF);
		break;
	
	case WM_NOTIFY:
		if (((LPNMHDR) lParam)->hwndFrom == editWnd && ((LPNMHDR) lParam)-> code == EN_KEYDOWN)
		{
			EKDNM *kdNotInfo = (EKDNM*) lParam;
			switch (kdNotInfo->wParam)
			{
			case VK_TAB:
			case VK_RETURN:
				if (GetKeyState(VK_CONTROL) >= 0)
				{
					*(kdNotInfo->stopProc) = true;
					SetFocus(wndHandle);
					EndModify();
					
					SendMessage(wndHandle, WM_KEYDOWN, kdNotInfo->wParam, kdNotInfo->lParam);
				}
				break;
			
			case VK_ESCAPE:
				*(kdNotInfo->stopProc) = true;
				ShowWindow(editWnd, SW_HIDE);
				SetFocus(wndHandle);
				break;
			}
		}
		break;
	
	case WM_MOUSEWHEEL:
		SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &tmp, 0);
		wheelDelta += (short) HIWORD(wParam);
		while (wheelDelta >= WHEEL_DELTA)
		{
			if (LOWORD(wParam) & MK_SHIFT)
				fstVisCol -= tmp;
			else
				fstVisRow -= tmp;
			wheelDelta -= WHEEL_DELTA;
			CheckVis();
			Redraw();
		}
		while(wheelDelta <= -WHEEL_DELTA)
		{
			if (LOWORD(wParam) & MK_SHIFT)
				fstVisCol += tmp;
			else
				fstVisRow += tmp;
			wheelDelta += WHEEL_DELTA;
			CheckVis();
			Redraw();
		}
		return 0;
	
	case WM_VSCROLL:
		if ((HWND) lParam != vertBar)
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		
		switch (LOWORD(wParam))
		{
		case SB_TOP:
			fstVisRow = 0;
			break;
		
		case SB_PAGEUP:
			PageUp(&fstVisRow);
			break;
		
		case SB_LINEUP:
			--fstVisRow;
			break;
		
		case SB_LINEDOWN:
			++fstVisRow;
			break;
		
		case SB_PAGEDOWN:
			PageDown(&fstVisRow);
			break;
		
		case SB_BOTTOM:
			fstVisRow = nRows-1;
			break;
		
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:	
			fstVisRow = HIWORD(wParam);
			break;
		}
		
		CheckVis();
		Redraw();
		break;
	
	case WM_HSCROLL:
		if ((HWND) lParam != horBar)
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		
		switch (LOWORD(wParam))
		{
		case SB_TOP:
			fstVisCol = 0;
			break;
		
		case SB_PAGELEFT:
			PageLeft(&fstVisCol);
			break;
		
		case SB_LINELEFT:
			--fstVisCol;
			break;
		
		case SB_LINERIGHT:
			++fstVisCol;
			break;
		
		case SB_PAGERIGHT:
			PageRight(&fstVisCol);
			break;
		
		case SB_BOTTOM:
			fstVisCol = nRows-1;
			break;
		
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:	
			fstVisCol = HIWORD(wParam);
			break;
		}
		
		CheckVis();
		Redraw();
		break;
	
	case WM_CUT:
		SendMessage(wndHandle, WM_COPY, 0, 0);
		SendMessage(wndHandle, WM_CLEAR, 0, 0);
		break;
	
	case WM_COPY:
		if (haveCells())
			CopyCmd();
		break;
	
	case WM_PASTE:
		if (haveCells())
			PasteCmd();
		break;
	
	case WM_CLEAR:
		if (haveCells())
			ClearCmd();
		break;
	
	case GM_SELALL:
		selRow = 0;
		selCol = 0;
		addSelRow = nRows-1;
		addSelCol = nCols-1;
		
		CheckSel();
		Redraw();
		break;
	
	case WM_UNDO:
		if (undoList.CanUndo())
		{
			UndoCmd();
			return TRUE;
		}
		return FALSE;
	
	case GM_REDO:
		if (undoList.CanRedo())
		{
			RedoCmd();
			return TRUE;
		}
		return FALSE;
	
	case GM_CANUNDO:
		SendUndoNotify();
		return 0;
	
	case WM_COMMAND:
		if ((HWND) lParam == editWnd)
		{
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				EndModify();
				Redraw();
			}
		}
		else if (lParam == 0)
			SendMessage(wndHandle, LOWORD(wParam), 0, 0);
		break;
	
	case WM_SETFOCUS:
		inFocus = true;
		if ((HWND) lParam != editWnd || !editWnd)
			SendMessage(GetParent(wndHandle), WM_COMMAND, MAKEWPARAM(GetWindowLong(wndHandle, GWL_ID), GN_SETFOCUS), (LPARAM) wndHandle);
		break;
	
	case WM_KILLFOCUS:
		inFocus = false;
		if ((HWND) lParam != editWnd || !editWnd)
			SendMessage(GetParent(wndHandle), WM_COMMAND, MAKEWPARAM(GetWindowLong(wndHandle, GWL_ID), GN_KILLFOCUS), (LPARAM) wndHandle);
		
		Redraw();
		break;
	}
	
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void GridWindow::Redraw(int *x /* = NULL */, int *y /* = NULL */)
{
	RECT inRect;
	if (wndHandle)
	{
		GetClientRect(wndHandle, &inRect);
		ClientToPaintRect(&inRect);
		
		if (y)
			inRect.top = *y;
		if (x)
			inRect.left = *x;
		RedrawWindow(wndHandle, &inRect, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
	}
}

void GridWindow::DeleteRow(int num)
{
	int i, j;
	size_t index;
	GridCell *oldCells;
	unsigned int *oldRowsHeight;
	
	if (num < 0 || num >= nRows)
		return;
	
	oldCells = cells;
	oldRowsHeight = rowsHeight;
	
	--nRows;
	cells = NULL;
	rowsHeight = NULL;
	EnsureCells();
	
	if (oldCells)
	{
		for (i = 0; i < nRows; ++i)
		{
			index = nCols;
			index *= i;
			
			for (j = 0; j < nCols; ++j)
				cells[index+j].Grab(oldCells + index+j + ((i < num) ? 0 : nCols));
		}
		delete[] oldCells;
	}
	if (oldRowsHeight)
	{
		for (i = 0; i < nRows; ++i)
			rowsHeight[i] = oldRowsHeight[(i < num) ? i : i+1];
		delete[] oldRowsHeight;
	}
	
	undoList.Clear();
	CheckVis();
	CheckPos();
}

void GridWindow::DeleteCol(int num)
{
	int i, j;
	size_t index;
	GridCell *oldCells;
	unsigned int *oldColsWidth;
	
	if (num < 0 || num >= nCols)
		return;
	
	oldCells = cells;
	oldColsWidth = colsWidth;
	
	--nCols;
	cells = NULL;
	colsWidth = NULL;
	EnsureCells();
	
	if (oldCells)
	{
		for (i = 0; i < nRows; ++i)
		{
			index = nCols;
			index *= i;
			for (j = 0; j < nCols; ++j)
				cells[index+j].Grab(oldCells + index+i + ((j < num) ? j : j+1));
		}
		delete[] oldCells;
	}
	if (oldColsWidth)
	{
		for (j = 0; j < nCols; ++j)
			colsWidth[j] = oldColsWidth[(j < num) ? j : j+1];
		delete[] oldColsWidth;
	}
	
	undoList.Clear();
	CheckVis();
	CheckPos();
}

void GridWindow::AddRow(int num)
{
	int i, j;
	size_t index;
	GridCell *oldCells;
	unsigned int *oldRowsHeight;
	
	if (nRows >= maxRowsCols)
		return;
	if (((size_t) nRows) * ((size_t) nCols) >= maxCells)
		return;
	
	if (num < 0)
		num = 0;
	if (num > nRows)
		num = nRows;
	
	oldCells = cells;
	oldRowsHeight = rowsHeight;
	
	++nRows;
	cells = NULL;
	rowsHeight = NULL;
	EnsureCells();
	
	if (oldCells)
	{
		for (i = 0; i < nRows; ++i)
		{
			if (i == num)
				continue;
			
			index = nCols;
			index *= i;
			for (j = 0; j < nCols; ++j)
				cells[index+j].Grab(oldCells + index+j - ((i < num) ? 0 : nCols));
		}
		delete[] oldCells;
	}
	if (oldRowsHeight)
	{
		for (i = 0; i < nRows; ++i)
		{
			if (i == num)
				continue;
			rowsHeight[i] = oldRowsHeight[(i < num) ? i : i-1];
		}
		delete[] oldRowsHeight;
	}
	
	undoList.Clear();
	CheckVis();
}

void GridWindow::AddCol(int num)
{
	int i, j;
	size_t index;
	GridCell *oldCells;
	unsigned int *oldColsWidth;
	
	if (nCols >= maxRowsCols)
		return;
	if (((size_t) nRows) * ((size_t) nCols) >= maxCells)
		return;
	
	if (num < 0)
		num = 0;
	if (num > nCols)
		num = nCols;
	
	oldCells = cells;
	oldColsWidth = colsWidth;
	
	++nCols;
	cells = NULL;
	colsWidth = NULL;
	EnsureCells();
	
	if (oldCells)
	{
		for (i = 0; i < nRows; ++i)
		{
			index = nCols;
			index *= i;
			for (j = 0; j < nCols; ++j)
			{
				if (j == num)
					continue;
				cells[index+j].Grab(oldCells + index-i + ((j < num) ? j : j-1));
			}
		}
		delete[] oldCells;
	}
	if (oldColsWidth)
	{
		for (j = 0; j < nCols; ++j)
		{
			if (j == num)
				continue;
			colsWidth[j] = oldColsWidth[(j < num) ? j : j-1];
		}
		delete[] oldColsWidth;
	}
	
	undoList.Clear();
	CheckVis();
}

void GridWindow::SetColsCount(int cols)
{
	size_t stMax = maxCells / ((nRows > 1) ? nRows : 1);
	
	if (cols < 0)
		cols = 0;
	if (cols > maxRowsCols)
		cols = maxRowsCols;
	if ((size_t) cols > stMax)
		cols = stMax;
	
	DeleteCells();
	undoList.Clear();
	if (cols == nCols)
		return;
	
	nCols = cols;
	CheckVis();
	CheckPos();
}

void GridWindow::SetRowsCount(int rows)
{
	size_t stMax = maxCells / ((nCols > 1) ? nCols : 1);
	
	if (rows < 0)
		rows = 0;
	if (rows > maxRowsCols)
		rows = maxRowsCols;
	if ((size_t) rows > stMax)
		rows = stMax;
	
	DeleteCells();
	undoList.Clear();
	if (rows == nRows)
		return;
	
	nRows = rows;
	CheckVis();
	CheckPos();
}

void GridWindow::SetSizes(int rows, int cols)
{
	size_t stMax = maxCells;
	
	if (cols < 0)
		cols = 0;
	if (rows < 0)
		rows = 0;
	if (cols > maxRowsCols)
		cols = maxRowsCols;
	if (rows > maxRowsCols)
		rows = maxRowsCols;
	
	stMax /= ((rows > 1) ? rows : 1);
	if ((size_t) cols > stMax)
		cols = stMax;
	
	DeleteCells();
	undoList.Clear();
	if (rows == nRows && cols == nCols)
		return;
	
	nRows = rows;
	nCols = cols;
	CheckVis();
	CheckPos();
}

void GridWindow::SelectCells(int row, int col, int dRows, int dCols, bool show /* = false */)
{
	selRow = row;
	selCol = col;
	CheckPos();
	
	if (dRows < -maxRowsCols)
		dRows = -maxRowsCols;
	if (dCols < -maxRowsCols)
		dCols = -maxRowsCols;
	if (dRows > maxRowsCols)
		dRows = maxRowsCols;
	if (dCols > maxRowsCols)
		dCols = maxRowsCols;
	
	addSelRow = dRows;
	addSelCol = dCols;
	CheckSel();
	
	if (show)
	{
		ShowCell(selRow, selCol);
		Redraw();
	}
}

void GridWindow::CheckVis(void)
{
	SCROLLINFO scrInfo;
	
	if (nRows <= 0 || fstVisRow < 0)
		fstVisRow = 0;
	else if (fstVisRow >= nRows)
		fstVisRow = nRows-1;
	
	if (nCols <= 0 || fstVisCol < 0)
		fstVisCol = 0;
	else if (fstVisCol >= nCols)
		fstVisCol = nCols-1;
	
	scrInfo.cbSize = sizeof(scrInfo);
	scrInfo.fMask = SIF_POS | SIF_RANGE;
	scrInfo.nMin = 0;
	
	if (vertBar)
	{
		scrInfo.nPos = fstVisRow;
		scrInfo.nMax = (nRows ? nRows-1 : 0);
		SetScrollInfo(vertBar, SB_CTL, &scrInfo, TRUE);
	}
	if (horBar)
	{
		scrInfo.nPos = fstVisCol;
		scrInfo.nMax = (nCols ? nCols-1 : 0);
		SetScrollInfo(horBar, SB_CTL, &scrInfo, TRUE);
	}
}

void GridWindow::CheckPos(void)
{
	if (nRows <= 0 || selRow < 0)
		selRow = 0;
	else if (selRow >= nRows)
		selRow = nRows-1;
	
	if (nCols <= 0 || selCol < 0)
		selCol = 0;
	else if (selCol >= nCols)
		selCol = nCols-1;
	
	addSelRow = 0;
	addSelCol = 0;
}

void GridWindow::CheckSel(void)
{
	if (addSelRow > 0 && selRow + addSelRow >= nRows)
		addSelRow = nRows-selRow - 1;
	if (addSelRow < 0 && selRow + addSelRow < 0)
		addSelRow = -selRow;
	
	if (addSelCol > 0 && selCol + addSelCol >= nCols)
		addSelCol = nCols-selCol - 1;
	if (addSelCol < 0 && selCol + addSelCol < 0)
		addSelCol = -selCol;
}

void GridWindow::DeleteCells(void)
{
	if (cells)
	{
		delete[] cells;
		cells = NULL;
	}
	if (colsWidth)
	{
		delete[] colsWidth;
		colsWidth = NULL;
	}
	if (rowsHeight)
	{
		delete[] rowsHeight;
		rowsHeight = NULL;
	}
}

void GridWindow::EnsureCells(void)
{
	size_t i, prod;
	
	prod = ((size_t) nRows) * ((size_t) nCols);
	if (!cells && prod > 0)
		cells = new GridCell [prod];
	if (!rowsHeight && nRows > 0)
	{
		rowsHeight = new unsigned int [nRows];
		for (i = 0; i < (size_t) nRows; ++i)
			rowsHeight[i] = defRowHeight;
	}
	if (!colsWidth && nCols > 0)
	{
		colsWidth = new unsigned int [nCols];
		for (i = 0; i < (size_t) nCols; ++i)
			colsWidth[i] = defColWidth;
	}
}

int GridWindow::GetColByCoord(int x)
{
	int i, xCur;
	RECT inRect;
	
	GetClientRect(wndHandle, &inRect);
	ClientToPaintRect(&inRect);
	
	if (x < inRect.left || x >= inRect.right)
		return 0;
	
	xCur = inRect.left;
	for (i = fstVisCol; i < nCols; ++i)
	{
		xCur += GetColWidth(i);
		if (x < xCur)
			return i+1;
		if (x == xCur)
			return -(i+1);
		++xCur;
	}
	return 0;
}

int GridWindow::GetRowByCoord(int y)
{
	int i, yCur;
	RECT inRect;
	
	GetClientRect(wndHandle, &inRect);
	ClientToPaintRect(&inRect);
	
	if (y < inRect.top || y >= inRect.bottom)
		return 0;
	
	yCur = inRect.top;
	for (i = fstVisRow; i < nRows; ++i)
	{
		yCur += GetRowHeight(i);
		if (y < yCur)
			return i+1;
		if (y == yCur)
			return -(i+1);
		++yCur;
	}
	return 0;
}

void GridWindow::ShowCell(int row, int col)
{
	RECT inRect;
	
	GetClientRect(wndHandle, &inRect);
	ClientToPaintRect(&inRect);

	if (row >= 0)
	{
		if (row < fstVisRow)
			fstVisRow = row;
		else if (row > fstVisRow)
		{
			int i, y;
			y = inRect.top - 2;
			
			for (i = fstVisRow; i <= row; ++i)
				y += GetRowHeight(i) + 1;
			while (y >= inRect.bottom && fstVisRow < row)
			{
				y -= GetRowHeight(fstVisRow) + 1;
				++fstVisRow;
			}
		}
	}
	if (col >= 0)
	{
		if (col < fstVisCol)
			fstVisCol = col;
		else if (col > fstVisCol)
		{
			int i, x;
			x = inRect.left - 2;
			
			for (i = fstVisCol; i <= col; ++i)
				x += GetColWidth(i) + 1;
			while (x >= inRect.right && fstVisCol < col)
			{
				x -= GetColWidth(fstVisCol) + 1;
				++fstVisCol;
			}
		}
	}
	CheckVis();
}

void GridWindow::SendUndoNotify(void)
{
	LONG id;
	GWNM_UNDO notMsg;
	id = GetWindowLong(wndHandle, GWL_ID);
	
	notMsg.hdr.idFrom = id;
	notMsg.hdr.code = GN_UNDOSTATE;
	notMsg.hdr.hwndFrom = wndHandle;
	notMsg.canUndo = undoList.CanUndo();
	notMsg.canRedo = undoList.CanRedo();
	
	SendMessage(GetParent(wndHandle), WM_NOTIFY, id, (LPARAM) &notMsg);
}

void GridWindow::SendChangeNotify(int row, int col)
{
	LONG id;
	GWNM_CHANGE notMsg;
	id = GetWindowLong(wndHandle, GWL_ID);
	
	notMsg.row = row;
	notMsg.col = col;
	notMsg.hdr.idFrom = id;
	notMsg.hdr.code = GN_CHANGE;
	notMsg.hdr.hwndFrom = wndHandle;
	
	SendMessage(GetParent(wndHandle), WM_NOTIFY, id, (LPARAM) &notMsg);
}

void GridWindow::PageUp(int *row)
{
	int y;
	RECT inRect;
	
	if (!row)
		return;
	
	GetClientRect(wndHandle, &inRect);
	ClientToPaintRect(&inRect);
	
	y = inRect.top + GetRowHeight(*row) - 1;
	while (*row > 0)
	{
		y += GetRowHeight(--(*row)) + 1;
		if (y >= inRect.bottom)
			break;
	}
}

void GridWindow::PageDown(int *row)
{
	int y;
	RECT inRect;
	
	if (!row)
		return;
	
	GetClientRect(wndHandle, &inRect);
	ClientToPaintRect(&inRect);
	
	y = inRect.top + GetRowHeight(*row) - 1;
	while (*row < nRows-1)
	{
		y += GetRowHeight(++(*row)) + 1;
		if (y >= inRect.bottom)
			break;
	}
}

void GridWindow::PageLeft(int *col)
{
	int x;
	RECT inRect;
	
	if (!col)
		return;
	
	GetClientRect(wndHandle, &inRect);
	ClientToPaintRect(&inRect);
	
	x = inRect.left + GetColWidth(*col) - 1;
	while (*col > 0)
	{
		x += GetColWidth(--(*col)) + 1;
		if (x >= inRect.right)
			break;
	}
}

void GridWindow::PageRight(int *col)
{
	int x;
	RECT inRect;
	
	if (!col)
		return;
	
	GetClientRect(wndHandle, &inRect);
	ClientToPaintRect(&inRect);
	
	x = inRect.left + GetColWidth(*col) - 1;
	while (*col < nCols-1)
	{
		x += GetColWidth(++(*col)) + 1;
		if (x >= inRect.right)
			break;
	}
}

bool GridWindow::FindUp(int* row, int* col, unsigned int skipMask, bool canStep /* = false */)
{
	int i, j;
	if (!row || !col)
		return false;
	i = *row;
	j = *col;
	
	for (;;)
	{
		if (i <= 0)
		{
			if (canStep && j > 0)
			{
				--j;
				i = nRows;
			}
			else
				return false;
		}
		if (!(GetCellProp(--i, j) & skipMask))
			break;
	}
	*row = i;
	*col = j;
	return true;
}

bool GridWindow::FindDown(int* row, int* col, unsigned int skipMask, bool canStep /* = false */)
{
	int i, j;
	if (!row || !col)
		return false;
	i = *row;
	j = *col;
	
	for (;;)
	{
		if (i >= nRows-1)
		{
			if (canStep && j < nCols-1)
			{
				++j;
				i = -1;
			}
			else
				return false;
		}
		if (!(GetCellProp(++i, j) & skipMask))
			break;
	}
	*row = i;
	*col = j;
	return true;
}

bool GridWindow::FindLeft(int* row, int* col, unsigned int skipMask, bool canStep /* = false */)
{
	int i, j;
	if (!row || !col)
		return false;
	i = *row;
	j = *col;
	
	for (;;)
	{
		if (j <= 0)
		{
			if (canStep && i > 0)
			{
				--i;
				j = nCols;
			}
			else
				return false;
		}
		if (!(GetCellProp(i, --j) & skipMask))
			break;
	}
	*row = i;
	*col = j;
	return true;
}

bool GridWindow::FindRight(int* row, int* col, unsigned int skipMask, bool canStep /* = false */)
{
	int i, j;
	if (!row || !col)
		return false;
	i = *row;
	j = *col;
	
	for (;;)
	{
		if (j >= nCols-1)
		{
			if (canStep && i < nRows-1)
			{
				++i;
				j = -1;
			}
			else
				return false;
		}
		if (!(GetCellProp(i, ++j) & skipMask))
			break;
	}
	*row = i;
	*col = j;
	return true;
}

void GridWindow::AddUndoOp(bool in, int i, int j, char type, bool newOp)
{
	unsigned int tag;
	GridUndoOperation *op;
	
	if (!(op = (GridUndoOperation*) undoList.GetUndo()))
		tag = 1;
	else
		tag = (newOp ? op->tag + 1 : op->tag);
	
	op = new GridUndoOperation;
	op->type = type;
	op->wnd = this;
	op->tag = tag;
	op->row = i;
	op->col = j;
	
	switch (type)
	{
	case GWUO_TEXT:
		if (in)
		{
			if (i < 0 || j < 0 || i >= nRows || j >= nCols)
				op->text = NULL;
			else
			{
				size_t index = nCols;
				index *= i;
				index += j;
				
				EnsureCells();
				op->text = cells[index].text;
				cells[index].text = NULL;
			}
		}
		else
		{
			size_t len;
			const char *cell;
			
			cell = GetCell(i, j);
			if (!cell || !(len = strlen(cell)))
				op->text = NULL;
			else
			{
				char *str;
				
				str = new char[len+1];
				memcpy(str, cell, len * sizeof(char));
				str[len] = 0;
				
				op->text = str;
			}
		}
		break;
	
	case GWUO_PROP:
		op->prop = GetCellProp(i, j);
		break;
	
	case GWUO_MARK:
		op->mark = GetCellMark(i, j);
		break;
	
	case GWUO_CCOL:
		op->cellColor = GetCellColor(i, j);
		break;
	
	case GWUO_TCOL:
		op->textColor = GetCellTextColor(i, j);
		break;
	
	case GWUO_SIZE:
		if (i < 0)
			op->size = GetColWidth(j);
		else
			op->size = GetRowHeight(i);
		break;
	}
	
	undoList.Add(op);
}

void GridWindow::BeginModify(const char *str /* = NULL */)
{
	size_t len;
	int x, y, i;
	RECT inRect;
	
	if (IsWindowVisible(editWnd))
		return;
	
	GetClientRect(wndHandle, &inRect);
	ClientToPaintRect(&inRect);
	
	x = inRect.left;
	for (i = fstVisCol; i < selCol; ++i)
		x += GetColWidth(i) + 1;
	y = inRect.top;
	for (i = fstVisRow; i < selRow; ++i)
		y += GetRowHeight(i) + 1;
	
	SetWindowPos(editWnd, NULL, x, y, GetColWidth(selCol), GetRowHeight(selRow), SWP_NOZORDER | SWP_NOACTIVATE);
	SetWindowText(editWnd, str);
	
	if (!str)
		len = 0;
	else
		len = strlen(str);
	SendMessage(editWnd, EM_SETSEL, len, len);
	SendMessage(editWnd, WM_SETFONT, (WPARAM) gridFont, 0);
	
	ShowWindow(editWnd, SW_SHOW);
	SendMessage(editWnd, EM_SCROLLCARET, 0, 0);
	SetFocus(editWnd);
}

void GridWindow::EndModify(void)
{
	int len;
	char *buf;
	
	if (!IsWindowVisible(editWnd))
		return;
	
	len = GetWindowTextLength(editWnd);
	buf = new char [len+1];
	
	GetWindowText(editWnd, buf, len+1);
	buf[len] = 0;
	
	AddUndoOp(true, selRow, selCol, GWUO_TEXT, true);
	SetCell(selRow, selCol, buf);
	delete[] buf;
	
	ShowWindow(editWnd, SW_HIDE);
	SendChangeNotify(selRow, selCol);
}

void GridWindow::DrawWindow(void)
{
	HDC hDC;
	int i, j;
	HPEN nullPen;
	PAINTSTRUCT paintInfo;
	HGDIOBJ oldBrush, oldFont, oldPen;
	HBRUSH nullBrush, grayBrush, whiteBrush;
	RECT wndClRect, inRect, paintRect, selRect;
	int minSelRow, maxSelRow, minSelCol, maxSelCol;
	
	nullBrush = (HBRUSH) GetStockObject(NULL_BRUSH);
	whiteBrush = GetSysColorBrush(COLOR_WINDOW);
	grayBrush = GetSysColorBrush(COLOR_BTNFACE);
	nullPen = (HPEN) GetStockObject(NULL_PEN);
	
	GetClientRect(wndHandle, &wndClRect);
	inRect = wndClRect;
	
	ClientToPaintRect(&inRect);
	paintRect = inRect;
	
	if (!haveCells() || !inFocus)
	{
		minSelRow = 0;
		maxSelRow = 0;
		minSelCol = 0;
		maxSelCol = 0;
		
		selRect.top = 0;
		selRect.left = 0;
		selRect.right = 0;
		selRect.bottom = 0;
	}
	else
	{
		minSelRow = ((addSelRow < 0) ? selRow + addSelRow : selRow);
		maxSelRow = ((addSelRow > 0) ? selRow + addSelRow : selRow);
		minSelCol = ((addSelCol < 0) ? selCol + addSelCol : selCol);
		maxSelCol = ((addSelCol > 0) ? selCol + addSelCol : selCol);
		
		if (minSelCol < fstVisCol)
			selRect.left = -1;
		else
		{
			selRect.left = paintRect.left - 1;
			for (i = fstVisCol; i < minSelCol; ++i)
				selRect.left += GetColWidth(i) + 1;
		}
		
		if (maxSelCol < fstVisCol)
			selRect.right = -1;
		else
		{
			selRect.right = paintRect.left - 1;
			for (i = fstVisCol; i <= maxSelCol; ++i)
				selRect.right += GetColWidth(i) + 1;
		}
		
		if (minSelRow < fstVisRow)
			selRect.top = -1;
		else
		{
			selRect.top = paintRect.top - 1;
			for (i = fstVisRow; i < minSelRow; ++i)
				selRect.top += GetRowHeight(i) + 1;
		}
		
		if (maxSelRow < fstVisRow)
			selRect.bottom = -1;
		else
		{
			selRect.bottom = paintRect.top - 1;
			for (i = fstVisRow; i <= maxSelRow; ++i)
				selRect.bottom += GetRowHeight(i) + 1;
		}
	}
	
	hDC = BeginPaint(wndHandle, &paintInfo);
	oldBrush = SelectObject(hDC, nullBrush);
	oldPen = SelectObject(hDC, nullPen);
	oldFont = NULL;
	
	if (haveCells())
	{
		int x, y;
		size_t index;
		RECT cellRect;
		bool selected;
		
		if (gridFont)
			oldFont = SelectObject(hDC, gridFont);
		SetBkMode(hDC, TRANSPARENT);
		
		EnsureCells();
		y = paintRect.top;
		for (i = fstVisRow; i < nRows; ++i)
		{
			x = paintRect.left;
			index = nCols;
			index *= i;
			for (j = fstVisCol; j < nCols; ++j)
			{
				cellRect.left = x;
				cellRect.top = y;
				
				cellRect.right = x + GetColWidth(j);
				cellRect.bottom = y + GetRowHeight(i);
				
				selected = (inFocus && i >= minSelRow && i <= maxSelRow && j >= minSelCol && j <= maxSelCol && (i != selRow || j != selCol));
				
				cells[index+j].DrawCell(hDC, cellRect, selected);
				cells[index+j].DrawCellText(hDC, cellRect);
				
				x += GetColWidth(j) + 1;
				if (x >= paintRect.right)
					break;
			}
			
			y += GetRowHeight(i) + 1;
			if (y >= paintRect.bottom)
				break;
		}
		if (y < paintRect.bottom)
		{
			SelectObject(hDC, nullPen);
			SelectObject(hDC, whiteBrush);
			Rectangle(hDC, paintRect.left, y, paintRect.right+1, paintRect.bottom+1);
			paintRect.bottom = y;
		}
		
		y = paintRect.top;
		x = paintRect.left;
		SelectObject(hDC, borderPen);
		for (i = fstVisCol; i < nCols; ++i)
		{
			x += GetColWidth(i);
			if (x >= paintRect.right)
				break;
			
			MoveToEx(hDC, x, paintRect.top, NULL);
			LineTo(hDC, x, paintRect.bottom);
			++x;
		}
		if (x < paintRect.right)
		{
			SelectObject(hDC, nullPen);
			SelectObject(hDC, whiteBrush);
			Rectangle(hDC, x, paintRect.top, paintRect.right+1, paintRect.bottom+1);
			paintRect.right = x;
		}
		
		SelectObject(hDC, borderPen);
		for (i = fstVisRow; i < nRows; ++i)
		{
			y += GetRowHeight(i);
			if (y >= paintRect.bottom)
				break;
			
			MoveToEx(hDC, paintRect.left, y, NULL);
			LineTo(hDC, paintRect.right, y);
			++y;
		}
		
		if (inFocus)
		{
			SelectObject(hDC, selPen);
			SelectObject(hDC, nullBrush);
			Rectangle(hDC, selRect.left, selRect.top, selRect.right+1, selRect.bottom+1);
		}
	}
	
	SelectObject(hDC, nullPen);
	SelectObject(hDC, grayBrush);
	i = GetSystemMetrics(SM_CXVSCROLL);
	j = GetSystemMetrics(SM_CYHSCROLL);
	Rectangle(hDC, inRect.right, inRect.bottom, inRect.right+i+1, inRect.bottom+j+1);
	
	if (oldBrush)
		SelectObject(hDC, oldBrush);
	if (oldFont)
		SelectObject(hDC, oldFont);
	if (oldPen)
		SelectObject(hDC, oldPen);
	
	DrawEdge(hDC, &wndClRect, EDGE_SUNKEN, BF_RECT);
	EndPaint(wndHandle, &paintInfo);
}

void GridWindow::ShowMenu(int x, int y)
{
	EnableMenuItem(popMenu, WM_UNDO, MF_BYCOMMAND | (undoList.CanUndo() ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(popMenu, GM_REDO, MF_BYCOMMAND | (undoList.CanRedo() ? MF_ENABLED : MF_GRAYED));
	
	if (haveCells())
	{
		EnableMenuItem(popMenu, WM_CUT, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(popMenu, WM_COPY, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(popMenu, WM_PASTE, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(popMenu, WM_CLEAR, MF_BYCOMMAND | MF_ENABLED);
		if (nRows > 1 || nCols > 1)
			EnableMenuItem(popMenu, GM_SELALL, MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem(popMenu, GM_SELALL, MF_BYCOMMAND | MF_GRAYED);
	}
	else
	{
		EnableMenuItem(popMenu, WM_CUT, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(popMenu, WM_COPY, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(popMenu, WM_PASTE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(popMenu, WM_CLEAR, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(popMenu, GM_SELALL, MF_BYCOMMAND | MF_GRAYED);
	}
	
	if (x >= 0 && y >= 0)
		TrackPopupMenu(popMenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON, x, y, 0, wndHandle, NULL);
	else
	{
		POINT pt;
		RECT inRect;
		
		GetClientRect(wndHandle, &inRect);
		ClientToPaintRect(&inRect);
		
		pt.y = inRect.top + 2;
		pt.x = inRect.left + 2;
		ClientToScreen(wndHandle, &pt);
		TrackPopupMenu(popMenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, 0, wndHandle, NULL);
	}
}

void GridWindow::ResizeWindow(void)
{
	RECT inRect;
	
	GetClientRect(wndHandle, &inRect);
	ClientToPaintRect(&inRect);
	
	SetWindowPos(horBar, NULL, inRect.left, inRect.bottom, inRect.right-inRect.left, GetSystemMetrics(SM_CYHSCROLL), SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(vertBar, NULL, inRect.right, inRect.top, GetSystemMetrics(SM_CXVSCROLL), inRect.bottom-inRect.top, SWP_NOACTIVATE | SWP_NOZORDER);
}

void GridWindow::MouseMove(DWORD, int x, int y)
{
	RECT inRect;
	bool needRedraw;
	int clkRow, clkCol;
	
	switch (moveMode)
	{
	case DRAG_ROW:
		if (y <= dragStart)
		{
			if (GetRowHeight(dragNum) > 0)
			{
				SetRowHeight(dragNum, 0);
				Redraw(NULL, &dragStart);
			}
		}
		else
		{
			if (GetRowHeight(dragNum) != (unsigned int) (y-dragStart))
			{
				SetRowHeight(dragNum, y-dragStart);
				Redraw(NULL, &dragStart);
			}
		}
		
		SetCursor(curVert);
		break;
	
	case DRAG_COL:
		if (x <= dragStart)
		{
			if (GetColWidth(dragNum) > 0)
			{
				SetColWidth(dragNum, 0);
				Redraw(&dragStart, NULL);
			}
		}
		else
		{
			if (GetColWidth(dragNum) != (unsigned int) (x-dragStart))
			{
				SetColWidth(dragNum, x-dragStart);
				Redraw(&dragStart, NULL);
			}
		}
		
		SetCursor(curHor);
		break;
	
	case SELECT_CELLS:
		GetClientRect(wndHandle, &inRect);
		ClientToPaintRect(&inRect);
		
		if (x < inRect.left)
			x = inRect.left;
		if (x >= inRect.right)
			x = inRect.right-1;
		
		if (y < inRect.top)
			y = inRect.top;
		if (y >= inRect.bottom)
			y = inRect.bottom-1;
		
		needRedraw = false;
		clkRow = GetRowByCoord(y);
		clkCol = GetColByCoord(x);
		if (clkRow < 0)
			clkRow = -clkRow;
		if (clkCol < 0)
			clkCol = -clkCol;
		
		if (clkRow > 0 && clkCol > 0 && !(GetCellProp(clkRow-1, clkCol-1) & GWCP_DISABLED))
		{
			int i, j;
			
			i = clkRow-1 - selRow;
			j = clkCol-1 - selCol;
			if (i != addSelRow || j != addSelCol)
			{
				needRedraw = true;
				addSelRow = i;
				addSelCol = j;
				CheckSel();
			}
		}
		
		if (canScroll)
		{
			if (x < inRect.left+scrollBorder)
			{
				canScroll = false;
				--fstVisCol;
			}
			else if (x >= inRect.right-scrollBorder)
			{
				canScroll = false;
				++fstVisCol;
			}
			
			if (y < inRect.top+scrollBorder)
			{
				canScroll = false;
				--fstVisRow;
			}
			else if (y >= inRect.bottom-scrollBorder)
			{
				canScroll = false;
				++fstVisRow;
			}
			
			if (!canScroll)
			{
				CheckVis();
				needRedraw = true;
				SetTimer(wndHandle, SCROLL_TIMERID, minScrollTime, NULL);
			}
		}
		
		if (needRedraw)
			Redraw();
		break;
	
	default:
		clkRow = GetRowByCoord(y);
		clkCol = GetColByCoord(x);
		if (clkRow < 0 && clkCol > 0)
			SetCursor(curVert);
		else if (clkRow != 0 && clkCol < 0)
			SetCursor(curHor);
	}
}

void GridWindow::MouseClick(DWORD keys, int x, int y)
{
	bool needRedraw;
	int i, clkRow, clkCol;
	
	clkRow = GetRowByCoord(y);
	clkCol = GetColByCoord(x);
	needRedraw = !inFocus;
	SetFocus(wndHandle);
	
	if (clkRow > 0 && clkCol > 0)
	{
		if (keys & MK_SHIFT)
		{
			if (!(GetCellProp(clkRow-1, clkCol-1) & GWCP_DISABLED))
			{
				int i, j;
				
				i = clkRow-1 - selRow;
				j = clkCol-1 - selCol;
				if (i != addSelRow || j != addSelCol)
				{
					needRedraw = true;
					addSelRow = i;
					addSelCol = j;
					CheckSel();
				}
				
				moveMode = SELECT_CELLS;
				SetCapture(wndHandle);
			}
		}
		else
		{
			if (clkRow-1 == selRow && clkCol-1 == selCol)
			{
				if (!dblClick)
				{
					moveMode = SELECT_CELLS;
					SetCapture(wndHandle);
					dblClick = true;
				}
				else
				{
					if (!(GetCellProp(selRow, selCol) & GWCP_READONLY))
					{
						if (!(GetCellProp(selRow, selCol) & GWCP_MARKCELL))
							BeginModify(GetCell(selRow, selCol));
						else
						{
							size_t index = nCols;
							index *= selRow;
							index += selCol;
							
							AddUndoOp(true, selRow, selCol, GWUO_MARK, true);
							if (cells[index].SwitchMark())
								needRedraw = true;
							
							SendChangeNotify(selRow, selCol);
						}
					}
					dblClick = false;
				}
			}
			else if (!(GetCellProp(clkRow-1, clkCol-1) & GWCP_DISABLED))
			{
				selRow = clkRow-1;
				selCol = clkCol-1;
				CheckPos();
				
				needRedraw = true;
				moveMode = SELECT_CELLS;
				SetCapture(wndHandle);
				
				dblClick = true;
			}
			
			SetTimer(wndHandle, DBLCLK_TIMERID, GetDoubleClickTime(), NULL);
		}
	}
	else if (clkRow < 0 && clkCol > 0)
	{
		POINT clPoint;
		RECT inRect, clRect;
		
		GetClientRect(wndHandle, &inRect);
		ClientToPaintRect(&inRect);
		dragStart = inRect.top;
		dragNum = -clkRow-1;
		
		AddUndoOp(true, dragNum, -1, GWUO_SIZE, true);
		for (i = fstVisRow; i < dragNum; ++i)
			dragStart += GetRowHeight(i) + 1;
		clPoint.y = dragStart;
		clPoint.x = 0;
		
		ClientToScreen(wndHandle, &clPoint);
		clRect.left = 0;
		clRect.top = clPoint.y;
		clRect.right = GetSystemMetrics(SM_CXSCREEN);
		clRect.bottom = GetSystemMetrics(SM_CYSCREEN);
		
		SetCursor(curVert);
		moveMode = DRAG_ROW;
		ClipCursor(&clRect);
		SetCapture(wndHandle);
	}
	else if (clkRow != 0 && clkCol < 0)
	{
		POINT clPoint;
		RECT inRect, clRect;
		
		GetClientRect(wndHandle, &inRect);
		ClientToPaintRect(&inRect);
		dragStart = inRect.left;
		dragNum = -clkCol-1;
		
		AddUndoOp(true, -1, dragNum, GWUO_SIZE, true);
		for (i = fstVisCol; i < dragNum; ++i)
			dragStart += GetColWidth(i) + 1;
		clPoint.x = dragStart;
		clPoint.y = 0;
		
		ClientToScreen(wndHandle, &clPoint);
		clRect.top = 0;
		clRect.left = clPoint.x;
		clRect.right = GetSystemMetrics(SM_CXSCREEN);
		clRect.bottom = GetSystemMetrics(SM_CYSCREEN);
		
		SetCursor(curHor);
		moveMode = DRAG_COL;
		ClipCursor(&clRect);
		SetCapture(wndHandle);
	}
	
	if (needRedraw)
		Redraw();
}

void GridWindow::KeyDown(int key, int nRep)
{
	switch (key)
	{
	case VK_UP:
	case VK_DOWN:
	case VK_LEFT:
	case VK_RIGHT:
		if (GetKeyState(VK_SHIFT) < 0)
		{
			int i, j;
			
			i = selRow + addSelRow;
			j = selCol + addSelCol;
			switch (key)
			{
			case VK_UP:
				if (GetKeyState(VK_CONTROL) < 0)
					nRep = selRow;
				while (--nRep >= 0)
					if (!FindUp(&i, &j, GWCP_DISABLED))
						break;
				break;
			
			case VK_LEFT:
				if (GetKeyState(VK_CONTROL) < 0)
					nRep = selCol;
				while (--nRep >= 0)
					if (!FindLeft(&i, &j, GWCP_DISABLED))
						break;
				break;
			
			case VK_DOWN:
				if (GetKeyState(VK_CONTROL) < 0)
					nRep = nRows-1 - selRow;
				while (--nRep >= 0)
					if (!FindDown(&i, &j, GWCP_DISABLED))
						break;
				break;
			
			case VK_RIGHT:
				if (GetKeyState(VK_CONTROL) < 0)
					nRep = nCols-1 - selCol;
				while (--nRep >= 0)
					if (!FindRight(&i, &j, GWCP_DISABLED))
						break;
				break;
			}
			addSelRow = i - selRow;
			addSelCol = j - selCol;
			CheckSel();
			
			if (key == VK_UP || key == VK_DOWN)
				ShowCell(selRow + addSelRow, -1);
			else
				ShowCell(-1, selCol + addSelCol);
		}
		else
		{
			switch (key)
			{
			case VK_UP:
				if (GetKeyState(VK_CONTROL) < 0)
					nRep = selRow;
				while (--nRep >= 0)
					if (!FindUp(&selRow, &selCol, GWCP_DISABLED))
						break;
				break;
			
			case VK_LEFT:
				if (GetKeyState(VK_CONTROL) < 0)
					nRep = selCol;
				while (--nRep >= 0)
					if (!FindLeft(&selRow, &selCol, GWCP_DISABLED))
						break;
				break;
			
			case VK_DOWN:
				if (GetKeyState(VK_CONTROL) < 0)
					nRep = nRows-1 - selRow;
				while (--nRep >= 0)
					if (!FindDown(&selRow, &selCol, GWCP_DISABLED))
						break;
				break;
			
			case VK_RIGHT:
				if (GetKeyState(VK_CONTROL) < 0)
					nRep = nCols-1 - selCol;
				while (--nRep >= 0)
					if (!FindRight(&selRow, &selCol, GWCP_DISABLED))
						break;
				break;
			}
			CheckPos();
			
			ShowCell(selRow, selCol);
		}
		Redraw();
		break;
	
	case VK_TAB:
	case VK_RETURN:
		if (GetKeyState(VK_SHIFT) < 0)
		{
			switch (key)
			{
			case VK_RETURN:
				while (--nRep >= 0)
					if (!FindUp(&selRow, &selCol, GWCP_READONLY | GWCP_DISABLED, true))
						break;
				break;
			
			case VK_TAB:
				while (--nRep >= 0)
					if (!FindLeft(&selRow, &selCol, GWCP_READONLY | GWCP_DISABLED, true))
						break;
				break;
			}
		}
		else
		{
			switch (key)
			{
			case VK_RETURN:
				while (--nRep >= 0)
					if (!FindDown(&selRow, &selCol, GWCP_READONLY | GWCP_DISABLED, true))
						break;
				break;
			
			case VK_TAB:
				while (--nRep >= 0)
					if (!FindRight(&selRow, &selCol, GWCP_READONLY | GWCP_DISABLED, true))
						break;
				break;
			}
		}
		CheckPos();
		
		ShowCell(selRow, selCol);
		Redraw();
		break;
	
	case VK_PRIOR:
		PageUp(&selRow);
		if (GetCellProp(selRow, selCol) & GWCP_DISABLED)
			FindDown(&selRow, &selCol, GWCP_DISABLED);
		CheckPos();
		
		ShowCell(selRow, selCol);
		Redraw();
		break;
	
	case VK_NEXT:
		PageDown(&selRow);
		if (GetCellProp(selRow, selCol) & GWCP_DISABLED)
			FindUp(&selRow, &selCol, GWCP_DISABLED);
		CheckPos();
		
		ShowCell(selRow, selCol);
		Redraw();
		break;
	
	case VK_DELETE:
		SendMessage(wndHandle, WM_CLEAR, 0, 0);
		break;
	
	default:
		if (GetKeyState(VK_CONTROL) < 0)
			switch (key)
			{
			case 'X':
				SendMessage(wndHandle, WM_CUT, 0, 0);
				break;
			
			case 'C':
				SendMessage(wndHandle, WM_COPY, 0, 0);
				break;
			
			case 'V':
				SendMessage(wndHandle, WM_PASTE, 0, 0);
				break;
			
			case 'A':
				SendMessage(wndHandle, GM_SELALL, 0, 0);
				break;
			
			case 'Z':
				SendMessage(wndHandle, WM_UNDO, 0, 0);
				break;
			
			case 'Y':
				SendMessage(wndHandle, GM_REDO, 0, 0);
				break;
			}
	}
}

void GridWindow::KeyChar(char key, int nRep)
{
	unsigned int prop;
	
	if (GetKeyState(VK_CONTROL) >= 0 && key != '\t' && key != '\n' && key != '\r' && key != '\b' && key != '\e')
	{
		if (haveCells() && !((prop = GetCellProp(selRow, selCol)) & GWCP_READONLY))
		{
			if (prop & GWCP_MARKCELL)
			{
				ShowCell(selRow, selCol);
				if (key != ' ')
					MessageBeep(MB_OK);
				else
				{
					size_t index;
					bool needRedraw;
					
					index = nCols;
					index *= selRow;
					index += selCol;
					needRedraw = false;
					AddUndoOp(true, selRow, selCol, GWUO_MARK, true);
					
					while(--nRep >= 0)
						needRedraw = needRedraw || cells[index].SwitchMark();
					if (needRedraw)
						Redraw();
				}
			}
			else
			{
				char *buf = new char [nRep+1];
				
				buf[nRep] = 0;
				while(--nRep >= 0)
					buf[nRep] = key;
				
				ShowCell(selRow, selCol);
				BeginModify(buf);
				delete[] buf;
			}
		}
	}
}

void GridWindow::UndoCmd(void)
{
	unsigned int tag;
	GridUndoOperation *op;
	
	op = (GridUndoOperation*) undoList.GetUndo();
	tag = op->tag;
	
	do
	{
		undoList.Undo();
		op = (GridUndoOperation*) undoList.GetUndo();
	}
	while (op && op->tag == tag);
	Redraw();
}

void GridWindow::RedoCmd(void)
{
	unsigned int tag;
	GridUndoOperation *op;
	
	op = (GridUndoOperation*) undoList.GetRedo();
	tag = op->tag;
	
	do
	{
		undoList.Redo();
		op = (GridUndoOperation*) undoList.GetRedo();
	}
	while (op && op->tag == tag);
	Redraw();
}

void GridWindow::CopyCmd(void)
{
	void* ptr;
	ExtString str;
	
	if (OpenClipboard(wndHandle))
	{
		if (EmptyClipboard())
		{
			UINT form;
			HGLOBAL hMem;
			
			if ((form = RegisterClipboardFormat(CF_HTMLFORMAT)))
			{
				MakeHTML(&str);
				hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (str.Length()+1) * sizeof(char));
				if (hMem)
				{
					if (!(ptr = GlobalLock(hMem)))
						GlobalFree(hMem);
					else
					{
						if (str.Length() > 0)
							memcpy(ptr, str, (str.Length()+1) * sizeof(char));
						else
							*((char*) ptr) = 0;
						GlobalUnlock(hMem);
						
						if (!SetClipboardData(form, hMem))
							GlobalFree(hMem);
					}
				}
			}
			
			MakeText(&str);
			hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (str.Length()+1) * sizeof(char));
			if (hMem)
			{
				if (!(ptr = GlobalLock(hMem)))
					GlobalFree(hMem);
				else
				{
					if (str.Length() > 0)
						memcpy(ptr, str, (str.Length()+1) * sizeof(char));
					else
						*((char*) ptr) = 0;
					GlobalUnlock(hMem);
					
					if (!SetClipboardData(CF_TEXT, hMem))
						GlobalFree(hMem);
				}
			}
		}
		CloseClipboard();
	}
}

void GridWindow::ClearCmd(void)
{
	int i, j;
	bool newOp;
	int minSelRow, maxSelRow, minSelCol, maxSelCol;
	
	minSelRow = ((addSelRow < 0) ? selRow + addSelRow : selRow);
	maxSelRow = ((addSelRow > 0) ? selRow + addSelRow : selRow);
	minSelCol = ((addSelCol < 0) ? selCol + addSelCol : selCol);
	maxSelCol = ((addSelCol > 0) ? selCol + addSelCol : selCol);
	
	newOp = true;
	for (i = minSelRow; i <= maxSelRow; ++i)
		for (j = minSelCol; j <= maxSelCol; ++j)
			if (!(GetCellProp(i, j) & GWCP_READONLY))
			{
				AddUndoOp(true, i, j, GWUO_TEXT, newOp);
				newOp = false;
				
				SetCell(i, j, NULL);
				SendChangeNotify(i, j);
			}
	Redraw();
}

void GridWindow::PasteCmd(void)
{
	char *buf, *ptr;
	
	buf = 0;
	if (OpenClipboard(wndHandle))
	{
		HGLOBAL hMem = GetClipboardData(CF_TEXT);
		if (hMem && (ptr = (char*) GlobalLock(hMem)))
		{
			size_t size = strlen(ptr);
			
			buf = new char [size+1];
			memcpy(buf, ptr, size * sizeof(char));
			buf[size] = 0;
			
			GlobalUnlock(hMem);
		}
		CloseClipboard();
	}
	
	if (buf)
	{
		ParseText(buf);
		delete[] buf;
		Redraw();
	}
}

void GridWindow::ParseText(char *text)
{
	char sym;
	int row, col;
	bool quot, newOp;
	size_t prev, cur;
	int minSelRow, maxSelRow, minSelCol, maxSelCol;
	
	if (!text)
		return;
	
	if (addSelRow || addSelCol)
	{
		minSelRow = ((addSelRow < 0) ? selRow + addSelRow : selRow);
		maxSelRow = ((addSelRow > 0) ? selRow + addSelRow : selRow);
		minSelCol = ((addSelCol < 0) ? selCol + addSelCol : selCol);
		maxSelCol = ((addSelCol > 0) ? selCol + addSelCol : selCol);
	}
	else
	{
		minSelRow = selRow;
		maxSelRow = nRows - 1;
		minSelCol = selCol;
		maxSelCol = nCols - 1;
	}
	row = minSelRow;
	col = minSelCol;
	
	quot = false;
	newOp = true;
	prev = cur = 0;
	for (;;)
	{
		sym = text[cur];
		if ((quot || (sym != '\t' && sym != '\n')) && sym != 0)
		{
			if (sym == '\"')
			{
				if (quot)
					quot = false;
				else if (text[prev] == '\"')
					quot = true;
			}
			++cur;
		}
		else
		{
			unsigned int prop = GetCellProp(row, col);
			if (col <= maxSelCol && !(prop & GWCP_READONLY))
			{
				size_t it = 0;
				while (it < cur && text[cur-it-1] == '\r')
					text[cur-(++it)] = 0;
				
				if (text[prev] != '\"')
					text[cur] = 0;
				else
				{
					bool lastQuot;
					size_t quotCnt;
					
					++prev;
					quotCnt = 0;
					lastQuot = false;
					for (it = prev; it < cur; ++it)
					{
						if (quotCnt)
							text[it-quotCnt] = text[it];
						if (lastQuot)
							lastQuot = false;
						else if (text[it] == '\"')
						{
							++quotCnt;
							lastQuot = true;
						}
					}
					for (it -= quotCnt; it <= cur; ++it)
						text[it] = 0;
				}
				
				if (!(prop & GWCP_MARKCELL))
				{
					AddUndoOp(true, row, col, GWUO_TEXT, newOp);
					newOp = false;
					
					SetCell(row, col, text + prev);
				}
				else
				{
					unsigned int mark = 0;
					switch (text[prev])
					{
					case 0:
					case ' ':
					case '\r':
					case '\n':
					case '\t':
						mark = GWCM_MARKN;
						break;
					
					case '*':
						mark = GWCM_MARKP;
						break;
					
					case 'v':
					case 'V':
						mark = GWCM_MARKV;
						break;
					
					case 'x':
					case 'X':
						mark = GWCM_MARKX;
						break;
					}
					
					AddUndoOp(true, row, col, GWUO_MARK, newOp);
					newOp = false;
					
					if (mark & prop)
						SetCellMark(row, col, mark);
					else
					{
						size_t index;
						
						index = nCols;
						index *= row;
						index += col;
						
						cells[index].mark = 1 << GWCM_MARKCNT;
						cells[index].SwitchMark();
					}
				}
				
				SendChangeNotify(row, col);
			}
			
			if (sym == 0)
				break;
			else if (sym == '\n')
			{
				if (++row > maxSelRow)
					break;
				
				col = minSelCol;
			}
			else if (col <= maxSelCol)
				++col;
			
			prev = ++cur;
		}
	}
}

void GridWindow::MakeText(ExtString *str)
{
	int i, j;
	bool quot;
	size_t it;
	const char *cell;
	int minSelRow, maxSelRow, minSelCol, maxSelCol;
	
	if (!str)
		return;
	*str = NULL;
	
	minSelRow = ((addSelRow < 0) ? selRow + addSelRow : selRow);
	maxSelRow = ((addSelRow > 0) ? selRow + addSelRow : selRow);
	minSelCol = ((addSelCol < 0) ? selCol + addSelCol : selCol);
	maxSelCol = ((addSelCol > 0) ? selCol + addSelCol : selCol);
	
	for (i = minSelRow; i <= maxSelRow; ++i)
	{
		for (j = minSelCol; j <= maxSelCol; ++j)
		{
			if (GetCellProp(i, j) & GWCP_MARKCELL)
				switch (GetCellMark(i, j))
				{
				case GWCM_MARKP:
					*str += '*';
					break;
				
				case GWCM_MARKV:
					*str += 'v';
					break;
				
				case GWCM_MARKX:
					*str += 'x';
					break;
				
				default:
					*str += ' ';
				}
			else if ((cell = GetCell(i, j)))
			{
				quot = (cell[0] == '\"');
				for (it = 0; !quot && cell[it]; ++it)
					quot = quot || cell[it] == '\t' || cell[it] == '\r' || cell[it] == '\n';
				
				if (quot)
				{
					*str += '\"';
					for (it = 0; cell[it]; ++it)
					{
						if (cell[it] == '\"')
						{
							*str += '\"';
							*str += '\"';
						}
						else
							*str += cell[it];
					}
					*str += '\"';
				}
				else
					*str += cell;
			}
			
			if (j < maxSelCol)
				*str += '\t';
		}
		if (i < maxSelRow)
		{
			*str += '\r';
			*str += '\n';
		}
	}
}

void GridWindow::MakeHTML(ExtString *str)
{
	HDC hDC;
	bool was;
	HFONT font;
	char buf[16];
	int i, j, dpi;
	LOGFONT fontData;
	const char *cell;
	unsigned int prop, align;
	COLORREF color, stdCellColor, stdTextColor;
	int minSelRow, maxSelRow, minSelCol, maxSelCol;
	int htmlStrCode, htmlEndCode, htmlStrFrag, htmlEndFrag;
	
	if (!str)
		return;
	*str = HTML_VER;
	
	*str += HTML_START;
	htmlStrCode = str->Length();
	*str += HTML_END;
	htmlEndCode = str->Length();
	
	*str += HTML_FRAGSTART;
	htmlStrFrag = str->Length();
	*str += HTML_FRAGEND;
	htmlEndFrag = str->Length();
	
	*str += "\r\n\r\n";
	SetVal(str, htmlStrCode, str->Length());
	
	*str += "<html>\r\n<head>\r\n";
	
	*str += "<meta http-equiv=Content-Type content=\"text/html; charset=utf-8\" />\r\n<style>\r\n<!--\r\n";
	
	*str += "br\r\n\t{mso-data-placement:same-cell;}\r\n";
	*str += "table\r\n\t{border-collapse:collapse;}\r\n";
	
	*str += "td\r\n\t{border:none";
	
	if (wndHandle && (hDC = GetWindowDC(wndHandle)))
	{
		dpi = GetDeviceCaps(hDC, LOGPIXELSY);
		ReleaseDC(wndHandle, hDC);
	}
	else
		dpi = 0;
	
	if (gridFont)
		font = gridFont;
	else
		font = (HFONT) GetStockObject(SYSTEM_FONT);
	if (font)
	{
		GetObject(font, sizeof(fontData), &fontData);
		
		if (dpi)
		{
			if (fontData.lfHeight > 0)
				sprintf(buf, "%d", (int) (72.0 * 0.85 * fontData.lfHeight / dpi));
			else if (fontData.lfHeight < 0)
				sprintf(buf, "%d", (int) (72.0 * (1-fontData.lfHeight) / dpi));
			
			*str += ";\r\n\tfont-size:";
			*str += buf;
			*str += "pt";
		}
		
		if (fontData.lfWeight > 0)
		{
			sprintf(buf, "%ld", fontData.lfWeight);
			*str += ";\r\n\tfont-weight:";
			*str += buf;
		}
		
		*str += ";\r\n\tfont-family:\"";
		*str += fontData.lfFaceName;
		*str += "\"";
	}
	
	*str += ";\r\n\ttext-align:";
	if (GWCP_NORMAL & GWCA_LEFT)
		*str += "left";
	else if (GWCP_NORMAL & GWCA_CENTER)
		*str += "center";
	else
		*str += "right";
	
	*str += ";\r\n\tvertical-align:";
	if (GWCP_NORMAL & GWCA_BOTTOM)
		*str += "bottom";
	else if (GWCP_NORMAL & GWCA_VCENTER)
		*str += "middle";
	else
		*str += "top";
	
	*str += ";}\r\n-->\r\n</style>\r\n</head>\r\n<body>\r\n\r\n<table border=0 cellpadding=0 cellspacing=0>\r\n";
	
	*str += HTML_FSTRTEXT;
	SetVal(str, htmlStrFrag, str->Length());
	
	stdCellColor = GetSysColor(COLOR_WINDOW);
	stdTextColor = GetSysColor(COLOR_BTNTEXT);
	
	minSelRow = ((addSelRow < 0) ? selRow + addSelRow : selRow);
	maxSelRow = ((addSelRow > 0) ? selRow + addSelRow : selRow);
	minSelCol = ((addSelCol < 0) ? selCol + addSelCol : selCol);
	maxSelCol = ((addSelCol > 0) ? selCol + addSelCol : selCol);
	
	for (i = minSelRow; i <= maxSelRow; ++i)
	{
		sprintf(buf, "%d", GetRowHeight(i));
		*str += "<tr height=";
		*str += buf;
		
		if (dpi > 0)
		{
			sprintf(buf, "%.2lf", 72.0 * GetRowHeight(i) / dpi);
			*str += " style=\"height:";
			*str += buf;
			*str += "pt\"";
		}
		*str += ">\r\n";
		
		for (j = minSelCol; j <= maxSelCol; ++j)
		{
			*str += "\t<td";
			
			was = false;
			prop = GetCellProp(i, j);
			
			if (i == minSelRow)
			{
				sprintf(buf, "%d", GetColWidth(j));
				*str += " width=";
				*str += buf;
				
				if (dpi > 0)
				{
					sprintf(buf, "%.2lf", 72.0 * GetColWidth(j) / dpi);
					*str += " style=\"width:";
					*str += buf;
					*str += "pt";
					was = true;
				}
			}
			
			if (prop & GWCA_LEFT)
				align = GWCA_LEFT;
			else if (prop & GWCA_CENTER)
				align = GWCA_CENTER;
			else
				align = GWCA_RIGHT;
			if (!(align & GWCP_NORMAL))
			{
				if (was)
					*str += "; ";
				else
				{
					*str += " style=\"";
					was = true;
				}
				
				*str += "text-align:";
				if (align == GWCA_LEFT)
					*str += "left";
				else if (align == GWCA_CENTER)
					*str += "center";
				else
					*str += "right";
			}
			
			if (prop & GWCA_BOTTOM)
				align = GWCA_BOTTOM;
			else if (prop & GWCA_VCENTER)
				align = GWCA_VCENTER;
			else
				align = GWCA_TOP;
			if (!(align & GWCP_NORMAL))
			{
				if (was)
					*str += "; ";
				else
				{
					*str += " style=\"";
					was = true;
				}
				
				*str += "vertical-align:";
				if (align == GWCA_BOTTOM)
					*str += "bottom";
				else if (align == GWCA_VCENTER)
					*str += "middle";
				else
					*str += "top";
			}
			
			if ((color = GetCellTextColor(i, j)) != stdTextColor)
			{
				if (was)
					*str += "; ";
				else
				{
					*str += " style=\"";
					was = true;
				}
				
				sprintf(buf, "%.2x%.2x%.2x", GetRValue(color), GetGValue(color), GetBValue(color));
				*str += "color:#";
				*str += buf;
			}
			
			if ((color = GetCellColor(i, j)) != stdCellColor)
			{
				if (was)
					*str += "; ";
				else
				{
					*str += " style=\"";
					was = true;
				}
				
				sprintf(buf, "%.2x%.2x%.2x", GetRValue(color), GetGValue(color), GetBValue(color));
				*str += "background:#";
				*str += buf;
			}
			
			if (was)
				*str += "\">";
			else
				*str += ">";
			if (prop & GWCP_MARKCELL)
				switch (GetCellMark(i, j))
				{
				case GWCM_MARKP:
					*str += '*';
					break;
				
				case GWCM_MARKV:
					*str += 'v';
					break;
				
				case GWCM_MARKX:
					*str += 'x';
					break;
				
				default:
					*str += ' ';
				}
			else if ((cell = GetCell(i, j)))
			{
				WCHAR *utf;
				int len, cur;
				
				len = MultiByteToWideChar(CP_ACP, 0, cell, -1, NULL, 0);
				utf = new WCHAR [len];
				
				MultiByteToWideChar(CP_ACP, 0, cell, -1, utf, len);
				for (cur = 0; cur < len; ++cur)
				{
					if (utf[cur] & 0xF800)
					{
						*str += 0xE0 | (utf[cur] >> 12);
						*str += 0x80 | ((utf[cur] >> 6) & 0x3F);
						*str += 0x80 | (utf[cur] & 0x3F);
					}
					else if (utf[cur] & 0xFF80)
					{
						*str += 0xC0 | (utf[cur] >> 6);
						*str += 0x80 | (utf[cur] & 0x3F);
					}
					else if (utf[cur] == '\n')
						*str += "<br>";
					else if (utf[cur] && utf[cur] != '\r')
						*str += utf[cur];
				}
				delete[] utf;
			}
			*str += "</td>\r\n";
		}
		*str += "</tr>\r\n";
	}
	
	SetVal(str, htmlEndFrag, str->Length());
	*str += HTML_FENDTEXT;
	
	*str += "</table>\r\n\r\n</body>\r\n</html>";
	SetVal(str, htmlEndCode, str->Length());
}

void GridWindow::InitEditBox(void)
{
	HINSTANCE hInst;
	
	hInst = (HINSTANCE) GetWindowLong(wndHandle, GWL_HINSTANCE);
	editWnd = CreateWindow("EDIT", NULL, WS_CHILD | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0, 0, defEditSize, defEditSize, wndHandle, 0, hInst, NULL);
	SetWindowSubclass(editWnd, EditNotifyProc, ENSC_CID, 0);
}

void GridWindow::InitScrollBar(void)
{
	HINSTANCE hInst;
	SCROLLINFO scrInfo;
	
	hInst = (HINSTANCE) GetWindowLong(wndHandle, GWL_HINSTANCE);
	horBar = CreateWindow("SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_HORZ, 0, 0, 0, 0, wndHandle, 0, hInst, NULL);
	vertBar = CreateWindow("SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_VERT, 0, 0, 0, 0, wndHandle, 0, hInst, NULL);
	
	scrInfo.cbSize = sizeof(scrInfo);
	scrInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	scrInfo.nPage = 1; 
    scrInfo.nMin = 0;
	
	scrInfo.nPos = fstVisRow;
	scrInfo.nMax = (nRows ? nRows-1 : 0);
	SetScrollInfo(vertBar, SB_CTL, &scrInfo, FALSE);
	
	scrInfo.nPos = fstVisCol;
	scrInfo.nMax = (nCols ? nCols-1 : 0);
	SetScrollInfo(horBar, SB_CTL, &scrInfo, FALSE);
}

void GridWindow::InitClass(HINSTANCE hInst)
{
	WNDCLASS wndClass;
	if (classReady) return;
	
	wndClass.style = 0;
	wndClass.hIcon = NULL;
	wndClass.cbClsExtra = 0;
    wndClass.hInstance = hInst;
    wndClass.lpszMenuName = NULL;
    wndClass.hbrBackground = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	
	wndClass.cbWndExtra = sizeof(LONG_PTR);
    wndClass.lpszClassName = GW_CLASSNAME;
	wndClass.lpfnWndProc = WinWndClasserProc;
	
	if (RegisterClass(&wndClass))
		classReady = true;
}

void GridWindow::ClientToPaintRect(RECT* clRect) const
{
	clRect->top += 2;
	clRect->left += 2;
	clRect->right -= 2 + GetSystemMetrics(SM_CXVSCROLL);
	clRect->bottom -= 2 + GetSystemMetrics(SM_CYHSCROLL);
}

LRESULT GridWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR wndPtr;
	if ((wndPtr = GetWindowLongPtr(hWnd, 0)))
		return ((GridWindow*) wndPtr)->WindowProc(hWnd, uMsg, wParam, lParam);
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT WINAPI EditNotifyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR)
{
	bool stopProc;
	EKDNM notInfo;
	
	switch (uMsg)
	{
	case WM_KEYDOWN:
		notInfo.hdr.idFrom = 0;
		notInfo.hdr.hwndFrom = hWnd;
		notInfo.hdr.code = EN_KEYDOWN;
		
		notInfo.wParam = wParam;
		notInfo.lParam = lParam;
		notInfo.stopProc = &stopProc;
		
		stopProc = false;
		SendMessage(GetParent(hWnd), WM_NOTIFY, 0, (LPARAM) &notInfo);
		
		if (stopProc)
			return 0;
		break;
	
	case WM_CHAR:
		if (!IsWindowVisible(hWnd))
			return 0;
		break;
	
	case WM_NCDESTROY: 
		RemoveWindowSubclass(hWnd, EditNotifyProc, uIdSubclass);
		break;
	}
	
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void SetVal(ExtString* str, int pos, int val)
{
	char c;
	int fs = HTML_FIELDSIZE;
	
	do
	{
		c = '0' + val%10;
		(*str)[--pos] = c;
	}
	while (--fs > 0 && (val /= 10) > 0);
}
