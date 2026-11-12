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
**	radl.c -- a plugin of the Windows installer (NSIS, amd64-unicode): downloads a file, shows
**	the progress in the installer's window and checks the file's SHA-256 digest.
**
**	    radl::download URL FILE SIZE SHA256 WHAT
**
**	URL     where to download from (redirects are followed)
**	FILE    where to save the file; a partial download already there is resumed
**	SIZE    the file's size in bytes
**	SHA256  the file's SHA-256 digest (hexadecimal)
**	WHAT    what is being downloaded, for the progress text ("the Allied disc")
**
**	Pushes "OK", or a message that says what went wrong. A complete file already there with
**	the right digest isn't downloaded again; one with another digest is deleted.
**
**	It uses WinHTTP, with the system's proxy settings and certificates (or the proxy that the
**	HTTPS_PROXY environment variable names). Build: see make-installer.sh.
*/

#define __USE_MINGW_ANSI_STDIO 1		/* C99 printf: %llu, %ls */
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <commctrl.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
#define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY	4
#endif

/*
**	The NSIS plugin interface (NSIS's Contrib/ExDLL/pluginapi.h): parameters and results are
**	strings on a stack.
*/
typedef struct stack_t {
	struct stack_t * next;
	WCHAR text[1];
} stack_t;

static int StringSize;
static stack_t ** StackTop;

static void Pop_String(WCHAR * text)
{
	text[0] = 0;
	if (StackTop == NULL || *StackTop == NULL) return;
	stack_t * top = *StackTop;
	lstrcpynW(text, top->text, StringSize);
	*StackTop = top->next;
	GlobalFree((HGLOBAL)top);
}

static void Push_String(const WCHAR * text)
{
	if (StackTop == NULL) return;
	stack_t * entry = (stack_t *)GlobalAlloc(GPTR, sizeof(stack_t) + StringSize * sizeof(WCHAR));
	if (entry == NULL) return;
	lstrcpynW(entry->text, text, StringSize);
	entry->next = *StackTop;
	*StackTop = entry;
}


/*
**	Progress, in the installer's progress bar and status line (none in a silent install).
*/
static HWND ProgressBar;
static HWND StatusText;
static ULONGLONG LastShown;

static void Find_Progress_Controls(HWND installer)
{
	ProgressBar = NULL;
	StatusText = NULL;
	if (installer == NULL) return;
	HWND page = FindWindowExW(installer, NULL, L"#32770", NULL);
	if (page == NULL) return;
	ProgressBar = GetDlgItem(page, 1004);
	StatusText = GetDlgItem(page, 1006);
}

static void Show_Status(const WCHAR * text)
{
	if (StatusText) SetWindowTextW(StatusText, text);
}

static void Show_Progress(const WCHAR * verb, const WCHAR * what, ULONGLONG done, ULONGLONG total, int always)
{
	ULONGLONG now = GetTickCount64();
	if (!always && now - LastShown < 200) return;
	LastShown = now;
	if (ProgressBar) {
		SendMessageW(ProgressBar, PBM_SETRANGE32, 0, 1000);
		SendMessageW(ProgressBar, PBM_SETPOS, total ? (WPARAM)(done * 1000 / total) : 0, 0);
	}
	WCHAR text[300];
	swprintf(text, 300, L"%ls %ls: %llu of %llu MB", verb, what, done >> 20, total >> 20);
	Show_Status(text);
}


/*
**	SHA-256 (Windows' BCrypt).
*/
typedef struct {
	BCRYPT_ALG_HANDLE Algorithm;
	BCRYPT_HASH_HANDLE Hash;
} Digest;

static int Digest_Start(Digest * digest)
{
	digest->Algorithm = NULL;
	digest->Hash = NULL;
	if (BCryptOpenAlgorithmProvider(&digest->Algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) != 0) return(0);
	return(BCryptCreateHash(digest->Algorithm, &digest->Hash, NULL, 0, NULL, 0, 0) == 0);
}

static void Digest_Add(Digest * digest, const void * data, DWORD size)
{
	BCryptHashData(digest->Hash, (PUCHAR)data, size, 0);
}

static void Digest_End(Digest * digest, WCHAR * hex)
{
	UCHAR value[32];
	hex[0] = 0;
	if (digest->Hash) {
		if (BCryptFinishHash(digest->Hash, value, sizeof(value), 0) == 0) {
			for (int i = 0; i < 32; i++) swprintf(hex + i * 2, 3, L"%02x", value[i]);
		}
		BCryptDestroyHash(digest->Hash);
	}
	if (digest->Algorithm) BCryptCloseAlgorithmProvider(digest->Algorithm, 0);
	digest->Hash = NULL;
	digest->Algorithm = NULL;
}


/*
**	The download.
*/
#define BUFFER_SIZE		(1 << 20)
#define ATTEMPTS			5

