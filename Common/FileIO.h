#ifndef _FILE_ERRORCHECK_IO_H
#define _FILE_ERRORCHECK_IO_H

static bool ReadBuf(HANDLE hFile, const void* val, DWORD size, bool *wasEOF = NULL)
{
	unsigned char* buf;
	DWORD read, cur;

	if (wasEOF)
		*wasEOF = false;
	buf = (unsigned char*) val;
	for (cur = 0; cur < size; cur += read)
		if (!ReadFile(hFile, buf+cur, size-cur, &read, NULL))
			return false;
		else if (read == 0)
		{
			if (wasEOF)
				*wasEOF = true;
			return false;
		}
	return true;
}

static bool WriteBuf(HANDLE hFile, const void* val, DWORD size)
{
	unsigned char* buf;
	DWORD wrote, cur;

	buf = (unsigned char*) val;
	for (cur = 0; cur < size; cur += wrote)
		if (!WriteFile(hFile, buf+cur, size-cur, &wrote, NULL) || wrote == 0)
			return false;
	return true;
}

#endif
