#include <cmath>
#include <cstdlib>

#include "CanAn.h"

#include "Common\nPoint.h"

const double EPS = 1e-12;
const double EIG_EPS = 1e-6;

size_t MatrInd(int row, int col);
bool CholeskyDecomp(double *matrix, int size);
double* InverseMatr(double *matrix, int size);
double* TransformMatr(double *matrix, double *transformation, int size);
nVector MatrMultVector(double *matrix, const nVector& vector, int size, bool is_symmetric);

DWORD WINAPI AnalysisThread(LPVOID param)
{
	if (param == NULL)
		return 0;
	return ((CanAn*) param) -> Analysis();
}

CanAn::CanAn(void):
	anThread(NULL),

	factCount(0),
	setCount(0),
	varCount(0),

	intMatr(NULL),
	extMatr(NULL),

	setMark(NULL),
	setVol(NULL),

	eigVal(),
	cumProp(),
	stdDev(),
	mean(NULL),
	varCoef(NULL),
	stdCoef(NULL),
	setCoord(NULL),

	factName(NULL),
	setName(NULL)
{}

CanAn::~CanAn()
{
	StopAnalysis();
	DeleteArrays();
}

void CanAn::StartAnalysis(void)
{
	DWORD tid;

	CheckThread();
	if (!anThread)
		anThread = CreateThread(NULL, 0, AnalysisThread, this, 0, &tid);
}

void CanAn::StopAnalysis(void)
{
	DWORD exitCode = 0xFFFFFFFF;
	if (exitCode == STILL_ACTIVE)
		exitCode = 0x80000000;

	if (anThread)
	{
		TerminateThread(anThread, exitCode);
		CheckThread();
	}
}

void CanAn::SetFactCount(int count)
{
	if (anThread)
		return;
	if (count < 0)
		count = 0;

	DeleteArrays();
	factCount = count;
	if (setCount > 1)
		varCount = ((setCount-1 < factCount) ? setCount-1 : factCount);
	else
		varCount = 0;
}

void CanAn::SetSetCount(int count)
{
	if (anThread)
		return;
	if (count < 0)
		count = 0;

	DeleteArrays();
	setCount = count;
	if (setCount > 1)
		varCount = ((setCount-1 < factCount) ? setCount-1 : factCount);
	else
		varCount = 0;
}

void CanAn::SetFactName(int i, const char *name)
{
	size_t size;
	if (anThread || i < 0 || i >= factCount)
		return;

	EnsureArrays();
	if (factName[i])
		delete[] factName[i];
	if (!name || !(size = strlen(name)))
		factName[i] = NULL;
	else
	{
		factName[i] = new char [size+1];
		memcpy(factName[i], name, size);
		factName[i][size] = 0;
	}
}

void CanAn::SetSetName(int i, const char *name)
{
	size_t size;
	if (anThread || i < 0 || i >= setCount)
		return;

	EnsureArrays();
	if (setName[i])
		delete[] setName[i];
	if (!name || !(size = strlen(name)))
		setName[i] = NULL;
	else
	{
		setName[i] = new char [size+1];
		memcpy(setName[i], name, size);
		setName[i][size] = 0;
	}
}

const char* CanAn::GetFactName(int i)
{
	if (anThread || i < 0 || i >= factCount)
		return NULL;

	EnsureArrays();
	return factName[i];
}

const char* CanAn::GetSetName(int i)
{
	if (anThread || i < 0 || i >= setCount)
		return NULL;

	EnsureArrays();
	return setName[i];
}

void CanAn::SetMean(int s, int f, double val)
{
	if (anThread || s < 0 || f < 0 || s >= setCount || f >= factCount)
		return;

	EnsureArrays();
	mean[s][f] = val;
}

void CanAn::SetMark(int i, unsigned int val)
{
	if (anThread || i < 0 || i >= setCount)
		return;

	EnsureArrays();
	setMark[i] = val;
}

void CanAn::SetVol(int i, unsigned int val)
{
	if (anThread || i < 0 || i >= setCount)
		return;

	EnsureArrays();
	setVol[i] = val;
}

