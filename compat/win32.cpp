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
**	win32.cpp -- the subset of the Win32 API used by the game, implemented with SDL2 and
**	the C/C++ standard library.
**
**	The game's platform glue (WINSTUB.CPP, KEY.CPP, RAWFILE.CPP, the sound and movie code)
**	talks to Windows through a few dozen API calls. Emulating those calls keeps that code
**	essentially unchanged:
**
**	  - Window messages: SDL events are translated into WM_xxx messages (WIN32LIB/SDL) and
**	    queued here; PeekMessage/GetMessage/DispatchMessage deliver them to the window
**	    procedure registered with RegisterClass(), exactly like Windows did.
**	  - Files: CreateFile/ReadFile/... map onto stdio, with DOS style paths ("C:\X\Y.INI")
**	    translated and resolved case insensitively so the original data file names work on
**	    case sensitive file systems.
**	  - Multimedia timers run on SDL timer threads, critical sections are SDL mutexes.
**	  - The registry does not exist; all queries fail ("not installed").
*/

#include "windows.h"
#include "mmsystem.h"
#include "win32_internal.h"

#include <SDL.h>

#include <deque>
#include <string>
#include <vector>
#include <algorithm>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#if defined(_WIN32)
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

/***************************************************************************************
**	Errors
*/
static thread_local DWORD LastError = 0;

extern "C" DWORD GetLastError(void)
{
	return(LastError);
}

static void Set_Error_From_Errno(void)
{
	switch (errno) {
		case ENOENT:	LastError = ERROR_FILE_NOT_FOUND; break;
		case EACCES:	LastError = ERROR_ACCESS_DENIED; break;
		case ENOSPC:	LastError = ERROR_DISK_FULL; break;
		case EROFS:		LastError = ERROR_WRITE_PROTECT; break;
		case EBADF:		LastError = ERROR_INVALID_HANDLE; break;
		default:			LastError = ERROR_ACCESS_DENIED; break;
	}
}


/***************************************************************************************
**	Paths. The game uses DOS paths, upper case names and a CD drive letter. Map them onto
**	the host file system: strip drive letters, use '/', and match each path component case
**	insensitively against what is actually on disk.
*/
static bool Is_Separator(char c) {return(c == '/' || c == '\\');}

std::string Win32_Resolve_Path(const char * name, bool for_create)
{
	if (name == NULL) return(std::string());

	std::string path(name);
	for (size_t i = 0; i < path.size(); i++) {
		if (path[i] == '\\') path[i] = '/';
	}

#if !defined(_WIN32)
	/*
	**	Strip a drive specification ("C:" or "C:/").
	*/
	if (path.size() >= 2 && isalpha((unsigned char)path[0]) && path[1] == ':') {
		path.erase(0, 2);
		if (path.empty()) path = ".";
	}

	/*
	**	If the path exists as given, use it.
	*/
	struct stat st;
	if (stat(path.c_str(), &st) == 0) return(path);

	/*
	**	Walk each component, matching case insensitively.
	*/
	std::string result;
	size_t pos = 0;
	if (!path.empty() && path[0] == '/') {
		result = "/";
		pos = 1;
	}
	while (pos <= path.size()) {
		size_t next = path.find('/', pos);
		if (next == std::string::npos) next = path.size();
		std::string comp = path.substr(pos, next - pos);
		pos = next + 1;
		if (comp.empty() || comp == ".") {
			if (next == path.size()) break;
			continue;
		}

		std::string dir = result.empty() ? std::string(".") : result;
		std::string candidate = result.empty() ? comp : (result == "/" ? "/" + comp : result + "/" + comp);
		if (comp != ".." && stat(candidate.c_str(), &st) != 0) {
			DIR * d = opendir(dir.c_str());
			if (d != NULL) {
				struct dirent * ent;
				while ((ent = readdir(d)) != NULL) {
					if (strcasecmp(ent->d_name, comp.c_str()) == 0) {
						candidate = result.empty() ? std::string(ent->d_name) : (result == "/" ? "/" + std::string(ent->d_name) : result + "/" + ent->d_name);
						break;
					}
				}
				closedir(d);
			}
		}
		result = candidate;
		if (next >= path.size()) break;
	}
	(void)for_create;
	return(result.empty() ? path : result);
#else
	(void)for_create;
	return(path);
#endif
}


/***************************************************************************************
**	Files
*/
struct Win32File {
	FILE *	File;
	std::string	Path;
};

extern "C" HANDLE CreateFile(LPCSTR name, DWORD access, DWORD , void * , DWORD creation, DWORD , HANDLE )
{
	std::string path = Win32_Resolve_Path(name, creation != OPEN_EXISTING);
	const char * mode = "rb";
	bool write = (access & GENERIC_WRITE) != 0;
	bool read = (access & GENERIC_READ) != 0;

	struct stat st;
	bool exists = (stat(path.c_str(), &st) == 0) && !S_ISDIR(st.st_mode);

	switch (creation) {
		case CREATE_NEW:
			if (exists) {LastError = ERROR_ACCESS_DENIED; return(INVALID_HANDLE_VALUE);}
			mode = read ? "w+b" : "wb";
			break;
		case CREATE_ALWAYS:
			mode = read ? "w+b" : "wb";
			break;
		case OPEN_ALWAYS:
			if (exists) {
				mode = write ? "r+b" : "rb";
			} else {
				mode = read ? "w+b" : "wb";
			}
			break;
		case TRUNCATE_EXISTING:
			if (!exists) {LastError = ERROR_FILE_NOT_FOUND; return(INVALID_HANDLE_VALUE);}
			mode = read ? "w+b" : "wb";
			break;
		case OPEN_EXISTING:
		default:
			if (!exists) {LastError = ERROR_FILE_NOT_FOUND; return(INVALID_HANDLE_VALUE);}
			mode = write ? "r+b" : "rb";
			break;
	}

	FILE * fp = fopen(path.c_str(), mode);
	if (fp == NULL) {
		Set_Error_From_Errno();
		return(INVALID_HANDLE_VALUE);
	}
	Win32File * file = new Win32File;
	file->File = fp;
	file->Path = path;
	LastError = 0;
	return((HANDLE)file);
}

