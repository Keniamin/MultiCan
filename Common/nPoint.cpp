#include <stdio.h>
#include <stdlib.h>

#include "nPoint.h"

nVector::~nVector(void)
{
	if (x)
		delete[] x;
}

nVector::nVector(const nVector& P):
	x(NULL), n(P.n)
{
	if (n > 0)
		x = new double[n];
	for (int i = 0; i < n; ++i)
		x[i] = P.x[i];
}

nVector::nVector(int dim, const double* arr /* = NULL */):
	x(NULL), n(dim)
{
	if (n > 0)
		x = new double[n];
	for (int i = 0; i < n; ++i)
		x[i] = (arr ? arr[i] : 0);
}

void nVector::Redim(int dim)
{
	double *old;

	old = x;
	if (dim > 0)
		x = new double[dim];
	else
		x = NULL;

	for (int i = 0; i < dim; ++i)
		x[i] = ((i < n) ? old[i] : 0);

	n = dim;
	if (old)
		delete[] old;
}

const nVector& nVector::operator *=(double k)
{
	for (int i = 0; i < n; ++i)
		x[i] *= k;
	return *this;
}

const nVector& nVector::operator =(const nVector& P)
{
	n = P.n;
	if (x)
		delete[] x;
	if (n > 0)
		x = new double[n];
	else
		x = NULL;

	for (int i = 0; i < n; ++i)
		x[i] = P.x[i];
	return *this;
}

const nVector& nVector::operator +=(const nVector& P)
{
	if (n == P.n)
	{
		for (int i = 0; i < n; ++i)
			x[i] += P.x[i];
		return *this;
	}
	return *((nPoint*) NULL);
}

const nVector& nVector::operator -=(const nVector& P)
{
	if (n == P.n)
	{
		for (int i = 0; i < n; ++i)
			x[i] -= P.x[i];
		return *this;
	}
	return *((nPoint*) NULL);
}

void nVector::Print(FILE* fd /* = NULL */) const
{
	int i;
	if (fd == NULL)
		fd = stdout;

	fprintf(fd, "(");
	for (i = 0; i < n; ++i)
	{
		fprintf(fd, "%11.8lf", x[i]);
		if (i < n-1) printf(", ");
	}
	fprintf(fd, ")");
}