void CanAn::SetCorMatr(int r, int c, double val)
{
	if (anThread || r < 0 || c < 0 || r >= factCount || c >= factCount)
		return;

	EnsureArrays();
	intMatr[MatrInd(r, c)] = val;
}

void CanAn::SetStdDev(int i, double val)
{
	if (anThread || i < 0 || i >= factCount)
		return;

	EnsureArrays();
	stdDev[i] = val;
}

double CanAn::GetVarCoef(int v, int f)
{
	if (anThread || v < 0 || f < 0 || v >= varCount || f >= factCount)
		return 0;

	EnsureArrays();
	if (f >= varCoef[v].Dim())
		return 0;
	return varCoef[v][f];
}

double CanAn::GetStdCoef(int v, int f)
{
	if (anThread || v < 0 || f < 0 || v >= varCount || f >= factCount)
		return 0;

	EnsureArrays();
	if (f >= stdCoef[v].Dim())
		return 0;
	return stdCoef[v][f];
}

double CanAn::GetSetCoord(int s, int v)
{
	if (anThread || s < 0 || v < 0 || s >= setCount || v >= varCount)
		return 0;

	EnsureArrays();
	if (v >= setCoord[s].Dim())
		return 0;
	return setCoord[s][v];
}

double CanAn::GetCumProp(int i)
{
	if (anThread || i < 0 || i >= varCount)
		return 0;

	EnsureArrays();
	if (i >= cumProp.Dim())
		return 0;
	return cumProp[i];
}

double CanAn::GetEigVal(int i)
{
	if (anThread || i < 0 || i >= varCount)
		return 0;

	EnsureArrays();
	if (i >= eigVal.Dim())
		return 0;
	return eigVal[i];
}

double CanAn::GetConst(int i)
{
	if (anThread || i < 0 || i >= varCount)
		return 0;

	EnsureArrays();
	if (factCount >= varCoef[i].Dim())
		return 0;
	return varCoef[i][factCount];
}

unsigned int CanAn::GetMark(int i)
{
	if (anThread || i < 0 || i >= setCount)
		return 0;

	EnsureArrays();
	return setMark[i];
}

