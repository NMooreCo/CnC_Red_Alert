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
**	Writable string literals.
**
**	The game was written for Watcom C/C++, whose string literals were writable, and it does
**	modify some of them in place (file names patched with a digit, upper cased names...).
**	Modern compilers put literals in read only memory, so such code would crash. Make the
**	program's own read only data writable at start up, before any of the game runs.
**	Shared libraries and code are not affected.
*/
#if defined(__linux__) || defined(__FreeBSD__)
#include <link.h>
#include <sys/mman.h>

static int Make_Program_Data_Writable(struct dl_phdr_info * info, size_t, void * data)
{
	int * done = (int *)data;
	if (*done) return(1);		// The first object reported is the program itself.
	*done = 1;

	uintptr_t page = (uintptr_t)sysconf(_SC_PAGESIZE);
	for (int index = 0; index < info->dlpi_phnum; index++) {
		ElfW(Phdr) const & header = info->dlpi_phdr[index];
		if (header.p_type == PT_LOAD && !(header.p_flags & PF_W) && !(header.p_flags & PF_X)) {
			uintptr_t start = (uintptr_t)info->dlpi_addr + header.p_vaddr;
			uintptr_t end = start + header.p_memsz;
			start &= ~(page - 1);
			end = (end + page - 1) & ~(page - 1);
			mprotect((void *)start, end - start, PROT_READ | PROT_WRITE);
		}
	}
	return(1);
}

__attribute__((constructor(101))) static void Writable_String_Literals(void)
{
	int done = 0;
	dl_iterate_phdr(Make_Program_Data_Writable, &done);
}

#elif defined(__APPLE__)
/*
**	macOS: the string literals are linked into the writable __DATA segment (CMakeLists.txt,
**	-rename_section). The constant data that the loader fixes up (__DATA_CONST) it then makes
**	read only: make that writable again, as the other systems' read only data is.
*/
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <sys/mman.h>
#include <unistd.h>

__attribute__((constructor(101))) static void Writable_String_Literals(void)
{
	unsigned long size = 0;
	uint8_t * data = getsegmentdata((const struct mach_header_64 *)_dyld_get_image_header(0), "__DATA_CONST", &size);
	if (data != NULL && size > 0) {
		uintptr_t page = (uintptr_t)sysconf(_SC_PAGESIZE);
		uintptr_t start = (uintptr_t)data & ~(page - 1);
		uintptr_t end = ((uintptr_t)data + size + page - 1) & ~(page - 1);
		mprotect((void *)start, end - start, PROT_READ | PROT_WRITE);
	}
}

#elif defined(_WIN32)
/*
**	Windows: the read only sections (.rdata...) of the program's PE image. The headers are
**	read directly; the stand-in <windows.h> doesn't describe them.
*/
extern "C" char __ImageBase;		/* Provided by the linker: where the image is loaded. */
extern "C" __declspec(dllimport) int __stdcall VirtualProtect(void * address, size_t size, unsigned long protection, unsigned long * old_protection);

__attribute__((constructor(101))) static void Writable_String_Literals(void)
{
	enum {
		SCN_MEM_DISCARDABLE = 0x02000000,
		SCN_MEM_EXECUTE = 0x20000000,
		SCN_MEM_READ = 0x40000000,
		SCN_MEM_WRITE = 0x80000000,
		PAGE_READWRITE_ACCESS = 0x04
	};
	unsigned char * base = (unsigned char *)&__ImageBase;
	unsigned char * nt = base + *(uint32_t *)(base + 0x3C);		// IMAGE_DOS_HEADER.e_lfanew
	if (memcmp(nt, "PE\0\0", 4) != 0) return;
	unsigned sections = *(uint16_t *)(nt + 6);						// FileHeader.NumberOfSections
	unsigned optional_size = *(uint16_t *)(nt + 20);				// FileHeader.SizeOfOptionalHeader
	unsigned char * section = nt + 24 + optional_size;				// IMAGE_SECTION_HEADER[]
	for (unsigned index = 0; index < sections; index++, section += 40) {
		uint32_t size = *(uint32_t *)(section + 8);						// Misc.VirtualSize
		uint32_t address = *(uint32_t *)(section + 12);					// VirtualAddress
		uint32_t flags = *(uint32_t *)(section + 36);					// Characteristics
		if ((flags & SCN_MEM_READ) && !(flags & (SCN_MEM_WRITE | SCN_MEM_EXECUTE | SCN_MEM_DISCARDABLE)) && size > 0) {
			unsigned long old = 0;
			VirtualProtect(base + address, size, PAGE_READWRITE_ACCESS, &old);
		}
	}
}

/*
**	A Windows program (as opposed to a console program) has no standard output or error.
**	Started from a console window, it uses that one, so that its messages can be seen there
**	(those of RA_EXTRACT, for instance). Output that was redirected is left alone.
*/
extern "C" __declspec(dllimport) void * __stdcall GetStdHandle(unsigned long which);
extern "C" __declspec(dllimport) int __stdcall AttachConsole(unsigned long process);

__attribute__((constructor(102))) static void Use_Parent_Console(void)
{
	const unsigned long STD_OUTPUT = (unsigned long)-11;
	const unsigned long STD_ERROR = (unsigned long)-12;
	const unsigned long PARENT_PROCESS = (unsigned long)-1;
	bool no_output = (GetStdHandle(STD_OUTPUT) == NULL);
	bool no_error = (GetStdHandle(STD_ERROR) == NULL);
	if ((no_output || no_error) && AttachConsole(PARENT_PROCESS)) {
		if (no_output) freopen("CONOUT$", "w", stdout);
		if (no_error) freopen("CONOUT$", "w", stderr);
	}
}
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
	/*
	**	1 GB free of 2 GB: the game multiplies these out in 32 bits.
	*/
	diskspace->total_clusters = 0x80000;
	diskspace->avail_clusters = 0x40000;
	diskspace->sectors_per_cluster = 8;
	diskspace->bytes_per_sector = 512;
	return(0);
}
