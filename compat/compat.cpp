/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	compat.cpp -- implementations of the non-standard C runtime functions declared in
**	compat/portable.h and the stand-in DOS headers in compat/include and compat/posix.
*/

#include "windows.h"
#include "dos.h"
#include "win32_internal.h"

#include <string>
#include <vector>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(_WIN32)
#include <unistd.h>
#include <dirent.h>
#include "conio.h"
#include "io.h"
#include "direct.h"
#endif


/***************************************************************************************
**	String functions.
*/
#if !defined(_WIN32)
extern "C" int stricmp(const char * a, const char * b)
{
	while (*a && *b) {
		int ca = toupper((unsigned char)*a);
		int cb = toupper((unsigned char)*b);
		if (ca != cb) return(ca - cb);
		a++;
		b++;
	}
	return(toupper((unsigned char)*a) - toupper((unsigned char)*b));
}

extern "C" int strnicmp(const char * a, const char * b, size_t n)
{
	while (n > 0 && *a && *b) {
		int ca = toupper((unsigned char)*a);
		int cb = toupper((unsigned char)*b);
		if (ca != cb) return(ca - cb);
		a++;
		b++;
		n--;
	}
	if (n == 0) return(0);
	return(toupper((unsigned char)*a) - toupper((unsigned char)*b));
}

extern "C" int memicmp(const void * va, const void * vb, size_t n)
{
	const unsigned char * a = (const unsigned char *)va;
	const unsigned char * b = (const unsigned char *)vb;
	for (size_t i = 0; i < n; i++) {
		int ca = toupper(a[i]);
		int cb = toupper(b[i]);
		if (ca != cb) return(ca - cb);
	}
	return(0);
}

extern "C" char * strupr(char * s)
{
	for (char * p = s; *p; p++) *p = (char)toupper((unsigned char)*p);
	return(s);
}

extern "C" char * strlwr(char * s)
{
	for (char * p = s; *p; p++) *p = (char)tolower((unsigned char)*p);
	return(s);
}

extern "C" char * strrev(char * s)
{
	size_t len = strlen(s);
	for (size_t i = 0; i < len / 2; i++) {
		char t = s[i];
		s[i] = s[len - 1 - i];
		s[len - 1 - i] = t;
	}
	return(s);
}