int CanAn::Analysis(void)
{
	nPoint v;
	int r, c, s, num;
	double tmp, *matr;

	if (varCount < 1)
		return 1;

	EnsureArrays();

	for (c = 0; c < factCount; ++c)
		for (r = 0; r <= c; ++r)
			intMatr[MatrInd(r, c)] *= stdDev[r] * stdDev[c];

	num = 0;
	mean[setCount] *= 0;
	for (s = 0; s < setCount; ++s)
	{
		num += setVol[s];
		mean[setCount] += setVol[s] * mean[s];
	}

	if (num < 1)
		return 2;

	tmp = 1.0 / num;
	for (c = 0; c < factCount; ++c)
		for (r = 0; r <= c; ++r)
		{
			extMatr[MatrInd(r, c)] = -tmp * mean[setCount][r] * mean[setCount][c];
			for (s = 0; s < setCount; ++s)
				extMatr[MatrInd(r, c)] += setVol[s] * mean[s][r] * mean[s][c];
		}

	if (!CholeskyDecomp(intMatr, factCount))
		return 3;

	if (!(matr = InverseMatr(intMatr, factCount)))
		return 4;
	delete[] intMatr;
	intMatr = matr;

	if (!(matr = TransformMatr(extMatr, intMatr, factCount)))
		return 5;
	delete[] extMatr;
	extMatr = matr;

	tmp = 0;
	for (r = 0; r < factCount; ++r)
		tmp += extMatr[MatrInd(r, r)];

	if (fabs(tmp) > EPS)
		tmp = 1.0 / tmp;
	cumProp.Redim(varCount);
	for (s = 0; s < varCount; ++s)
		cumProp[s] = tmp;

	eigVal.Redim(varCount);
	for (s = 0; s < varCount; ++s)
	{
		varCoef[s].Redim(factCount);
		for (r = 0; r < factCount; ++r)
			varCoef[s][r] = 1;

		tmp = 1;
		do
		{
			eigVal[s] = tmp;
			v = MatrMultVector(extMatr, varCoef[s], factCount, true);

			tmp = 0;
			for (r = 0; r < factCount; ++r)
				if (tmp < fabs(v[r]))
					tmp = fabs(v[r]);

			if (tmp > EPS)
				varCoef[s] = v * (1.0 / tmp);
			else
				break;
		}
		while (fabs(eigVal[s] - tmp) > EIG_EPS);

		eigVal[s] = tmp;
		cumProp[s] *= eigVal[s];
		varCoef[s] *= 1.0 / norm(varCoef[s]);

		for (c = 0; c < factCount; ++c)
			for (r = 0; r <= c; ++r)
				extMatr[MatrInd(r, c)] -= tmp * varCoef[s][r] * varCoef[s][c];
	}

	tmp = 1.0 / (setCount - 1);
	for (s = 0; s < varCount; ++s)
	{
		varCoef[s] = MatrMultVector(intMatr, varCoef[s], factCount, false);
		eigVal[s] *= tmp;
	}

	for (s = 0; s < varCount; ++s)
	{
		stdCoef[s] = varCoef[s];
		for (r = 0; r < factCount; ++r)
			stdCoef[s][r] *= stdDev[r];

		tmp = scal(varCoef[s], mean[setCount]);
		varCoef[s].Redim(factCount + 1);
		varCoef[s][factCount] = tmp / num;
	}

	for (s = 0; s < setCount; ++s)
	{
		mean[s].Redim(factCount + 1);
		mean[s][factCount] = -1;

		setCoord[s].Redim(varCount);
		for (r = 0; r < varCount; ++r)
			setCoord[s][r] = scal(mean[s], varCoef[r]);
	}
	return 0;
}

void CanAn::EnsureArrays(void)
{
	int i, prod;

	prod = factCount * (factCount+1) / 2;
	if (!intMatr && prod > 0)
	{
		intMatr = new double [prod];
		for (i = 0; i < prod; ++i)
			intMatr[i] = 0;
	}
	if (!extMatr && prod > 0)
		extMatr = new double [prod];

	if (!setMark && setCount > 0)
		setMark = new unsigned int [setCount];
	if (!setVol && setCount > 0)
	{
		setVol = new unsigned int [setCount];
		for (i = 0; i < setCount; ++i)
			setVol[i] = 0;
	}
	if (!setCoord && setCount > 0)
		setCoord = new nVector [setCount];

	if (stdDev.Dim() != factCount)
		stdDev = nVector(factCount);
	if (!mean)
	{
		mean = new nVector [setCount+1];
		for (i = 0; i <= setCount; ++i)
			mean[i].Redim(factCount);
	}
	if (!varCoef && varCount > 0)
		varCoef = new nVector [varCount];
	if (!stdCoef && varCount > 0)
		stdCoef = new nVector [varCount];

	if (!factName && factCount > 0)
	{
		factName = new char* [factCount];
		for (i = 0; i < factCount; ++i)
			factName[i] = NULL;
	}
	if (!setName && setCount > 0)
	{
		setName = new char* [setCount];
		for (i = 0; i < setCount; ++i)
			setName[i] = NULL;
	}
}

void CanAn::DeleteArrays(void)
{
	int i;

	if (intMatr)
	{
		delete[] intMatr;
		intMatr = NULL;
	}
	if (extMatr)
	{
		delete[] extMatr;
		extMatr = NULL;
	}

	if (setMark)
	{
		delete[] setMark;
		setMark = NULL;
	}
	if (setVol)
	{
		delete[] setVol;
		setVol = NULL;
	}
	if (setCoord)
	{
		delete[] setCoord;
		setCoord = NULL;
	}

	if (mean)
	{
		delete[] mean;
		mean = NULL;
	}
	if (varCoef)
	{
		delete[] varCoef;
		varCoef = NULL;
	}
	if (stdCoef)
	{
		delete[] stdCoef;
		stdCoef = NULL;
	}

	if (factName)
	{
		for (i = 0; i < factCount; ++i)
			if (factName[i])
				delete[] factName[i];
		delete[] factName;
		factName = NULL;
	}
	if (setName)
	{
		for (i = 0; i < setCount; ++i)
			if (setName[i])
				delete[] setName[i];
		delete[] setName;
		setName = NULL;
	}
}