static Win32File * As_File(HANDLE handle)
{
	if (handle == NULL || handle == INVALID_HANDLE_VALUE) return(NULL);
	return((Win32File *)handle);
}

extern "C" BOOL ReadFile(HANDLE handle, LPVOID buffer, DWORD bytes, LPDWORD read, LPOVERLAPPED )
{
	Win32File * file = As_File(handle);
	if (file == NULL) {LastError = ERROR_INVALID_HANDLE; return(FALSE);}
	size_t got = fread(buffer, 1, bytes, file->File);
	if (read) *read = (DWORD)got;
	if (got < bytes && ferror(file->File)) {
		clearerr(file->File);
		LastError = ERROR_ACCESS_DENIED;
		return(FALSE);
	}
	clearerr(file->File);
	return(TRUE);
}

extern "C" BOOL WriteFile(HANDLE handle, LPCVOID buffer, DWORD bytes, LPDWORD written, LPOVERLAPPED )
{
	Win32File * file = As_File(handle);
	if (file == NULL) {LastError = ERROR_INVALID_HANDLE; return(FALSE);}
	size_t put = fwrite(buffer, 1, bytes, file->File);
	if (written) *written = (DWORD)put;
	if (put < bytes) {
		Set_Error_From_Errno();
		return(FALSE);
	}
	return(TRUE);
}

extern "C" DWORD SetFilePointer(HANDLE handle, LONG distance, LONG * high, DWORD method)
{
	Win32File * file = As_File(handle);
	if (file == NULL) {LastError = ERROR_INVALID_HANDLE; return(INVALID_SET_FILE_POINTER);}
	long long offset = distance;
	if (high) offset = ((long long)*high << 32) | (DWORD)distance;
	int whence = (method == FILE_CURRENT) ? SEEK_CUR : ((method == FILE_END) ? SEEK_END : SEEK_SET);
	if (fseek(file->File, (long)offset, whence) != 0) {
		Set_Error_From_Errno();
		return(INVALID_SET_FILE_POINTER);
	}
	long pos = ftell(file->File);
	if (high) *high = 0;
	return((DWORD)pos);
}

extern "C" DWORD GetFileSize(HANDLE handle, LPDWORD high)
{
	Win32File * file = As_File(handle);
	if (file == NULL) {LastError = ERROR_INVALID_HANDLE; return(INVALID_FILE_SIZE);}
	fflush(file->File);
	struct stat st;
	if (fstat(fileno(file->File), &st) != 0) return(INVALID_FILE_SIZE);
	if (high) *high = 0;
	return((DWORD)st.st_size);
}

extern "C" BOOL FlushFileBuffers(HANDLE handle)
{
	Win32File * file = As_File(handle);
	if (file == NULL) return(FALSE);
	fflush(file->File);
	return(TRUE);
}

static void Time_To_FileTime(time_t t, FILETIME * ft)
{
	/*
	**	FILETIME is 100ns intervals since 1601-01-01.
	*/
	unsigned long long v = ((unsigned long long)t + 11644473600ULL) * 10000000ULL;
	ft->dwLowDateTime = (DWORD)v;
	ft->dwHighDateTime = (DWORD)(v >> 32);
}

