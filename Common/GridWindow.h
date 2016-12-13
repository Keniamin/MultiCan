#ifndef _GRIDWINDOW_H
#define _GRIDWINDOW_H

#include <windows.h>
#include "UndoList.h"
#include "ExtString.h"

#define GWCA_TOP 0x10
#define GWCA_LEFT 0x04
#define GWCA_RIGHT 0x01
#define GWCA_BOTTOM 0x40
#define GWCA_CENTER 0x02
#define GWCA_VCENTER 0x20

#define GWCA_HMASK 0x0F
#define GWCA_VMASK 0xF0

#define GWCP_DISABLED 0x0100
#define GWCP_READONLY 0x0200
#define GWCP_MARKCELL 0x0400
#define GWCP_NORMAL (GWCA_BOTTOM | GWCA_CENTER)

#define GWCM_MARKCNT 4
#define GWCM_MARKBASE 16
#define GWCM_MARKN 0x010000
#define GWCM_MARKP 0x020000
#define GWCM_MARKV 0x040000
#define GWCM_MARKX 0x080000

#define GWUO_TEXT 'T'
#define GWUO_PROP 'P'
#define GWUO_MARK 'M'
#define GWUO_CCOL 'B'
#define GWUO_TCOL 'F'
#define GWUO_SIZE 'S'

#define GM_CANUNDO (WM_USER+1)
#define GM_SELALL (WM_USER+2)
#define GM_REDO (WM_USER+3)

#define GN_CHANGE WM_COMMAND
#define GN_SETFOCUS WM_SETFOCUS
#define GN_KILLFOCUS WM_KILLFOCUS
#define GN_UNDOSTATE WM_UNDO

struct GWNM_CHANGE
{
	NMHDR hdr;
	int row;
	int col;
};

struct GWNM_UNDO
{
	NMHDR hdr;
	bool canUndo;
	bool canRedo;
};

class GridUndoOperation;
class GridWindow;

class GridCell
{
private:
	static const int markPadding = 2;
	static const int markSize = 11;
	
	char *text;
	unsigned int prop;
	unsigned int mark;
	COLORREF textColor, cellColor, selColor, markBoxColor;
	
	void InitColors(void);
	
	friend class GridWindow;
	friend class GridUndoOperation;
	
public:
	GridCell(void);
	~GridCell(void) { if (text) delete[] text; }
	
	void Grab(GridCell*);
	
	bool SwitchMark(void);
	void DrawCellText(HDC device_context, const RECT& cell) const;
	void DrawCell(HDC device_context, const RECT& cell, bool selected) const;
};

class GridWindow
{
private:
	static const size_t maxCells = 2097152;
	static const int maxRowsCols = 8192;
	
	static const int minScrollTime = 175;
	static const int defRowHeight = 20;
	static const int defColWidth = 75;
	static const int defEditSize = 25;
	static const int scrollBorder = 5;
	static const int defSize = 150;
	
	static bool classReady;
	
	HWND wndHandle, editWnd;
	HWND horBar, vertBar;
	HMENU popMenu;
	
	HFONT gridFont;
	HPEN borderPen, selPen;
	HCURSOR curHor, curVert;
	
	bool inFocus, dblClick, canScroll;
	unsigned char moveMode;
	int dragNum, dragStart;
	
	int nCols, nRows;
	GridCell *cells;
	
	unsigned int *colsWidth, *rowsHeight;
	int fstVisRow, fstVisCol;
	int addSelRow, addSelCol;
	int selRow, selCol;
	int wheelDelta;
	
	UndoList undoList;
	
	void CheckVis(void);
	void CheckPos(void);
	void CheckSel(void);
	
	void DeleteCells(void);
	void EnsureCells(void);
	
	int GetColByCoord(int x);
	int GetRowByCoord(int y);
	void ShowCell(int row, int col);
	
	void SendUndoNotify(void);
	void SendChangeNotify(int row, int col);
	
	void PageUp(int *row);
	void PageDown(int *row);
	void PageLeft(int *col);
	void PageRight(int *col);
	
	bool FindUp(int *row, int *col, unsigned int skip_mask, bool can_change_col = false);
	bool FindDown(int *row, int *col, unsigned int skip_mask, bool can_change_col = false);
	bool FindLeft(int *row, int *col, unsigned int skip_mask, bool can_change_row = false);
	bool FindRight(int *row, int *col, unsigned int skip_mask, bool can_change_row = false);
	
	void AddUndoOp(bool internal, int row, int col, char type, bool new_operation);
	void BeginModify(const char *initial_text = NULL);
	void EndModify(void);
	
	void DrawWindow(void);
	void ResizeWindow(void);
	void ShowMenu(int x, int y);
	void MouseMove(DWORD keys, int x, int y);
	void MouseClick(DWORD keys, int x, int y);
	
	void KeyDown(int key, int repeats);
	void KeyChar(char key, int repeats);
	
	void UndoCmd(void);
	void RedoCmd(void);
	
	void CopyCmd(void);
	void ClearCmd(void);
	void PasteCmd(void);
	
	void ParseText(char *text);
	void MakeText(ExtString *string);
	void MakeHTML(ExtString *string);
	
	void InitEditBox(void);
	void InitScrollBar(void);
	void InitClass(HINSTANCE);
	
	void ClientToPaintRect(RECT *client_rect) const;
	
	bool haveCells(void) const { return (nRows > 0 && nCols > 0); }
	
	friend class GridUndoOperation;
	
public:
	GridWindow(void);
	~GridWindow(void);
	
