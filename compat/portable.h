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
**	portable.h -- force-included into every translation unit of the modern port.
**
**	The original code was written for Watcom C/C++ 10.6 (32-bit DOS/Win95). This header
**	papers over the compiler/runtime specific bits so the code builds with modern
**	GCC/Clang/MSVC on 64-bit hosts:
**
**	  - Watcom/MSVC calling convention and memory model keywords are neutralised.
**	  - "bool/true/false" are real keywords now, so the code's own definitions are disabled.
**	  - Non-standard C runtime helpers (stricmp, strupr, _splitpath, itoa ...) are provided.
**
**	Everything here is deliberately small and dependency free; OS specific services live in
**	the SDL2 platform layer (see WIN32LIB/SDL).
*/
#ifndef RA_PORTABLE_H
#define RA_PORTABLE_H

/*
**	The legacy BSD functions index()/rindex() (declared by <string.h> on POSIX systems)
**	collide with the code's many "index" variables that rely on the pre-standard rule that
**	a variable declared in a for() statement remains visible after the loop. Declare the
**	library functions under a different name so that "index" stays a plain identifier.
**	The same applies to the Bessel functions y0/y1/j0/j1 in <math.h>.
*/
#define index	__ra_posix_index
#define rindex	__ra_posix_rindex
#define y0		__ra_posix_y0
#define y1		__ra_posix_y1
#define yn		__ra_posix_yn
#define j0		__ra_posix_j0
#define j1		__ra_posix_j1
#define jn		__ra_posix_jn
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32)
#include <strings.h>
#endif
#include <math.h>
#undef index
#undef rindex
#undef y0
#undef y1
#undef yn
#undef j0
#undef j1
#undef jn
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#ifdef __cplusplus
#include <new>
#include <type_traits>
#include <algorithm>
#include <vector>
#include <string>
#include <chrono>
#endif

/*
**	The old code defines its own bool type unless this is set.
*/
#define TRUE_FALSE_DEFINED

/*
**	Compiler specific keywords.
*/
#if !defined(_MSC_VER)
#  if !defined(_WIN32)
#    define __cdecl
#    define _cdecl
#    define __stdcall
#    define _stdcall
#    define __fastcall
#    define _fastcall
#    define __declspec(x)
#  endif
#  define __pascal
#  define _pascal
#endif
#define cdecl
#define pascal
#define __watcall
#define __syscall
#define __far
#define _far
#define far
#define __near
#define _near
#define near
#define __huge
#define _huge
#define huge
#define __loadds
#define _loadds
#define __saveregs
#define __export
#define _export
#ifndef __int64
#  if !defined(_MSC_VER)
#    define __int64 long long
#  endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define RA_UNUSED __attribute__((unused))
#else
#  define RA_UNUSED
#endif

/*
**	Byte order helpers. The data files are little endian.
*/
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#  define RA_BIG_ENDIAN 1
#endif

/*
**	Non standard runtime library functions used throughout the code.
*/
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WIN32)
int stricmp(const char *a, const char *b);
int strnicmp(const char *a, const char *b, size_t n);
int memicmp(const void *a, const void *b, size_t n);
char *strupr(char *s);
char *strlwr(char *s);
char *strrev(char *s);
char *itoa(int value, char *buffer, int radix);
char *ltoa(int value, char *buffer, int radix);
char *ultoa(unsigned int value, char *buffer, int radix);
#define _stricmp	stricmp
#define strcmpi	stricmp
#define _strcmpi	stricmp
#define _strnicmp	strnicmp
#define strncmpi	strnicmp
#define _memicmp	memicmp
#define _strupr	strupr
#define _strlwr	strlwr
#define _strrev	strrev
#define _itoa		itoa
#define _ltoa		ltoa
#define _ultoa		ultoa
#endif

#ifndef _MAX_PATH
#define _MAX_PATH	260
#endif
#ifndef _MAX_DRIVE
#define _MAX_DRIVE	3
#endif
#ifndef _MAX_DIR
#define _MAX_DIR	256
#endif
#ifndef _MAX_FNAME
#define _MAX_FNAME	256
#endif
#ifndef _MAX_EXT
#define _MAX_EXT	256
#endif
#ifndef PATH_MAX
#define PATH_MAX	4096
#endif

#if !defined(_WIN32)
void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext);
void _makepath(char *path, const char *drive, const char *dir, const char *fname, const char *ext);
#endif

/*
**	Rotate helpers (intrinsics on Watcom/MSVC).
*/
#if !defined(_MSC_VER)
static inline unsigned int _rotl(unsigned int value, int shift) { shift &= 31; return shift ? (value << shift) | (value >> (32 - shift)) : value; }
static inline unsigned int _rotr(unsigned int value, int shift) { shift &= 31; return shift ? (value >> shift) | (value << (32 - shift)) : value; }
static inline unsigned int _lrotl(unsigned int value, int shift) { shift &= 31; return shift ? (value << shift) | (value >> (32 - shift)) : value; }
static inline unsigned int _lrotr(unsigned int value, int shift) { shift &= 31; return shift ? (value >> shift) | (value << (32 - shift)) : value; }
#endif

#ifdef __cplusplus
}
#endif

/*
**	The Win32 build got min()/max() as macros from <windows.h>. Provide function templates
**	with the same "usual arithmetic conversion" result type, without polluting the macro
**	namespace (which would break modern C++ headers).
*/
#ifdef __cplusplus
#include <type_traits>
#undef min
#undef max
template<class A, class B> static inline typename std::common_type<A, B>::type min(A a, B b) { return (a < b) ? a : b; }
template<class A, class B> static inline typename std::common_type<A, B>::type max(A a, B b) { return (a > b) ? a : b; }
#endif

/*
**	Structure packing. The original was compiled with Watcom's byte packing (/zp1, which
**	was also the Watcom default), and plenty of structures are overlaid directly on file
**	data (shape, MIX, AUD, WSA, VQA, font... headers) or rely on that layout in other ways.
**	Everything of the game and its libraries is therefore byte packed too. All system
**	headers have been included above with their natural layout, and the Win32 stand-in
**	headers in compat/ switch back to the natural layout themselves.
*/
#if defined(RA_PACK_STRUCTS)
#pragma pack(push, 1)
#endif

#endif /* RA_PORTABLE_H */