static time_t FileTime_To_Time(const FILETIME * ft)
{
	unsigned long long v = ((unsigned long long)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
	return((time_t)(v / 10000000ULL - 11644473600ULL));
}

extern "C" BOOL GetFileInformationByHandle(HANDLE handle, LPBY_HANDLE_FILE_INFORMATION info)
{
	Win32File * file = As_File(handle);
	if (file == NULL || info == NULL) return(FALSE);
	fflush(file->File);
	struct stat st;
	if (fstat(fileno(file->File), &st) != 0) return(FALSE);
	memset(info, 0, sizeof(*info));
	info->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	Time_To_FileTime(st.st_mtime, &info->ftCreationTime);
	Time_To_FileTime(st.st_atime, &info->ftLastAccessTime);
	Time_To_FileTime(st.st_mtime, &info->ftLastWriteTime);
	info->nFileSizeLow = (DWORD)st.st_size;
	info->nNumberOfLinks = 1;
	return(TRUE);
}

extern "C" BOOL GetFileTime(HANDLE handle, FILETIME * created, FILETIME * accessed, FILETIME * written)
{
	BY_HANDLE_FILE_INFORMATION info;
	if (!GetFileInformationByHandle(handle, &info)) return(FALSE);
	if (created) *created = info.ftCreationTime;
	if (accessed) *accessed = info.ftLastAccessTime;
	if (written) *written = info.ftLastWriteTime;
	return(TRUE);
}

extern "C" BOOL SetFileTime(HANDLE , const FILETIME * , const FILETIME * , const FILETIME * )
{
	return(TRUE);
}

extern "C" BOOL FileTimeToDosDateTime(const FILETIME * ft, LPWORD date, LPWORD dostime)
{
	time_t t = FileTime_To_Time(ft);
	struct tm * tm = localtime(&t);
	if (tm == NULL) return(FALSE);
	if (date) *date = (WORD)(((tm->tm_year - 80) << 9) | ((tm->tm_mon + 1) << 5) | tm->tm_mday);
	if (dostime) *dostime = (WORD)((tm->tm_hour << 11) | (tm->tm_min << 5) | (tm->tm_sec / 2));
	return(TRUE);
}

extern "C" BOOL DosDateTimeToFileTime(WORD date, WORD dostime, FILETIME * ft)
{
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	tm.tm_year = ((date >> 9) & 0x7F) + 80;
	tm.tm_mon = ((date >> 5) & 0x0F) - 1;
	tm.tm_mday = date & 0x1F;
	tm.tm_hour = (dostime >> 11) & 0x1F;
	tm.tm_min = (dostime >> 5) & 0x3F;
	tm.tm_sec = (dostime & 0x1F) * 2;
	tm.tm_isdst = -1;
	Time_To_FileTime(mktime(&tm), ft);
	return(TRUE);
}

extern "C" BOOL CloseHandle(HANDLE handle)
{
	Win32File * file = As_File(handle);
	if (file == NULL) return(FALSE);
	fclose(file->File);
	delete file;
	return(TRUE);
}

extern "C" BOOL DeleteFile(LPCSTR name)
{
	std::string path = Win32_Resolve_Path(name, false);
	if (remove(path.c_str()) != 0) {
		Set_Error_From_Errno();
		return(FALSE);
	}
	return(TRUE);
}

extern "C" BOOL CreateDirectory(LPCSTR name, void * )
{
	std::string path = Win32_Resolve_Path(name, true);
#if defined(_WIN32)
	return(_mkdir(path.c_str()) == 0);
#else
	return(mkdir(path.c_str(), 0755) == 0);
#endif
}

extern "C" BOOL RemoveDirectory(LPCSTR name)
{
	std::string path = Win32_Resolve_Path(name, false);
	return(rmdir(path.c_str()) == 0);
}

extern "C" DWORD GetFileAttributes(LPCSTR name)
{
	std::string path = Win32_Resolve_Path(name, false);
	struct stat st;
	if (stat(path.c_str(), &st) != 0) return(INVALID_FILE_ATTRIBUTES);
	if (S_ISDIR(st.st_mode)) return(FILE_ATTRIBUTE_DIRECTORY);
	return(FILE_ATTRIBUTE_NORMAL);
}

extern "C" DWORD GetCurrentDirectory(DWORD size, LPSTR buffer)
{
	if (getcwd(buffer, size) == NULL) return(0);
	return((DWORD)strlen(buffer));
}

extern "C" BOOL SetCurrentDirectory(LPCSTR path)
{
	std::string p = Win32_Resolve_Path(path, false);
	return(chdir(p.c_str()) == 0);
}

extern "C" DWORD GetModuleFileName(HMODULE , LPSTR buffer, DWORD size)
{
	char * base = SDL_GetBasePath();
	std::string path = base ? base : "./";
	if (base) SDL_free(base);
	path += "redalert";
	SDL_strlcpy(buffer, path.c_str(), size);
	return((DWORD)strlen(buffer));
}

extern "C" BOOL GetDiskFreeSpace(LPCSTR , LPDWORD sectors_per_cluster, LPDWORD bytes_per_sector, LPDWORD free_clusters, LPDWORD total_clusters)
{
	/*
	**	Plenty of space (the game only checks for a few megabytes): 1 GB free of 2 GB. The
	**	game multiplies these out in 32 bits, so stay well below 4 GB.
	*/
	if (sectors_per_cluster) *sectors_per_cluster = 8;
	if (bytes_per_sector) *bytes_per_sector = 512;
	if (free_clusters) *free_clusters = 0x40000;
	if (total_clusters) *total_clusters = 0x80000;
	return(TRUE);
}

extern "C" UINT GetDriveType(LPCSTR )
{
	return(DRIVE_FIXED);
}

extern "C" DWORD GetLogicalDrives(void)
{
	return(1 << 2);		// Just drive C:
}

extern "C" BOOL GetVolumeInformation(LPCSTR , LPSTR volume, DWORD volume_size, LPDWORD serial, LPDWORD maxlen, LPDWORD flags, LPSTR fsname, DWORD fsname_size)
{
	if (volume && volume_size) volume[0] = 0;
	if (serial) *serial = 0;
	if (maxlen) *maxlen = 255;
	if (flags) *flags = 0;
	if (fsname && fsname_size) fsname[0] = 0;
	return(FALSE);
}

extern "C" void GlobalMemoryStatus(LPMEMORYSTATUS status)
{
	memset(status, 0, sizeof(*status));
	status->dwLength = sizeof(*status);
	status->dwTotalPhys = (SIZE_T_RA)SDL_GetSystemRAM() * 1024 * 1024;
	status->dwAvailPhys = status->dwTotalPhys / 2;
	status->dwTotalPageFile = status->dwTotalPhys;
	status->dwAvailPageFile = status->dwAvailPhys;
	status->dwTotalVirtual = 0x7FFFFFFF;
	status->dwAvailVirtual = 0x7FFFFFFF;
}


/***************************************************************************************
**	FindFirstFile / FindNextFile. Patterns may contain '*' and '?' in the last path
**	component only (which is all the game uses).
*/
struct Win32Find {
	std::vector<std::string> Names;
	std::vector<bool> Dirs;
	std::vector<unsigned int> Sizes;
	std::vector<time_t> Times;
	size_t Next;
};

bool Win32_Wildcard_Match(const char * pattern, const char * name)
{
	while (*pattern) {
		if (*pattern == '*') {
			pattern++;
			if (*pattern == 0) return(true);
			while (*name) {
				if (Win32_Wildcard_Match(pattern, name)) return(true);
				name++;
			}
			return(Win32_Wildcard_Match(pattern, name));
		}
		if (*name == 0) {
			/*
			**	DOS semantics: "*.*" also matches names without an extension.
			*/
			if (*pattern == '.' && pattern[1] == '*' && pattern[2] == 0) return(true);
			return(false);
		}
		if (*pattern != '?' && toupper((unsigned char)*pattern) != toupper((unsigned char)*name)) return(false);
		pattern++;
		name++;
	}
	return(*name == 0);
}

static void Fill_Find_Data(Win32Find * find, LPWIN32_FIND_DATA data)
{
	size_t i = find->Next++;
	memset(data, 0, sizeof(*data));
	SDL_strlcpy(data->cFileName, find->Names[i].c_str(), sizeof(data->cFileName));
	SDL_strlcpy(data->cAlternateFileName, find->Names[i].c_str(), sizeof(data->cAlternateFileName));
	data->dwFileAttributes = find->Dirs[i] ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	data->nFileSizeLow = find->Sizes[i];
	Time_To_FileTime(find->Times[i], &data->ftLastWriteTime);
	data->ftCreationTime = data->ftLastWriteTime;
	data->ftLastAccessTime = data->ftLastWriteTime;
}

extern "C" HANDLE FindFirstFile(LPCSTR pattern, LPWIN32_FIND_DATA data)
{
	std::string pat(pattern ? pattern : "*");
	for (size_t i = 0; i < pat.size(); i++) if (pat[i] == '\\') pat[i] = '/';
	std::string dir = ".";
	std::string mask = pat;
	size_t slash = pat.rfind('/');
	if (slash != std::string::npos) {
		dir = Win32_Resolve_Path(pat.substr(0, slash).c_str(), false);
		if (dir.empty()) dir = "/";
		mask = pat.substr(slash + 1);
	} else if (pat.size() >= 2 && pat[1] == ':') {
		mask = pat.substr(2);
	}

	Win32Find * find = new Win32Find;
	find->Next = 0;
#if !defined(_WIN32)
	DIR * d = opendir(dir.c_str());
	if (d != NULL) {
		struct dirent * ent;
		while ((ent = readdir(d)) != NULL) {
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
			if (!Win32_Wildcard_Match(mask.c_str(), ent->d_name)) continue;
			std::string full = dir + "/" + ent->d_name;
			struct stat st;
			if (stat(full.c_str(), &st) != 0) continue;
			find->Names.push_back(ent->d_name);
			find->Dirs.push_back(S_ISDIR(st.st_mode));
			find->Sizes.push_back((unsigned int)st.st_size);
			find->Times.push_back(st.st_mtime);
		}
		closedir(d);
	}
#else
	/*
	**	List the whole directory and match the names as above (the same rules as elsewhere).
	*/
	struct _finddata_t entry;
	intptr_t list = _findfirst((dir + "/*").c_str(), &entry);
	if (list != -1) {
		do {
			if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;
			if (!Win32_Wildcard_Match(mask.c_str(), entry.name)) continue;
			find->Names.push_back(entry.name);
			find->Dirs.push_back((entry.attrib & _A_SUBDIR) != 0);
			find->Sizes.push_back((unsigned int)entry.size);
			find->Times.push_back(entry.time_write);
		} while (_findnext(list, &entry) == 0);
		_findclose(list);
	}
#endif
	if (find->Names.empty()) {
		delete find;
		LastError = ERROR_FILE_NOT_FOUND;
		return(INVALID_HANDLE_VALUE);
	}
	Fill_Find_Data(find, data);
	return((HANDLE)find);
}

extern "C" BOOL FindNextFile(HANDLE handle, LPWIN32_FIND_DATA data)
{
	if (handle == NULL || handle == INVALID_HANDLE_VALUE) return(FALSE);
	Win32Find * find = (Win32Find *)handle;
	if (find->Next >= find->Names.size()) {
		LastError = ERROR_NO_MORE_FILES;
		return(FALSE);
	}
	Fill_Find_Data(find, data);
	return(TRUE);
}

extern "C" BOOL FindClose(HANDLE handle)
{
	if (handle == NULL || handle == INVALID_HANDLE_VALUE) return(FALSE);
	delete (Win32Find *)handle;
	return(TRUE);
}


/***************************************************************************************
**	Registry -- there is none.
*/
extern "C" LONG RegOpenKeyEx(HKEY , LPCSTR , DWORD , DWORD , HKEY * result)
{
	if (result) *result = NULL;
	return(ERROR_FILE_NOT_FOUND);
}

extern "C" LONG RegCreateKeyEx(HKEY , LPCSTR , DWORD , LPSTR , DWORD , DWORD , void * , HKEY * result, LPDWORD disposition)
{
	if (result) *result = NULL;
	if (disposition) *disposition = 0;
	return(ERROR_ACCESS_DENIED);
}

extern "C" LONG RegQueryValueEx(HKEY , LPCSTR , LPDWORD , LPDWORD , LPBYTE , LPDWORD )
{
	return(ERROR_FILE_NOT_FOUND);
}

extern "C" LONG RegSetValueEx(HKEY , LPCSTR , DWORD , DWORD , const BYTE * , DWORD )
{
	return(ERROR_ACCESS_DENIED);
}

extern "C" LONG RegDeleteValue(HKEY , LPCSTR )
{
	return(ERROR_FILE_NOT_FOUND);
}

extern "C" LONG RegCloseKey(HKEY )
{
	return(ERROR_SUCCESS);
}


/***************************************************************************************
**	Window & message queue emulation.
*/
#define	RA_MAIN_WINDOW		((HWND)(intptr_t)0x0001)

static WNDPROC			WindowProcedure = NULL;
static std::deque<MSG>	MessageQueue;
static SDL_mutex *		QueueLock = NULL;
static BYTE					KeyStates[256];
static int					CursorShowCount = 0;
static UINT					NextWindowMessage = 0xC000;

static void Lock_Queue(void)
{
	if (QueueLock == NULL) QueueLock = SDL_CreateMutex();
	SDL_LockMutex(QueueLock);
}

static void Unlock_Queue(void)
{
	SDL_UnlockMutex(QueueLock);
}

void Win32_Post_Input(UINT message, WPARAM wparam, LPARAM lparam)
{
	/*
	**	Track key states for GetAsyncKeyState/GetKeyState.
	*/
	switch (message) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (wparam < 256) {
				if (!(KeyStates[wparam] & 0x80)) KeyStates[wparam] ^= 0x01;	// toggle state
				KeyStates[wparam] |= 0x80;
			}
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (wparam < 256) KeyStates[wparam] &= ~0x80;
			break;
		case WM_LBUTTONDOWN:	KeyStates[VK_LBUTTON] |= 0x80; break;
		case WM_LBUTTONUP:	KeyStates[VK_LBUTTON] &= ~0x80; break;
		case WM_RBUTTONDOWN:	KeyStates[VK_RBUTTON] |= 0x80; break;
		case WM_RBUTTONUP:	KeyStates[VK_RBUTTON] &= ~0x80; break;
		case WM_MBUTTONDOWN:	KeyStates[VK_MBUTTON] |= 0x80; break;
		case WM_MBUTTONUP:	KeyStates[VK_MBUTTON] &= ~0x80; break;
		default: break;
	}
	PostMessage(RA_MAIN_WINDOW, message, wparam, lparam);
}

