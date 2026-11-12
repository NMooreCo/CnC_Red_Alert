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
**	winsock.h -- Winsock 1.1, as the game uses it.
**
**	On Windows the sockets are the real Winsock's (ws2_32.dll). Its own headers can't be
**	used next to the stand-in <windows.h>, so what the game needs of it is declared here,
**	with the Windows layouts and values. Elsewhere, Winsock is emulated on BSD sockets.
**
**	The network code (CODE/WSPROTO.CPP, WSPUDP.CPP...) uses non-blocking sockets driven by
**	WSAAsyncSelect() window messages. The game's window is the port's own, so that part is
**	emulated everywhere: registered sockets are polled from the main event pump and the
**	messages handed to the game's window procedure (see compat/winsock.cpp).
*/
#ifndef RA_COMPAT_WINSOCK_H
#define RA_COMPAT_WINSOCK_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()

#include "windows.h"

#if defined(_WIN32)

#include <time.h>

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int			u_int;
typedef unsigned long		u_long;			/* 32 bits on Windows */
typedef UINT_PTR				SOCKET;

#define INVALID_SOCKET			((SOCKET)(~0))
#define SOCKET_ERROR				(-1)

struct in_addr {
	union {
		struct {u_char s_b1, s_b2, s_b3, s_b4;} S_un_b;
		struct {u_short s_w1, s_w2;} S_un_w;
		u_long S_addr;
	} S_un;
};
#define s_addr		S_un.S_addr

struct sockaddr {
	u_short	sa_family;
	char		sa_data[14];
};

struct sockaddr_in {
	short				sin_family;
	u_short			sin_port;
	struct in_addr	sin_addr;
	char				sin_zero[8];
};

struct hostent {
	char *	h_name;
	char **	h_aliases;
	short		h_addrtype;
	short		h_length;
	char **	h_addr_list;
};
#define h_addr		h_addr_list[0]

struct linger {
	u_short	l_onoff;
	u_short	l_linger;
};

#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED
struct timeval {
	long		tv_sec;
	long		tv_usec;
};
#endif

#ifndef FD_SETSIZE
#define FD_SETSIZE				64
#endif
typedef struct fd_set {
	u_int		fd_count;
	SOCKET	fd_array[FD_SETSIZE];
} fd_set;

#define WSADESCRIPTION_LEN		256
#define WSASYS_STATUS_LEN		128

typedef struct WSAData {
	WORD		wVersion;
	WORD		wHighVersion;
#if defined(_WIN64)
	unsigned short	iMaxSockets;
	unsigned short	iMaxUdpDg;
	char *	lpVendorInfo;
	char		szDescription[WSADESCRIPTION_LEN+1];
	char		szSystemStatus[WSASYS_STATUS_LEN+1];
#else
	char		szDescription[WSADESCRIPTION_LEN+1];
	char		szSystemStatus[WSASYS_STATUS_LEN+1];
	unsigned short	iMaxSockets;
	unsigned short	iMaxUdpDg;
	char *	lpVendorInfo;
#endif
} WSADATA, *LPWSADATA;

#define AF_UNSPEC				0
#define AF_INET				2
#define AF_IPX					6
#define AF_NS					AF_IPX
#define PF_UNSPEC				AF_UNSPEC
#define PF_INET				AF_INET
#define PF_IPX					AF_IPX
#define PF_NS					AF_NS

#define SOCK_STREAM			1
#define SOCK_DGRAM			2
#define SOCK_RAW				3

#define IPPROTO_IP			0
#define IPPROTO_TCP			6
#define IPPROTO_UDP			17

#define SOL_SOCKET			0xffff
#define SO_DEBUG				0x0001
#define SO_ACCEPTCONN		0x0002
#define SO_REUSEADDR			0x0004
#define SO_KEEPALIVE			0x0008
#define SO_DONTROUTE			0x0010
#define SO_BROADCAST			0x0020
#define SO_LINGER				0x0080
#define SO_OOBINLINE			0x0100
#define SO_DONTLINGER		((int)(~SO_LINGER))
#define SO_SNDBUF				0x1001
#define SO_RCVBUF				0x1002
#define SO_SNDTIMEO			0x1005
#define SO_RCVTIMEO			0x1006
#define SO_ERROR				0x1007
#define SO_TYPE				0x1008
#define TCP_NODELAY			0x0001

#define MSG_OOB				0x1
#define MSG_PEEK				0x2
#define MSG_DONTROUTE		0x4

#define SOMAXCONN				5

#define FIONREAD				((long)0x4004667FL)
#define FIONBIO				((long)0x8004667EUL)