typedef struct {
	const WCHAR * Url;
	const WCHAR * What;
	ULONGLONG Size;
	HANDLE File;
	ULONGLONG Have;			// Bytes in the file so far.
	Digest Hash;				// Of those bytes.
	unsigned char * Buffer;
	WCHAR Error[512];
} Download;

static void Start_Over(Download * d)
{
	LARGE_INTEGER zero;
	zero.QuadPart = 0;
	SetFilePointerEx(d->File, zero, NULL, FILE_BEGIN);
	SetEndOfFile(d->File);
	d->Have = 0;
	WCHAR ignored[65];
	Digest_End(&d->Hash, ignored);
	Digest_Start(&d->Hash);
}

/*
**	Reads what an earlier attempt left in the file into the digest.
*/
static int Hash_Existing(Download * d)
{
	LARGE_INTEGER size;
	if (!GetFileSizeEx(d->File, &size)) return(0);
	if ((ULONGLONG)size.QuadPart > d->Size) {
		Start_Over(d);
		return(1);
	}
	ULONGLONG left = (ULONGLONG)size.QuadPart;
	d->Have = 0;
	while (left > 0) {
		DWORD chunk = (left > BUFFER_SIZE) ? BUFFER_SIZE : (DWORD)left;
		DWORD got = 0;
		if (!ReadFile(d->File, d->Buffer, chunk, &got, NULL) || got == 0) return(0);
		Digest_Add(&d->Hash, d->Buffer, got);
		d->Have += got;
		left -= got;
		Show_Progress(L"Checking the earlier download of", d->What, d->Have, d->Size, 0);
	}
	return(1);
}