void Win32_Set_Key_State(int vkey, bool down)
{
	if (vkey < 0 || vkey > 255) return;
	if (down) {
		KeyStates[vkey] |= 0x80;
	} else {
		KeyStates[vkey] &= ~0x80;
	}
}

void Win32_Set_Toggle_State(int vkey, bool on)
{
	if (vkey < 0 || vkey > 255) return;
	if (on) {
		KeyStates[vkey] |= 0x01;
	} else {
		KeyStates[vkey] &= ~0x01;
	}
}

void Win32_Clear_Key_States(void)
{
	for (int i = 0; i < 256; i++) KeyStates[i] &= 0x01;
}

LRESULT Win32_Call_Window_Procedure(UINT message, WPARAM wparam, LPARAM lparam)
{
	if (WindowProcedure == NULL) return(0);
	return(WindowProcedure(RA_MAIN_WINDOW, message, wparam, lparam));
}

static void Pump_System(void)
{
	Platform_Pump_Events();
	Winsock_Poll();
}

extern "C" ATOM RegisterClass(const WNDCLASS * wndclass)
{
	if (wndclass) WindowProcedure = wndclass->lpfnWndProc;
	return(1);
}

extern "C" HWND CreateWindowEx(DWORD , LPCSTR , LPCSTR title, DWORD , int , int , int , int , HWND , HMENU , HINSTANCE , LPVOID )
{
	if (title) Video_Set_Title(title);
	Win32_Call_Window_Procedure(WM_CREATE, 0, 0);
	return(RA_MAIN_WINDOW);
}