void CanAn::CheckThread(void)
{
	DWORD state;
	if (anThread)
	{
		if (GetExitCodeThread(anThread, &state))
			if (state != STILL_ACTIVE)
			{
				CloseHandle(anThread);
				anThread = 0;
			}
	}
}

size_t MatrInd(int r, int c)
{
	size_t index;

	if (r <= c)
	{
		index = c * (c+1) / 2;
		index += r;
	}
	else
	{
		index = r * (r+1) / 2;
		index += c;
	}
	return index;
}

bool CholeskyDecomp(double *matr, int n)
{
	double tmp;
	int r, c, i;

	if (!matr)
		return false;

	for (r = 0; r < n; ++r)
	{
		for (i = 0; i < r; ++i)
		{
			tmp = matr[MatrInd(i, r)];
			matr[MatrInd(r, r)] -= tmp * tmp;
		}
		if (matr[MatrInd(r, r)] < EPS)
			return false;

		matr[MatrInd(r, r)] = sqrt(matr[MatrInd(r, r)]);
		tmp = 1.0 / matr[MatrInd(r, r)];
		for (c = r+1; c < n; ++c)
		{
			for (i = 0; i < r; ++i)
				matr[MatrInd(r, c)] -= matr[MatrInd(i, r)] * matr[MatrInd(i, c)];
			matr[MatrInd(r, c)] *= tmp;
		}
	}
	return true;
}

double* InverseMatr(double *matr, int n)
{
	int r, c, i;
	double tmp, *inv;

	if (n < 1 || !matr)
		return NULL;

	inv = new double [n * (n+1) / 2];
	for (c = 0; c < n; ++c)
	{
		for (r = 0; r < c; ++r)
			inv[MatrInd(r, c)] = 0;
		inv[MatrInd(c, c)] = 1;
	}

	for (r = n-1; r >= 0; --r)
	{
		if (matr[MatrInd(r, r)] < EPS)
		{
			delete[] inv;
			return NULL;
		}
		tmp = 1 / matr[MatrInd(r, r)];
		for (c = r; c < n; ++c)
			inv[MatrInd(r, c)] *= tmp;

		for (i = 0; i < r; ++i)
		{
			tmp = matr[MatrInd(i, r)];
			for (c = r; c < n; ++c)
				inv[MatrInd(i, c)] -= inv[MatrInd(r, c)] * tmp;
		}
	}
	return inv;
}

double* TransformMatr(double *matr, double *trans, int n)
{
	int r, c, i;
	double tmp, *res;

	if (n < 1 || !matr || !trans)
		return NULL;

	res = new double [n * (n+1) / 2];
	for (c = 0; c < n; ++c)
		for (r = 0; r <= c; ++r)
			res[MatrInd(r, c)] = 0;

	for (i = 0; i < n; ++i)
		for (c = i; c < n; ++c)
		{
			tmp = 0;
			for (r = 0; r <= c; ++r)
				tmp += trans[MatrInd(r, c)] * matr[MatrInd(i, r)];

			for (r = i; r <= c; ++r)
				res[MatrInd(r, c)] += tmp * trans[MatrInd(i, r)];
		}
	return res;
}

nVector MatrMultVector(double *matr, const nVector& v, int n, bool sym)
{
	double tmp;
	nVector res;
	int r, c, k;

	if (n < 1 || !matr)
		return res;

	res.Redim(n);
	for (c = 0; c < n; ++c)
	{
		tmp = v[c];
		k = (sym ? n-1 : c);
		for (r = 0; r <= k; ++r)
			res[r] += tmp * matr[MatrInd(r, c)];
	}
	return res;
}
