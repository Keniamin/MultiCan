#ifndef _CAN_ANALIS_H
#define _CAN_ANALIS_H

#include <windows.h>

#include "Common\nPoint.h"

class CanAn
{
private:
	HANDLE anThread;

	int factCount, setCount, varCount;

	double *intMatr, *extMatr;
	unsigned int *setMark, *setVol;
	nVector eigVal, cumProp, stdDev, *mean, *varCoef, *stdCoef, *setCoord;

	char **factName;
	char **setName;

	int Analysis(void);
	void EnsureArrays(void);
	void DeleteArrays(void);

	void CheckThread(void);
	friend DWORD WINAPI AnalysisThread(LPVOID);

public:
	CanAn();
	~CanAn();

	void StartAnalysis(void);
	void StopAnalysis(void);

	void SetFactCount(int count);
	void SetSetCount(int count);

	void SetFactName(int factor_number, const char *name);
	void SetSetName(int set_number, const char *name);
	const char* GetFactName(int factor_number);
	const char* GetSetName(int set_number);

	void SetMean(int set_number, int factor_number, double val);
	void SetMark(int set_number, unsigned int val);
	void SetVol(int set_number, unsigned int val);
	void SetCorMatr(int row, int col, double val);
	void SetStdDev(int factor_number, double val);

	double GetVarCoef(int variable_number, int factor_number);
	double GetStdCoef(int variable_number, int factor_number);
	double GetSetCoord(int set_number, int variable_number);
	double GetCumProp(int variable_number);
	double GetEigVal(int variable_number);
	double GetConst(int variable_number);
	unsigned int GetMark(int set_number);

	int GetVarCount(void) { return varCount; }
	int GetSetCount(void) { return setCount; }
	int GetFactCount(void) { return factCount; }
	bool AnalysisActive(void) { CheckThread(); return (anThread != 0); }
};

#endif