extern "C" BOOL DestroyWindow(HWND ) {return(TRUE);}
extern "C" BOOL ShowWindow(HWND , int ) {return(TRUE);}
extern "C" BOOL UpdateWindow(HWND ) {return(TRUE);}
extern "C" HWND SetFocus(HWND window) {return(window);}
extern "C" BOOL SetForegroundWindow(HWND ) {return(TRUE);}
extern "C" HWND FindWindow(LPCSTR , LPCSTR ) {return(NULL);}
extern "C" BOOL SetWindowPos(HWND , HWND , int , int , int , int , UINT ) {return(TRUE);}
extern "C" HICON LoadIcon(HINSTANCE , LPCSTR ) {return(NULL);}
extern "C" HCURSOR LoadCursor(HINSTANCE , LPCSTR ) {return(NULL);}
extern "C" HCURSOR SetCursor(HCURSOR ) {return(NULL);}
extern "C" HANDLE GetStockObject(int ) {return(NULL);}
extern "C" BOOL EndDialog(HWND , INT_PTR ) {return(TRUE);}
extern "C" INT_PTR DialogBox(HINSTANCE , LPCSTR , HWND , DLGPROC ) {return(-1);}

extern "C" int GetSystemMetrics(int index)
{
	SDL_DisplayMode mode;
	if (SDL_GetDesktopDisplayMode(0, &mode) != 0) {
		mode.w = 1280;
		mode.h = 800;
	}
	switch (index) {
		case SM_CXSCREEN: return(mode.w);
		case SM_CYSCREEN: return(mode.h);
		default: return(0);
	}
}

extern "C" int ShowCursor(BOOL show)
{
	/*
	**	The system cursor is always hidden over the game window; the game draws its own.
	*/
	CursorShowCount += show ? 1 : -1;
	return(CursorShowCount);
}

