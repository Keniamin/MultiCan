#ifndef _NPOINT_H
#define _NPOINT_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

class nVector
{
private:
	double *x;
	int n;

public:

	~nVector(void);
	nVector(const nVector&);
	nVector(int, const double* = NULL);
	nVector(void): x(NULL), n(0) {}

	void Redim(int);

	const nVector& operator *= (double);
	const nVector& operator = (const nVector&);
	const nVector& operator += (const nVector&);
	const nVector& operator -= (const nVector&);

	void Print(FILE* = NULL) const;

	int Dim(void) const { return n; }
	double& operator [] (int i) { return *(x+i); }
	double operator [] (int i) const { if (i < 0 || i >= n) return 0; return x[i]; }

	friend nVector operator - (const nVector&);
	friend double scal(const nVector&, const nVector&);
};

typedef nVector nPoint;

inline double norm(const nVector& P)
{
	return sqrt(scal(P, P));
}

inline double scal(const nVector& P, const nVector& Q)
{
	double sum = 0;
	if (P.n == Q.n)
	{
		for (int i = 0; i < P.n; ++i)
			sum += P.x[i] * Q.x[i];
		return sum;
	}
	return 3e+9;
}

inline nVector operator - (const nVector& P)
{
	nVector T(P);
	for (int i = 0; i < T.n; ++i)
		T.x[i] = -T.x[i];
	return T;
}

inline nVector operator * (double k, const nVector& P)
{
	nVector T(P);
	T *= k;
	return T;
}

inline nVector operator *(const nVector& P, double k)
{
	nVector T(P);
	T *= k;
	return T;
}

inline nVector operator +(const nVector& P, const nVector& Q)
{
	nVector T(P);
	T += Q;
	return T;
}

inline nVector operator -(const nVector& P, const nVector& Q)
{
	nVector T(P);
	T -= Q;
	return T;
}

#endif
