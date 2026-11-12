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
**	Minimal stand-in for <windows.h>.
**
**	The game was written against the Win32 API. The port replaces every OS service with
**	SDL2, but the code still uses the Win32 type names (BOOL, DWORD, HWND ...), the
**	virtual key codes (VK_xxx) and a handful of trivial helpers. This header provides
**	exactly that, with fixed-size types so that the layout is identical on 32 and 64 bit
**	hosts. It is used on every platform -- the real <windows.h> is never included by game
**	code.
*/
#ifndef RA_COMPAT_WINDOWS_H
#define RA_COMPAT_WINDOWS_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*
**	On Windows the program is also linked with the real system libraries, and the C/C++
**	runtime uses some of the functions emulated here (Sleep, critical sections...). There
**	the emulated functions get link names of their own, so that the two can't clash. These
**	are the emulated functions whose names the system libraries export as they are (most
**	Win32 functions are exported with an A or W suffix, CreateFileA for CreateFile...).
*/
#if defined(_WIN32)
#define CloseHandle						RA_CloseHandle
#define DeleteCriticalSection			RA_DeleteCriticalSection
#define DosDateTimeToFileTime			RA_DosDateTimeToFileTime
#define EnterCriticalSection			RA_EnterCriticalSection
#define ExitProcess						RA_ExitProcess
#define FileTimeToDosDateTime			RA_FileTimeToDosDateTime
#define FindClose							RA_FindClose
#define FlushFileBuffers				RA_FlushFileBuffers
#define GetFileInformationByHandle	RA_GetFileInformationByHandle
#define GetFileSize						RA_GetFileSize
#define GetFileTime						RA_GetFileTime
#define GetLastError						RA_GetLastError
#define GetLocalTime						RA_GetLocalTime
#define GetLogicalDrives				RA_GetLogicalDrives
#define GetSystemTime					RA_GetSystemTime
#define GetTickCount						RA_GetTickCount
#define GlobalMemoryStatus				RA_GlobalMemoryStatus
#define InitializeCriticalSection	RA_InitializeCriticalSection
#define LeaveCriticalSection			RA_LeaveCriticalSection
#define ReadFile							RA_ReadFile
#define SetFilePointer					RA_SetFilePointer
#define SetFileTime						RA_SetFileTime
#define Sleep								RA_Sleep
#define WriteFile							RA_WriteFile
#define ClientToScreen					RA_ClientToScreen
#define ClipCursor						RA_ClipCursor
#define DestroyWindow					RA_DestroyWindow
#define EndDialog							RA_EndDialog
#define GetAsyncKeyState				RA_GetAsyncKeyState
#define GetClientRect					RA_GetClientRect
#define GetCursorPos						RA_GetCursorPos
#define GetKeyState						RA_GetKeyState
#define GetKeyboardState				RA_GetKeyboardState
#define GetSystemMetrics				RA_GetSystemMetrics
#define PostQuitMessage					RA_PostQuitMessage
#define ScreenToClient					RA_ScreenToClient
#define SetCursor							RA_SetCursor
#define SetCursorPos						RA_SetCursorPos
#define SetFocus							RA_SetFocus
#define SetForegroundWindow			RA_SetForegroundWindow
#define SetWindowPos						RA_SetWindowPos
#define ShowCursor						RA_ShowCursor
#define ShowWindow						RA_ShowWindow
#define ToAscii							RA_ToAscii
#define TranslateMessage				RA_TranslateMessage
#define UpdateWindow						RA_UpdateWindow
#define GetStockObject					RA_GetStockObject
#define RegCloseKey						RA_RegCloseKey
#define timeBeginPeriod					RA_timeBeginPeriod
#define timeEndPeriod					RA_timeEndPeriod
#define timeGetDevCaps					RA_timeGetDevCaps
#define timeGetTime						RA_timeGetTime
#define timeKillEvent					RA_timeKillEvent
#define timeSetEvent						RA_timeSetEvent
#endif