extern "C" BOOL GetCursorPos(LPPOINT point)
{
	int x, y;
	Video_Get_Mouse(x, y);
	point->x = x;
	point->y = y;
	return(TRUE);
}

extern "C" BOOL SetCursorPos(int x, int y)
{
	Video_Set_Mouse(x, y);
	return(TRUE);
}

extern "C" BOOL ClipCursor(const RECT * rect)
{
	Video_Confine_Mouse(rect != NULL);		// The game only ever confines it to its whole screen.
	return(TRUE);
}
extern "C" BOOL ScreenToClient(HWND , LPPOINT ) {return(TRUE);}
extern "C" BOOL ClientToScreen(HWND , LPPOINT ) {return(TRUE);}

extern "C" BOOL GetClientRect(HWND , LPRECT rect)
{
	int w, h;
	Video_Get_Logical_Size(w, h);
	rect->left = 0;
	rect->top = 0;
	rect->right = w;
	rect->bottom = h;
	return(TRUE);
}

/*
**	Is this exact message waiting in the queue?
*/
bool Win32_Message_Pending(UINT message, WPARAM wparam, LPARAM lparam)
{
	bool found = false;
	Lock_Queue();
	for (size_t i = 0; i < MessageQueue.size(); i++) {
		if (MessageQueue[i].message == message && MessageQueue[i].wParam == wparam && MessageQueue[i].lParam == lparam) {
			found = true;
			break;
		}
	}
	Unlock_Queue();
	return(found);
}

extern "C" BOOL PostMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	MSG msg;
	memset(&msg, 0, sizeof(msg));
	msg.hwnd = window ? window : RA_MAIN_WINDOW;
	msg.message = message;
	msg.wParam = wparam;
	msg.lParam = lparam;
	msg.time = SDL_GetTicks();
	Lock_Queue();
	MessageQueue.push_back(msg);
	Unlock_Queue();
	return(TRUE);
}

extern "C" LRESULT SendMessage(HWND , UINT message, WPARAM wparam, LPARAM lparam)
{
	return(Win32_Call_Window_Procedure(message, wparam, lparam));
}

extern "C" void PostQuitMessage(int code)
{
	PostMessage(RA_MAIN_WINDOW, WM_QUIT, (WPARAM)code, 0);
}

static bool Pop_Message(LPMSG msg, bool remove)
{
	bool found = false;
	Lock_Queue();
	if (!MessageQueue.empty()) {
		*msg = MessageQueue.front();
		if (remove) MessageQueue.pop_front();
		found = true;
	}
	Unlock_Queue();
	return(found);
}

/*
**	The game waits by polling for messages in a loop (the frame rate limiter, every dialog),
**	which kept a processor core fully busy. When polls keep coming back empty in quick
**	succession, the caller is only waiting: give the processor away for a moment. Loops that
**	do real work between polls are not slowed down.
*/
static void Idle_Throttle(void)
{
	static Uint64 window_start = 0;
	static int empty_polls = 0;

	Uint64 now = SDL_GetPerformanceCounter();
	if (now - window_start > SDL_GetPerformanceFrequency() / 1000) {
		window_start = now;
		empty_polls = 0;
	}
	if (++empty_polls >= 20) {
		SDL_Delay(1);
		window_start = SDL_GetPerformanceCounter();
		empty_polls = 0;
	}
}

extern "C" BOOL PeekMessage(LPMSG msg, HWND , UINT , UINT , UINT remove)
{
	if (Pop_Message(msg, (remove & PM_REMOVE) != 0)) return(TRUE);
	Pump_System();
	if (Pop_Message(msg, (remove & PM_REMOVE) != 0)) return(TRUE);
	Idle_Throttle();
	return(FALSE);
}

extern "C" BOOL GetMessage(LPMSG msg, HWND , UINT , UINT )
{
	while (!Pop_Message(msg, true)) {
		Pump_System();
		if (Pop_Message(msg, true)) break;
		SDL_Delay(1);
	}
	return(msg->message != WM_QUIT);
}

extern "C" BOOL TranslateMessage(const MSG * )
{
	return(TRUE);
}

extern "C" LRESULT DispatchMessage(const MSG * msg)
{
	return(Win32_Call_Window_Procedure(msg->message, msg->wParam, msg->lParam));
}

extern "C" LRESULT DefWindowProc(HWND , UINT message, WPARAM , LPARAM )
{
	if (message == WM_CLOSE) {
		PostQuitMessage(0);
	}
	return(0);
}

extern "C" UINT RegisterWindowMessage(LPCSTR )
{
	return(NextWindowMessage++);
}

extern "C" SHORT GetAsyncKeyState(int vkey)
{
	if (vkey < 0 || vkey > 255) return(0);
	return((KeyStates[vkey] & 0x80) ? (SHORT)0x8000 : 0);
}

extern "C" SHORT GetKeyState(int vkey)
{
	if (vkey < 0 || vkey > 255) return(0);
	SHORT result = 0;
	if (KeyStates[vkey] & 0x80) result |= (SHORT)0x8000;
	if (KeyStates[vkey] & 0x01) result |= 0x0001;
	return(result);
}

extern "C" BOOL GetKeyboardState(BYTE * state)
{
	memcpy(state, KeyStates, sizeof(KeyStates));
	return(TRUE);
}

