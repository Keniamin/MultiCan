#ifndef _CMDLINE_H
#define _CMDLINE_H

#include <stdlib.h>

class CmdLine
{
private:
	int nArg;
	char** args;
	
	void ClearArgs(void);
	void ParceCmdLine(const char *command_line);
	int CountCmdLine(const char *command_line, int *max_arg_len);
	
public:
	CmdLine(const char* str): nArg(0), args(NULL) { *this = str; }
	CmdLine(void): nArg(0), args(NULL) {}
	~CmdLine(void) { ClearArgs(); }
	
	const char* operator [] (int argument_number) const;
	void operator = (const char *command_line);
	
	int count(void) const { return nArg; }
};

void CmdLine::ClearArgs(void)
{
	if (args)
	{
		for (int i = 0; i < nArg; ++i)
			delete[] args[i];
		delete[] args;
		args = NULL;
	}
}
	
const char* CmdLine::operator [] (int i) const
{
	if (i < 0 || i >= nArg)
		return NULL;
	return args[i];
}

void CmdLine::operator = (const char *str)
{
	int i, aLen;
	
	ClearArgs();
	nArg = CountCmdLine(str, &aLen);
	if (nArg > 0)
	{
		args = new char* [nArg];
		if (aLen > 0)
			for (i = 0; i < nArg; ++i)
				args[i] = new char [aLen];
		ParceCmdLine(str);
	}
}

void CmdLine::ParceCmdLine(const char *str)
{
	int i, len, num;
	bool inQuote;
	
	i = 0;
	len = 0;
	num = 0;
	inQuote = false;
	for(i = 0;; ++i)
	{
		if (str[i] == 0)
		{
			if (len)
				args[num][len] = 0;
			break;
		}
		else if (str[i] == '\"')
		{
			inQuote = !inQuote;
			if (len)
			{
				args[num][len] = 0;
				len = 0; ++num;
			}
		}
		else
		{
			if (str[i] != ' ' || inQuote)
			{
				args[num][len] = str[i];
				++len;
			}
			else if (len)
			{
				args[num][len] = 0;
				len = 0; ++num;
			}
		}
	}
}

int CmdLine::CountCmdLine(const char *str, int *maxArgLen)
{
	int i, len;
	bool inQuote;
	int argsCount, argLen;
	
	len = 0;
	argLen = 0;
	argsCount = 0;
	inQuote = false;
	for(i = 0;; ++i)
	{
		if (str[i] == 0)
		{
			if (len > argLen)
				argLen = len;
			if (len)
				++argsCount;
			break;
		}
		else if (str[i] == '\"')
		{
			inQuote = !inQuote;
			if (len > argLen)
				argLen = len;
			if (len)
				++argsCount;
			len = 0;
		}
		else
		{
			if (str[i] != ' ' || inQuote)
				++len;
			else
			{
				if (len > argLen)
					argLen = len;
				if (len)
					++argsCount;
				len = 0;
			}
		}
	}
	
	if (maxArgLen) *maxArgLen = argLen;
	return argsCount;
}

#endif
