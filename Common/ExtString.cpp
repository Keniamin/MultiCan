#include <string.h>
#include <stdlib.h>

#include "ExtString.h"

const ExtString& ExtString::operator += (char sym)
{
	Append(&sym, 1);
	return *this;
}

const ExtString& ExtString::operator = (const char *arr)
{
	if (!arr)
		Clear();
	else
	{
		len = 0;
		Append(arr, strlen(arr));
	}
	return *this;
}

const ExtString& ExtString::operator += (const char *arr)
{
	if (arr)
		Append(arr, strlen(arr));
	return *this;
}

const ExtString& ExtString::operator = (const ExtString& s)
{
	if (s.len < 1)
		Clear();
	else
	{
		len = 0;
		Append(s.str, s.len);
	}
	return *this;
}

const ExtString& ExtString::operator += (const ExtString& s)
{
	if (s.len > 0)
		Append(s.str, s.len);
	return *this;
}

void ExtString::Clear(void)
{
	if (str)
		delete[] str;
	len = cap = 0;
	str = NULL;
}

void ExtString::Ensure(size_t length)
{
	char *tmp;
	if (length <= cap)
		return;

	do
		cap += memBlock;
	while (cap < length);

	tmp = new char [cap];
	if (str)
	{
		if (len > 0)
			memcpy(tmp, str, len * sizeof(char));
		delete[] str;
	}
	str = tmp;
}

void ExtString::Append(const char *arr, size_t length)
{
	if (len < 0)
		len = 0;
	Ensure(len+length+1);

	memcpy(str+len, arr, length * sizeof(char));
	len += length;
	str[len] = 0;
}