/*
**	US keyboard layout translation of a virtual key into ASCII.
*/
extern "C" int ToAscii(UINT vkey, UINT , const BYTE * keystate, WORD * buffer, UINT )
{
	const BYTE * ks = keystate ? keystate : KeyStates;
	bool shift = (ks[VK_SHIFT] & 0x80) || (ks[VK_LSHIFT] & 0x80) || (ks[VK_RSHIFT] & 0x80);
	bool caps = (ks[VK_CAPITAL] & 0x01) != 0;
	bool ctrl = (ks[VK_CONTROL] & 0x80) != 0;
	int c = 0;

	if (vkey >= 'A' && vkey <= 'Z') {
		bool upper = shift ^ caps;
		c = upper ? (int)vkey : (int)(vkey - 'A' + 'a');
		if (ctrl) c = vkey - 'A' + 1;
	} else if (vkey >= '0' && vkey <= '9') {
		static const char shifted[] = ")!@#$%^&*(";
		c = shift ? shifted[vkey - '0'] : (int)vkey;
	} else if (vkey >= VK_NUMPAD0 && vkey <= VK_NUMPAD9) {
		c = '0' + (vkey - VK_NUMPAD0);
	} else {
		switch (vkey) {
			case VK_SPACE:			c = ' '; break;
			case VK_RETURN:		c = '\r'; break;
			case VK_BACK:			c = '\b'; break;
			case VK_TAB:			c = '\t'; break;
			case VK_ESCAPE:		c = 27; break;
			case VK_MULTIPLY:		c = '*'; break;
			case VK_ADD:			c = '+'; break;
			case VK_SUBTRACT:		c = '-'; break;
			case VK_DECIMAL:		c = '.'; break;
			case VK_DIVIDE:		c = '/'; break;
			case VK_OEM_1:			c = shift ? ':' : ';'; break;
			case VK_OEM_PLUS:		c = shift ? '+' : '='; break;
			case VK_OEM_COMMA:	c = shift ? '<' : ','; break;
			case VK_OEM_MINUS:	c = shift ? '_' : '-'; break;
			case VK_OEM_PERIOD:	c = shift ? '>' : '.'; break;
			case VK_OEM_2:			c = shift ? '?' : '/'; break;
			case VK_OEM_3:			c = shift ? '~' : '`'; break;
			case VK_OEM_4:			c = shift ? '{' : '['; break;
			case VK_OEM_5:			c = shift ? '|' : '\\'; break;
			case VK_OEM_6:			c = shift ? '}' : ']'; break;
			case VK_OEM_7:			c = shift ? '"' : '\''; break;
			default: break;
		}
	}
	if (c == 0) return(0);
	if (buffer) *buffer = (WORD)c;
	return(1);
}

extern "C" UINT MapVirtualKey(UINT code, UINT )
{
	return(code);
}

extern "C" void ExitProcess(UINT code)
{
	exit((int)code);
}