/*
**	Basic types.
*/
typedef int32_t				BOOL;
typedef uint8_t				BYTE;
typedef uint16_t				WORD;
typedef uint32_t				DWORD;
typedef uint32_t				UINT;
typedef int32_t				INT;
typedef int32_t				LONG;
typedef uint32_t				ULONG;
typedef int16_t				SHORT;
typedef uint16_t				USHORT;
typedef char					CHAR;
typedef unsigned char		UCHAR;
typedef float					FLOAT;
typedef int64_t				LONGLONG;
typedef uint64_t				ULONGLONG;
typedef intptr_t				INT_PTR;
typedef uintptr_t				UINT_PTR;
typedef intptr_t				LONG_PTR;
typedef uintptr_t				ULONG_PTR;
typedef uintptr_t				DWORD_PTR;
typedef size_t					SIZE_T_RA;
typedef size_t					SIZE_T;
typedef void					VOID;
typedef void *					PVOID;
typedef void *					LPVOID;
typedef const void *			LPCVOID;
typedef char *					LPSTR;
typedef const char *			LPCSTR;
typedef char *					PSTR;
typedef const char *			PCSTR;
typedef char					TCHAR;
typedef char *					LPTSTR;
typedef const char *			LPCTSTR;
typedef BYTE *					LPBYTE;
typedef BYTE *					PBYTE;
typedef WORD *					LPWORD;
typedef DWORD *				LPDWORD;
typedef LONG *					LPLONG;
typedef BOOL *					LPBOOL;
typedef INT *					LPINT;
typedef UINT_PTR				WPARAM;
typedef LONG_PTR				LPARAM;
typedef LONG_PTR				LRESULT;
typedef int32_t				HRESULT;
typedef WORD					ATOM;
typedef DWORD					COLORREF;

typedef void *					HANDLE;
typedef HANDLE					HWND;
typedef HANDLE					HINSTANCE;
typedef HANDLE					HMODULE;
typedef HANDLE					HDC;
typedef HANDLE					HGLOBAL;
typedef HANDLE					HLOCAL;
typedef HANDLE					HKEY;
typedef HANDLE					HMENU;
typedef HANDLE					HICON;
typedef HANDLE					HCURSOR;
typedef HANDLE					HBRUSH;
typedef HANDLE					HPALETTE;
typedef HANDLE					HBITMAP;
typedef HANDLE					HFONT;
typedef HANDLE					HGDIOBJ;
typedef HANDLE					HRGN;
typedef HANDLE					HPEN;
typedef HANDLE					HTASK;
typedef HANDLE					HFILE;
typedef HANDLE					HMMIO;
typedef HANDLE					HCONV;
typedef HANDLE					HSZ;
typedef HANDLE					HDDEDATA;
typedef HANDLE					*LPHANDLE;
typedef int (*FARPROC)(void);
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif
#ifndef NULL
#ifdef __cplusplus
#define NULL	0
#else
#define NULL	((void *)0)
#endif
#endif

#define CONST			const
#define FAR
#define NEAR
#define PASCAL
#define WINAPI
#define APIENTRY
#define CALLBACK
#define STDMETHODCALLTYPE
#define EXPORT
#define IN
#define OUT
#define OPTIONAL

#ifndef MAX_PATH
#define MAX_PATH		260
#endif

#define LOBYTE(w)		((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)		((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define LOWORD(l)		((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)		((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define MAKEWORD(a, b)	((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)	((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define MAKELPARAM(l, h)	((LPARAM)(DWORD)MAKELONG(l, h))
#define RGB(r,g,b)		((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#define SUCCEEDED(hr)	(((HRESULT)(hr)) >= 0)
#define FAILED(hr)		(((HRESULT)(hr)) < 0)
#define S_OK				((HRESULT)0)
#define S_FALSE			((HRESULT)1)
#define E_FAIL				((HRESULT)0x80004005)
#define E_OUTOFMEMORY	((HRESULT)0x8007000E)
#define E_INVALIDARG		((HRESULT)0x80070057)
#define ERROR_SUCCESS	0

#define INVALID_HANDLE_VALUE	((HANDLE)(LONG_PTR)-1)
#define INFINITE				0xFFFFFFFF

typedef struct tagRECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT, *PRECT, *LPRECT;

typedef struct tagPOINT {
	LONG x;
	LONG y;
} POINT, *PPOINT, *LPPOINT;