static char * Unsigned_To_String(unsigned int value, char * buffer, int radix, bool negative)
{
	char temp[40];
	int pos = 0;
	if (radix < 2 || radix > 36) radix = 10;
	do {
		int digit = value % radix;
		temp[pos++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
		value /= radix;
	} while (value);
	char * out = buffer;
	if (negative) *out++ = '-';
	while (pos) *out++ = temp[--pos];
	*out = 0;
	return(buffer);
}

extern "C" char * itoa(int value, char * buffer, int radix)
{
	if (radix == 10 && value < 0) {
		return(Unsigned_To_String((unsigned int)(-(long long)value), buffer, radix, true));
	}
	return(Unsigned_To_String((unsigned int)value, buffer, radix, false));
}

extern "C" char * ltoa(int value, char * buffer, int radix)
{
	return(itoa(value, buffer, radix));
}

extern "C" char * ultoa(unsigned int value, char * buffer, int radix)
{
	return(Unsigned_To_String(value, buffer, radix, false));
}


/***************************************************************************************
**	Path splitting in the DOS/Win32 style.
*/
extern "C" void _splitpath(const char * path, char * drive, char * dir, char * fname, char * ext)
{
	if (drive) drive[0] = 0;
	if (dir) dir[0] = 0;
	if (fname) fname[0] = 0;
	if (ext) ext[0] = 0;
	if (path == NULL) return;

	const char * p = path;
	if (isalpha((unsigned char)p[0]) && p[1] == ':') {
		if (drive) {
			drive[0] = p[0];
			drive[1] = ':';
			drive[2] = 0;
		}
		p += 2;
	}

	const char * last_sep = NULL;
	for (const char * q = p; *q; q++) {
		if (*q == '/' || *q == '\\') last_sep = q;
	}
	const char * name = p;
	if (last_sep) {
		if (dir) {
			size_t len = (size_t)(last_sep - p) + 1;
			memcpy(dir, p, len);
			dir[len] = 0;
		}
		name = last_sep + 1;
	}

	const char * dot = strrchr(name, '.');
	if (dot == NULL) dot = name + strlen(name);
	if (fname) {
		size_t len = (size_t)(dot - name);
		memcpy(fname, name, len);
		fname[len] = 0;
	}
	if (ext) strcpy(ext, dot);
}

extern "C" void _makepath(char * path, const char * drive, const char * dir, const char * fname, const char * ext)
{
	path[0] = 0;
	if (drive && drive[0]) {
		strncat(path, drive, 1);
		strcat(path, ":");
	}
	if (dir && dir[0]) {
		strcat(path, dir);
		char last = dir[strlen(dir) - 1];
		if (last != '/' && last != '\\') strcat(path, "/");
	}
	if (fname) strcat(path, fname);
	if (ext && ext[0]) {
		if (ext[0] != '.') strcat(path, ".");
		strcat(path, ext);
	}
}


/***************************************************************************************
**	Low level file helpers.
*/
extern "C" int filelength(int handle)
{
	struct stat st;
	if (fstat(handle, &st) != 0) return(-1);
	return((int)st.st_size);
}

extern "C" int ra_mkdir(const char * path)
{
	std::string p = Win32_Resolve_Path(path, true);
	return(mkdir(p.c_str(), 0755));
}

/*
**	Console input: there is no console; nothing is ever pending.
*/
extern "C" int getch(void)
{
	return(0);
}

extern "C" int kbhit(void)
{
	return(0);
}
#endif


/***************************************************************************************
**	DOS directory searching (_dos_findfirst & friends), built on FindFirstFile.
*/
extern "C" unsigned _dos_findfirst(const char * path, unsigned , struct find_t * buffer)
{
	WIN32_FIND_DATA data;
	HANDLE handle = FindFirstFile(path, &data);
	buffer->handle = NULL;
	if (handle == INVALID_HANDLE_VALUE) return(ERROR_FILE_NOT_FOUND);

	buffer->handle = handle;
	strncpy(buffer->name, data.cFileName, sizeof(buffer->name) - 1);
	buffer->name[sizeof(buffer->name) - 1] = 0;
	buffer->size = data.nFileSizeLow;
	buffer->attrib = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? _A_SUBDIR : _A_NORMAL;
	WORD date = 0, time = 0;
	FileTimeToDosDateTime(&data.ftLastWriteTime, &date, &time);
	buffer->wr_date = date;
	buffer->wr_time = time;
	return(0);
}

extern "C" unsigned _dos_findnext(struct find_t * buffer)
{
	if (buffer->handle == NULL) return(ERROR_NO_MORE_FILES);
	WIN32_FIND_DATA data;
	if (!FindNextFile((HANDLE)buffer->handle, &data)) {
		FindClose((HANDLE)buffer->handle);
		buffer->handle = NULL;
		return(ERROR_NO_MORE_FILES);
	}
	strncpy(buffer->name, data.cFileName, sizeof(buffer->name) - 1);
	buffer->name[sizeof(buffer->name) - 1] = 0;
	buffer->size = data.nFileSizeLow;
	buffer->attrib = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? _A_SUBDIR : _A_NORMAL;
	WORD date = 0, time = 0;
	FileTimeToDosDateTime(&data.ftLastWriteTime, &date, &time);
	buffer->wr_date = date;
	buffer->wr_time = time;
	return(0);
}

extern "C" unsigned _dos_findclose(struct find_t * buffer)
{
	if (buffer->handle) {
		FindClose((HANDLE)buffer->handle);
		buffer->handle = NULL;
	}
	return(0);
}

extern "C" void _dos_getdrive(unsigned * drive)
{
	*drive = 3;		// C:
}

extern "C" void _dos_setdrive(unsigned , unsigned * total)
{
	if (total) *total = 26;
}

extern "C" unsigned _dos_getdiskfree(unsigned , struct diskfree_t * diskspace)
{
	diskspace->total_clusters = 0x200000;
	diskspace->avail_clusters = 0x100000;
	diskspace->sectors_per_cluster = 8;
	diskspace->bytes_per_sector = 512;
	return(0);
}