#define INADDR_ANY			((u_long)0x00000000)
#define INADDR_LOOPBACK		0x7f000001
#define INADDR_BROADCAST	((u_long)0xffffffff)
#define INADDR_NONE			0xffffffff

#define WSABASEERR			10000
#define WSAEINTR				(WSABASEERR+4)
#define WSAEBADF				(WSABASEERR+9)
#define WSAEACCES				(WSABASEERR+13)
#define WSAEFAULT				(WSABASEERR+14)
#define WSAEINVAL				(WSABASEERR+22)
#define WSAEMFILE				(WSABASEERR+24)
#define WSAEWOULDBLOCK		(WSABASEERR+35)
#define WSAEINPROGRESS		(WSABASEERR+36)
#define WSAEALREADY			(WSABASEERR+37)
#define WSAENOTSOCK			(WSABASEERR+38)
#define WSAEDESTADDRREQ		(WSABASEERR+39)
#define WSAEMSGSIZE			(WSABASEERR+40)
#define WSAEPROTOTYPE		(WSABASEERR+41)
#define WSAENOPROTOOPT		(WSABASEERR+42)
#define WSAEPROTONOSUPPORT	(WSABASEERR+43)
#define WSAESOCKTNOSUPPORT	(WSABASEERR+44)
#define WSAEOPNOTSUPP		(WSABASEERR+45)
#define WSAEPFNOSUPPORT		(WSABASEERR+46)
#define WSAEAFNOSUPPORT		(WSABASEERR+47)
#define WSAEADDRINUSE		(WSABASEERR+48)
#define WSAEADDRNOTAVAIL	(WSABASEERR+49)
#define WSAENETDOWN			(WSABASEERR+50)
#define WSAENETUNREACH		(WSABASEERR+51)
#define WSAENETRESET			(WSABASEERR+52)
#define WSAECONNABORTED		(WSABASEERR+53)
#define WSAECONNRESET		(WSABASEERR+54)
#define WSAENOBUFS			(WSABASEERR+55)
#define WSAEISCONN			(WSABASEERR+56)
#define WSAENOTCONN			(WSABASEERR+57)
#define WSAESHUTDOWN			(WSABASEERR+58)
#define WSAETIMEDOUT			(WSABASEERR+60)
#define WSAECONNREFUSED		(WSABASEERR+61)
#define WSAEHOSTUNREACH		(WSABASEERR+65)
#define WSASYSNOTREADY		(WSABASEERR+91)
#define WSAVERNOTSUPPORTED	(WSABASEERR+92)
#define WSANOTINITIALISED	(WSABASEERR+93)
#define WSAHOST_NOT_FOUND	(WSABASEERR+1001)
#define WSATRY_AGAIN			(WSABASEERR+1002)
#define WSANO_RECOVERY		(WSABASEERR+1003)
#define WSANO_DATA			(WSABASEERR+1004)

#ifdef __cplusplus
extern "C" {
#endif
#define RA_WSAAPI		__stdcall
SOCKET				RA_WSAAPI accept(SOCKET s, struct sockaddr * addr, int * addrlen);
int					RA_WSAAPI bind(SOCKET s, const struct sockaddr * name, int namelen);
int					RA_WSAAPI closesocket(SOCKET s);
int					RA_WSAAPI connect(SOCKET s, const struct sockaddr * name, int namelen);
int					RA_WSAAPI ioctlsocket(SOCKET s, long command, u_long * argp);
int					RA_WSAAPI getpeername(SOCKET s, struct sockaddr * name, int * namelen);
int					RA_WSAAPI getsockname(SOCKET s, struct sockaddr * name, int * namelen);
int					RA_WSAAPI getsockopt(SOCKET s, int level, int name, char * value, int * length);
u_long				RA_WSAAPI htonl(u_long value);
u_short				RA_WSAAPI htons(u_short value);
unsigned long		RA_WSAAPI inet_addr(const char * address);
char *				RA_WSAAPI inet_ntoa(struct in_addr address);
int					RA_WSAAPI listen(SOCKET s, int backlog);
u_long				RA_WSAAPI ntohl(u_long value);
u_short				RA_WSAAPI ntohs(u_short value);
int					RA_WSAAPI recv(SOCKET s, char * buffer, int length, int flags);
int					RA_WSAAPI recvfrom(SOCKET s, char * buffer, int length, int flags, struct sockaddr * from, int * fromlen);
int					RA_WSAAPI select(int count, fd_set * readable, fd_set * writable, fd_set * failed, const struct timeval * timeout);
int					RA_WSAAPI send(SOCKET s, const char * buffer, int length, int flags);
int					RA_WSAAPI sendto(SOCKET s, const char * buffer, int length, int flags, const struct sockaddr * to, int tolen);
int					RA_WSAAPI setsockopt(SOCKET s, int level, int name, const char * value, int length);
int					RA_WSAAPI shutdown(SOCKET s, int how);
SOCKET				RA_WSAAPI socket(int family, int type, int protocol);
struct hostent *	RA_WSAAPI gethostbyaddr(const char * address, int length, int type);
struct hostent *	RA_WSAAPI gethostbyname(const char * name);
int					RA_WSAAPI gethostname(char * name, int length);
int					RA_WSAAPI WSAStartup(WORD version, LPWSADATA data);
int					RA_WSAAPI WSACleanup(void);
void					RA_WSAAPI WSASetLastError(int error);
int					RA_WSAAPI WSAGetLastError(void);
#ifdef __cplusplus
}
#endif