/***************************************************************************************
**	Miscellaneous services.
*/
extern "C" int MessageBox(HWND , LPCSTR text, LPCSTR caption, UINT type)
{
	fprintf(stderr, "%s: %s\n", caption ? caption : "Red Alert", text ? text : "");

	Uint32 flags = SDL_MESSAGEBOX_INFORMATION;
	switch (type & 0xF0) {
		case MB_ICONHAND:			flags = SDL_MESSAGEBOX_ERROR; break;
		case MB_ICONEXCLAMATION:	flags = SDL_MESSAGEBOX_WARNING; break;
		default: break;
	}

	SDL_MessageBoxButtonData buttons[3];
	int count = 0;
	switch (type & 0x0F) {
		case MB_YESNO:
			buttons[count++] = {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES, "Yes"};
			buttons[count++] = {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDNO, "No"};
			break;
		case MB_YESNOCANCEL:
			buttons[count++] = {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES, "Yes"};
			buttons[count++] = {0, IDNO, "No"};
			buttons[count++] = {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel"};
			break;
		case MB_OKCANCEL:
			buttons[count++] = {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDOK, "OK"};
			buttons[count++] = {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel"};
			break;
		case MB_RETRYCANCEL:
			buttons[count++] = {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDRETRY, "Retry"};
			buttons[count++] = {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel"};
			break;
		default:
			buttons[count++] = {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDOK, "OK"};
			break;
	}

	SDL_MessageBoxData data;
	memset(&data, 0, sizeof(data));
	data.flags = flags;
	data.window = (SDL_Window *)Video_Get_SDL_Window();
	data.title = caption ? caption : "Red Alert";
	data.message = text ? text : "";
	data.numbuttons = count;
	data.buttons = buttons;

	/*
	**	Without a display (e.g. automated runs) just take the default answer.
	*/
	int pressed = buttons[0].buttonid;
	if (getenv("RA_NO_MESSAGEBOX") == NULL) {
		Video_Set_Relative_Mouse(false);
		bool released = Video_Release_Mouse();
		if (SDL_ShowMessageBox(&data, &pressed) != 0) {
			pressed = buttons[0].buttonid;
		}
		if (released) Video_Restore_Mouse();
	}
	return(pressed);
}

extern "C" void OutputDebugString(LPCSTR text)
{
	static int enabled = -1;
	if (enabled == -1) enabled = (getenv("RA_DEBUG") != NULL) ? 1 : 0;
	if (enabled && text) fputs(text, stderr);
}

extern "C" void Sleep(DWORD milliseconds)
{
	SDL_Delay(milliseconds);
}

extern "C" DWORD GetTickCount(void)
{
	return((DWORD)SDL_GetTicks());
}

extern "C" DWORD timeGetTime(void)
{
	return((DWORD)SDL_GetTicks());
}

static void Fill_System_Time(struct tm * tm, LPSYSTEMTIME st, int ms)
{
	st->wYear = (WORD)(tm->tm_year + 1900);
	st->wMonth = (WORD)(tm->tm_mon + 1);
	st->wDayOfWeek = (WORD)tm->tm_wday;
	st->wDay = (WORD)tm->tm_mday;
	st->wHour = (WORD)tm->tm_hour;
	st->wMinute = (WORD)tm->tm_min;
	st->wSecond = (WORD)tm->tm_sec;
	st->wMilliseconds = (WORD)ms;
}

extern "C" void GetLocalTime(LPSYSTEMTIME st)
{
	time_t now = time(NULL);
	Fill_System_Time(localtime(&now), st, 0);
}

extern "C" void GetSystemTime(LPSYSTEMTIME st)
{
	time_t now = time(NULL);
	Fill_System_Time(gmtime(&now), st, 0);
}

/*
**	Critical sections.
*/
extern "C" void InitializeCriticalSection(LPCRITICAL_SECTION section)
{
	section->Mutex = SDL_CreateMutex();
}

extern "C" void DeleteCriticalSection(LPCRITICAL_SECTION section)
{
	if (section->Mutex) SDL_DestroyMutex((SDL_mutex *)section->Mutex);
	section->Mutex = NULL;
}

extern "C" void EnterCriticalSection(LPCRITICAL_SECTION section)
{
	if (section->Mutex == NULL) section->Mutex = SDL_CreateMutex();
	SDL_LockMutex((SDL_mutex *)section->Mutex);
}

extern "C" void LeaveCriticalSection(LPCRITICAL_SECTION section)
{
	if (section->Mutex) SDL_UnlockMutex((SDL_mutex *)section->Mutex);
}

/*
**	Multimedia timers.
*/
struct MMTimer {
	SDL_TimerID		ID;
	LPTIMECALLBACK	Callback;
	DWORD				User;
	UINT				Delay;
	bool				Periodic;
	SDL_atomic_t	Running;		// The callback is running (on SDL's timer thread).
	SDL_atomic_t	Killed;		// timeKillEvent() was called: don't call back any more.
	SDL_threadID	Thread;		// Thread the callback runs on.
};
static std::vector<MMTimer *> Timers;
static SDL_mutex * TimerLock = NULL;

static Uint32 SDLCALL MM_Timer_Thunk(Uint32 interval, void * param)
{
	MMTimer * timer = (MMTimer *)param;
	SDL_AtomicSet(&timer->Running, 1);
	if (SDL_AtomicGet(&timer->Killed)) {
		SDL_AtomicSet(&timer->Running, 0);
		return(0);
	}
	timer->Thread = SDL_ThreadID();
	timer->Callback((UINT)timer->ID, 0, timer->User, 0, 0);
	SDL_AtomicSet(&timer->Running, 0);
	return(timer->Periodic ? interval : 0);
}

extern "C" UINT timeSetEvent(UINT delay, UINT , LPTIMECALLBACK callback, DWORD user, UINT flags)
{
	if (!SDL_WasInit(SDL_INIT_TIMER)) SDL_InitSubSystem(SDL_INIT_TIMER);
	if (TimerLock == NULL) TimerLock = SDL_CreateMutex();
	MMTimer * timer = new MMTimer;
	timer->Callback = callback;
	timer->User = user;
	timer->Delay = delay ? delay : 1;
	timer->Periodic = (flags & TIME_PERIODIC) != 0;
	SDL_AtomicSet(&timer->Running, 0);
	SDL_AtomicSet(&timer->Killed, 0);
	timer->Thread = 0;
	timer->ID = SDL_AddTimer(timer->Delay, MM_Timer_Thunk, timer);
	if (timer->ID == 0) {
		delete timer;
		return(0);
	}
	SDL_LockMutex(TimerLock);
	Timers.push_back(timer);
	SDL_UnlockMutex(TimerLock);
	return((UINT)timer->ID);
}

extern "C" MMRESULT timeKillEvent(UINT id)
{
	if (TimerLock == NULL) return(TIMERR_NOCANDO);
	SDL_LockMutex(TimerLock);
	for (size_t i = 0; i < Timers.size(); i++) {
		if ((UINT)Timers[i]->ID == id) {
			MMTimer * timer = Timers[i];
			SDL_AtomicSet(&timer->Killed, 1);
			SDL_RemoveTimer(timer->ID);
			Timers.erase(Timers.begin() + i);
			SDL_UnlockMutex(TimerLock);

			/*
			**	A callback that is running right now finishes before this returns, so that the
			**	caller can free what the callback uses (the sound system does). Unless this is
			**	the callback itself. The (tiny) timer record is not freed: SDL may still look at
			**	it once more.
			*/
			if (timer->Thread != SDL_ThreadID()) {
				while (SDL_AtomicGet(&timer->Running)) {
					SDL_Delay(1);
				}
			}
			return(TIMERR_NOERROR);
		}
	}
	SDL_UnlockMutex(TimerLock);
	return(TIMERR_NOCANDO);
}

extern "C" MMRESULT timeBeginPeriod(UINT ) {return(TIMERR_NOERROR);}
extern "C" MMRESULT timeEndPeriod(UINT ) {return(TIMERR_NOERROR);}

extern "C" MMRESULT timeGetDevCaps(LPTIMECAPS caps, UINT )
{
	if (caps) {
		caps->wPeriodMin = 1;
		caps->wPeriodMax = 1000000;
	}
	return(TIMERR_NOERROR);
}
