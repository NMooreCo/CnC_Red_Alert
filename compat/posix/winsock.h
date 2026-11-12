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
**	winsock.h -- Winsock 1.1 on top of BSD sockets.
**
**	The network code (CODE/WSPUDP.CPP etc.) uses non-blocking sockets driven by
**	WSAAsyncSelect() window messages. The port emulates that by polling registered sockets
**	from the main event pump and handing the resulting messages to the game's window
**	procedure (see compat/compat.cpp).
*/
#ifndef RA_COMPAT_WINSOCK_H
#define RA_COMPAT_WINSOCK_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()

#include "windows.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

typedef int						SOCKET;
#define INVALID_SOCKET			(-1)
#define SOCKET_ERROR				(-1)

typedef struct in_addr			IN_ADDR, *PIN_ADDR, *LPIN_ADDR;
typedef struct sockaddr		SOCKADDR, *PSOCKADDR, *LPSOCKADDR;
typedef struct sockaddr_in	SOCKADDR_IN, *PSOCKADDR_IN, *LPSOCKADDR_IN;
typedef struct hostent			HOSTENT, *PHOSTENT, *LPHOSTENT;
typedef struct linger			LINGER, *PLINGER, *LPLINGER;

#ifndef INADDR_NONE
#define INADDR_NONE				0xffffffff
#endif

#define MAXGETHOSTSTRUCT		1024

#define WSADESCRIPTION_LEN		256
#define WSASYS_STATUS_LEN		128

typedef struct WSAData {
	WORD		wVersion;
	WORD		wHighVersion;
	char		szDescription[WSADESCRIPTION_LEN+1];
	char		szSystemStatus[WSASYS_STATUS_LEN+1];
	unsigned short	iMaxSockets;
	unsigned short	iMaxUdpDg;
	char *	lpVendorInfo;
} WSADATA, *LPWSADATA;

/*
**	WSAAsyncSelect event flags.
*/
#define FD_READ				0x01
#define FD_WRITE				0x02
#define FD_OOB					0x04
#define FD_ACCEPT				0x08
#define FD_CONNECT			0x10
#define FD_CLOSE				0x20

#define WSAMAKEASYNCREPLY(buflen,error)	MAKELONG(buflen,error)
#define WSAMAKESELECTREPLY(event,error)	MAKELONG(event,error)
#define WSAGETASYNCBUFLEN(lParam)			LOWORD(lParam)
#define WSAGETASYNCERROR(lParam)				HIWORD(lParam)
#define WSAGETSELECTEVENT(lParam)			LOWORD(lParam)
#define WSAGETSELECTERROR(lParam)			HIWORD(lParam)

#define WSABASEERR			10000
#define WSAEWOULDBLOCK		EWOULDBLOCK
#define WSAEINPROGRESS		EINPROGRESS
#define WSAECONNRESET		ECONNRESET
#define WSAENOTSOCK			ENOTSOCK
#define WSAEADDRINUSE		EADDRINUSE
#define WSAECONNREFUSED		ECONNREFUSED
#define WSAEMSGSIZE			EMSGSIZE
#define WSAENOBUFS			ENOBUFS

/*
**	Novell IPX is no longer supported by modern operating systems; creating such a socket
**	simply fails and the game falls back to UDP.
*/
#ifndef AF_IPX
#define AF_IPX					4
#endif
#ifndef AF_NS
#define AF_NS					AF_IPX
#endif

#ifdef __cplusplus
extern "C" {
#endif
int		WSAStartup(WORD version, LPWSADATA data);
int		WSACleanup(void);
int		WSAGetLastError(void);
void		WSASetLastError(int error);
int		closesocket(SOCKET s);
int		ioctlsocket(SOCKET s, int command, unsigned int * argp);
int		WSAAsyncSelect(SOCKET s, HWND window, UINT message, int events);
HANDLE	WSAAsyncGetHostByName(HWND window, UINT message, const char * name, char * buffer, int buflen);
HANDLE	WSAAsyncGetHostByAddr(HWND window, UINT message, const char * addr, int len, int type, char * buffer, int buflen);
int		WSACancelAsyncRequest(HANDLE request);
#ifdef __cplusplus
}
#endif

#ifndef FIONBIO
#include <sys/ioctl.h>
#endif

#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