typedef struct tagSIZE {
	LONG cx;
	LONG cy;
} SIZE, *PSIZE, *LPSIZE;

typedef struct tagMSG {
	HWND		hwnd;
	UINT		message;
	WPARAM	wParam;
	LPARAM	lParam;
	DWORD		time;
	POINT		pt;
} MSG, *LPMSG;

typedef struct tagPALETTEENTRY {
	BYTE peRed;
	BYTE peGreen;
	BYTE peBlue;
	BYTE peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD;

#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER {
	WORD	bfType;
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER {
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD				bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;

#define BI_RGB	0

typedef union _LARGE_INTEGER {
	struct {
		DWORD LowPart;
		LONG HighPart;
	} u;
	LONGLONG QuadPart;
} LARGE_INTEGER;

typedef struct _FILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME;

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

typedef struct _GUID {
	DWORD	Data1;
	WORD	Data2;
	WORD	Data3;
	BYTE	Data4[8];
} GUID;
typedef GUID IID;
typedef GUID CLSID;

/*
**	Message box flags (only used to pick an icon in the SDL message box).
*/
#define MB_OK						0x00000000
#define MB_OKCANCEL				0x00000001
#define MB_ABORTRETRYIGNORE	0x00000002
#define MB_YESNOCANCEL			0x00000003
#define MB_YESNO					0x00000004
#define MB_RETRYCANCEL			0x00000005
#define MB_ICONHAND				0x00000010
#define MB_ICONQUESTION			0x00000020
#define MB_ICONEXCLAMATION		0x00000030
#define MB_ICONASTERISK			0x00000040
#define MB_ICONSTOP				MB_ICONHAND
#define MB_ICONERROR				MB_ICONHAND
#define MB_ICONWARNING			MB_ICONEXCLAMATION
#define MB_ICONINFORMATION		MB_ICONASTERISK
#define MB_SETFOREGROUND		0x00010000
#define MB_TOPMOST				0x00040000
#define MB_SYSTEMMODAL			0x00001000
#define MB_TASKMODAL				0x00002000
#define MB_APPLMODAL				0x00000000

#define IDOK			1
#define IDCANCEL		2
#define IDABORT		3
#define IDRETRY		4
#define IDIGNORE		5
#define IDYES			6
#define IDNO			7

/*
**	Show window codes.
*/
#define SW_HIDE				0
#define SW_SHOWNORMAL		1
#define SW_NORMAL				1
#define SW_SHOWMINIMIZED	2
#define SW_SHOWMAXIMIZED	3
#define SW_MAXIMIZE			3
#define SW_SHOWNOACTIVATE	4
#define SW_SHOW				5
#define SW_MINIMIZE			6
#define SW_SHOWMINNOACTIVE	7
#define SW_SHOWNA				8
#define SW_RESTORE			9

/*
**	Window messages (the keyboard class still speaks this protocol internally).
*/
#define WM_NULL				0x0000
#define WM_CREATE				0x0001
#define WM_DESTROY			0x0002
#define WM_MOVE				0x0003
#define WM_SIZE				0x0005
#define WM_ACTIVATE			0x0006
#define WM_SETFOCUS			0x0007
#define WM_KILLFOCUS			0x0008
#define WM_PAINT				0x000F
#define WM_CLOSE				0x0010
#define WM_QUIT				0x0012
#define WM_ACTIVATEAPP		0x001C
#define WM_SETCURSOR			0x0020
#define WM_KEYDOWN			0x0100
#define WM_KEYUP				0x0101
#define WM_CHAR				0x0102
#define WM_SYSKEYDOWN		0x0104
#define WM_SYSKEYUP			0x0105
#define WM_SYSCHAR			0x0106
#define WM_COMMAND			0x0111
#define WM_SYSCOMMAND		0x0112
#define WM_TIMER				0x0113
#define WM_MOUSEMOVE			0x0200
#define WM_LBUTTONDOWN		0x0201
#define WM_LBUTTONUP			0x0202
#define WM_LBUTTONDBLCLK	0x0203
#define WM_RBUTTONDOWN		0x0204
#define WM_RBUTTONUP			0x0205
#define WM_RBUTTONDBLCLK	0x0206
#define WM_MBUTTONDOWN		0x0207
#define WM_MBUTTONUP			0x0208
#define WM_MBUTTONDBLCLK	0x0209
#define WM_USER				0x0400

#define WA_INACTIVE			0
#define WA_ACTIVE				1
#define WA_CLICKACTIVE		2

/*
**	Virtual key codes.
*/
#define VK_LBUTTON		0x01
#define VK_RBUTTON		0x02
#define VK_CANCEL			0x03
#define VK_MBUTTON		0x04
#define VK_BACK			0x08
#define VK_TAB				0x09
#define VK_CLEAR			0x0C
#define VK_RETURN			0x0D
#define VK_SHIFT			0x10
#define VK_CONTROL		0x11
#define VK_MENU			0x12
#define VK_PAUSE			0x13
#define VK_CAPITAL		0x14
#define VK_ESCAPE			0x1B
#define VK_SPACE			0x20
#define VK_PRIOR			0x21
#define VK_NEXT			0x22
#define VK_END				0x23
#define VK_HOME			0x24
#define VK_LEFT			0x25
#define VK_UP				0x26
#define VK_RIGHT			0x27
#define VK_DOWN			0x28
#define VK_SELECT			0x29
#define VK_PRINT			0x2A
#define VK_EXECUTE		0x2B
#define VK_SNAPSHOT		0x2C
#define VK_INSERT			0x2D
#define VK_DELETE			0x2E
#define VK_HELP			0x2F
#define VK_LWIN			0x5B
#define VK_RWIN			0x5C
#define VK_APPS			0x5D
#define VK_NUMPAD0		0x60
#define VK_NUMPAD1		0x61
#define VK_NUMPAD2		0x62
#define VK_NUMPAD3		0x63
#define VK_NUMPAD4		0x64
#define VK_NUMPAD5		0x65
#define VK_NUMPAD6		0x66
#define VK_NUMPAD7		0x67
#define VK_NUMPAD8		0x68
#define VK_NUMPAD9		0x69
#define VK_MULTIPLY		0x6A
#define VK_ADD				0x6B
#define VK_SEPARATOR		0x6C
#define VK_SUBTRACT		0x6D
#define VK_DECIMAL		0x6E
#define VK_DIVIDE			0x6F
#define VK_F1				0x70
#define VK_F2				0x71
#define VK_F3				0x72
#define VK_F4				0x73
#define VK_F5				0x74
#define VK_F6				0x75
#define VK_F7				0x76
#define VK_F8				0x77
#define VK_F9				0x78
#define VK_F10				0x79
#define VK_F11				0x7A
#define VK_F12				0x7B
#define VK_F13				0x7C
#define VK_F14				0x7D
#define VK_F15				0x7E
#define VK_F16				0x7F
#define VK_F17				0x80
#define VK_F18				0x81
#define VK_F19				0x82
#define VK_F20				0x83
#define VK_F21				0x84
#define VK_F22				0x85
#define VK_F23				0x86
#define VK_F24				0x87
#define VK_NUMLOCK		0x90
#define VK_SCROLL			0x91
#define VK_LSHIFT			0xA0
#define VK_RSHIFT			0xA1
#define VK_LCONTROL		0xA2
#define VK_RCONTROL		0xA3
#define VK_LMENU			0xA4
#define VK_RMENU			0xA5
#define VK_OEM_1			0xBA	// ';:'
#define VK_OEM_PLUS		0xBB	// '=+'
#define VK_OEM_COMMA		0xBC	// ',<'
#define VK_OEM_MINUS		0xBD	// '-_'
#define VK_OEM_PERIOD	0xBE	// '.>'
#define VK_OEM_2			0xBF	// '/?'
#define VK_OEM_3			0xC0	// '`~'
#define VK_OEM_4			0xDB	// '[{'
#define VK_OEM_5			0xDC	// '\|'
#define VK_OEM_6			0xDD	// ']}'
#define VK_OEM_7			0xDE	// ''"'
#define VK_NONAME			0xFC

/*
**	Critical sections (implemented with SDL mutexes, which are recursive like Win32's).
*/
typedef struct _CRITICAL_SECTION {
	void * Mutex;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

#ifdef __cplusplus
extern "C" {
#endif
void		InitializeCriticalSection(LPCRITICAL_SECTION section);
void		DeleteCriticalSection(LPCRITICAL_SECTION section);
void		EnterCriticalSection(LPCRITICAL_SECTION section);
void		LeaveCriticalSection(LPCRITICAL_SECTION section);
#ifdef __cplusplus
}
#endif

/*
**	A few trivial Win32 services implemented by the SDL platform layer (compat/compat.cpp,
**	WIN32LIB/SDL). Anything needing real OS support goes through SDL.
*/
#ifdef __cplusplus
extern "C" {
#endif

int		MessageBox(HWND window, LPCSTR text, LPCSTR caption, UINT type);
#define	MessageBoxA	MessageBox
void		OutputDebugString(LPCSTR text);
#define	OutputDebugStringA	OutputDebugString
void		Sleep(DWORD milliseconds);
DWORD		GetTickCount(void);
DWORD		timeGetTime(void);
DWORD		GetCurrentDirectory(DWORD size, LPSTR buffer);
BOOL		SetCurrentDirectory(LPCSTR path);
DWORD		GetModuleFileName(HMODULE module, LPSTR buffer, DWORD size);
int		ShowCursor(BOOL show);
DWORD		GetLastError(void);
void		GetLocalTime(LPSYSTEMTIME time);
void		GetSystemTime(LPSYSTEMTIME time);
BOOL		DeleteFile(LPCSTR name);
BOOL		CreateDirectory(LPCSTR name, void *security);

#ifdef __cplusplus
}
#endif

/*
**	Windows, messages and input. There is a single (SDL) window. Its window procedure is
**	the one registered with RegisterClass(); SDL events are translated into the matching
**	WM_xxx messages and delivered through the usual PeekMessage/DispatchMessage loop.
*/
#define CS_VREDRAW			0x0001
#define CS_HREDRAW			0x0002
#define CS_DBLCLKS			0x0008
#define WS_OVERLAPPED		0x00000000
#define WS_POPUP				0x80000000
#define WS_VISIBLE			0x10000000
#define WS_CAPTION			0x00C00000
#define WS_SYSMENU			0x00080000
#define WS_EX_TOPMOST		0x00000008
#define WS_EX_APPWINDOW		0x00040000
#define PM_NOREMOVE			0x0000
#define PM_REMOVE				0x0001
#define PM_NOYIELD			0x0002
#define SC_SCREENSAVE		0xF140
#define SC_CLOSE				0xF060
#define SC_KEYMENU			0xF100
#define SC_MINIMIZE			0xF020
#define SM_CXSCREEN			0
#define SM_CYSCREEN			1
#define IDI_APPLICATION		((LPCSTR)32512)
#define IDC_ARROW				((LPCSTR)32512)
#define MAKEINTRESOURCE(i)	((LPSTR)((ULONG_PTR)((WORD)(i))))
#define COLOR_WINDOW			5
#define BLACK_BRUSH			4
#define SWP_NOSIZE			0x0001
#define SWP_NOMOVE			0x0002
#define SWP_NOZORDER			0x0004
#define HWND_TOP				((HWND)0)
#define HWND_TOPMOST			((HWND)-1)
#define GWL_STYLE				(-16)
#define GWL_EXSTYLE			(-20)

typedef BOOL (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct tagWNDCLASS {
	UINT			style;
	LRESULT		(*lpfnWndProc)(HWND, UINT, WPARAM, LPARAM);
	int			cbClsExtra;
	int			cbWndExtra;
	HINSTANCE	hInstance;
	HICON			hIcon;
	HCURSOR		hCursor;
	HBRUSH		hbrBackground;
	LPCSTR		lpszMenuName;
	LPCSTR		lpszClassName;
} WNDCLASS, *LPWNDCLASS;

/*
**	File system.
*/
#define GENERIC_READ					0x80000000
#define GENERIC_WRITE				0x40000000
#define FILE_SHARE_READ				0x00000001
#define FILE_SHARE_WRITE			0x00000002
#define CREATE_NEW					1
#define CREATE_ALWAYS				2
#define OPEN_EXISTING				3
#define OPEN_ALWAYS					4
#define TRUNCATE_EXISTING			5
#define FILE_ATTRIBUTE_READONLY	0x00000001
#define FILE_ATTRIBUTE_HIDDEN		0x00000002
#define FILE_ATTRIBUTE_SYSTEM		0x00000004
#define FILE_ATTRIBUTE_DIRECTORY	0x00000010
#define FILE_ATTRIBUTE_ARCHIVE	0x00000020
#define FILE_ATTRIBUTE_NORMAL		0x00000080
#define FILE_ATTRIBUTE_TEMPORARY	0x00000100
#define FILE_FLAG_SEQUENTIAL_SCAN	0x08000000
#define FILE_FLAG_RANDOM_ACCESS	0x10000000
#define FILE_BEGIN					0
#define FILE_CURRENT					1
#define FILE_END						2
#define INVALID_FILE_SIZE			0xFFFFFFFF
#define INVALID_SET_FILE_POINTER	0xFFFFFFFF
#define INVALID_FILE_ATTRIBUTES	0xFFFFFFFF
#define ERROR_FILE_NOT_FOUND		2
#define ERROR_PATH_NOT_FOUND		3
#define ERROR_ACCESS_DENIED		5
#define ERROR_INVALID_HANDLE		6
#define ERROR_NOT_ENOUGH_MEMORY	8
#define ERROR_HANDLE_EOF			38
#define ERROR_NO_MORE_FILES		18
#define ERROR_WRITE_PROTECT		19
#define ERROR_NOT_READY				21
#define ERROR_SHARING_VIOLATION	32
#define ERROR_DISK_FULL				112
#define DRIVE_UNKNOWN				0
#define DRIVE_NO_ROOT_DIR			1
#define DRIVE_REMOVABLE				2
#define DRIVE_FIXED					3
#define DRIVE_REMOTE					4
#define DRIVE_CDROM					5
#define DRIVE_RAMDISK				6

typedef struct _WIN32_FIND_DATA {
	DWORD		dwFileAttributes;
	FILETIME	ftCreationTime;
	FILETIME	ftLastAccessTime;
	FILETIME	ftLastWriteTime;
	DWORD		nFileSizeHigh;
	DWORD		nFileSizeLow;
	DWORD		dwReserved0;
	DWORD		dwReserved1;
	CHAR		cFileName[MAX_PATH];
	CHAR		cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;

typedef struct _BY_HANDLE_FILE_INFORMATION {
	DWORD		dwFileAttributes;
	FILETIME	ftCreationTime;
	FILETIME	ftLastAccessTime;
	FILETIME	ftLastWriteTime;
	DWORD		dwVolumeSerialNumber;
	DWORD		nFileSizeHigh;
	DWORD		nFileSizeLow;
	DWORD		nNumberOfLinks;
	DWORD		nFileIndexHigh;
	DWORD		nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

typedef struct _MEMORYSTATUS {
	DWORD		dwLength;
	DWORD		dwMemoryLoad;
	SIZE_T_RA	dwTotalPhys;
	SIZE_T_RA	dwAvailPhys;
	SIZE_T_RA	dwTotalPageFile;
	SIZE_T_RA	dwAvailPageFile;
	SIZE_T_RA	dwTotalVirtual;
	SIZE_T_RA	dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

typedef struct _OVERLAPPED {
	ULONG_PTR	Internal;
	ULONG_PTR	InternalHigh;
	DWORD			Offset;
	DWORD			OffsetHigh;
	HANDLE		hEvent;
} OVERLAPPED, *LPOVERLAPPED;

/*
**	Registry (there is none: every query fails, which the game treats as "not installed").
*/
#define HKEY_CLASSES_ROOT		((HKEY)(ULONG_PTR)0x80000000)
#define HKEY_CURRENT_USER		((HKEY)(ULONG_PTR)0x80000001)
#define HKEY_LOCAL_MACHINE		((HKEY)(ULONG_PTR)0x80000002)
#define KEY_READ					0x20019
#define KEY_WRITE					0x20006
#define KEY_ALL_ACCESS			0xF003F
#define REG_SZ						1
#define REG_DWORD					4
#define REG_OPTION_NON_VOLATILE	0

#ifdef __cplusplus
extern "C" {
#endif

/* windows & messages */
ATOM		RegisterClass(const WNDCLASS * wndclass);
HWND		CreateWindowEx(DWORD exstyle, LPCSTR classname, LPCSTR title, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param);
BOOL		DestroyWindow(HWND window);
BOOL		ShowWindow(HWND window, int command);
BOOL		UpdateWindow(HWND window);
HWND		SetFocus(HWND window);
BOOL		SetForegroundWindow(HWND window);
HWND		FindWindow(LPCSTR classname, LPCSTR title);
BOOL		SetWindowPos(HWND window, HWND after, int x, int y, int w, int h, UINT flags);
int		GetSystemMetrics(int index);
HICON		LoadIcon(HINSTANCE instance, LPCSTR name);
HCURSOR	LoadCursor(HINSTANCE instance, LPCSTR name);
HCURSOR	SetCursor(HCURSOR cursor);
HANDLE	GetStockObject(int object);
BOOL		GetCursorPos(LPPOINT point);
BOOL		SetCursorPos(int x, int y);
BOOL		ClipCursor(const RECT * rect);
BOOL		ScreenToClient(HWND window, LPPOINT point);
BOOL		ClientToScreen(HWND window, LPPOINT point);
BOOL		GetClientRect(HWND window, LPRECT rect);
BOOL		PeekMessage(LPMSG msg, HWND window, UINT first, UINT last, UINT remove);
BOOL		GetMessage(LPMSG msg, HWND window, UINT first, UINT last);
BOOL		TranslateMessage(const MSG * msg);
LRESULT	DispatchMessage(const MSG * msg);
BOOL		PostMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
LRESULT	SendMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
void		PostQuitMessage(int code);
LRESULT	DefWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
UINT		RegisterWindowMessage(LPCSTR name);
INT_PTR	DialogBox(HINSTANCE instance, LPCSTR name, HWND parent, DLGPROC proc);
BOOL		EndDialog(HWND dialog, INT_PTR result);
SHORT		GetAsyncKeyState(int vkey);
SHORT		GetKeyState(int vkey);
BOOL		GetKeyboardState(BYTE * state);
int		ToAscii(UINT vkey, UINT scancode, const BYTE * keystate, WORD * buffer, UINT flags);
UINT		MapVirtualKey(UINT code, UINT maptype);
void		ExitProcess(UINT code);

/* files */
HANDLE	CreateFile(LPCSTR name, DWORD access, DWORD share, void * security, DWORD creation, DWORD flags, HANDLE templ);
BOOL		ReadFile(HANDLE file, LPVOID buffer, DWORD bytes, LPDWORD read, LPOVERLAPPED overlapped);
BOOL		WriteFile(HANDLE file, LPCVOID buffer, DWORD bytes, LPDWORD written, LPOVERLAPPED overlapped);
DWORD		SetFilePointer(HANDLE file, LONG distance, LONG * high, DWORD method);
DWORD		GetFileSize(HANDLE file, LPDWORD high);
BOOL		FlushFileBuffers(HANDLE file);
BOOL		CloseHandle(HANDLE handle);
BOOL		GetFileInformationByHandle(HANDLE file, LPBY_HANDLE_FILE_INFORMATION info);
BOOL		GetFileTime(HANDLE file, FILETIME * created, FILETIME * accessed, FILETIME * written);
BOOL		SetFileTime(HANDLE file, const FILETIME * created, const FILETIME * accessed, const FILETIME * written);
BOOL		FileTimeToDosDateTime(const FILETIME * time, LPWORD date, LPWORD dostime);
BOOL		DosDateTimeToFileTime(WORD date, WORD dostime, FILETIME * time);
DWORD		GetFileAttributes(LPCSTR name);
HANDLE	FindFirstFile(LPCSTR pattern, LPWIN32_FIND_DATA data);
BOOL		FindNextFile(HANDLE find, LPWIN32_FIND_DATA data);
BOOL		FindClose(HANDLE find);
BOOL		RemoveDirectory(LPCSTR name);
BOOL		GetDiskFreeSpace(LPCSTR root, LPDWORD sectors_per_cluster, LPDWORD bytes_per_sector, LPDWORD free_clusters, LPDWORD total_clusters);
UINT		GetDriveType(LPCSTR root);
DWORD		GetLogicalDrives(void);
BOOL		GetVolumeInformation(LPCSTR root, LPSTR volume, DWORD volume_size, LPDWORD serial, LPDWORD maxlen, LPDWORD flags, LPSTR fsname, DWORD fsname_size);
void		GlobalMemoryStatus(LPMEMORYSTATUS status);

/* registry */
LONG		RegOpenKeyEx(HKEY key, LPCSTR subkey, DWORD options, DWORD access, HKEY * result);
LONG		RegCreateKeyEx(HKEY key, LPCSTR subkey, DWORD reserved, LPSTR cls, DWORD options, DWORD access, void * security, HKEY * result, LPDWORD disposition);
LONG		RegQueryValueEx(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size);
LONG		RegSetValueEx(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE * data, DWORD size);
LONG		RegDeleteValue(HKEY key, LPCSTR name);
LONG		RegCloseKey(HKEY key);

#ifdef __cplusplus
}
#endif

/*
**	Threads and processes: only used to raise the priority of the timer/sound threads.
*/
#define THREAD_ALL_ACCESS					0x1F03FF
#define DUPLICATE_SAME_ACCESS				0x00000002
#define THREAD_PRIORITY_NORMAL			0
#define THREAD_PRIORITY_ABOVE_NORMAL	1
#define THREAD_PRIORITY_HIGHEST			2
#define THREAD_PRIORITY_TIME_CRITICAL	15
#define SEM_FAILCRITICALERRORS			0x0001
#define SEM_NOOPENFILEERRORBOX			0x8000

static inline HANDLE GetCurrentProcess(void) {return((HANDLE)(intptr_t)-1);}
static inline HANDLE GetCurrentThread(void) {return((HANDLE)(intptr_t)-2);}
static inline BOOL DuplicateHandle(HANDLE , HANDLE source, HANDLE , LPHANDLE target, DWORD , BOOL , DWORD ) {if (target) *target = source; return(TRUE);}
static inline BOOL SetThreadPriority(HANDLE , int ) {return(TRUE);}
static inline int GetThreadPriority(HANDLE ) {return(THREAD_PRIORITY_NORMAL);}
static inline UINT SetErrorMode(UINT ) {return(0);}
static inline DWORD GetVersion(void) {return(0x0000000A);}		// "Windows NT 10"
#define NORMAL_PRIORITY_CLASS		0x00000020
#define HIGH_PRIORITY_CLASS			0x00000080
#define REALTIME_PRIORITY_CLASS	0x00000100
static inline DWORD GetPriorityClass(HANDLE ) {return(NORMAL_PRIORITY_CLASS);}
static inline BOOL SetPriorityClass(HANDLE , DWORD ) {return(TRUE);}

/*
**	Serial ports (the modem/null-modem game). There are no serial ports: opening "COMx"
**	fails, so these are never reached with a valid handle.
*/
#define SETDTR		5
#define CLRDTR		6
#define SETRTS		3
#define CLRRTS		4
static inline BOOL SetCommBreak(HANDLE ) {return(FALSE);}
static inline BOOL ClearCommBreak(HANDLE ) {return(FALSE);}
static inline BOOL EscapeCommFunction(HANDLE , DWORD ) {return(FALSE);}

#define ZeroMemory(d, n)		memset((d), 0, (n))
#define FillMemory(d, n, c)	memset((d), (c), (n))
#define CopyMemory(d, s, n)	memcpy((d), (s), (n))
#define MoveMemory(d, s, n)	memmove((d), (s), (n))
#define lstrcpy					strcpy
#define lstrcat					strcat
#define lstrlen					strlen
#define lstrcmp					strcmp
#define lstrcmpi					stricmp
#define wsprintf					sprintf

#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif /* RA_COMPAT_WINDOWS_H */