	bool Create(HWND);
	void Destroy(void);
	void DestroyAll(void);
	LRESULT WindowProc(HWND, UINT, WPARAM, LPARAM);
	void Redraw(int *from_x = NULL, int *from_y = NULL);
	
	void DeleteRow(int row);
	void DeleteCol(int col);
	void AddRow(int new_row);
	void AddCol(int new_col);
	
	void SetRowsCount(int new_rows_count);
	void SetColsCount(int new_cols_count);
	void SetSizes(int new_rows_count, int new_cols_count);
	
	unsigned int GetColWidth(int col);
	unsigned int GetRowHeight(int row);
	void SetColWidth(int col, unsigned int width);
	void SetRowHeight(int row, unsigned int height);
	
	const char* GetCell(int row, int col);
	void SetCell(int row, int col, const char *value);
	
	COLORREF GetCellColor(int row, int col);
	COLORREF GetCellTextColor(int row, int col);
	void SetCellColor(int row, int col, COLORREF color);
	void SetCellTextColor(int row, int col, COLORREF color);
	
	unsigned int GetCellProp(int row, int col);
	void SetCellProp(int row, int col, unsigned int properties);
	
	unsigned int GetCellMark(int row, int col);
	void SetCellMark(int row, int col, unsigned int mark);
	
	void SelectCells(int row, int col, int additional_rows_selected, int additional_cols_selected, bool show_cells = false);
	
	operator HWND() { return wndHandle; }
	
	int GetColsCount(void) const { return nCols; }
	int GetRowsCount(void) const { return nRows; }
	
	HFONT ClearFont(void) { return SetFont(NULL); }
	HFONT SetFont(HFONT font) { HFONT tmp = gridFont; gridFont = font; return tmp; }
	
	void SaveToUndo(int row, int col, char type, bool new_operation = true) { AddUndoOp(false, row, col, type, new_operation); }
};

inline void GridWindow::Destroy(void)
{
	if (wndHandle)
		DestroyWindow(wndHandle);
}

inline void GridWindow::DestroyAll(void)
{
	Destroy();
	DeleteCells();
	undoList.Clear();
}

inline unsigned int GridWindow::GetColWidth(int i)
{
	if (i < 0 || i >= nCols)
		return 0;
	EnsureCells();
	return colsWidth[i];
}

inline unsigned int GridWindow::GetRowHeight(int i)
{
	if (i < 0 || i >= nRows)
		return 0;
	EnsureCells();
	return rowsHeight[i];
}

inline void GridWindow::SetColWidth(int i, unsigned int width)
{
	if (i < 0 || i >= nCols)
		return;
	EnsureCells();
	colsWidth[i] = width;
}

inline void GridWindow::SetRowHeight(int i, unsigned int height)
{
	if (i < 0 || i >= nRows)
		return;
	EnsureCells();
	rowsHeight[i] = height;
}

inline const char* GridWindow::GetCell(int i, int j)
{
	size_t index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return NULL;
	index = nCols;
	index *= i;
	index += j;
	
	EnsureCells();
	return cells[index].text;
}

inline void GridWindow::SetCell(int i, int j, const char *val)
{
	size_t size, index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return;
	index = nCols;
	index *= i;
	index += j;
	
	EnsureCells();
	if (cells[index].text)
		delete[] cells[index].text;
	if (!val || !(size = strlen(val)))
		cells[index].text = NULL;
	else
	{
		cells[index].text = new char [size+1];
		memcpy(cells[index].text, val, size * sizeof(char));
		cells[index].text[size] = 0;
	}
}

inline COLORREF GridWindow::GetCellColor(int i, int j)
{
	size_t index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return 0;
	index = nCols;
	index *= i;
	index += j;
	
	EnsureCells();
	return cells[index].cellColor;
}

inline COLORREF GridWindow::GetCellTextColor(int i, int j)
{
	size_t index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return 0;
	index = nCols;
	index *= i;
	index += j;
	
	EnsureCells();
	return cells[index].textColor;
}

inline void GridWindow::SetCellColor(int i, int j, COLORREF color)
{
	size_t index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return;
	index = nCols;
	index *= i;
	index += j;
	
	EnsureCells();
	cells[index].cellColor = color;
	cells[index].InitColors();
}

inline void GridWindow::SetCellTextColor(int i, int j, COLORREF color)
{
	size_t index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return;
	index = nCols;
	index *= i;
	index += j;
	
	EnsureCells();
	cells[index].textColor = color;
}

inline unsigned int GridWindow::GetCellProp(int i, int j)
{
	size_t index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return 0;
	index = nCols;
	index *= i;
	index += j;
	
	EnsureCells();
	return cells[index].prop;
}

inline void GridWindow::SetCellProp(int i, int j, unsigned int prop)
{
	size_t index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return;
	index = nCols;
	index *= i;
	index += j;
	
	if (!(prop & GWCA_VMASK))
		prop |= (GWCP_NORMAL & GWCA_VMASK);
	if (!(prop & GWCA_HMASK))
		prop |= (GWCP_NORMAL & GWCA_HMASK);
	
	EnsureCells();
	cells[index].prop = prop;
}

inline unsigned int GridWindow::GetCellMark(int i, int j)
{
	size_t index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return 0;
	index = nCols;
	index *= i;
	index += j;
	
	EnsureCells();
	return (cells[index].mark << GWCM_MARKBASE);
}

inline void GridWindow::SetCellMark(int i, int j, unsigned int mark)
{
	size_t index;
	if (i < 0 || j < 0 || i >= nRows || j >= nCols)
		return;
	index = nCols;
	index *= i;
	index += j;
	
	EnsureCells();
	cells[index].mark = mark >> GWCM_MARKBASE;
}

#endif