#else	/* Not Windows: BSD sockets. */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

typedef int						SOCKET;

#ifdef __cplusplus
/*
**	Winsock passes address and option lengths as int, POSIX as socklen_t.
*/
static inline int recvfrom(SOCKET s, char * buffer, int length, int flags, struct sockaddr * from, int * fromlen)
{
	socklen_t len = fromlen ? (socklen_t)*fromlen : 0;
	int result = (int)::recvfrom(s, buffer, (size_t)length, flags, from, fromlen ? &len : NULL);
	if (fromlen) *fromlen = (int)len;
	return(result);
}

static inline int getsockopt(SOCKET s, int level, int name, char * value, int * length)
{
	socklen_t len = length ? (socklen_t)*length : 0;
	int result = ::getsockopt(s, level, name, value, length ? &len : NULL);
	if (length) *length = (int)len;
	return(result);
}
#endif
#define INVALID_SOCKET			(-1)
#define SOCKET_ERROR				(-1)

#ifndef INADDR_NONE
#define INADDR_NONE				0xffffffff
#endif

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

#define WSABASEERR			10000
#define WSAEWOULDBLOCK		EWOULDBLOCK
#define WSAEINPROGRESS		EINPROGRESS
#define WSAECONNRESET		ECONNRESET
#define WSAENOTSOCK			ENOTSOCK
#define WSAEADDRINUSE		EADDRINUSE
#define WSAECONNREFUSED		ECONNREFUSED
#define WSAEMSGSIZE			EMSGSIZE
#define WSAENOBUFS			ENOBUFS
#define WSAHOST_NOT_FOUND	11001

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
int		ioctlsocket(SOCKET s, int command, unsigned int * argp);
#ifdef __cplusplus
}
#endif

#endif	/* Not Windows */

typedef struct in_addr			IN_ADDR, *PIN_ADDR, *LPIN_ADDR;
typedef struct sockaddr		SOCKADDR, *PSOCKADDR, *LPSOCKADDR;
typedef struct sockaddr_in	SOCKADDR_IN, *PSOCKADDR_IN, *LPSOCKADDR_IN;
typedef struct hostent			HOSTENT, *PHOSTENT, *LPHOSTENT;
typedef struct linger			LINGER, *PLINGER, *LPLINGER;

#define MAXGETHOSTSTRUCT		1024

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

/*
**	Emulated on every system, as the replies go to the port's own windows. closesocket()
**	also forgets the socket's WSAAsyncSelect() registration. See compat/winsock.cpp.
*/
#define WSAAsyncSelect				Winsock_Async_Select
#define WSAAsyncGetHostByName		Winsock_Async_Get_Host_By_Name
#define WSAAsyncGetHostByAddr		Winsock_Async_Get_Host_By_Addr
#define WSACancelAsyncRequest		Winsock_Cancel_Async_Request
#define closesocket					Winsock_Close_Socket

#ifdef __cplusplus
extern "C" {
#endif
int		Winsock_Async_Select(SOCKET s, HWND window, UINT message, int events);
HANDLE	Winsock_Async_Get_Host_By_Name(HWND window, UINT message, const char * name, char * buffer, int buflen);
HANDLE	Winsock_Async_Get_Host_By_Addr(HWND window, UINT message, const char * addr, int len, int type, char * buffer, int buflen);
int		Winsock_Cancel_Async_Request(HANDLE request);
int		Winsock_Close_Socket(SOCKET s);

/*
**	Port helper: the IPv4 addresses of this machine's network interfaces (network byte order).
**	Returns how many were stored.
*/
int		Winsock_Get_Local_Addresses(unsigned int * addresses, int max);
#ifdef __cplusplus
}
#endif

#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