static HINTERNET Open_Session(void)
{
	const WCHAR * agent = L"RedAlertSetup/1.0";
	const char * env = getenv("HTTPS_PROXY");
	if (env == NULL || *env == 0) env = getenv("https_proxy");
	if (env != NULL && *env) {
		/*
		**	WinHTTP wants "host:port", without the scheme or a trailing slash.
		*/
		WCHAR proxy[512];
		const char * start = strstr(env, "://");
		MultiByteToWideChar(CP_UTF8, 0, start ? start + 3 : env, -1, proxy, 512);
		size_t length = wcslen(proxy);
		if (length > 0 && proxy[length - 1] == L'/') proxy[length - 1] = 0;
		HINTERNET session = WinHttpOpen(agent, WINHTTP_ACCESS_TYPE_NAMED_PROXY, proxy, L"<local>;localhost;127.0.0.1", 0);
		if (session) return(session);
	}
	HINTERNET session = WinHttpOpen(agent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (session == NULL) {	// Before Windows 8.1.
		session = WinHttpOpen(agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	}
	return(session);
}

/*
**	One attempt: requests what is still missing and appends it to the file. Returns whether the
**	file is complete; d->Error says why not.
*/
static int Attempt(Download * d)
{
	int complete = 0;
	HINTERNET session = NULL, connection = NULL, request = NULL;

	URL_COMPONENTS url;
	WCHAR host[256], path[2048], extra[1024];
	ZeroMemory(&url, sizeof(url));
	url.dwStructSize = sizeof(url);
	url.lpszHostName = host;
	url.dwHostNameLength = 256;
	url.lpszUrlPath = path;
	url.dwUrlPathLength = 2048;
	url.lpszExtraInfo = extra;
	url.dwExtraInfoLength = 1024;
	if (!WinHttpCrackUrl(d->Url, 0, 0, &url)) {
		swprintf(d->Error, 512, L"The address %ls is not valid.", d->Url);
		return(0);
	}
	wcsncat(path, extra, 2047 - wcslen(path));

	session = Open_Session();
	if (session == NULL) {
		swprintf(d->Error, 512, L"Windows' HTTP service could not be started (error %lu).", GetLastError());
		goto done;
	}
	WinHttpSetTimeouts(session, 30000, 30000, 30000, 60000);
	connection = WinHttpConnect(session, host, url.nPort, 0);
	request = connection ? WinHttpOpenRequest(connection, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
		(url.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0) : NULL;
	if (request == NULL) {
		swprintf(d->Error, 512, L"Could not connect to %ls (error %lu).", host, GetLastError());
		goto done;
	}

	WCHAR range[64] = L"";
	if (d->Have > 0) swprintf(range, 64, L"Range: bytes=%llu-", d->Have);
	if (!WinHttpSendRequest(request, range[0] ? range : WINHTTP_NO_ADDITIONAL_HEADERS, range[0] ? (DWORD)-1L : 0,
			WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request, NULL)) {
		swprintf(d->Error, 512, L"No answer from %ls (error %lu). Is the computer connected to the Internet?", host, GetLastError());
		goto done;
	}

	DWORD status = 0, length = sizeof(status);
	WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
		&status, &length, WINHTTP_NO_HEADER_INDEX);
	if (status == 200 && d->Have > 0) {
		Start_Over(d);				// The server sends the whole file after all.
	} else if (status != 200 && status != 206) {
		swprintf(d->Error, 512, L"The server answered %lu.", status);
		goto done;
	}

	for (;;) {
		DWORD got = 0;
		if (!WinHttpReadData(request, d->Buffer, BUFFER_SIZE, &got)) {
			swprintf(d->Error, 512, L"The connection was lost (error %lu).", GetLastError());
			break;
		}
		if (got == 0) break;
		if (d->Have + got > d->Size) {
			swprintf(d->Error, 512, L"The file is larger than expected.");
			Start_Over(d);
			break;
		}
		DWORD written = 0;
		if (!WriteFile(d->File, d->Buffer, got, &written, NULL) || written != got) {
			swprintf(d->Error, 512, L"Could not write the file (error %lu): is the disk full?", GetLastError());
			break;
		}
		Digest_Add(&d->Hash, d->Buffer, got);
		d->Have += got;
		Show_Progress(L"Downloading", d->What, d->Have, d->Size, 0);
	}
	complete = (d->Have == d->Size);
	if (!complete && d->Error[0] == 0) {
		swprintf(d->Error, 512, L"The download stopped after %llu of %llu MB.", d->Have >> 20, d->Size >> 20);
	}

done:
	if (request) WinHttpCloseHandle(request);
	if (connection) WinHttpCloseHandle(connection);
	if (session) WinHttpCloseHandle(session);
	return(complete);
}

static void Run_Download(Download * d, const WCHAR * path, const WCHAR * sha256, WCHAR * result)
{
	lstrcpyW(result, L"OK");
	d->Error[0] = 0;
	d->Buffer = (unsigned char *)malloc(BUFFER_SIZE);
	d->File = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (d->Buffer == NULL || d->File == INVALID_HANDLE_VALUE || !Digest_Start(&d->Hash)) {
		swprintf(result, 512, L"Could not create %ls (error %lu).", path, GetLastError());
		goto done;
	}
	if (!Hash_Existing(d)) {
		Start_Over(d);
	}
	if (d->Have == d->Size) {
		WCHAR hex[65];
		Digest_End(&d->Hash, hex);
		if (_wcsicmp(hex, sha256) == 0) goto done;		// Downloaded already.
		Start_Over(d);
	}

	for (int attempt = 0; attempt < ATTEMPTS && d->Have < d->Size; attempt++) {
		if (attempt > 0) {
			Show_Status(L"Trying again...");
			Sleep(3000);
		}
		d->Error[0] = 0;
		Attempt(d);
	}
	Show_Progress(L"Downloading", d->What, d->Have, d->Size, 1);
	if (d->Have < d->Size) {
		lstrcpynW(result, d->Error, 512);
		goto done;
	}

	WCHAR hex[65];
	Digest_End(&d->Hash, hex);
	if (_wcsicmp(hex, sha256) != 0) {
		CloseHandle(d->File);
		d->File = INVALID_HANDLE_VALUE;
		DeleteFileW(path);
		swprintf(result, 512, L"The downloaded file is not the expected one (its SHA-256 digest is %ls), so it was deleted.", hex);
	}

done:
	if (d->File != INVALID_HANDLE_VALUE) CloseHandle(d->File);
	WCHAR ignored[65];
	Digest_End(&d->Hash, ignored);
	free(d->Buffer);
}

__declspec(dllexport) void download(HWND installer, int string_size, WCHAR * variables, stack_t ** stack_top, void * extra)
{
	(void)variables;
	(void)extra;
	StringSize = string_size;
	StackTop = stack_top;

	WCHAR * url = (WCHAR *)malloc(string_size * sizeof(WCHAR));
	WCHAR * path = (WCHAR *)malloc(string_size * sizeof(WCHAR));
	WCHAR * size = (WCHAR *)malloc(string_size * sizeof(WCHAR));
	WCHAR * sha256 = (WCHAR *)malloc(string_size * sizeof(WCHAR));
	WCHAR * what = (WCHAR *)malloc(string_size * sizeof(WCHAR));
	WCHAR result[512];
	if (url == NULL || path == NULL || size == NULL || sha256 == NULL || what == NULL) {
		lstrcpyW(result, L"Out of memory.");
	} else {
		Pop_String(url);
		Pop_String(path);
		Pop_String(size);
		Pop_String(sha256);
		Pop_String(what);

		Find_Progress_Controls(installer);
		LastShown = 0;
		Download d;
		ZeroMemory(&d, sizeof(d));
		d.Url = url;
		d.What = what;
		d.Size = wcstoull(size, NULL, 10);
		Run_Download(&d, path, sha256, result);
	}
	Push_String(result);
	free(url);
	free(path);
	free(size);
	free(sha256);
	free(what);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	(void)instance;
	(void)reason;
	(void)reserved;
	return(TRUE);
}
