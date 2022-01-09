#ifndef _MULTICAN_MAINWINDOW_H
#define _MULTICAN_MAINWINDOW_H

#include <vector>
#include <windows.h>
#include <commctrl.h>

#include "Common/GridWindow.h"

enum class FileTypeEnum : char
{
	DATA = 'D',
	RESULT = 'R',
	// with Mahalanobis distances
	RESULT_V2 = 'M',
};

class MainWindow
{
private:
	static const int minWndHeight = 450;
	static const int minWndWidth = 600;

	static bool classReady;

	HMENU wndMenu;
	HWND wndHandle;
	HWND tabCtrl;

	GridWindow factGrid, setGrid, corGrid;
	GridWindow varGrid, pntGrid, stdGrid, distGrid;
	HFONT gFont;

	char *workFile;
	FileTypeEnum fileType;
	bool changed;
	int pathLen;

	void CreateTabs(void);
	void UpdateTitle(void);
	void ResizeChildren(void);
	void ShowTab(int tab_number);
	void SetFileType(FileTypeEnum new_type);

	void ResetFile(void);
	bool SaveFile(char *filename);
	bool LoadFile(char *filename);

	bool SaveData(HANDLE file);
	bool SaveSizes(HANDLE file);
	bool LoadData(HANDLE file, FileTypeEnum new_type);
	bool LoadSizes(HANDLE file, FileTypeEnum new_type);

	void SetResultGridsFixedCells(bool var_grid, bool pnt_grid, bool std_grid, bool dist_grid, bool set_sizes = true);
	void SetDataGridsFixedCells(bool fact_grid, bool set_grid, bool cor_grid, bool set_sizes = true);

	void ShowIntervalError(const char *msg, const char *interval);
	std::vector<std::pair<unsigned int, unsigned int>> ParseIntervals(const char *intervals_string, unsigned int item_count);

	void DeleteSets(const char *intervals_string);
	void DeleteFactors(const char *intervals_string);
	void AddSets(unsigned int count);
	void AddFactors(unsigned int count);

	void ChildNotify(NMHDR *notify_info);
	void ChangeDecSepCmd(void);
	void AssociateCmd(void);
	void DelAddCmd(int id);
	void SaveAsCmd(void);
	void AboutCmd(void);
	void HelpCmd(void);
	void OpenCmd(void);

	void StartCanAn(void);

	void InitTabCtrl(void);
	void InitGridsFont(void);
	void InitClass(HINSTANCE);

public:
	~MainWindow();
	MainWindow(void);

	bool AskSave(void);
	bool Create(HINSTANCE);
	void SetWorkFile(const char *name, int path_length = -1);
	LRESULT WindowProc(HWND, UINT, WPARAM, LPARAM);

	operator HWND() { return wndHandle; }
};

#endif
