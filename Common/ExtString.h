#ifndef _EXTSTRING_H
#define _EXTSTRING_H

#include <stdlib.h>

class ExtString
{
private:
	static const size_t memBlock = 4096;
	
	size_t cap;
	char *str;
	int len;
	
	void Clear(void);
	void Ensure(size_t length);
	
	void Append(const char *arr, size_t length);
	
public:
	~ExtString() { if (str) delete[] str; }
	ExtString(): cap(0), str(NULL), len(0) {}
	ExtString(const char* s): cap(0), str(NULL), len(0) { *this = s; }
	ExtString(const ExtString& s): cap(0), str(NULL), len(0) { *this = s; }
	
	const ExtString& operator += (char symbol);
	
	const ExtString& operator = (const char *array);
	const ExtString& operator += (const char *array);
	
	const ExtString& operator = (const ExtString& string);
	const ExtString& operator += (const ExtString& string);
	
	int Length(void) { return len; }
	operator const char*(void) { return str; }
	char& operator [] (int i) { return *(str+i); }
	char operator [] (int i) const { if (i < 0 || i >= len) return 0; return str[i]; }
};

inline ExtString operator +(const ExtString& s, const ExtString& t)
{
	ExtString r(s);
	r += t;
	return r;
}

inline ExtString operator +(const ExtString& s, const char* t)
{
	ExtString r(s);
	r += t;
	return r;
}

inline ExtString operator +(const char* s, const ExtString& t)
{
	ExtString r(s);
	r += t;
	return r;
}

inline ExtString operator +(const ExtString& s, char c)
{
	ExtString r(s);
	r += c;
	return r;
}

#endif
